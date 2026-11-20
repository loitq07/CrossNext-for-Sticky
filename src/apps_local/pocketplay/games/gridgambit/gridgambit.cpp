#include "gridgambit.h"

#include <Arduino.h>
#include <esp_random.h>

#include <cstdio>
#include <cstdlib>

#include "core/graphics.h"
#include "core/storage.h"
#include "core/text.h"
#include "gridgambit_flow.h"
#include "gridgambit_icon.h"
#include "gridgambit_pieces.h"
#include "gridgambit_rules.h"
#include "gridgambit_save.h"
#include "ui/ui.h"

namespace gg = gridgambit;
namespace pieces = gridgambitPieces;

namespace {

constexpr const char* SAVE_FILE = "gridgambit";

// The status capsule and the record line under the band, the board, a message line, the
// captured tray, and three buttons along the bottom.
constexpr int MARGIN = 16;
constexpr int GAP = 12;
constexpr int INNER_WIDTH = SCREEN_WIDTH - 2 * MARGIN;
constexpr int STATUS_TOP = ui::CONTENT_TOP;
constexpr int STATUS_HEIGHT = 44;
constexpr int RECORD_TOP = STATUS_TOP + STATUS_HEIGHT + 4;
constexpr int RECORD_HEIGHT = 30;
constexpr int BUTTON_TOP = SCREEN_HEIGHT - MARGIN - ui::PILL_HEIGHT;
constexpr int SLOT = 46;
constexpr int SLOT_GAP = 6;
constexpr int TRAY_TOP = BUTTON_TOP - GAP - SLOT;
constexpr int MESSAGE_HEIGHT = 30;
constexpr int MESSAGE_TOP = TRAY_TOP - 8 - MESSAGE_HEIGHT;
constexpr int CELL = 106;
constexpr int BOARD_SIDE = gg::kSide * CELL + 2 * ui::BOARD_FRAME;
constexpr int BOARD_LEFT = (SCREEN_WIDTH - BOARD_SIDE) / 2;
constexpr int BOARD_TOP = MESSAGE_TOP - 8 - BOARD_SIDE;
constexpr int CELLS_LEFT = BOARD_LEFT + ui::BOARD_FRAME;
constexpr int CELLS_TOP = BOARD_TOP + ui::BOARD_FRAME;
constexpr int BUTTONS = 3;
constexpr int BRACKET = 22;

static_assert(RECORD_TOP + RECORD_HEIGHT <= BOARD_TOP, "the board overlaps the record line");

// The cache that grades puzzles: 8192 slots, 64 KB, which the core puts in PSRAM. Taken once,
// the first time a puzzle is made, and kept.
constexpr uint32_t MEMO_SLOTS = 8192;
constexpr uint32_t MEMO_SLOTS_FALLBACK = 512;

// How long update() works on a puzzle before letting the loop read the buttons.
constexpr unsigned long MAKE_SLICE_MS = 25;

enum Button { UNDO, NEW, LEVEL };

// The level button is as wide as its longest name needs at Size14, so it keeps one cut on
// every level, and UNDO and NEW share the rest. draw() and onTap() both ask.
struct Box {
  int left;
  int width;
};

Box buttonBox(int b) {
  static int levelWidth = 0;
  if (!levelWidth) {
    for (const gg::LevelDef& def : gg::kLevelList) {
      const int w = textWidth(Font::Size14, def.name) + 2 * ui::PILL_RADIUS + 8;
      if (w > levelWidth) levelWidth = w;
    }
  }
  const int other = (INNER_WIDTH - levelWidth - 2 * GAP) / 2;
  if (b == LEVEL) return {MARGIN + INNER_WIDTH - levelWidth, levelWidth};
  return {MARGIN + b * (other + GAP), other};
}
int cellLeft(int i) { return CELLS_LEFT + gg::colOf(i) * CELL; }
int cellTop(int i) { return CELLS_TOP + gg::rowOf(i) * CELL; }

// The cell under a screen position, or kNoCell. draw() places cells with the same sums.
int cellAt(int x, int y) {
  if (x < CELLS_LEFT || y < CELLS_TOP) return gg::kNoCell;
  const int c = (x - CELLS_LEFT) / CELL;
  const int r = (y - CELLS_TOP) / CELL;
  if (c >= gg::kSide || r >= gg::kSide) return gg::kNoCell;
  return r * gg::kSide + c;
}

// Corner brackets inside a cell, for a piece the selected one can take.
void drawBrackets(uint8_t* frame, int left, int top, int side) {
  const int w = ui::FRAME;
  const int in = 6;
  const int l = left + in, t = top + in, r = left + side - in, b = top + side - in;
  fillRect(frame, l, t, BRACKET, w, true);
  fillRect(frame, l, t, w, BRACKET, true);
  fillRect(frame, r - BRACKET, t, BRACKET, w, true);
  fillRect(frame, r - w, t, w, BRACKET, true);
  fillRect(frame, l, b - w, BRACKET, w, true);
  fillRect(frame, l, b - BRACKET, w, BRACKET, true);
  fillRect(frame, r - BRACKET, b - w, BRACKET, w, true);
  fillRect(frame, r - w, b - BRACKET, w, BRACKET, true);
}

void drawTextRight(uint8_t* frame, Font font, int right, int top, int height, const char* text) {
  ui::drawTextLeft(frame, font, right - textWidth(font, text), top, height, text);
}

class GridGambit final : public Game {
 public:
  const char* name() const override { return "GRID GAMBIT"; }
  const uint8_t* icon() const override { return gridgambitIcon::ICON; }

