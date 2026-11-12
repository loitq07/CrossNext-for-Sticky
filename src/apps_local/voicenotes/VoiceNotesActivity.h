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
