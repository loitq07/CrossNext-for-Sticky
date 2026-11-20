#include "tictactoe_activity.h"
#include "../../bridge/PocketPlayActivity.h"
#include "tictactoe.h"

namespace pocketplay {
std::unique_ptr<Activity> createTicTacToe(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<PocketPlayActivity>(ticTacToe(), renderer, mappedInput);
}
}