  void open() override {
    if (!loaded_) {
      loaded_ = true;
      rng_ = gg::Rng(esp_random());
      // A solved puzzle isn't offered again: the next one at the same level is made instead.
      // A lost one is kept, so UNDO can still take it back.
      if (!load() || gg::isWon(state_)) startMaking(state_.level);
    }
    state_.sel = gg::kNoCell;
    state_.message = gg::Message::Help;
  }

  TapResult onTap(int x, int y) override {
    if (ui::isOnBack(x, y)) return TapResult::Leave;
    if (making_) return TapResult::Nothing;
    for (int b = 0; b < BUTTONS; b++) {
      const Box box = buttonBox(b);
      if (ui::isInBox(x, y, box.left, BUTTON_TOP, box.width, ui::PILL_HEIGHT)) return tapButton(static_cast<Button>(b));
    }
    const int cell = cellAt(x, y);
    if (cell != gg::kNoCell) return apply(gg::tapCell(state_, cell));
    return apply(gg::tapOff(state_));
  }

  // Makes the puzzle a slice at a time, so BACK still answers while it's being made.
  bool update() override {
    if (!making_) return false;
    if (!memo_.slots && !takeMemo()) return false;
    const unsigned long start = millis();
    while (!maker_.step(rng_, memo_)) {
      if (millis() - start >= MAKE_SLICE_MS) return false;
    }
    making_ = false;
    gg::newPuzzle(state_, maker_.board());
    persist();
    return true;
  }

  bool busy() const override { return making_; }

  void draw(uint8_t* frame) override {
    ui::drawBand(frame, name(), nullptr, true);
    drawStatus(frame);
    drawRecord(frame);
    drawBoard(frame);
    drawTextInBox(frame, Font::Size14, MARGIN, MESSAGE_TOP, INNER_WIDTH, MESSAGE_HEIGHT, messageText());
    drawTray(frame);
    drawButtons(frame);
  }

  bool saveFailed() const override { return saveFailed_; }
  void retrySave() override { persist(); }

 private:
  TapResult tapButton(Button b) {
    switch (b) {
      case UNDO:
        return apply(gg::undoMove(state_));
      case NEW:
        startMaking(state_.level);
        return TapResult::RedrawFull;
      case LEVEL:
        startMaking(gg::nextLevel(state_));
        return TapResult::RedrawFull;
    }
    return TapResult::Nothing;
  }

