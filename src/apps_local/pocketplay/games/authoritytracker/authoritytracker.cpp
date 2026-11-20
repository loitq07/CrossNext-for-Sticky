#include "authoritytracker.h"

#include <Arduino.h>

#include <cstdio>

#include "authoritytracker_icon.h"
#include "authoritytracker_rules.h"
#include "authoritytracker_save.h"
#include "core/graphics.h"
#include "core/storage.h"
#include "core/text.h"
#include "ui/ui.h"

namespace at = authoritytracker;

namespace {

constexpr const char* SAVE_FILE = "authoritytracker";

// Two halves, one a player, split by a rule: the name, the score, a row of + and a row
// of -. RESET runs along the bottom.
constexpr int MARGIN = 16;
constexpr int GAP = 12;
constexpr int INNER_WIDTH = SCREEN_WIDTH - 2 * MARGIN;
constexpr int BUTTON_TOP = SCREEN_HEIGHT - MARGIN - ui::PILL_HEIGHT;  // 732
constexpr int CONTENT_BOTTOM = BUTTON_TOP - GAP;                      // 720
constexpr int DIVIDER_TOP = (ui::CONTENT_TOP + CONTENT_BOTTOM - ui::RULE) / 2;
constexpr int LABEL_HEIGHT = 40;
constexpr int STEP_COLUMNS = 3;
constexpr int STEP_WIDTH = (INNER_WIDTH - (STEP_COLUMNS - 1) * GAP) / STEP_COLUMNS;  // 141
constexpr int kSteps[STEP_COLUMNS] = {1, 5, 10};
constexpr const char* kPlusLabels[STEP_COLUMNS] = {"+1", "+5", "+10"};
constexpr const char* kMinusLabels[STEP_COLUMNS] = {"-1", "-5", "-10"};

// A half's top and bottom. The first runs from the content top to a gap above the
// rule, and the second from a gap below the rule to the content bottom.
int halfTop(int player) { return player == 0 ? ui::CONTENT_TOP : DIVIDER_TOP + ui::RULE + GAP; }
int halfBottom(int player) { return player == 0 ? DIVIDER_TOP - GAP : CONTENT_BOTTOM; }

// Both rows of buttons sit at the bottom of the half, - under +.
int minusTop(int player) { return halfBottom(player) - ui::PILL_HEIGHT; }
int plusTop(int player) { return minusTop(player) - GAP - ui::PILL_HEIGHT; }
int scoreTop(int player) { return halfTop(player) + LABEL_HEIGHT; }
int scoreHeight(int player) { return plusTop(player) - GAP - scoreTop(player); }
int stepLeft(int column) { return MARGIN + column * (STEP_WIDTH + GAP); }

const char* playerName(int player) { return player == 0 ? "PLAYER 1" : "PLAYER 2"; }

class AuthorityTracker final : public Game {
 public:
  const char* name() const override { return "AUTHORITY TRACKER"; }
  const uint8_t* icon() const override { return authoritytrackerIcon::ICON; }

  void open() override {
    armed_ = false;
    if (loaded_) return;
    loaded_ = true;
    load();
  }

  TapResult onTap(int x, int y) override {
    if (ui::isOnBack(x, y)) return TapResult::Leave;
    if (ui::isInBox(x, y, MARGIN, BUTTON_TOP, INNER_WIDTH, ui::PILL_HEIGHT)) return tapReset();

    // Anything but RESET takes back its first tap.
    const bool wasArmed = armed_;
    armed_ = false;
    for (int p = 0; p < at::kPlayers; p++) {
      for (int c = 0; c < STEP_COLUMNS; c++) {
        if (ui::isInBox(x, y, stepLeft(c), plusTop(p), STEP_WIDTH, ui::PILL_HEIGHT)) return step(p, kSteps[c], wasArmed);
        if (ui::isInBox(x, y, stepLeft(c), minusTop(p), STEP_WIDTH, ui::PILL_HEIGHT)) return step(p, -kSteps[c], wasArmed);
      }
    }
    return wasArmed ? TapResult::Redraw : TapResult::Nothing;
  }

