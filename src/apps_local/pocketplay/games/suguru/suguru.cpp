#include "suguru.h"

#include <Arduino.h>

#include <cstdio>

#include "core/graphics.h"
#include "core/storage.h"
#include "core/text.h"
#include "suguru_flow.h"
#include "suguru_icon.h"
#include "suguru_rules.h"
#include "suguru_save.h"
#include "ui/ui.h"

namespace sg = suguru;

namespace {

constexpr const char* SAVE_FILE = "suguru";

// The status capsule under the band, the board, the 1-5 pad, a line of help, and four
// buttons along the bottom. The board is sized for the bank, but everything under it
// sits where the largest board would leave it, so the pad never moves between banks.
constexpr int MARGIN = 16;
constexpr int GAP = 12;
constexpr int INNER_WIDTH = SCREEN_WIDTH - 2 * MARGIN;
constexpr int STATUS_TOP = ui::CONTENT_TOP;
constexpr int STATUS_HEIGHT = 44;
constexpr int BOARD_TOP = STATUS_TOP + STATUS_HEIGHT + 8;
constexpr int GRID_ROOM = INNER_WIDTH - 2 * ui::BOARD_FRAME;  // 430
constexpr int REGION_RULE = 5;
constexpr int CLASH_INSET = 3;   // clear of a region border's half inside the cell
constexpr int SELECT_INSET = 4;  // a pixel of white between a region border and a bracket
constexpr int PAD_TOP = BOARD_TOP + INNER_WIDTH + GAP;
constexpr int PAD_KEYS = sg::kMaxDigit;
constexpr int PAD_WIDTH = (INNER_WIDTH - (PAD_KEYS - 1) * GAP) / PAD_KEYS;
constexpr int PAD_HEIGHT = ui::PILL_HEIGHT;
constexpr int MESSAGE_TOP = PAD_TOP + PAD_HEIGHT + 8;
constexpr int BUTTON_TOP = SCREEN_HEIGHT - MARGIN - ui::PILL_HEIGHT;
constexpr int BUTTONS = 4;
constexpr int BUTTON_WIDTH = (INNER_WIDTH - (BUTTONS - 1) * GAP) / BUTTONS;

constexpr const char* HELP_TEXT = "TAP A SQUARE, THEN A NUMBER.";
constexpr const char* CELL_FIRST_TEXT = "TAP A SQUARE FIRST.";
constexpr const char* GIVEN_TEXT = "THAT SQUARE IS A GIVEN.";
constexpr const char* SOLVED_TEXT = "SOLVED! NEXT FOR ANOTHER.";

enum Button { ERASE, CHECK, NEXT, BANK };

// Where the board goes for a grid of n, which draw() and onTap() both ask for, so the
// two can't drift apart.
struct Layout {
  int n = 5;
  int cell = GRID_ROOM / 5;
  int gridLeft = 0;
  int gridTop = BOARD_TOP + ui::BOARD_FRAME;
};

Layout layoutFor(int n) {
  Layout l;
  l.n = n;
  l.cell = GRID_ROOM / n;
  const int outer = l.cell * n + 2 * ui::BOARD_FRAME;
  l.gridLeft = (SCREEN_WIDTH - outer) / 2 + ui::BOARD_FRAME;
  l.gridTop = BOARD_TOP + ui::BOARD_FRAME;
  return l;
}

// Corner brackets round a square, black or white: a selection that leaves the number
// in the middle of the cell clear, however small the cell.
void brackets(uint8_t* frame, int left, int top, int side, bool black) {
  const int w = ui::RULE;
  const int l = side / 4;
  const int far = left + side;
  const int bottom = top + side;
  fillRect(frame, left, top, l, w, black);
  fillRect(frame, left, top, w, l, black);
  fillRect(frame, far - l, top, l, w, black);
  fillRect(frame, far - w, top, w, l, black);
  fillRect(frame, left, bottom - w, l, w, black);
  fillRect(frame, left, bottom - l, w, l, black);
  fillRect(frame, far - l, bottom - w, l, w, black);
  fillRect(frame, far - w, bottom - l, w, l, black);
}

int padLeft(int key) { return MARGIN + key * (PAD_WIDTH + GAP); }
int buttonLeft(int b) { return MARGIN + b * (BUTTON_WIDTH + GAP); }

class Suguru final : public Game {
 public:
  const char* name() const override { return "SUGURU"; }
  const uint8_t* icon() const override { return suguruIcon::ICON; }

