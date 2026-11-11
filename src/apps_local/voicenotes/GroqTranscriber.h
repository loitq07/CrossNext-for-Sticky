#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "VoiceConfig.h"

namespace voicenotes {

enum class TranscribeStatus {
  SUCCESS,
  NO_WIFI,
  NO_API_KEY,
  CONNECT_FAIL,
  HTTP_ERROR,
  PARSE_ERROR,
  AUDIO_TOO_SHORT
};

struct TranscribeResult {
  TranscribeStatus status = TranscribeStatus::NO_API_KEY;
  int httpCode = 0;
  std::string text;
  std::string errorMessage;
};

// Send audio to Groq Whisper API and return the transcribed text
TranscribeResult transcribeAudio(const Config& config, const int16_t* pcmData, size_t pcmBytes);

}  // namespace voicenotes