  void draw(uint8_t* frame) override {
    ui::drawBand(frame, name(), nullptr, true);
    const int loser = at::loser(state_);
    for (int p = 0; p < at::kPlayers; p++) drawHalf(frame, p, loser);
    fillRect(frame, MARGIN, DIVIDER_TOP, INNER_WIDTH, ui::RULE, true);

    const char* label = armed_ ? "TAP AGAIN TO RESET" : "RESET";
    if (at::atStart(state_)) {
      ui::drawDimmedPill(frame, MARGIN, BUTTON_TOP, INNER_WIDTH, label);
    } else {
      ui::drawPill(frame, MARGIN, BUTTON_TOP, INNER_WIDTH, label, true);
    }
  }

  bool saveFailed() const override { return saveFailed_; }
  void retrySave() override { persist(); }

 private:
  void drawHalf(uint8_t* frame, int p, int loser) {
    const int top = halfTop(p);
    if (loser >= 0 && loser != p) {
      // The winner's name becomes a solid capsule, drawn with the game-over blink.
      char text[24];
      std::snprintf(text, sizeof(text), "%s WINS", playerName(p));
      const int width = textWidth(Font::Size20, text) + 2 * ui::PILL_RADIUS;
      fillRoundRect(frame, MARGIN, top, width, LABEL_HEIGHT, LABEL_HEIGHT / 2, true);
      drawTextInBox(frame, Font::Size20, MARGIN, top, width, LABEL_HEIGHT, text, false);
    } else {
      ui::drawTextLeft(frame, Font::Size20, MARGIN, top, LABEL_HEIGHT, playerName(p));
    }

    char score[8];
    std::snprintf(score, sizeof(score), "%d", state_.score[p]);
    drawTextInBox(frame, Font::Size64, MARGIN, scoreTop(p), INNER_WIDTH, scoreHeight(p), score);

    for (int c = 0; c < STEP_COLUMNS; c++) {
      drawStep(frame, stepLeft(c), plusTop(p), kPlusLabels[c], at::canApply(state_, p, kSteps[c]));
      drawStep(frame, stepLeft(c), minusTop(p), kMinusLabels[c], at::canApply(state_, p, -kSteps[c]));
    }
  }

  static void drawStep(uint8_t* frame, int left, int top, const char* label, bool live) {
    if (live) {
      ui::drawPill(frame, left, top, STEP_WIDTH, label, false);
    } else {
      ui::drawDimmedPill(frame, left, top, STEP_WIDTH, label);
    }
  }

  TapResult step(int player, int delta, bool wasArmed) {
    const bool overBefore = at::loser(state_) >= 0;
    if (!at::apply(state_, player, delta)) return wasArmed ? TapResult::Redraw : TapResult::Nothing;
    persist();
    // Reaching 0, or taking it back, is the game ending or restarting: blink.
    return (at::loser(state_) >= 0) != overBefore ? TapResult::RedrawFull : TapResult::Redraw;
  }

  // During a game RESET asks twice, because one stray tap would lose it. At the start
  // there's nothing to reset, and after the end it goes at once.
  TapResult tapReset() {
    if (at::atStart(state_)) return TapResult::Nothing;
    if (at::loser(state_) < 0 && !armed_) {
      armed_ = true;
      return TapResult::Redraw;
    }
    armed_ = false;
    at::reset(state_);
    persist();
    return TapResult::RedrawFull;
  }

  void load() {
    if (!storage::ready()) return;
    uint8_t data[at::SAVE_BYTES];
    uint8_t version = 0;
    size_t size = 0;
    if (storage::load(SAVE_FILE, version, data, sizeof(data), size) && !at::unpack(version, data, size, state_)) {
      Serial.println("Ignoring saved Authority Tracker scores");
    }
  }

  // Both scores, after every change. A finished game stays saved, so the loser can
  // still take back a tap after a sleep; RESET is the way to start again.
  void persist() {
    if (!storage::ready()) {
      saveFailed_ = false;
      return;
    }
    uint8_t data[at::SAVE_BYTES];
    const size_t size = at::pack(state_, data);
    saveFailed_ = !storage::save(SAVE_FILE, at::SAVE_VERSION, data, size);
  }

  at::State state_{};
  bool armed_ = false;
  bool loaded_ = false;
  bool saveFailed_ = false;
};

}  // namespace

Game& authorityTrackerGame() {
  static AuthorityTracker game;
  return game;
}
