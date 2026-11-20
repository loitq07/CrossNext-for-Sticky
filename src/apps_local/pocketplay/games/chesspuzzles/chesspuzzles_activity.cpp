#include "chesspuzzles_activity.h"
#include "../../bridge/PocketPlayActivity.h"
#include "chesspuzzles.h"

namespace pocketplay {
std::unique_ptr<Activity> createChessPuzzles(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<PocketPlayActivity>(chessPuzzlesGame(), renderer, mappedInput);
}
}
