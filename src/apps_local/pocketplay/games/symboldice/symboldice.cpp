#include "symboldice.h"

#include <Arduino.h>
#include <esp_random.h>

#include <cstdio>

#include "core/graphics.h"
#include "core/storage.h"
#include "core/text.h"
#include "symboldice_icon.h"
#include "symboldice_rules.h"
#include "symboldice_save.h"
#include "ui/ui.h"

namespace sd = symboldice;

namespace {

constexpr const char* SAVE_FILE = "symboldice";

// The turn capsule and a pip a turn under the band, the two dice in the middle with their
// names under them, and ROLL and RESET along the bottom.
constexpr int MARGIN = 16;
constexpr int GAP = 12;
constexpr int INNER_WIDTH = SCREEN_WIDTH - 2 * MARGIN;
constexpr int STATUS_TOP = ui::CONTENT_TOP;
constexpr int STATUS_HEIGHT = 52;
constexpr int PIP_TOP = STATUS_TOP + STATUS_HEIGHT + 16;
constexpr int PIP_RADIUS = 11;
constexpr int PIP_STEP = INNER_WIDTH / sd::kTurns;
constexpr int DIE_GAP = 24;
constexpr int DIE = (INNER_WIDTH - DIE_GAP) / 2;  // 212: as big as the margins allow
constexpr int DIE_RADIUS = 28;
constexpr int NAME_HEIGHT = 40;
constexpr int BUTTON_TOP_FOR_DICE = SCREEN_HEIGHT - MARGIN - ui::PILL_HEIGHT;
// The dice and their names, centred in the room between the pips and the buttons.
constexpr int DICE_ROOM_TOP = PIP_TOP + 2 * PIP_RADIUS;
constexpr int DIE_TOP = DICE_ROOM_TOP + (BUTTON_TOP_FOR_DICE - DICE_ROOM_TOP - (DIE + 12 + NAME_HEIGHT)) / 2;
constexpr int NAME_TOP = DIE_TOP + DIE + 12;
constexpr int SYMBOL = DIE * 6 / 10;  // the symbol's span, centred in the die
constexpr int STROKE = 14;            // heavy, to read from across a table
constexpr int BUTTON_TOP = SCREEN_HEIGHT - MARGIN - ui::PILL_HEIGHT;
constexpr int BUTTON_WIDTH = (INNER_WIDTH - GAP) / 2;
constexpr int ROLL_LEFT = MARGIN;
constexpr int RESET_LEFT = MARGIN + BUTTON_WIDTH + GAP;

int dieLeft(int d) { return (SCREEN_WIDTH - 2 * DIE - DIE_GAP) / 2 + d * (DIE + DIE_GAP); }

// A straight stroke `width` thick with round ends, from (x0, y0) to (x1, y1): every symbol
// but the circle is made of these.
void drawStroke(uint8_t* frame, int x0, int y0, int x1, int y1, int width) {
  const float r = width / 2.0f;
  const int left = (x0 < x1 ? x0 : x1) - width;
  const int right = (x0 > x1 ? x0 : x1) + width;
  const int top = (y0 < y1 ? y0 : y1) - width;
  const int bottom = (y0 > y1 ? y0 : y1) + width;
  const float dx = static_cast<float>(x1 - x0);
  const float dy = static_cast<float>(y1 - y0);
  const float length2 = dx * dx + dy * dy;
  for (int y = top; y <= bottom; y++) {
    for (int x = left; x <= right; x++) {
      float t = length2 > 0 ? ((x - x0) * dx + (y - y0) * dy) / length2 : 0;
      if (t < 0) t = 0;
      if (t > 1) t = 1;
      const float ex = x - (x0 + t * dx);
      const float ey = y - (y0 + t * dy);
      if (ex * ex + ey * ey <= r * r) setPixel(frame, x, y, true);
    }
  }
}

void drawSymbol(uint8_t* frame, int cx, int cy, uint8_t symbol) {
  const int h = SYMBOL / 2;
  switch (symbol) {
    case sd::Triangle: {
      // Pointing up, its middle on the die's.
      const int top = cy - h + STROKE / 2;
      const int base = cy + h * 7 / 10;
      drawStroke(frame, cx, top, cx - h, base, STROKE);
      drawStroke(frame, cx - h, base, cx + h, base, STROKE);
      drawStroke(frame, cx + h, base, cx, top, STROKE);
      break;
    }
    case sd::Cross:
      drawStroke(frame, cx - h, cy - h, cx + h, cy + h, STROKE);
      drawStroke(frame, cx + h, cy - h, cx - h, cy + h, STROKE);
      break;
    case sd::Circle:
      drawRing(frame, cx, cy, h, STROKE, true);
      break;
    case sd::Hash: {
      const int third = h / 2;
      drawStroke(frame, cx - third, cy - h, cx - third, cy + h, STROKE);
      drawStroke(frame, cx + third, cy - h, cx + third, cy + h, STROKE);
      drawStroke(frame, cx - h, cy - third, cx + h, cy - third, STROKE);
      drawStroke(frame, cx - h, cy + third, cx + h, cy + third, STROKE);
      break;
    }
    case sd::Lines:
      for (int k = -1; k <= 1; k++) drawStroke(frame, cx - h, cy + k * h * 7 / 10, cx + h, cy + k * h * 7 / 10, STROKE);
      break;
    case sd::Slash:
      drawStroke(frame, cx - h, cy + h, cx + h, cy - h, STROKE);
      break;
  }
}

class SymbolDice final : public Game {
 public:
  const char* name() const override { return "SYMBOL DICE"; }
  const uint8_t* icon() const override { return symboldiceIcon::ICON; }