  // Memory is the truth once loaded. A solved puzzle stays until NEXT, as in the source.
  void open() override {
    if (!loaded_) {
      loaded_ = true;
      sg::loadPuzzle(state_);
      load();
    }
    state_.message = sg::Message::Help;
  }

  TapResult onTap(int x, int y) override {
    if (ui::isOnBack(x, y)) return TapResult::Leave;

    for (int b = 0; b < BUTTONS; b++) {
      if (ui::isInBox(x, y, buttonLeft(b), BUTTON_TOP, BUTTON_WIDTH, ui::PILL_HEIGHT)) return tapButton(static_cast<Button>(b));
    }
    for (int k = 0; k < PAD_KEYS; k++) {
      if (ui::isInBox(x, y, padLeft(k), PAD_TOP, PAD_WIDTH, PAD_HEIGHT)) return apply(sg::setCell(state_, static_cast<uint8_t>(k + 1)));
    }
    const Layout l = layoutFor(state_.puzzle.n);
    const int side = l.cell * l.n;
    if (!ui::isInBox(x, y, l.gridLeft, l.gridTop, side, side)) return TapResult::Nothing;
    const int cell = ((y - l.gridTop) / l.cell) * l.n + (x - l.gridLeft) / l.cell;
    return apply(sg::select(state_, cell));
  }

  // The source's side buttons: the previous and next puzzle in the bank.
  TapResult onUpPressed() override { return apply(sg::step(state_, -1)); }
  TapResult onDownPressed() override { return apply(sg::step(state_, 1)); }

  void draw(uint8_t* frame) override {
    ui::drawBand(frame, name(), nullptr, true);

    // Outlined while playing, and solid once solved: a fill that appears once, with
    // the full refresh, is the payoff rather than a cost.
    const sg::Bank& bank = sg::kBankList[state_.bank];
    char status[40];
    std::snprintf(status, sizeof(status), "%s   %d/%d", state_.solved ? "SOLVED" : bank.label,
                  state_.place[state_.bank] + 1, sg::kBankPuzzles);
    if (state_.solved) {
      fillRect(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, true);
    } else {
      drawRectOutline(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, 2);
    }
    drawTextInBox(frame, Font::Size20, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, status, !state_.solved);

    drawBoard(frame);

    const bool canEnter = sg::canEnter(state_);
    for (int k = 0; k < PAD_KEYS; k++) {
      const char label[2] = {static_cast<char>('1' + k), '\0'};
      const int left = padLeft(k);
      // Pills like the buttons below, with the number in the board's own cut.
      if (canEnter) {
        ui::drawPill(frame, left, PAD_TOP, PAD_WIDTH, "", false, PAD_HEIGHT);
      } else {
        ui::drawDimmedPill(frame, left, PAD_TOP, PAD_WIDTH, "", PAD_HEIGHT);
      }
      drawTextInBox(frame, Font::Size30, left, PAD_TOP, PAD_WIDTH, PAD_HEIGHT, label);
    }

    drawTextInBox(frame, Font::Size14, MARGIN, MESSAGE_TOP, INNER_WIDTH, BUTTON_TOP - MESSAGE_TOP, messageText());

    static const char* const LABELS[BUTTONS] = {"ERASE", nullptr, "NEXT", "BANK"};
    for (int b = 0; b < BUTTONS; b++) {
      const char* label = b == CHECK ? (state_.check ? "CHECK ON" : "CHECK OFF") : LABELS[b];
      if (b == ERASE && !sg::canErase(state_)) {
        ui::drawDimmedPill(frame, buttonLeft(b), BUTTON_TOP, BUTTON_WIDTH, label);
      } else {
        // CHECK is a setting, not an action, so it's outlined.
        ui::drawPill(frame, buttonLeft(b), BUTTON_TOP, BUTTON_WIDTH, label, b != CHECK);
      }
    }
  }

  bool saveFailed() const override { return saveFailed_; }
  void retrySave() override { persist(); }

 private:
  TapResult tapButton(Button b) {
    switch (b) {
      case ERASE:
        return apply(sg::setCell(state_, 0));
      case CHECK:
        return apply(sg::toggleCheck(state_));
      case NEXT:
        return apply(sg::step(state_, 1));
      case BANK:
        return apply(sg::nextBank(state_));
    }
    return TapResult::Nothing;
  }

