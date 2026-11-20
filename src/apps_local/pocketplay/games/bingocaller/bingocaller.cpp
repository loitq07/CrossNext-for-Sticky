#include "bingocaller.h"

#include <Arduino.h>
#include <esp_random.h>

#include <cstdio>
#include <cstring>

#include "bingocaller_icon.h"
#include "bingocaller_rules.h"
#include "bingocaller_save.h"
#include "core/graphics.h"
#include "core/storage.h"
#include "core/text.h"
#include "ui/ui.h"

namespace bc = bingocaller;

namespace {

constexpr const char* SAVE_FILE = "bingocaller";

// The chooser: one list row per game, as the home page and Settings draw them.
constexpr int ROW_WIDTH = SCREEN_WIDTH - 2 * ui::MARGIN;
constexpr int ROW_LABEL_INSET = 18;

int rowTop(int mode) { return ui::CONTENT_TOP + mode * (ui::ROW_HEIGHT + ui::ROW_GAP); }
constexpr int NOTE_TOP = ui::CONTENT_TOP + bc::kModes * (ui::ROW_HEIGHT + ui::ROW_GAP) + 8;
constexpr int NOTE_HEIGHT = 28;

// The caller, from the top: the last call big in a frame, the four before it, the board of every
// number, then NEW GAME and CALL along the bottom.
constexpr int MARGIN = 16;
constexpr int GAP = 12;
constexpr int INNER_WIDTH = SCREEN_WIDTH - 2 * MARGIN;
constexpr int BIG_TOP = ui::CONTENT_TOP;
constexpr int BIG_HEIGHT = 132;
constexpr int BEFORE_TOP = BIG_TOP + BIG_HEIGHT + 8;
constexpr int BEFORE_HEIGHT = 32;
constexpr int BOARD_TOP = 292;
constexpr int BUTTON_TOP = SCREEN_HEIGHT - MARGIN - ui::PILL_HEIGHT;
constexpr int BOARD_BOTTOM = BUTTON_TOP - GAP;
constexpr int NEW_WIDTH = (INNER_WIDTH - GAP) / 3;
constexpr int CALL_LEFT = MARGIN + NEW_WIDTH + GAP;
constexpr int CALL_WIDTH = INNER_WIDTH - NEW_WIDTH - GAP;
constexpr int BEFORE_COUNT = 4;

static_assert(BEFORE_TOP + BEFORE_HEIGHT <= BOARD_TOP, "the earlier calls must end above the board");

// The board: 30-ball in 6 columns of big numbers, the others in 10 columns (1-10, 11-20 ...,
// as a flashboard reads), with the next cut down so every number has room round it.
struct Board {
  int columns, rows, cellWidth, cellHeight, left;
  Font font;
};

Board boardFor(int balls) {
  Board b{};
  b.columns = balls == 30 ? 6 : 10;
  b.rows = (balls + b.columns - 1) / b.columns;
  b.cellWidth = INNER_WIDTH / b.columns;
  b.cellHeight = (BOARD_BOTTOM - BOARD_TOP) / b.rows;
  b.left = MARGIN + (INNER_WIDTH - b.cellWidth * b.columns) / 2;
  b.font = balls == 30 ? Font::Size30 : Font::Size14;
  return b;
}

class BingoCaller final : public Game {
 public:
  const char* name() const override { return "BINGO CALLER"; }
  const uint8_t* icon() const override { return bingocallerIcon::ICON; }

  void open() override {
    if (!loaded_) {
      loaded_ = true;
      load();
    }
    // A game in play opens straight onto the caller, so a wake carries on where it was.
    screen_ = inPlay_ ? Screen::Caller : Screen::Chooser;
    pendingMode_ = -1;
    newArmed_ = false;
  }

  TapResult onTap(int x, int y) override { return screen_ == Screen::Chooser ? tapChooser(x, y) : tapCaller(x, y); }

  // The Down button calls too, for calling without looking.
  TapResult onDownPressed() override {
    if (screen_ != Screen::Caller) return TapResult::Nothing;
    return callNext();
  }

  void draw(uint8_t* frame) override {
    if (screen_ == Screen::Chooser) {
      drawChooser(frame);
    } else {
      drawCaller(frame);
    }
  }

  bool saveFailed() const override { return saveFailed_; }
  void retrySave() override { persist(); }

 private:
  enum class Screen { Chooser, Caller };

  // ---- The chooser ----