  // The selection and messages live only in memory; every capture is saved at once.
  TapResult apply(gg::Change change) {
    switch (change) {
      case gg::Change::None:
        return TapResult::Nothing;
      case gg::Change::View:
        return TapResult::Redraw;
      case gg::Change::Move:
        persist();
        return TapResult::Redraw;
      case gg::Change::Full:
        persist();
        return TapResult::RedrawFull;
    }
    return TapResult::Nothing;
  }

  // The level changes at once, so the screen names it while the puzzle is made; the save
  // keeps the old puzzle until the new one is ready.
  void startMaking(int level) {
    state_.level = static_cast<uint8_t>(level);
    state_.sel = gg::kNoCell;
    state_.message = gg::Message::Help;
    maker_.start(rng_, level);
    making_ = true;
  }

  bool takeMemo() {
    for (uint32_t slots : {MEMO_SLOTS, MEMO_SLOTS_FALLBACK}) {
      auto* p = static_cast<uint64_t*>(std::malloc(slots * sizeof(uint64_t)));
      if (p) {
        memo_.slots = p;
        memo_.mask = slots - 1;
        return true;
      }
    }
    Serial.println("Grid Gambit: no memory to make a puzzle");
    return false;
  }

  void drawStatus(uint8_t* frame) const {
    // Outlined while playing, and solid at the end: a fill that appears once, with the full
    // refresh, is the payoff.
    char text[32];
    const bool won = !making_ && gg::isWon(state_);
    const bool lost = !making_ && gg::isLost(state_);
    if (making_) {
      std::snprintf(text, sizeof(text), "MAKING A PUZZLE");
    } else if (won) {
      std::snprintf(text, sizeof(text), "SOLVED!");
    } else if (lost) {
      std::snprintf(text, sizeof(text), "NO CAPTURES LEFT");
    } else {
      const int left = gg::pieceCount(state_.board) - 1;
      std::snprintf(text, sizeof(text), "%d %s TO GO", left, left == 1 ? "CAPTURE" : "CAPTURES");
    }
    const bool solid = won || lost;
    if (solid) {
      fillRect(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, true);
    } else {
      drawRectOutline(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, 2);
    }
    drawTextInBox(frame, Font::Size20, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, text, !solid);
  }

  void drawRecord(uint8_t* frame) const {
    char text[40];
    const unsigned solved = state_.solved[state_.level];
    std::snprintf(text, sizeof(text), "%s: %u SOLVED", gg::kLevelList[state_.level].name, solved);
    ui::drawTextLeft(frame, Font::Size14, MARGIN, RECORD_TOP, RECORD_HEIGHT, text);
    if (!making_) {
      std::snprintf(text, sizeof(text), "%d PIECES", gg::startPieces(state_));
      drawTextRight(frame, Font::Size14, MARGIN + INNER_WIDTH, RECORD_TOP, RECORD_HEIGHT, text);
    }
  }

  // Pieces are outlines and cells hairlines, so a move repaints little black. The selected
  // piece is framed, and the pieces it can take are bracketed.
  void drawBoard(uint8_t* frame) const {
    drawRectOutline(frame, BOARD_LEFT, BOARD_TOP, BOARD_SIDE, BOARD_SIDE, ui::BOARD_FRAME);
    for (int k = 1; k < gg::kSide; k++) {
      fillRect(frame, CELLS_LEFT + k * CELL, CELLS_TOP, ui::HAIRLINE, gg::kSide * CELL, true);
      fillRect(frame, CELLS_LEFT, CELLS_TOP + k * CELL, gg::kSide * CELL, ui::HAIRLINE, true);
    }
    if (making_) return;
    bool target[gg::kCells] = {};
    if (state_.sel != gg::kNoCell) {
      int t[8];
      const int n = gg::capturesFrom(state_.board, state_.sel, t);
      for (int k = 0; k < n; k++) target[t[k]] = true;
    }
    const int inset = (CELL - pieces::BIG) / 2;
    for (int i = 0; i < gg::kCells; i++) {
      const uint8_t p = state_.board.cell[i];
      if (p == gg::None) continue;
      drawIcon(frame, pieces::BIG_PIECES[p], pieces::BIG, pieces::BIG, cellLeft(i) + inset, cellTop(i) + inset);
      if (i == state_.sel) drawRectOutline(frame, cellLeft(i) + 5, cellTop(i) + 5, CELL - 9, CELL - 9, ui::FRAME);
      if (target[i]) drawBrackets(frame, cellLeft(i), cellTop(i), CELL);
    }
  }

