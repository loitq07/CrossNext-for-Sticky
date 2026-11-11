#include "VoiceNotesActivity.h"

#include <Arduino.h>
#include <HalStorage.h>
#include <I18n.h>
#include <Logging.h>
#include <WiFi.h>
#include <ctime>

#include "../../CrossPointSettings.h"
#include "../../WifiCredentialStore.h"
#include "../../fontIds.h"
#include "../ui/ExtraIcons.h"
#include "../ui/ToyboxFonts.h"
#include "../ui/ToyboxTheme.h"

namespace voicenotes {

namespace {

// Break text into lines fitting within maxWidth pixels
std::vector<std::string> wrapText(const GfxRenderer& renderer, int fontId, const std::string& text, int maxWidth) {
  std::vector<std::string> lines;
  if (text.empty()) return lines;

  std::string currentLine;
  size_t start = 0;

  while (start < text.length()) {
    size_t nextSpace = text.find_first_of(" \n\r", start);
    std::string word;
    char delimiter = 0;

    if (nextSpace == std::string::npos) {
      word = text.substr(start);
      start = text.length();
    } else {
      word = text.substr(start, nextSpace - start);
      delimiter = text[nextSpace];
      start = nextSpace + 1;
    }

    if (!word.empty()) {
      std::string testLine = currentLine.empty() ? word : (currentLine + " " + word);
      int w = renderer.getTextWidth(fontId, testLine.c_str());

      if (w <= maxWidth) {
        currentLine = testLine;
      } else {
        if (!currentLine.empty()) {
          lines.push_back(currentLine);
        }
        currentLine = word;
      }
    }

    if (delimiter == '\n') {
      lines.push_back(currentLine);
      currentLine.clear();
    }
  }

  if (!currentLine.empty()) {
    lines.push_back(currentLine);
  }

  return lines;
}

}  // namespace

VoiceNotesActivity::VoiceNotesActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("VoiceNotes", renderer, mappedInput) {}

VoiceNotesActivity::~VoiceNotesActivity() {
  recorder_.stopRecording();
}

std::unique_ptr<Activity> VoiceNotesActivity::create(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<VoiceNotesActivity>(renderer, mappedInput);
}

void VoiceNotesActivity::onEnter() {
  Activity::onEnter();
  toybox::ensureFonts(renderer);

  loadConfig(config_);
  recorder_.begin();

  state_ = State::READY;
  statusMessage_ = "Press OK / Tap to Record";
  requestUpdate();
}

void VoiceNotesActivity::onExit() {
  recorder_.stopRecording();
  Activity::onExit();
}

void VoiceNotesActivity::startRecording() {
  if (state_ == State::RECORDING) return;

  if (!recorder_.startRecording()) {
    state_ = State::ERROR_STATE;
    errorMessage_ = "Cannot start microphone";
    requestUpdate();
    return;
  }

  state_ = State::RECORDING;
  lastRenderedSec_ = -1;
  requestUpdate();
}

void VoiceNotesActivity::finishRecordingAndTranscribe() {
  recorder_.stopRecording();
  state_ = State::TRANSCRIBING;
  statusMessage_ = "Transcribing with Groq Whisper...";
  requestUpdateAndWait();

  // If Wi-Fi is disconnected, attempt quick reconnect with saved credentials
  if (WiFi.status() != WL_CONNECTED) {
    const std::string lastSsid = WIFI_STORE.getLastConnectedSsid();
    if (!lastSsid.empty()) {
      auto cred = WIFI_STORE.findCredential(lastSsid);
      if (cred.has_value()) {
        LOG_INF("VOICE", "Connecting to Wi-Fi (%s)...", cred->ssid.c_str());
        WiFi.begin(cred->ssid.c_str(), cred->password.c_str());
        const uint32_t startWait = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startWait < 8000)) {
          delay(100);
        }
      }
    }
  }

  const TranscribeResult res = transcribeAudio(config_, recorder_.pcmData(), recorder_.recordedBytes());

  if (res.status == TranscribeStatus::SUCCESS) {
    transcribedText_ = res.text;
    state_ = State::RESULT;
    saveTranscribedNote();
  } else {
    state_ = State::ERROR_STATE;
    errorMessage_ = res.errorMessage;
  }

  requestUpdate();
}

void VoiceNotesActivity::cancelRecording() {
  recorder_.stopRecording();
  state_ = State::READY;
  statusMessage_ = "Recording cancelled";
  requestUpdate();
}

