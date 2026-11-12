#include "VoiceRecorder.h"

#include <Arduino.h>
#include <HalStorage.h>
#include <Logging.h>
#include <cmath>
#include <cstring>
#include <esp_heap_caps.h>

namespace voicenotes {

VoiceRecorder::VoiceRecorder() {}

VoiceRecorder::~VoiceRecorder() {
  stopRecording();
  if (pcmBuffer_) {
    free(pcmBuffer_);
    pcmBuffer_ = nullptr;
  }
}

bool VoiceRecorder::begin() {
  if (pcmBuffer_) return true;

  // Try allocating full 60s in PSRAM (approx 1.92 MB)
  bufferCapacitySamples_ = kMaxSamples;
  pcmBuffer_ = static_cast<int16_t*>(heap_caps_malloc(kMaxBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!pcmBuffer_) {
    LOG_INF("VOICE", "Could not allocate 60s in PSRAM, falling back to 30s");
    bufferCapacitySamples_ = kSampleRate * 30;
    pcmBuffer_ = static_cast<int16_t*>(
        heap_caps_malloc(bufferCapacitySamples_ * sizeof(int16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  }

  if (!pcmBuffer_) {
    LOG_INF("VOICE", "SPIRAM allocation failed, attempting standard malloc for 15s");
    bufferCapacitySamples_ = kSampleRate * 15;
    pcmBuffer_ = static_cast<int16_t*>(malloc(bufferCapacitySamples_ * sizeof(int16_t)));
  }

  if (!pcmBuffer_) {
    LOG_ERR("VOICE", "Out of memory: cannot allocate audio buffer");
    bufferCapacitySamples_ = 0;
    return false;
  }

  LOG_INF("VOICE", "Allocated audio buffer: %u samples (%u KB)", static_cast<unsigned>(bufferCapacitySamples_),
          static_cast<unsigned>((bufferCapacitySamples_ * sizeof(int16_t)) / 1024));
  return true;
}

bool VoiceRecorder::startRecording() {
  if (!begin()) return false;

  samplesCount_ = 0;
  currentLevel_ = 0;
  elapsedMs_ = 0;
  maxRecordedPeak_ = 0;

  if (!mic_.begin(kSampleRate)) {
    LOG_ERR("VOICE", "Microphone begin failed");
    return false;
  }

  recording_ = true;
  startMs_ = millis();
  LOG_INF("VOICE", "Recording started (16kHz mono PDM)");
  return true;
}

void VoiceRecorder::update() {
  if (!recording_ || !pcmBuffer_) return;

  elapsedMs_ = millis() - startMs_;
  if (samplesCount_ >= bufferCapacitySamples_ || elapsedMs_ >= (bufferCapacitySamples_ / kSampleRate) * 1000) {
    LOG_INF("VOICE", "Recording reached buffer limit, stopping automatically");
    stopRecording();
    return;
  }

  // Read up to 512 samples per pump
  constexpr size_t kChunkSize = 512;
  int16_t temp[kChunkSize];
  const size_t room = bufferCapacitySamples_ - samplesCount_;
  const size_t toRead = std::min<size_t>(kChunkSize, room);

  const int readCount = mic_.read(temp, toRead, 25);
  if (readCount > 0) {
    std::memcpy(&pcmBuffer_[samplesCount_], temp, readCount * sizeof(int16_t));
    samplesCount_ += readCount;

    // Calculate peak level for UI meter
    int32_t peak = 0;
    for (int i = 0; i < readCount; i++) {
      int32_t sample = std::abs(static_cast<int32_t>(temp[i]));
      if (sample > peak) peak = sample;
    }
    if (peak > maxRecordedPeak_) {
      maxRecordedPeak_ = peak;
    }

    // Map 0..32767 to 0..100 with gentle decay
    int level = std::min(100, static_cast<int>((peak * 100) / 16000));
    currentLevel_ = (currentLevel_ * 3 + level) / 4;
  }
}

void VoiceRecorder::stopRecording() {
  if (!recording_) return;
  recording_ = false;
  elapsedMs_ = millis() - startMs_;
  mic_.end();
  currentLevel_ = 0;
  LOG_INF("VOICE", "Recording stopped: %u samples (%u ms), maxPeak: %ld", static_cast<unsigned>(samplesCount_),
          static_cast<unsigned>(durationMs()), static_cast<long>(maxRecordedPeak_));

  // Software AGC / normalization:
  // If recording has valid speech (peak >= 30) but is quiet (peak < 16000),
  // boost it so Whisper gets loud and clear audio!
  if (maxRecordedPeak_ >= 30 && maxRecordedPeak_ < 16000 && pcmBuffer_ && samplesCount_ > 0) {
    const float targetPeak = 20000.0f;
    float gain = targetPeak / static_cast<float>(maxRecordedPeak_);
    if (gain > 12.0f) gain = 12.0f;
    LOG_INF("VOICE", "Normalizing audio: peak %ld -> target 20000 (gain x%.2f)", static_cast<long>(maxRecordedPeak_), gain);
    for (size_t i = 0; i < samplesCount_; ++i) {
      int32_t val = static_cast<int32_t>(pcmBuffer_[i] * gain);
      if (val > 32767) val = 32767;
      else if (val < -32767) val = -32767;
      pcmBuffer_[i] = static_cast<int16_t>(val);
    }
    maxRecordedPeak_ = 20000;
  }
}

uint32_t VoiceRecorder::durationMs() const {
  if (recording_) return millis() - startMs_;
  if (samplesCount_ == 0) return 0;
  return static_cast<uint32_t>((static_cast<uint64_t>(samplesCount_) * 1000) / kSampleRate);
}

void VoiceRecorder::buildWavHeader(uint8_t header[44], size_t pcmDataBytes) {
  const uint32_t totalDataLen = pcmDataBytes;
  const uint32_t totalFileLen = totalDataLen + 36;
  const uint32_t sampleRate = kSampleRate;
  const uint16_t channels = 1;
  const uint16_t bitsPerSample = 16;
  const uint32_t byteRate = sampleRate * channels * (bitsPerSample / 8);
  const uint16_t blockAlign = channels * (bitsPerSample / 8);

  // RIFF Chunk Descriptor
  header[0] = 'R';
  header[1] = 'I';
  header[2] = 'F';
  header[3] = 'F';
  header[4] = static_cast<uint8_t>(totalFileLen & 0xff);
  header[5] = static_cast<uint8_t>((totalFileLen >> 8) & 0xff);
  header[6] = static_cast<uint8_t>((totalFileLen >> 16) & 0xff);
  header[7] = static_cast<uint8_t>((totalFileLen >> 24) & 0xff);
  header[8] = 'W';
  header[9] = 'A';
  header[10] = 'V';
  header[11] = 'E';

  // "fmt " sub-chunk
  header[12] = 'f';
  header[13] = 'm';
  header[14] = 't';
  header[15] = ' ';
  header[16] = 16;  // Subchunk1Size for PCM
  header[17] = 0;
  header[18] = 0;
  header[19] = 0;
  header[20] = 1;  // AudioFormat 1 = PCM
  header[21] = 0;
  header[22] = static_cast<uint8_t>(channels & 0xff);
  header[23] = 0;
  header[24] = static_cast<uint8_t>(sampleRate & 0xff);
  header[25] = static_cast<uint8_t>((sampleRate >> 8) & 0xff);
  header[26] = static_cast<uint8_t>((sampleRate >> 16) & 0xff);
  header[27] = static_cast<uint8_t>((sampleRate >> 24) & 0xff);
  header[28] = static_cast<uint8_t>(byteRate & 0xff);
  header[29] = static_cast<uint8_t>((byteRate >> 8) & 0xff);
  header[30] = static_cast<uint8_t>((byteRate >> 16) & 0xff);
  header[31] = static_cast<uint8_t>((byteRate >> 24) & 0xff);
  header[32] = static_cast<uint8_t>(blockAlign & 0xff);
  header[33] = 0;
  header[34] = bitsPerSample;
  header[35] = 0;

  // "data" sub-chunk
  header[36] = 'd';
  header[37] = 'a';
  header[38] = 't';
  header[39] = 'a';
  header[40] = static_cast<uint8_t>(totalDataLen & 0xff);
  header[41] = static_cast<uint8_t>((totalDataLen >> 8) & 0xff);
  header[42] = static_cast<uint8_t>((totalDataLen >> 16) & 0xff);
  header[43] = static_cast<uint8_t>((totalDataLen >> 24) & 0xff);
}

bool VoiceRecorder::saveWav(const char* path) {
  if (samplesCount_ == 0 || !pcmBuffer_) return false;

  HalFile file;
  if (!Storage.openFileForWrite("VOICE", path, file)) {
    LOG_ERR("VOICE", "Failed to open %s for saving WAV", path);
    return false;
  }

  uint8_t header[44];
  buildWavHeader(header, recordedBytes());

  if (file.write(header, sizeof(header)) != sizeof(header)) {
    file.close();
    return false;
  }

  const uint8_t* pcmBytes = reinterpret_cast<const uint8_t*>(pcmBuffer_);
  const size_t totalBytes = recordedBytes();
  size_t written = 0;

  while (written < totalBytes) {
    const size_t chunkSize = std::min<size_t>(1024, totalBytes - written);
    const size_t wrote = file.write(&pcmBytes[written], chunkSize);
    if (wrote == 0) {
      file.close();
      return false;
    }
    written += wrote;
  }

  file.close();
  LOG_INF("VOICE", "Saved %s (%u bytes)", path, static_cast<unsigned>(sizeof(header) + totalBytes));
  return true;
}

}  // namespace voicenotes
