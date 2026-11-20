#include "gridgambit_activity.h"
#include "../../bridge/PocketPlayActivity.h"
#include "gridgambit.h"

namespace pocketplay {
std::unique_ptr<Activity> createGridGambit(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<PocketPlayActivity>(gridGambitGame(), renderer, mappedInput);
}
}
