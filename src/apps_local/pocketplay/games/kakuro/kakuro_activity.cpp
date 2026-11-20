#include "kakuro_activity.h"
#include "../../bridge/PocketPlayActivity.h"
#include "kakuro.h"

namespace pocketplay {
std::unique_ptr<Activity> createKakuro(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<PocketPlayActivity>(kakuroGame(), renderer, mappedInput);
}
}
