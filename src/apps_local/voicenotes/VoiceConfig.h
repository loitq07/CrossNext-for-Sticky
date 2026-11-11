#pragma once

#include <string>

namespace voicenotes {

struct Config {
  std::string apiKey;
  std::string model = "whisper-large-v3-turbo";
  std::string language = "vi";
  std::string host = "api.groq.com";
  std::string path = "/openai/v1/audio/transcriptions";
  int port = 443;
};

// Load configuration from /settings/voice_ai.json or /notes/voice_ai.json.
// If not found, generates a default template on the SD card.
bool loadConfig(Config& out);

// Save configuration to SD card
bool saveConfig(const Config& in);

// Ensure the template file exists on SD card so users can paste their Groq API key
void ensureConfigTemplate();

}  // namespace voicenotes
