#include "polyominopanic_activity.h"
#include "../../bridge/PocketPlayActivity.h"
#include "polyominopanic.h"

namespace pocketplay {
std::unique_ptr<Activity> createPolyominoPanic(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<PocketPlayActivity>(polyominoPanicGame(), renderer, mappedInput);
}
}
