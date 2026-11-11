#pragma once

#include <Microphone.h>
#include <cstddef>
#include <cstdint>

namespace voicenotes {

class VoiceRecorder {
 public:
  static constexpr uint32_t kSampleRate = 16000;
  static constexpr int kMaxSeconds = 60;
  static constexpr size_t kMaxSamples = kSampleRate * kMaxSeconds;
  static constexpr size_t kMaxBytes = kMaxSamples * sizeof(int16_t);

  VoiceRecorder();
  ~VoiceRecorder();

  // Initialize and allocate PSRAM buffer
  bool begin();

  // Start recording audio from the PDM microphone
  bool startRecording();

  // Call frequently in the activity loop during recording to drain samples from I2S
  void update();

  // Stop recording
  void stopRecording();

  bool isRecording() const { return recording_; }
  size_t recordedSamples() const { return samplesCount_; }
  size_t recordedBytes() const { return samplesCount_ * sizeof(int16_t); }
  uint32_t durationMs() const;

  // Real-time audio activity level (0 to 100) for UI VU meter
  int audioLevel() const { return currentLevel_; }

  // Access the raw PCM samples
  const int16_t* pcmData() const { return pcmBuffer_; }

  // Generate 44-byte WAV header for the recorded samples
  static void buildWavHeader(uint8_t header[44], size_t pcmDataBytes);

  // Save the recording to an SD card .wav file
  bool saveWav(const char* path);

 private:
  freeink::Microphone mic_;
  int16_t* pcmBuffer_ = nullptr;
  size_t bufferCapacitySamples_ = 0;
  size_t samplesCount_ = 0;
  bool recording_ = false;
  uint32_t startMs_ = 0;
  uint32_t elapsedMs_ = 0;
  int currentLevel_ = 0;
};

}  // namespace voicenotes