  TapResult tapChooser(int x, int y) {
    if (ui::isOnBack(x, y)) return TapResult::Leave;
    for (int mode = 0; mode < bc::kModes; mode++) {
      if (!ui::isInBox(x, y, ui::MARGIN, rowTop(mode), ROW_WIDTH, ui::ROW_HEIGHT)) continue;
      if (inPlay_ && state_.mode == mode) {
        pendingMode_ = -1;
        screen_ = Screen::Caller;
        return TapResult::Redraw;
      }
      // Another game with calls in it would be lost, so the first tap only asks.
      if (inPlay_ && state_.count > 0 && pendingMode_ != mode) {
        pendingMode_ = mode;
        return TapResult::Redraw;
      }
      bc::start(state_, mode);
      inPlay_ = true;
      pendingMode_ = -1;
      newArmed_ = false;
      screen_ = Screen::Caller;
      persist();
      return TapResult::RedrawFull;
    }
    if (pendingMode_ >= 0) {
      pendingMode_ = -1;
      return TapResult::Redraw;
    }
    return TapResult::Nothing;
  }

  void drawChooser(uint8_t* frame) {
    ui::drawBand(frame, name(), nullptr, true);
    const int cap = capHeight(Font::Size20);
    for (int mode = 0; mode < bc::kModes; mode++) {
      char label[16];
      std::snprintf(label, sizeof(label), "%d BALL", bc::kBalls[mode]);
      ui::drawRow(frame, rowTop(mode), label, nullptr);

      char right[32];
      if (mode == pendingMode_) {
        std::snprintf(right, sizeof(right), "TAP AGAIN");
      } else if (inPlay_ && state_.mode == mode) {
        std::snprintf(right, sizeof(right), "IN PLAY  %d/%d", state_.count, bc::balls(state_));
      } else {
        std::snprintf(right, sizeof(right), "1 TO %d", bc::kBalls[mode]);
      }
      drawText(frame, Font::Size20, ui::MARGIN + ROW_WIDTH - ROW_LABEL_INSET, rowTop(mode) + (ui::ROW_HEIGHT - cap) / 2 + cap, right,
               Align::Right);
    }
    if (pendingMode_ >= 0) {
      char note[64];
      std::snprintf(note, sizeof(note), "THIS ENDS THE %d BALL GAME", bc::balls(state_));
      ui::drawTextLeft(frame, Font::Size14, MARGIN, NOTE_TOP, NOTE_HEIGHT, note);
    }
  }

  // ---- The caller ----

  TapResult tapCaller(int x, int y) {
    if (ui::isOnBack(x, y)) {
      newArmed_ = false;
      screen_ = Screen::Chooser;
      return TapResult::Redraw;
    }
    if (ui::isInBox(x, y, CALL_LEFT, BUTTON_TOP, CALL_WIDTH, ui::PILL_HEIGHT)) return callNext();
    if (ui::isInBox(x, y, MARGIN, BUTTON_TOP, NEW_WIDTH, ui::PILL_HEIGHT)) {
      if (state_.count == 0) return TapResult::Nothing;  // dimmed: nothing to clear
      if (!newArmed_) {
        newArmed_ = true;
        return TapResult::Redraw;
      }
      bc::start(state_, state_.mode);
      newArmed_ = false;
      persist();
      return TapResult::RedrawFull;
    }
    return disarm();
  }

  TapResult callNext() {
    if (bc::finished(state_)) return disarm();  // dimmed: every number is out
    const uint64_t entropy = (static_cast<uint64_t>(esp_random()) << 32) ^ static_cast<uint64_t>(micros());
    bc::call(state_, source_, entropy);
    newArmed_ = false;
    persist();
    // The last number ends the game, so it gets the full refresh.
    return bc::finished(state_) ? TapResult::RedrawFull : TapResult::Redraw;
  }

  // A tap anywhere else takes back a NEW GAME that was waiting for its second tap.
  TapResult disarm() {
    if (!newArmed_) return TapResult::Nothing;
    newArmed_ = false;
    return TapResult::Redraw;
  }

