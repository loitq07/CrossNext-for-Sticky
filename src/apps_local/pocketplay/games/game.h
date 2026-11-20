#pragma once

#include <cstdint>

// What every game provides. To add one, follow src/games/README.md, and follow the
// save rules in CLAUDE.md.
class Game {
 public:
  enum class TapResult {
    Nothing,     // nothing changed, so nothing is redrawn
    Redraw,      // a move: fast refresh
    RedrawFull,  // a game ending or starting: full refresh
    Leave,       // BACK: return to the home page
    OpenWifi,    // show the Wi-Fi screen, then come back and call onWifiClosed()
  };

  // Shown on the home page, and saved to reopen this game after waking. Changing
  // it means a game that was open at sleep wakes on the home page once instead.
  virtual const char* name() const = 0;

  // The home page's icon for this game: 32 x 32 in ui::ICON_SIZE's format, or
  // nullptr for none.
  virtual const uint8_t* icon() const { return nullptr; }

  // Called each time the game is entered. Picks up its save, or starts fresh.
  virtual void open() = 0;

  virtual TapResult onTap(int x, int y) = 0;
  virtual void draw(uint8_t* frame) = 0;

  virtual bool saveFailed() const = 0;
  virtual void retrySave() = 0;  // called before sleep if saveFailed()

  // For a game with work going on between taps. Called every loop while it's on
  // screen, and returns true if it needs redrawing.
  virtual bool update() { return false; }

  // True stops the idle timer, for work that a sleep would throw away.
  virtual bool busy() const { return false; }

  // Back from OpenWifi. network::state() says whether it joined.
  virtual void onWifiClosed() {}

  // True draws the game in landscape, 800 wide by 480 tall (core/graphics.h), and
  // taps arrive in the same frame. The shell turns the screen back for everything
  // else, including the Wi-Fi screen and the sleep screen.
  virtual bool landscape() const { return false; }

  // For a game that acts on a held finger, such as planting a flag. True sends it
  // the three calls below, and a hold then swallows its own lift, so the onTap a
  // short press would have given never comes. Other games only ever get onTap, so
  // a slow tap still taps. Positions are screen positions, as onTap's.
  virtual bool usesHold() const { return false; }
  // A finger went down, before anything is known about it. For showing what's
  // under it while a hold builds.
  virtual TapResult onPress(int /*x*/, int /*y*/) { return TapResult::Nothing; }
  // The finger stayed still for HOLD_MS (main.cpp), at the place it went down.
  virtual TapResult onHold(int /*x*/, int /*y*/) { return TapResult::Nothing; }
  // The press is over: lifted, moved too far to be a tap or a hold, or held. Comes
  // before the onTap a lift gives.
  virtual TapResult onRelease() { return TapResult::Nothing; }

  // The Up and Down buttons, while the game is open, such as for turning pages. A
  // game that doesn't use them ignores them.
  virtual TapResult onUpPressed() { return TapResult::Nothing; }
  virtual TapResult onDownPressed() { return TapResult::Nothing; }

  // A finger drawn across the screen: the SDK's swipe, at least 60 px within 700 ms,
  // from where it went down to where it lifted, in screen positions as onTap's. A
  // swipe is never also a tap. A game that doesn't use swipes ignores them.
  virtual TapResult onSwipe(int /*fromX*/, int /*fromY*/, int /*toX*/, int /*toY*/) { return TapResult::Nothing; }

 protected:
  ~Game() = default;
};
