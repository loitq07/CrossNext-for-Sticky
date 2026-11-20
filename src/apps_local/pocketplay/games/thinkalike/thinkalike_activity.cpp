#include "thinkalike_activity.h"
#include "../../bridge/PocketPlayActivity.h"
#include "thinkalike.h"

namespace pocketplay {
std::unique_ptr<Activity> createThinkAlike(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<PocketPlayActivity>(thinkAlikeGame(), renderer, mappedInput);
}
}
