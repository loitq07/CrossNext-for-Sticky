#pragma once

#include <memory>
#include "../../activities/Activity.h"
#include "games/game.h"

namespace pocketplay {

class PocketPlayActivity final : public Activity {
 public:
  PocketPlayActivity(Game& game, GfxRenderer& renderer, MappedInputManager& mappedInput);
  ~PocketPlayActivity() override;

  bool allowsEdgeBack() const override { return true; }
  void onEnter() override;
  void onExit() override;
  void prepareForSleep() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  Game& game_;
  bool dirty_ = true;
  bool fullRefresh_ = true;
  bool wasLandscape_ = false;

  void applyResult(Game::TapResult res);
};

}  // namespace pocketplay
