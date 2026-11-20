#include "symboldice_activity.h"
#include "../../bridge/PocketPlayActivity.h"
#include "symboldice.h"

namespace pocketplay {
std::unique_ptr<Activity> createSymbolDice(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<PocketPlayActivity>(symbolDiceGame(), renderer, mappedInput);
}
}
