#include "wordladder_activity.h"
#include "../../bridge/PocketPlayActivity.h"
#include "wordladder.h"

namespace pocketplay {
std::unique_ptr<Activity> createWordLadder(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<PocketPlayActivity>(wordLadderGame(), renderer, mappedInput);
}
}
