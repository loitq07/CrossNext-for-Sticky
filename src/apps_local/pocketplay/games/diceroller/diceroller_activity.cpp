#include "diceroller_activity.h"
#include "../../bridge/PocketPlayActivity.h"
#include "diceroller.h"

namespace pocketplay {
std::unique_ptr<Activity> createDiceRoller(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<PocketPlayActivity>(diceRollerGame(), renderer, mappedInput);
}
}
