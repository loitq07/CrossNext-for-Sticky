#include "queens_activity.h"
#include "../../bridge/PocketPlayActivity.h"
#include "queens.h"

namespace pocketplay {
std::unique_ptr<Activity> createQueens(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<PocketPlayActivity>(queensGame(), renderer, mappedInput);
}
}
