#include "pegsolitaire_activity.h"
#include "../../bridge/PocketPlayActivity.h"
#include "pegsolitaire.h"

namespace pocketplay {
std::unique_ptr<Activity> createPegSolitaire(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<PocketPlayActivity>(pegSolitaireGame(), renderer, mappedInput);
}
}
