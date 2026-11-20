#include "sokoban_activity.h"
#include "../../bridge/PocketPlayActivity.h"
#include "sokoban.h"

namespace pocketplay {
std::unique_ptr<Activity> createSokoban(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<PocketPlayActivity>(sokobanGame(), renderer, mappedInput);
}
}