  void drawCaller(uint8_t* frame) {
    char title[16];
    char counter[16];
    std::snprintf(title, sizeof(title), "%d BALL", bc::balls(state_));
    std::snprintf(counter, sizeof(counter), "%d/%d", state_.count, bc::balls(state_));
    ui::drawBand(frame, title, counter, true);

    drawRectOutline(frame, MARGIN, BIG_TOP, INNER_WIDTH, BIG_HEIGHT, ui::RULE);
    const int last = bc::last(state_);
    if (last == 0) {
      drawTextInBox(frame, Font::Size30, MARGIN, BIG_TOP, INNER_WIDTH, BIG_HEIGHT, "TAP CALL");
    } else {
      char big[8];
      const char letter = bc::letter(state_, last);
      if (letter != '\0') {
        std::snprintf(big, sizeof(big), "%c %d", letter, last);
      } else {
        std::snprintf(big, sizeof(big), "%d", last);
      }
      drawTextInBox(frame, Font::Size64, MARGIN, BIG_TOP, INNER_WIDTH, BIG_HEIGHT, big);
    }

    if (state_.count >= 2) {
      char before[64] = "BEFORE:";
      for (int i = state_.count - 2, shown = 0; i >= 0 && shown < BEFORE_COUNT; i--, shown++) {
        char number[8];
        std::snprintf(number, sizeof(number), "   %d", state_.order[i]);
        std::strncat(before, number, sizeof(before) - std::strlen(before) - 1);
      }
      ui::drawTextLeft(frame, Font::Size20, MARGIN + 4, BEFORE_TOP, BEFORE_HEIGHT, before);
    }

    drawBoard(frame, last);

    if (state_.count == 0) {
      ui::drawDimmedPill(frame, MARGIN, BUTTON_TOP, NEW_WIDTH, "NEW GAME");
    } else {
      ui::drawPill(frame, MARGIN, BUTTON_TOP, NEW_WIDTH, newArmed_ ? "SURE?" : "NEW GAME", false);
    }
    if (bc::finished(state_)) {
      ui::drawDimmedPill(frame, CALL_LEFT, BUTTON_TOP, CALL_WIDTH, "ALL CALLED");
    } else {
      ui::drawPill(frame, CALL_LEFT, BUTTON_TOP, CALL_WIDTH, "CALL", true);
    }
  }

  // Called numbers are black squares; the latest is a white square in a thick frame instead, so it
  // stands out from the rest.
  void drawBoard(uint8_t* frame, int last) {
    const Board b = boardFor(bc::balls(state_));
    for (int number = 1; number <= bc::balls(state_); number++) {
      const int x = b.left + ((number - 1) % b.columns) * b.cellWidth;
      const int y = BOARD_TOP + ((number - 1) / b.columns) * b.cellHeight;
      char text[12];
      std::snprintf(text, sizeof(text), "%d", number);
      if (number == last) {
        drawRectOutline(frame, x, y, b.cellWidth + 1, b.cellHeight + 1, ui::FRAME);
        drawTextInBox(frame, b.font, x, y, b.cellWidth, b.cellHeight, text);
      } else if (bc::isCalled(state_, number)) {
        fillRect(frame, x, y, b.cellWidth + 1, b.cellHeight + 1, true);
        drawTextInBox(frame, b.font, x, y, b.cellWidth, b.cellHeight, text, false);
      } else {
        drawRectOutline(frame, x, y, b.cellWidth + 1, b.cellHeight + 1, ui::HAIRLINE);
        drawTextInBox(frame, b.font, x, y, b.cellWidth, b.cellHeight, text);
      }
    }
  }

  // ---- Saves ----

  void load() {
    source_.stir((static_cast<uint64_t>(esp_random()) << 32) | esp_random());
    if (!storage::ready()) return;
    uint8_t data[bc::MAX_SAVE_BYTES];
    uint8_t version = 0;
    size_t size = 0;
    if (!storage::load(SAVE_FILE, version, data, sizeof(data), size)) return;
    if (bc::unpack(version, data, size, state_)) {
      inPlay_ = true;
    } else {
      Serial.println("Ignoring saved Bingo Caller game");
    }
  }

  // Every call is saved as it's made. A game with nothing called has nothing worth keeping, so its
  // save is erased: that's how NEW GAME and a newly chosen game end the old one.
  void persist() {
    if (!storage::ready()) {
      saveFailed_ = false;
      return;
    }
    if (state_.count == 0) {
      saveFailed_ = !storage::erase(SAVE_FILE);
      return;
    }
    uint8_t data[bc::MAX_SAVE_BYTES];
    const size_t size = bc::pack(state_, data);
    saveFailed_ = !storage::save(SAVE_FILE, bc::SAVE_VERSION, data, size);
  }

  bc::State state_{};
  bc::Source source_{};
  Screen screen_ = Screen::Chooser;
  bool inPlay_ = false;
  int pendingMode_ = -1;
  bool newArmed_ = false;
  bool loaded_ = false;
  bool saveFailed_ = false;
};

}  // namespace

Game& bingoCallerGame() {
  static BingoCaller game;
  return game;
}