  void open() override {
    if (loaded_) return;
    loaded_ = true;
    load();
  }

  TapResult onTap(int x, int y) override {
    if (ui::isOnBack(x, y)) return TapResult::Leave;
    if (ui::isInBox(x, y, ROLL_LEFT, BUTTON_TOP, BUTTON_WIDTH, ui::PILL_HEIGHT)) return rollDice();
    if (ui::isInBox(x, y, RESET_LEFT, BUTTON_TOP, BUTTON_WIDTH, ui::PILL_HEIGHT)) return resetGame();
    return TapResult::Nothing;
  }

  TapResult onDownPressed() override { return rollDice(); }

  void draw(uint8_t* frame) override {
    ui::drawBand(frame, name(), nullptr, true);

    // Outlined while playing, and solid once the 12th turn is in: a fill that appears once,
    // with the full refresh, marks the end.
    const bool over = sd::over(game_);
    char text[32];
    if (!sd::started(game_)) {
      std::snprintf(text, sizeof(text), "READY: TAP ROLL");
    } else if (over) {
      std::snprintf(text, sizeof(text), "GAME OVER: %d TURNS", sd::kTurns);
    } else {
      std::snprintf(text, sizeof(text), "TURN %d OF %d", game_.turn, sd::kTurns);
    }
    if (over) {
      fillRect(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, true);
    } else {
      drawRectOutline(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, 2);
    }
    // The largest cut that fits, never shortened.
    const Font font = textWidth(Font::Size30, text) <= INNER_WIDTH - 24 ? Font::Size30 : Font::Size20;
    drawTextInBox(frame, font, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, text, !over);

    // A pip a turn, filled once it's taken, so the turn reads at a glance.
    for (int t = 0; t < sd::kTurns; t++) {
      const int cx = MARGIN + t * PIP_STEP + PIP_STEP / 2;
      const int cy = PIP_TOP + PIP_RADIUS;
      drawRing(frame, cx, cy, PIP_RADIUS, t < game_.turn ? PIP_RADIUS : 2, true);
    }

    for (int d = 0; d < sd::kDice; d++) {
      const int left = dieLeft(d);
      if (!sd::started(game_)) {
        // Before the first roll the dice are there, dimmed and blank.
        ui::drawDimmedPill(frame, left, DIE_TOP, DIE, "", DIE);
        continue;
      }
      drawRoundRectOutline(frame, left, DIE_TOP, DIE, DIE, DIE_RADIUS, ui::FRAME);
      drawSymbol(frame, left + DIE / 2, DIE_TOP + DIE / 2, game_.dice[d]);
      drawTextInBox(frame, Font::Size20, left, NAME_TOP, DIE, NAME_HEIGHT, sd::kNames[game_.dice[d]]);
    }

    if (over) {
      ui::drawDimmedPill(frame, ROLL_LEFT, BUTTON_TOP, BUTTON_WIDTH, "ROLL");
    } else {
      ui::drawPill(frame, ROLL_LEFT, BUTTON_TOP, BUTTON_WIDTH, "ROLL", true);
    }
    if (sd::started(game_)) {
      ui::drawPill(frame, RESET_LEFT, BUTTON_TOP, BUTTON_WIDTH, "RESET", false);
    } else {
      ui::drawDimmedPill(frame, RESET_LEFT, BUTTON_TOP, BUTTON_WIDTH, "RESET");
    }
  }

  bool saveFailed() const override { return saveFailed_; }
  void retrySave() override { persist(); }

 private:
  // Both dice together, from the chip's hardware random numbers.
  TapResult rollDice() {
    if (!sd::roll(game_, esp_random())) return TapResult::Nothing;
    persist();
    return sd::over(game_) ? TapResult::RedrawFull : TapResult::Redraw;
  }

  TapResult resetGame() {
    if (!sd::started(game_)) return TapResult::Nothing;
    sd::reset(game_);
    persist();
    return TapResult::RedrawFull;
  }

  void load() {
    if (!storage::ready()) return;
    uint8_t data[sd::SAVE_BYTES];
    uint8_t version = 0;
    size_t size = 0;
    if (storage::load(SAVE_FILE, version, data, sizeof(data), size) && !sd::unpack(version, data, size, game_)) {
      Serial.println("Ignoring saved Symbol Dice game");
    }
  }

  // A game that hasn't been rolled has no save. A finished one keeps its last roll until
  // RESET, so it's still on screen after a wake.
  void persist() {
    if (!storage::ready()) {
      saveFailed_ = false;
      return;
    }
    if (!sd::started(game_)) {
      saveFailed_ = !storage::erase(SAVE_FILE);
      return;
    }
    uint8_t data[sd::SAVE_BYTES];
    const size_t size = sd::pack(game_, data);
    saveFailed_ = !storage::save(SAVE_FILE, sd::SAVE_VERSION, data, size);
  }

  sd::Game game_{};
  bool loaded_ = false;
  bool saveFailed_ = false;
};

}  // namespace

Game& symbolDiceGame() {
  static SymbolDice game;
  return game;
}
