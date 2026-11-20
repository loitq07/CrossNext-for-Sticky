#include "PocketPlayActivity.h"

#include <cstring>

#include "../Shelf.h"
#include "core/graphics.h"
#include "core/storage.h"

namespace pocketplay {

PocketPlayActivity::PocketPlayActivity(Game& game, GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity(game.name(), renderer, mappedInput), game_(game) {}

PocketPlayActivity::~PocketPlayActivity() {
  if (wasLandscape_) {
    renderer.setOrientation(GfxRenderer::Orientation::Portrait);
    setOrientation(Orientation::Portrait);
  }
}

void PocketPlayActivity::onEnter() {
  Activity::onEnter();
  storage::begin();
  game_.open();
  wasLandscape_ = game_.landscape();
  if (wasLandscape_) {
    renderer.setOrientation(GfxRenderer::Orientation::LandscapeCounterClockwise);
    setOrientation(Orientation::Landscape);
  } else {
    renderer.setOrientation(GfxRenderer::Orientation::Portrait);
    setOrientation(Orientation::Portrait);
  }
  dirty_ = true;
  fullRefresh_ = true;
  requestUpdate();
}

void PocketPlayActivity::onExit() {
  game_.retrySave();
  if (wasLandscape_) {
    renderer.setOrientation(GfxRenderer::Orientation::Portrait);
    setOrientation(Orientation::Portrait);
  }
  Activity::onExit();
}

void PocketPlayActivity::prepareForSleep() {
  game_.retrySave();
}

void PocketPlayActivity::applyResult(Game::TapResult res) {
  switch (res) {
    case Game::TapResult::Nothing:
      break;
    case Game::TapResult::Redraw:
      dirty_ = true;
      requestUpdate();
      break;
    case Game::TapResult::RedrawFull:
      dirty_ = true;
      fullRefresh_ = true;
      requestUpdate();
      break;
    case Game::TapResult::Leave:
      game_.retrySave();
      shelf::leave(renderer, mappedInput);
      break;
    case Game::TapResult::OpenWifi:
      // Offline games ignore or request redraw
      dirty_ = true;
      requestUpdate();
      break;
  }
}

void PocketPlayActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    game_.retrySave();
    shelf::leave(renderer, mappedInput);
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Up)) {
    applyResult(game_.onUpPressed());
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
    applyResult(game_.onDownPressed());
    return;
  }

  int tapX = 0, tapY = 0;
  if (game_.usesHold()) {
    if (mappedInput.wasScreenTouchPressed(tapX, tapY)) {
      applyResult(game_.onPress(tapX, tapY));
    }
    if (mappedInput.wasScreenLongPress(tapX, tapY)) {
      applyResult(game_.onHold(tapX, tapY));
    }
    if (mappedInput.wasScreenTouchReleased()) {
      applyResult(game_.onRelease());
    }
  }

  MappedInputManager::SwipeDir dir = mappedInput.wasSwipe();
  if (dir != MappedInputManager::SwipeDir::None) {
    int sx = 200, sy = 200, ex = 200, ey = 200;
    if (dir == MappedInputManager::SwipeDir::Left) ex = 100;
    else if (dir == MappedInputManager::SwipeDir::Right) ex = 300;
    else if (dir == MappedInputManager::SwipeDir::Up) ey = 100;
    else if (dir == MappedInputManager::SwipeDir::Down) ey = 300;
    applyResult(game_.onSwipe(sx, sy, ex, ey));
    return;
  }

  if (mappedInput.wasScreenTapped(tapX, tapY)) {
    if (game_.usesHold() && mappedInput.tapWasHeldLong()) {
      applyResult(game_.onHold(tapX, tapY));
    } else {
      applyResult(game_.onTap(tapX, tapY));
    }
    return;
  }

  if (game_.update()) {
    dirty_ = true;
    requestUpdate();
  }
}

void PocketPlayActivity::render(RenderLock&& lock) {
  if (!dirty_) return;
  dirty_ = false;

  uint8_t* frame = renderer.getFrameBuffer();
  if (!frame) return;

  // Clear frame to white
  memset(frame, 0xFF, renderer.getBufferSize());

  game_.draw(frame);

  if (fullRefresh_) {
    fullRefresh_ = false;
    renderer.displayBuffer(HalDisplay::FULL_REFRESH);
  } else {
    renderer.displayBuffer(HalDisplay::FAST_REFRESH);
  }
}

}  // namespace pocketplay