void VoiceNotesActivity::saveTranscribedNote() {
  if (transcribedText_.empty()) return;

  Storage.ensureDirectoryExists("/notes");

  // Format filename with timestamp or counter
  char filename[64];
  const time_t now = time(nullptr);
  struct tm t;
  if (now > 1600000000 && localtime_r(&now, &t)) {
    std::snprintf(filename, sizeof(filename), "/notes/Voice_%04d-%02d-%02d_%02d%02d%02d.md", t.tm_year + 1900,
                  t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
  } else {
    std::snprintf(filename, sizeof(filename), "/notes/Voice_Note_%lu.md", static_cast<unsigned long>(millis() / 1000));
  }

  std::string content = "# Voice Note\n\n";
  content += transcribedText_;
  content += "\n\n---\n*Transcribed by Groq Whisper on reTerminal Sticky*\n";

  if (Storage.writeFile(filename, String(content.c_str()))) {
    lastSavedPath_ = filename;
    LOG_INF("VOICE", "Saved note to %s", filename);
  } else {
    LOG_ERR("VOICE", "Failed to save note to %s", filename);
  }
}

void VoiceNotesActivity::handleInput() {
  int tapX = 0, tapY = 0;
  const bool tapped = mappedInput.wasScreenTapped(tapX, tapY);
  const bool confirmPressed = mappedInput.wasPressed(MappedInputManager::Button::Confirm);
  const bool backPressed = mappedInput.wasPressed(MappedInputManager::Button::Back);

  if (backPressed) {
    if (state_ == State::RECORDING) {
      cancelRecording();
    } else if (state_ == State::RESULT || state_ == State::ERROR_STATE) {
      state_ = State::READY;
      requestUpdate();
    } else {
      finish();
    }
    return;
  }

  switch (state_) {
    case State::READY:
      if (confirmPressed || tapped) {
        startRecording();
      }
      break;

    case State::RECORDING:
      if (confirmPressed || tapped) {
        finishRecordingAndTranscribe();
      }
      break;

    case State::RESULT:
      if (confirmPressed || tapped) {
        // Start another recording
        startRecording();
      }
      break;

    case State::ERROR_STATE:
      if (confirmPressed || tapped) {
        state_ = State::READY;
        requestUpdate();
      }
      break;

    case State::TRANSCRIBING:
      // Busy transcribing, ignore inputs
      break;
  }
}

void VoiceNotesActivity::loop() {
  handleInput();

  if (state_ == State::RECORDING) {
    recorder_.update();

    if (!recorder_.isRecording()) {
      // Auto-stopped due to buffer full or max duration
      finishRecordingAndTranscribe();
      return;
    }

    // Refresh UI roughly once per second or when level changes
    const int sec = static_cast<int>(recorder_.durationMs() / 1000);
    const int lvl = recorder_.audioLevel() / 10;
    if (sec != lastRenderedSec_ || lvl != lastRenderedLevel_ || (millis() - lastRenderMs_ > 300)) {
      lastRenderedSec_ = sec;
      lastRenderedLevel_ = lvl;
      lastRenderMs_ = millis();
      requestUpdate(true);
    }
  }
}

void VoiceNotesActivity::render(RenderLock&&) {
  const int width = renderer.getScreenWidth();
  const int height = renderer.getScreenHeight();

  // White background
  renderer.fillRect(0, 0, width, height, false);

  // Top header bar
  renderer.fillRect(0, 0, width, 56, true);
  renderer.drawText(toybox::kUiFontId, 20, 36, "VOICE NOTES", false);

  // Wi-Fi status indicator in header
  const bool wifiOk = (WiFi.status() == WL_CONNECTED);
  const char* wifiStr = wifiOk ? "Wi-Fi: OK" : "Wi-Fi: --";
  const int bw = renderer.getTextWidth(toybox::kReadingFontId, wifiStr);
  renderer.drawText(toybox::kReadingFontId, width - bw - 20, 36, wifiStr, false);

  // Content area
  const int contentTop = 80;

  switch (state_) {
    case State::READY: {
      // Big Mic Icon in center
      const int iconX = (width - 32) / 2;
      renderer.drawIcon(extra_mic_32_bits, iconX, contentTop + 40, 32);

      // Prompt Box
      const int boxY = contentTop + 110;
      renderer.drawRect(30, boxY, width - 60, 90, 2, true);
      renderer.drawText(toybox::kUiFontId, 50, boxY + 40, "Press OK to Record", true);
      renderer.drawText(toybox::kReadingFontId, 50, boxY + 70, "Bấm hoặc chạm để bắt đầu ghi âm", true);

      // Instructions / Configuration status
      const int hintY = boxY + 130;
      if (config_.apiKey.empty()) {
        renderer.drawText(toybox::kReadingFontId, 30, hintY, "! Missing Groq API Key", true);
        renderer.drawText(toybox::kReadingFontId, 30, hintY + 28, "Please set key in /settings/voice_ai.json", true);
        renderer.drawText(toybox::kReadingFontId, 30, hintY + 56, "(Free key at https://console.groq.com)", true);
      } else {
        renderer.drawText(toybox::kReadingFontId, 30, hintY, "Model: whisper-large-v3-turbo (Groq)", true);
        renderer.drawText(toybox::kReadingFontId, 30, hintY + 28, "Language: vi (Tiếng Việt) / Auto", true);
        renderer.drawText(toybox::kReadingFontId, 30, hintY + 56, "Saved directly to /notes/", true);
      }

      if (!lastSavedPath_.empty()) {
        renderer.drawRect(30, height - 90, width - 60, 60, 1, true);
        renderer.drawText(toybox::kReadingFontId, 45, height - 52, "Last note saved to:", true);
        renderer.drawText(toybox::kReadingFontId, 45, height - 30, lastSavedPath_.c_str(), true);
      }
      break;
    }

    case State::RECORDING: {
      // Flashing / Solid Recording header
      renderer.fillRect(40, contentTop + 10, width - 80, 50, true);
      renderer.drawText(toybox::kUiFontId, 60, contentTop + 44, "● RECORDING...", false);

      // Timer (e.g. "00:07 / 01:00")
      const uint32_t totalSec = recorder_.durationMs() / 1000;
      char timeBuf[32];
      std::snprintf(timeBuf, sizeof(timeBuf), "%02lu:%02lu / 01:00", static_cast<unsigned long>(totalSec / 60),
                    static_cast<unsigned long>(totalSec % 60));

      const int tw = renderer.getTextWidth(toybox::kDisplayFontId, timeBuf);
      renderer.drawText(toybox::kDisplayFontId, (width - tw) / 2, contentTop + 130, timeBuf, true);

      // VU Meter / Audio level bar
      const int meterX = 50;
      const int meterY = contentTop + 160;
      const int meterW = width - 100;
      const int meterH = 24;
      renderer.drawRect(meterX, meterY, meterW, meterH, 2, true);

      const int level = recorder_.audioLevel();
      const int fillW = (meterW - 4) * level / 100;
      if (fillW > 0) {
        renderer.fillRect(meterX + 2, meterY + 2, fillW, meterH - 4, true);
      }

      // Stop hint
      renderer.drawRect(40, contentTop + 220, width - 80, 60, 1, true);
      renderer.drawText(toybox::kUiFontId, 60, contentTop + 258, "Press OK to Finish", true);
      break;
    }

    case State::TRANSCRIBING: {
      const int boxY = contentTop + 80;
      renderer.drawRect(30, boxY, width - 60, 120, 2, true);
      renderer.drawText(toybox::kUiFontId, 50, boxY + 45, "Transcribing...", true);
      renderer.drawText(toybox::kReadingFontId, 50, boxY + 75, "Đang kết nối Groq Whisper...", true);
      renderer.drawText(toybox::kReadingFontId, 50, boxY + 100, "Vui lòng đợi trong giây lát...", true);
      break;
    }

    case State::RESULT: {
      renderer.drawText(toybox::kUiFontId, 30, contentTop + 25, "Transcribed Text (Kết quả):", true);

      // Box containing transcribed text
      const int boxY = contentTop + 40;
      const int boxH = height - boxY - 100;
      renderer.drawRect(20, boxY, width - 40, boxH, 2, true);

      const auto wrapped = wrapText(renderer, toybox::kReadingFontId, transcribedText_, width - 80);
      int lineY = boxY + 30;
      for (const auto& line : wrapped) {
        if (lineY + 22 > boxY + boxH) break;
        renderer.drawText(toybox::kReadingFontId, 35, lineY, line.c_str(), true);
        lineY += 26;
      }

      // Bottom action bar
      renderer.fillRect(20, height - 70, width - 40, 50, true);
      renderer.drawText(toybox::kUiFontId, 40, height - 38, "Saved to /notes/ [OK: Record Again]", false);
      break;
    }

    case State::ERROR_STATE: {
      renderer.fillRect(30, contentTop + 20, width - 60, 50, true);
      renderer.drawText(toybox::kUiFontId, 50, contentTop + 54, "ERROR / LỖI", false);

      const int boxY = contentTop + 90;
      renderer.drawRect(30, boxY, width - 60, 140, 2, true);

      const auto wrappedErr = wrapText(renderer, toybox::kReadingFontId, errorMessage_, width - 100);
      int ey = boxY + 35;
      for (const auto& el : wrappedErr) {
        renderer.drawText(toybox::kReadingFontId, 50, ey, el.c_str(), true);
        ey += 26;
      }

      renderer.drawRect(40, height - 90, width - 80, 50, 1, true);
      renderer.drawText(toybox::kUiFontId, 60, height - 58, "Press OK to Retry", true);
      break;
    }
  }
}

}  // namespace voicenotes
