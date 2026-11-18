#pragma once

#include <memory>
#include <string>

#include "../../activities/Activity.h"
#include "../ui/ToyboxScreen.h"
#include "GroqTranscriber.h"
#include "VoiceConfig.h"
#include "VoiceRecorder.h"

namespace voicenotes {

class VoiceNotesActivity final : public Activity {
 public:
  VoiceNotesActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  ~VoiceNotesActivity() override;

  static std::unique_ptr<Activity> create(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

  // The main loop otherwise sleeps 10-50 ms per pass (and downclocks the CPU
  // after 300 ms without a touch). The I2S RX DMA ring only holds ~90 ms of
  // audio, so at a 50 ms poll most of the recording overflowed and was lost,
  // leaving Whisper a short, chopped, near-silent clip to hallucinate on.
  bool skipLoopDelay() override { return state_ == State::RECORDING; }
  bool preventAutoSleep() override { return state_ == State::RECORDING || state_ == State::TRANSCRIBING; }

 private:
  enum class State {
    READY,
    RECORDING,
    TRANSCRIBING,
    RESULT,
    ERROR_STATE
  };

  void handleInput();
  void handleAction(freeink::ui::ActionId action);
  void startRecording();
  void finishRecordingAndTranscribe();
  void cancelRecording();
  void saveTranscribedNote();
  void handleReloadConfig();

  Config config_;
  VoiceRecorder recorder_;
  State state_ = State::READY;

  std::string transcribedText_;
  std::string lastSavedPath_;
  std::string statusMessage_;
  std::string errorMessage_;

  uint32_t lastRenderMs_ = 0;
  uint32_t recordingStartMs_ = 0;
  int lastRenderedSec_ = -1;
  int lastRenderedLevel_ = -1;

  toybox::Interactions interactions_;
};

}  // namespace voicenotes
