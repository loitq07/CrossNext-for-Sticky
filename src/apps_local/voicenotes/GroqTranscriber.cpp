#include "GroqTranscriber.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Logging.h>
#include <SecureClient.h>
#include <WiFi.h>

#include "../../network/WifiPowerSaveGuard.h"
#include "VoiceRecorder.h"

namespace voicenotes {

TranscribeResult transcribeAudio(const Config& config, const int16_t* pcmData, size_t pcmBytes) {
  TranscribeResult result;

  if (pcmBytes < 3200) {  // less than 0.1s
    result.status = TranscribeStatus::AUDIO_TOO_SHORT;
    result.errorMessage = "Bản ghi âm quá ngắn (dưới 0.1 giây)";
    return result;
  }

  if (config.apiKey.empty()) {
    result.status = TranscribeStatus::NO_API_KEY;
    result.errorMessage = "Chưa có Groq API Key trong /settings/voice_ai.json";
    return result;
  }

  if (WiFi.status() != WL_CONNECTED) {
    result.status = TranscribeStatus::NO_WIFI;
    result.errorMessage = "Chưa kết nối Wi-Fi";
    return result;
  }

  WifiPowerSaveGuard psGuard;

  freeink::SecureClient client;
  client.setInsecure();
  client.setTimeout(30000);

  LOG_INF("VOICE", "Connecting to %s:%d ...", config.host.c_str(), config.port);
  if (!client.connect(config.host.c_str(), config.port)) {
    LOG_ERR("VOICE", "Connection failed to %s:%d", config.host.c_str(), config.port);
    result.status = TranscribeStatus::CONNECT_FAIL;
    result.errorMessage = "Không thể kết nối tới máy chủ Groq AI";
    return result;
  }

  const std::string boundary = "----CrossNextVoiceBoundary789456123";

  std::string bodyStart;
  bodyStart += "--" + boundary + "\r\n";
  bodyStart += "Content-Disposition: form-data; name=\"model\"\r\n\r\n";
  bodyStart += config.model + "\r\n";

  if (!config.language.empty()) {
    bodyStart += "--" + boundary + "\r\n";
    bodyStart += "Content-Disposition: form-data; name=\"language\"\r\n\r\n";
    bodyStart += config.language + "\r\n";
  }

  bodyStart += "--" + boundary + "\r\n";
  bodyStart += "Content-Disposition: form-data; name=\"response_format\"\r\n\r\n";
  bodyStart += "json\r\n";

  bodyStart += "--" + boundary + "\r\n";
  bodyStart += "Content-Disposition: form-data; name=\"temperature\"\r\n\r\n";
  bodyStart += "0.0\r\n";

  bodyStart += "--" + boundary + "\r\n";
  bodyStart += "Content-Disposition: form-data; name=\"prompt\"\r\n\r\n";
  bodyStart += "Ghi chú công việc, hội thoại tiếng Việt.\r\n";

  bodyStart += "--" + boundary + "\r\n";
  bodyStart += "Content-Disposition: form-data; name=\"file\"; filename=\"speech.wav\"\r\n";
  bodyStart += "Content-Type: audio/wav\r\n\r\n";

  const std::string bodyEnd = "\r\n--" + boundary + "--\r\n";

  const size_t wavTotalBytes = 44 + pcmBytes;
  const size_t totalContentLength = bodyStart.length() + wavTotalBytes + bodyEnd.length();

  // Send HTTP headers
  client.print("POST ");
  client.print(config.path.c_str());
  client.print(" HTTP/1.1\r\nHost: ");
  client.print(config.host.c_str());
  client.print("\r\nAuthorization: Bearer ");
  client.print(config.apiKey.c_str());
  client.print("\r\nContent-Type: multipart/form-data; boundary=");
  client.print(boundary.c_str());
  client.print("\r\nContent-Length: ");
  client.print(static_cast<uint32_t>(totalContentLength));
  client.print("\r\nUser-Agent: CrossNext-Sticky/1.4\r\nConnection: close\r\n\r\n");

  LOG_INF("VOICE", "Streaming audio payload (%u bytes) ...", static_cast<unsigned>(totalContentLength));

  // 1. Multipart header
  client.write(reinterpret_cast<const uint8_t*>(bodyStart.data()), bodyStart.length());

  // 2. 44-byte WAV header
  uint8_t wavHeader[44];
  VoiceRecorder::buildWavHeader(wavHeader, pcmBytes);
  client.write(wavHeader, sizeof(wavHeader));

  // 3. Audio PCM bytes
  const uint8_t* rawBytes = reinterpret_cast<const uint8_t*>(pcmData);
  size_t sent = 0;
  while (sent < pcmBytes && client.connected()) {
    const size_t chunk = std::min<size_t>(1024, pcmBytes - sent);
    client.write(&rawBytes[sent], chunk);
    sent += chunk;
    delay(1);
  }

  // 4. Multipart trailer
  client.write(reinterpret_cast<const uint8_t*>(bodyEnd.data()), bodyEnd.length());
  client.flush();

  LOG_INF("VOICE", "Waiting for Whisper response ...");

  // Read response line
  String statusLine = client.readStringUntil('\n');
  int statusCode = 0;
  if (statusLine.startsWith("HTTP/1.")) {
    const int codeIdx = statusLine.indexOf(' ');
    if (codeIdx > 0) {
      statusCode = statusLine.substring(codeIdx + 1, codeIdx + 4).toInt();
    }
  }
  result.httpCode = statusCode;
  LOG_INF("VOICE", "HTTP status: %d", statusCode);

  bool isChunked = false;
  int contentLength = -1;

  // Read HTTP headers
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    line.trim();
    if (line.isEmpty()) break;
    String lower = line;
    lower.toLowerCase();
    if (lower.startsWith("transfer-encoding:") && lower.indexOf("chunked") >= 0) {
      isChunked = true;
    }
    if (lower.startsWith("content-length:")) {
      contentLength = line.substring(15).toInt();
    }
  }

