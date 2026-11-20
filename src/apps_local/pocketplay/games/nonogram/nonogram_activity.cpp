#include "nonogram_activity.h"
#include "../../bridge/PocketPlayActivity.h"
#include "nonogram.h"

namespace pocketplay {
std::unique_ptr<Activity> createNonogram(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<PocketPlayActivity>(nonogramGame(), renderer, mappedInput);
}
}
