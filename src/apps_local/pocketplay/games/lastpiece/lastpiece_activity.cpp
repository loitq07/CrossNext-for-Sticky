#include "lastpiece_activity.h"
#include "../../bridge/PocketPlayActivity.h"
#include "lastpiece.h"

namespace pocketplay {
std::unique_ptr<Activity> createLastPiece(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<PocketPlayActivity>(lastPieceGame(), renderer, mappedInput);
}
}