  // A slot for every capture the puzzle needs, filled in the order they were made: the
  // player's own progress.
  void drawTray(uint8_t* frame) const {
    if (making_) return;
    const int slots = gg::startPieces(state_) - 1;
    const int width = slots * SLOT + (slots - 1) * SLOT_GAP;
    int left = (SCREEN_WIDTH - width) / 2;
    const int inset = (SLOT - pieces::SMALL) / 2;
    for (int k = 0; k < slots; k++, left += SLOT + SLOT_GAP) {
      if (k < state_.moveCount) {
        drawIcon(frame, pieces::SMALL_PIECES[state_.taken[k]], pieces::SMALL, pieces::SMALL, left + inset, TRAY_TOP + inset);
      } else {
        drawRectOutline(frame, left, TRAY_TOP, SLOT, SLOT, ui::HAIRLINE);
      }
    }
  }

  void drawButtons(uint8_t* frame) const {
    for (int b = 0; b < BUTTONS; b++) {
      const char* label = b == UNDO ? "UNDO" : b == NEW ? "NEW" : gg::kLevelList[state_.level].name;
      const bool can = !making_ && (b != UNDO || gg::canUndo(state_));
      const Box box = buttonBox(b);
      if (!can) {
        ui::drawDimmedPill(frame, box.left, BUTTON_TOP, box.width, label);
      } else {
        // The level button names the level you're on, so it's outlined like a setting.
        ui::drawPill(frame, box.left, BUTTON_TOP, box.width, label, b != LEVEL);
      }
    }
  }

  const char* messageText() const {
    if (making_) return "ONE MOMENT.";
    if (gg::isWon(state_)) return "ONE PIECE LEFT. TAP NEW FOR ANOTHER.";
    if (gg::isLost(state_)) return "NOTHING CAN TAKE: UNDO, OR NEW.";
    switch (state_.message) {
      case gg::Message::Selected:
        return "TAP A BRACKETED PIECE TO TAKE IT.";
      case gg::Message::CannotCapture:
        return "THAT PIECE CAN'T TAKE ANYTHING.";
      case gg::Message::Help:
        break;
    }
    return "EVERY MOVE TAKES. LEAVE ONE PIECE.";
  }

  bool load() {
    if (!storage::ready()) return false;
    uint8_t data[gg::SAVE_MAX];
    uint8_t version = 0;
    size_t size = 0;
    if (!storage::load(SAVE_FILE, version, data, sizeof(data), size)) return false;
    if (!gg::unpack(version, data, size, state_)) {
      Serial.println("Ignoring saved Grid Gambit game");
      return false;
    }
    return true;
  }

  // The save holds the solved counts as well as the puzzle, so it's kept after a solve.
  void persist() {
    if (!storage::ready()) {
      saveFailed_ = false;
      return;
    }
    uint8_t data[gg::SAVE_MAX];
    const size_t size = gg::pack(state_, data);
    saveFailed_ = !storage::save(SAVE_FILE, gg::SAVE_VERSION, data, size);
  }

  gg::State state_{};
  gg::Rng rng_{1};
  gg::Maker maker_;
  gg::Memo memo_;
  bool making_ = false;
  bool loaded_ = false;
  bool saveFailed_ = false;
};

}  // namespace

Game& gridGambitGame() {
  static GridGambit game;
  return game;
}
