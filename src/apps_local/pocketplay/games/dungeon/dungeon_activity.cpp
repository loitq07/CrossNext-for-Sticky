#include "dungeon_activity.h"
#include "../../bridge/PocketPlayActivity.h"
#include "dungeon.h"

namespace pocketplay {
std::unique_ptr<Activity> createDungeon(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<PocketPlayActivity>(dungeonGame(), renderer, mappedInput);
}
}
