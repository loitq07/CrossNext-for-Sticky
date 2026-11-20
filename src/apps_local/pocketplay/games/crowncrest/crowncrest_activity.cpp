#include "crowncrest_activity.h"
#include "../../bridge/PocketPlayActivity.h"
#include "crowncrest.h"

namespace pocketplay {
std::unique_ptr<Activity> createCrownCrest(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<PocketPlayActivity>(crownCrestGame(), renderer, mappedInput);
}
}
