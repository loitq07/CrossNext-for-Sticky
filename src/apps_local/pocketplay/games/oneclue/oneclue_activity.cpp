#include "oneclue_activity.h"
#include "../../bridge/PocketPlayActivity.h"
#include "oneclue.h"

namespace pocketplay {
std::unique_ptr<Activity> createOneClue(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<PocketPlayActivity>(oneClueGame(), renderer, mappedInput);
}
}
