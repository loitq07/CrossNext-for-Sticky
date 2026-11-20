#include "islandmaps_activity.h"
#include "../../bridge/PocketPlayActivity.h"
#include "islandmaps.h"

namespace pocketplay {
std::unique_ptr<Activity> createIslandMaps(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<PocketPlayActivity>(islandMapsGame(), renderer, mappedInput);
}
}
