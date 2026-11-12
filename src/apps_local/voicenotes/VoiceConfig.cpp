#include "VoiceConfig.h"

#include <ArduinoJson.h>
#include <HalStorage.h>
#include <Logging.h>

namespace voicenotes {
namespace {

constexpr const char* kConfigDir = "/settings";
constexpr const char* kConfigPath = "/settings/voice_ai.json";
constexpr const char* kAltConfigPath = "/notes/voice_ai.json";

}  // namespace

bool loadConfig(Config& out) {
  const char* activePath = nullptr;
  if (Storage.exists(kConfigPath)) {
    activePath = kConfigPath;
  } else if (Storage.exists(kAltConfigPath)) {
    activePath = kAltConfigPath;
  }

  if (!activePath) {
    LOG_INF("VOICE", "No config file found. Generating template at %s", kConfigPath);
    ensureConfigTemplate();
    return false;
  }

  String content = Storage.readFile(activePath);
  if (content.isEmpty()) {
    LOG_ERR("VOICE", "Config file empty or read failed: %s", activePath);
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, content);
  if (err) {
    LOG_ERR("VOICE", "Failed to parse JSON config: %s", err.c_str());
    return false;
  }

  const char* apiKey = doc["api_key"] | "";
  if (strlen(apiKey) == 0) {
    apiKey = doc["groq_api_key"] | "";
  }
  out.apiKey = apiKey;

  const char* model = doc["model"] | "whisper-large-v3-turbo";
  out.model = model;

  const char* lang = doc["language"] | "vi";
  out.language = lang;

  const char* host = doc["host"] | "api.groq.com";
  out.host = host;

  const char* path = doc["path"] | "/openai/v1/audio/transcriptions";
  out.path = path;

  out.port = doc["port"] | 443;

  LOG_INF("VOICE", "Loaded config: host=%s model=%s lang=%s key_len=%u", out.host.c_str(), out.model.c_str(),
          out.language.c_str(), static_cast<unsigned>(out.apiKey.length()));
  return !out.apiKey.empty();
}

bool saveConfig(const Config& in) {
  Storage.ensureDirectoryExists(kConfigDir);

  JsonDocument doc;
  doc["service"] = "groq";
  doc["api_key"] = in.apiKey;
  doc["model"] = in.model;
  doc["language"] = in.language;
  doc["host"] = in.host;
  doc["path"] = in.path;
  doc["port"] = in.port;
  doc["instructions"] = "Get a free Groq API key at https://console.groq.com/keys and paste it into api_key.";

  String content;
  serializeJsonPretty(doc, content);

  return Storage.writeFile(kConfigPath, content);
}

void ensureConfigTemplate() {
  if (Storage.exists(kConfigPath) || Storage.exists(kAltConfigPath)) {
    return;
  }

  Config defaultCfg;
  defaultCfg.apiKey = "";  // prompt user to fill in
  saveConfig(defaultCfg);
}

}  // namespace voicenotes
