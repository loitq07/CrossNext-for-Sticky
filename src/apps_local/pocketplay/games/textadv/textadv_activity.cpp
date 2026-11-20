#include "textadv_activity.h"
#include "../../bridge/PocketPlayActivity.h"
#include "textadv.h"

namespace pocketplay {
std::unique_ptr<Activity> createTextAdv(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<PocketPlayActivity>(textAdventuresGame(), renderer, mappedInput);
}
}
