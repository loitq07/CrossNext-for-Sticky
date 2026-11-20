#include "diceroller.h"

#include <Arduino.h>
#include <esp_random.h>

#include <cstdio>

#include "core/graphics.h"
#include "core/storage.h"
#include "core/text.h"
#include "diceroller_icon.h"
#include "diceroller_rules.h"
#include "diceroller_save.h"
#include "ui/ui.h"

namespace dr = diceroller;

namespace {

constexpr const char* SAVE_FILE = "diceroller";

// Two big dice side by side, the total in a capsule under them, the roll count under that,
// and RESET and ROLL sharing the bottom row.
constexpr int MARGIN = 16;
constexpr int GAP = 12;
constexpr int INNER_WIDTH = SCREEN_WIDTH - 2 * MARGIN;
constexpr int DIE_GAP = 24;
constexpr int DIE = (INNER_WIDTH - DIE_GAP) / 2;  // 212: as big as the margins allow
constexpr int DIE_RADIUS = 28;
constexpr int PIP_RADIUS = 20;
constexpr int PIP_OFFSET = DIE * 27 / 100;  // a corner pip's distance from the middle, each way
constexpr int TOTAL_HEIGHT = 96;
constexpr int COUNT_HEIGHT = 40;
constexpr int BUTTON_TOP = SCREEN_HEIGHT - MARGIN - ui::PILL_HEIGHT;
constexpr int BUTTON_WIDTH = (INNER_WIDTH - GAP) / 2;
constexpr int RESET_LEFT = MARGIN;
constexpr int ROLL_LEFT = MARGIN + BUTTON_WIDTH + GAP;
// The dice, the total and the count, centred in the room between the band and the buttons.
constexpr int GROUP_HEIGHT = DIE + 2 * GAP + TOTAL_HEIGHT + GAP + COUNT_HEIGHT;
constexpr int DIE_TOP = ui::CONTENT_TOP + (BUTTON_TOP - GAP - ui::CONTENT_TOP - GROUP_HEIGHT) / 2;
constexpr int TOTAL_TOP = DIE_TOP + DIE + 2 * GAP;
constexpr int COUNT_TOP = TOTAL_TOP + TOTAL_HEIGHT + GAP;

int dieLeft(int d) { return (SCREEN_WIDTH - 2 * DIE - DIE_GAP) / 2 + d * (DIE + DIE_GAP); }

// The pips of each face, as (column, row) from -1 to 1: the standard layout.
struct Pip {
  int8_t x, y;
};
constexpr Pip kPips[dr::kFaces][6] = {
    {{0, 0}},
    {{-1, -1}, {1, 1}},
    {{-1, -1}, {0, 0}, {1, 1}},
    {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}},
    {{-1, -1}, {1, -1}, {0, 0}, {-1, 1}, {1, 1}},
    {{-1, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {1, 1}},
};

void drawDie(uint8_t* frame, int left, int top, int value) {
  drawRoundRectOutline(frame, left, top, DIE, DIE, DIE_RADIUS, ui::FRAME);
  const int cx = left + DIE / 2;
  const int cy = top + DIE / 2;
  for (int k = 0; k < value; k++) {
    const Pip& p = kPips[value - 1][k];
    drawRing(frame, cx + p.x * PIP_OFFSET, cy + p.y * PIP_OFFSET, PIP_RADIUS, PIP_RADIUS, true);
  }
}

class DiceRoller final : public Game {
 public:
  const char* name() const override { return "DICE ROLLER"; }
  const uint8_t* icon() const override { return dicerollerIcon::ICON; }

  void open() override {
    if (loaded_) return;
    loaded_ = true;
    load();
  }

  TapResult onTap(int x, int y) override {
    if (ui::isOnBack(x, y)) return TapResult::Leave;
    if (ui::isInBox(x, y, ROLL_LEFT, BUTTON_TOP, BUTTON_WIDTH, ui::PILL_HEIGHT)) return rollDice();
    if (ui::isInBox(x, y, RESET_LEFT, BUTTON_TOP, BUTTON_WIDTH, ui::PILL_HEIGHT)) return resetDice();
    return TapResult::Nothing;
  }

