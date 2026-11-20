#include "authoritytracker_activity.h"
#include "../../bridge/PocketPlayActivity.h"
#include "authoritytracker.h"

namespace pocketplay {
std::unique_ptr<Activity> createAuthorityTracker(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<PocketPlayActivity>(authorityTrackerGame(), renderer, mappedInput);
}
}
