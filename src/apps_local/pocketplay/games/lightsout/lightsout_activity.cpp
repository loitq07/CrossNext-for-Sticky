#include "lightsout_activity.h"
#include "../../bridge/PocketPlayActivity.h"
#include "lightsout.h"

namespace pocketplay {
std::unique_ptr<Activity> createLightsOut(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<PocketPlayActivity>(lightsOutGame(), renderer, mappedInput);
}
}