  LOG_INF("VOICE", "Headers parsed: code=%d chunked=%d len=%d", statusCode, isChunked ? 1 : 0, contentLength);

  String body;
  const uint32_t readTimeout = 15000;
  const uint32_t startRead = millis();

  if (isChunked) {
    while ((client.connected() || client.available()) && (millis() - startRead < readTimeout)) {
      String hexLine = client.readStringUntil('\n');
      hexLine.trim();
      if (hexLine.isEmpty()) continue;
      long chunkSize = strtol(hexLine.c_str(), nullptr, 16);
      if (chunkSize <= 0) break;

      while (chunkSize > 0 && (client.connected() || client.available()) && (millis() - startRead < readTimeout)) {
        if (client.available()) {
          body += static_cast<char>(client.read());
          chunkSize--;
        } else {
          delay(1);
        }
      }
      client.readStringUntil('\n');  // trailing \r\n
    }
  } else if (contentLength > 0) {
    while (body.length() < static_cast<size_t>(contentLength) && (client.connected() || client.available()) &&
           (millis() - startRead < readTimeout)) {
      if (client.available()) {
        body += static_cast<char>(client.read());
      } else {
        delay(1);
      }
    }
  } else {
    while ((client.connected() || client.available()) && (millis() - startRead < readTimeout)) {
      if (client.available()) {
        body += static_cast<char>(client.read());
      } else {
        delay(5);
      }
    }
  }
  client.stop();

  LOG_INF("VOICE", "Raw response body (%u bytes): %s", static_cast<unsigned>(body.length()), body.c_str());

  // Extract pure JSON { ... } to prevent chunk bytes / carriage returns from breaking parsing
  int firstBrace = body.indexOf('{');
  int lastBrace = body.lastIndexOf('}');
  String jsonStr = (firstBrace >= 0 && lastBrace > firstBrace) ? body.substring(firstBrace, lastBrace + 1) : body;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, jsonStr);
  if (err) {
    LOG_ERR("VOICE", "JSON parse error: %s (raw body: %s)", err.c_str(), body.c_str());
    result.status = TranscribeStatus::PARSE_ERROR;
    result.errorMessage = "Không thể phân tích phản hồi từ Groq API (Mã: " + std::to_string(statusCode) + ")";
    return result;
  }

  if (statusCode == 200) {
    const char* text = doc["text"] | "";
    result.text = text;
    if (result.text.empty()) {
      result.text = "(Không nhận diện được giọng nói trong bản ghi âm)";
    }
    result.status = TranscribeStatus::SUCCESS;
    LOG_INF("VOICE", "Transcription success! (%u chars)", static_cast<unsigned>(result.text.length()));
  } else {
    result.status = TranscribeStatus::HTTP_ERROR;
    const char* errMsg = doc["error"]["message"] | "";
    if (strlen(errMsg) > 0) {
      result.errorMessage = std::string(errMsg);
    } else {
      result.errorMessage = "Lỗi máy chủ Groq (HTTP " + std::to_string(statusCode) + ")";
    }
    LOG_ERR("VOICE", "API Error %d: %s", statusCode, result.errorMessage.c_str());
  }

  return result;
}

}  // namespace voicenotes
