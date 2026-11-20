#include "suguru_activity.h"
#include "../../bridge/PocketPlayActivity.h"
#include "suguru.h"

namespace pocketplay {
std::unique_ptr<Activity> createSuguru(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<PocketPlayActivity>(suguruGame(), renderer, mappedInput);
}
}