  // The Down button rolls too, for rolling without looking.
  TapResult onDownPressed() override { return rollDice(); }

  void draw(uint8_t* frame) override {
    ui::drawBand(frame, name(), nullptr, true);
    const bool rolled = dr::rolled(roll_);
    for (int d = 0; d < dr::kDice; d++) {
      if (rolled) {
        drawDie(frame, dieLeft(d), DIE_TOP, roll_.dice[d]);
      } else {
        // Before the first roll the dice are there, dimmed and blank.
        ui::drawDimmedPill(frame, dieLeft(d), DIE_TOP, DIE, "", DIE);
      }
    }
    drawRectOutline(frame, MARGIN, TOTAL_TOP, INNER_WIDTH, TOTAL_HEIGHT, 2);
    char text[16];
    if (rolled) {
      std::snprintf(text, sizeof(text), "TOTAL %d", dr::total(roll_));
    } else {
      std::snprintf(text, sizeof(text), "TAP ROLL");
    }
    drawTextInBox(frame, Font::Size30, MARGIN, TOTAL_TOP, INNER_WIDTH, TOTAL_HEIGHT, text);
    std::snprintf(text, sizeof(text), "ROLLS %u", static_cast<unsigned>(roll_.rolls));
    drawTextInBox(frame, Font::Size20, MARGIN, COUNT_TOP, INNER_WIDTH, COUNT_HEIGHT, text);
    // Nothing to reset before the first roll, so RESET is dimmed until then.
    if (rolled) {
      ui::drawPill(frame, RESET_LEFT, BUTTON_TOP, BUTTON_WIDTH, "RESET", false);
    } else {
      ui::drawDimmedPill(frame, RESET_LEFT, BUTTON_TOP, BUTTON_WIDTH, "RESET");
    }
    ui::drawPill(frame, ROLL_LEFT, BUTTON_TOP, BUTTON_WIDTH, "ROLL", true);
  }

  bool saveFailed() const override { return saveFailed_; }
  void retrySave() override { persist(); }

 private:
  // Each roll stirs in the chip's random number and the microsecond of the tap. With the
  // radio off the chip's number alone isn't guaranteed random, but a person's timing is.
  TapResult rollDice() {
    const uint64_t entropy = (static_cast<uint64_t>(esp_random()) << 32) ^ static_cast<uint64_t>(micros());
    dr::roll(roll_, source_, entropy);
    persist();
    return TapResult::Redraw;
  }

  // Blank dice and a count of 0, as at the very start, so it's a full refresh.
  TapResult resetDice() {
    if (!dr::rolled(roll_)) return TapResult::Nothing;
    dr::reset(roll_);
    persist();
    return TapResult::RedrawFull;
  }

  void load() {
    source_.stir((static_cast<uint64_t>(esp_random()) << 32) | esp_random());
    if (!storage::ready()) return;
    uint8_t data[dr::SAVE_BYTES];
    uint8_t version = 0;
    size_t size = 0;
    if (storage::load(SAVE_FILE, version, data, sizeof(data), size) && !dr::unpack(version, data, size, roll_)) {
      Serial.println("Ignoring saved Dice Roller roll");
    }
  }

  // The last roll and the count, so a wake shows the same dice. After a reset there's
  // nothing to keep, so the save is erased and a wake starts blank.
  void persist() {
    if (!storage::ready()) {
      saveFailed_ = false;
      return;
    }
    if (!dr::rolled(roll_)) {
      saveFailed_ = !storage::erase(SAVE_FILE);
      return;
    }
    uint8_t data[dr::SAVE_BYTES];
    const size_t size = dr::pack(roll_, data);
    saveFailed_ = !storage::save(SAVE_FILE, dr::SAVE_VERSION, data, size);
  }

  dr::Roll roll_{};
  dr::Source source_{};
  bool loaded_ = false;
  bool saveFailed_ = false;
};

}  // namespace

Game& diceRollerGame() {
  static DiceRoller game;
  return game;
}
