#include "bingocaller_activity.h"
#include "../../bridge/PocketPlayActivity.h"
#include "bingocaller.h"

namespace pocketplay {
std::unique_ptr<Activity> createBingoCaller(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<PocketPlayActivity>(bingoCallerGame(), renderer, mappedInput);
}
}