  // Selecting and messages live only in memory; everything else is saved at once.
  TapResult apply(sg::Change change) {
    switch (change) {
      case sg::Change::None:
        return TapResult::Nothing;
      case sg::Change::View:
        return TapResult::Redraw;
      case sg::Change::Move:
        persist();
        return TapResult::Redraw;
      case sg::Change::NewPuzzle:
      case sg::Change::Solved:
        persist();
        return TapResult::RedrawFull;
    }
    return TapResult::Nothing;
  }

  const char* messageText() const {
    if (state_.solved) return SOLVED_TEXT;
    switch (state_.message) {
      case sg::Message::CellFirst:
        return CELL_FIRST_TEXT;
      case sg::Message::Given:
        return GIVEN_TEXT;
      case sg::Message::Help:
        break;
    }
    return HELP_TEXT;
  }

  void drawBoard(uint8_t* frame) {
    const Layout l = layoutFor(state_.puzzle.n);
    const int n = l.n;
    const int c = l.cell;
    const int side = c * n;
    const char* regions = state_.puzzle.regions;

    drawRectOutline(frame, l.gridLeft - ui::BOARD_FRAME, l.gridTop - ui::BOARD_FRAME, side + 2 * ui::BOARD_FRAME,
                    side + 2 * ui::BOARD_FRAME, ui::BOARD_FRAME);
    for (int k = 1; k < n; k++) {
      fillRect(frame, l.gridLeft + k * c, l.gridTop, ui::HAIRLINE, side, true);
      fillRect(frame, l.gridLeft, l.gridTop + k * c, side, ui::HAIRLINE, true);
    }
    // Thick borders between regions, centred on the cell line and run on past the
    // corners so that they join.
    const int half = REGION_RULE / 2;
    for (int r = 0; r < n; r++) {
      for (int col = 0; col < n; col++) {
        const int i = r * n + col;
        const int x = l.gridLeft + col * c;
        const int y = l.gridTop + r * c;
        if (col < n - 1 && regions[i] != regions[i + 1]) fillRect(frame, x + c - half, y - half, REGION_RULE, c + REGION_RULE, true);
        if (r < n - 1 && regions[i] != regions[i + n]) fillRect(frame, x - half, y + c - half, c + REGION_RULE, REGION_RULE, true);
      }
    }

    bool bad[sg::kMaxCells] = {};
    if (state_.check) sg::conflicts(state_.puzzle, state_.cells, bad);
    const int cap = capHeight(Font::Size30);
    for (int i = 0; i < n * n; i++) {
      const int x = l.gridLeft + (i % n) * c;
      const int y = l.gridTop + (i / n) * c;
      // A number that breaks a rule turns its cell black, as a clash does in Sudoku:
      // black is only ever the player's own trouble, and it reads from across the room.
      const uint8_t v = state_.cells[i];
      const bool clash = v != 0 && bad[i];
      if (clash) fillRect(frame, x + CLASH_INSET, y + CLASH_INSET, c - 2 * CLASH_INSET, c - 2 * CLASH_INSET, true);
      // The brackets sit just inside the region borders, and turn white on a black cell.
      if (i == state_.sel) brackets(frame, x + SELECT_INSET, y + SELECT_INSET, c - 2 * SELECT_INSET, !clash);
      if (v == 0) continue;
      const bool ink = !clash;
      const char label[2] = {static_cast<char>('0' + v), '\0'};
      drawTextInBox(frame, Font::Size30, x, y, c, c, label, ink);
      if (!sg::isGiven(state_, i)) {
        // Your own numbers are underlined, as in the source; givens aren't.
        fillRect(frame, x + c / 2 - 9, y + (c + cap) / 2 + 3, 18, 2, ink);
      }
    }
  }

  void load() {
    if (!storage::ready()) return;
    uint8_t data[sg::SAVE_MAX];
    uint8_t version = 0;
    size_t size = 0;
    if (storage::load(SAVE_FILE, version, data, sizeof(data), size) && !sg::unpack(version, data, size, state_)) {
      Serial.println("Ignoring saved Suguru grid");
    }
  }

  // The save is your place in every bank, so it outlasts a solved puzzle.
  void persist() {
    if (!storage::ready()) {
      saveFailed_ = false;
      return;
    }
    uint8_t data[sg::SAVE_MAX];
    const size_t size = sg::pack(state_, data);
    saveFailed_ = !storage::save(SAVE_FILE, sg::SAVE_VERSION, data, size);
  }

  sg::State state_{};
  bool loaded_ = false;
  bool saveFailed_ = false;
};

}  // namespace

Game& suguruGame() {
  static Suguru game;
  return game;
}
