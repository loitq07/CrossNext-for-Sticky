#include "nonogram.h"

#include <Arduino.h>

#include <cstdio>
#include <string>

#include "core/graphics.h"
#include "core/storage.h"
#include "core/text.h"
#include "nonogram_flow.h"
#include "nonogram_icon.h"
#include "nonogram_rules.h"
#include "nonogram_save.h"
#include "ui/ui.h"

namespace ng = nonogram;

namespace {

constexpr const char* SAVE_FILE = "nonograms";
constexpr const char* GRIDS_BLOB = "nonograms-grids";

// From the top: the status capsule, the clues and grid, a message line, and five pills.
constexpr int MARGIN = 16;
constexpr int GAP = 12;
constexpr int INNER_WIDTH = SCREEN_WIDTH - 2 * MARGIN;
constexpr int STATUS_TOP = ui::CONTENT_TOP;
constexpr int STATUS_HEIGHT = 44;
constexpr int BOARD_TOP = STATUS_TOP + STATUS_HEIGHT + 10;
constexpr int BUTTON_TOP = SCREEN_HEIGHT - MARGIN - ui::PILL_HEIGHT;
constexpr int MESSAGE_HEIGHT = 30;
constexpr int MESSAGE_TOP = BUTTON_TOP - 6 - MESSAGE_HEIGHT;
constexpr int BOARD_BOTTOM = MESSAGE_TOP - 8;
constexpr int BUTTONS = 5;
constexpr int BUTTON_GAP = 8;
constexpr int BUTTON_WIDTH = (INNER_WIDTH - (BUTTONS - 1) * BUTTON_GAP) / BUTTONS;
constexpr int CLUE_PAD = 8;
constexpr int THICK = 3;       // the outside and every fifth line

enum Button { MODE, UNDO, CLEAR, NEXT, SIZE };

int buttonLeft(int b) { return MARGIN + b * (BUTTON_WIDTH + BUTTON_GAP); }

// Where the board goes for a size, which draw() and the taps both ask for. The clue room is
// sized once per size to the widest row clue in its bank, so nothing moves between puzzles.
struct Layout {
  int n = 10;
  int cell = 0;
  int gridLeft = 0;
  int gridTop = 0;
  int clueWidth = 0;
  int clueHeight = 0;
  Font clueFont = Font::Size14;
  int clueStep = 22;  // a column clue number's height
};

std::string clueText(const ng::Clue& clue) {
  std::string s;
  for (size_t k = 0; k < clue.size(); k++) {
    if (k) s += " ";
    s += std::to_string(clue[k]);
  }
  return s;
}

// The board for a size with its clues in one cut.
Layout layoutWith(int size, Font font, int step) {
  Layout l;
  l.n = ng::kSizeList[size];
  l.clueFont = font;
  l.clueStep = step;
  int widest = 0;
  for (int k = 0; k < ng::kBankPuzzles; k++) {
    ng::Puzzle p;
    ng::decode(ng::bankFor(size)[k], l.n, p);
    for (int r = 0; r < l.n; r++) {
      const int w = textWidth(font, clueText(p.rows[r]).c_str());
      if (w > widest) widest = w;
    }
  }
  l.clueWidth = widest + 2 * CLUE_PAD;
  l.clueHeight = ng::kCap[size] * step + CLUE_PAD;
  const int byWidth = (INNER_WIDTH - l.clueWidth) / l.n;
  const int byHeight = (BOARD_BOTTOM - BOARD_TOP - l.clueHeight) / l.n;
  l.cell = byWidth < byHeight ? byWidth : byHeight;
  return l;
}

// Size20 clues where the cells stay big enough with them (5x5 and 8x8), else Size14.
constexpr int MIN_CELL_FOR_BIG_CLUES = 40;

const Layout& layoutFor(int size) {
  static Layout cache[ng::kSizes];
  Layout& l = cache[size];
  if (l.cell > 0) return l;
  l = layoutWith(size, Font::Size20, 28);
  if (l.cell < MIN_CELL_FOR_BIG_CLUES) l = layoutWith(size, Font::Size14, 22);
  const int total = l.clueWidth + l.n * l.cell;
  l.gridLeft = (SCREEN_WIDTH - total) / 2 + l.clueWidth;
  // Any height left over goes above and below, so the board sits in the middle of its room.
  const int spare = BOARD_BOTTOM - BOARD_TOP - l.clueHeight - l.n * l.cell;
  l.gridTop = BOARD_TOP + spare / 2 + l.clueHeight;
  return l;
}

int cellAt(const Layout& l, int x, int y) {
  const int side = l.n * l.cell;
  if (x < l.gridLeft || y < l.gridTop || x >= l.gridLeft + side || y >= l.gridTop + side) return -1;
  return ((y - l.gridTop) / l.cell) * l.n + (x - l.gridLeft) / l.cell;
}

void drawCross(uint8_t* frame, int left, int top, int size) {
  for (int k = 0; k < size; k++) {
    for (int o = 0; o < 2; o++) {
      setPixel(frame, left + k + o, top + k, true);
      setPixel(frame, left + size - 1 - k + o, top + k, true);
    }
  }
}

class Nonograms final : public Game {
 public:
  const char* name() const override { return "NONOGRAMS"; }
  const uint8_t* icon() const override { return nonogramIcon::ICON; }
  bool usesHold() const override { return true; }

  void open() override {
    if (!loaded_) {
      loaded_ = true;
      load();
      ng::loadPuzzle(state_);
      return;
    }
    state_.view = ng::View::Board;
  }

  TapResult onTap(int x, int y) override {
    if (ui::isOnBack(x, y)) {
      if (state_.view != ng::View::Board) return apply(ng::closePage(state_));
      return TapResult::Leave;
    }
    switch (state_.view) {
      case ng::View::ConfirmClear:
        if (ui::isInBox(x, y, buttonLeftOfTwo(0), BUTTON_TOP, TWO_WIDTH, ui::PILL_HEIGHT)) return apply(ng::closePage(state_));
        if (ui::isInBox(x, y, buttonLeftOfTwo(1), BUTTON_TOP, TWO_WIDTH, ui::PILL_HEIGHT)) return apply(ng::confirmClear(state_));
        return TapResult::Nothing;
      case ng::View::Sizes:
        for (int s = 0; s < ng::kSizes; s++) {
          if (ui::isInBox(x, y, ui::MARGIN, sizeRowTop(s), SCREEN_WIDTH - 2 * ui::MARGIN, ui::ROW_HEIGHT)) {
            return apply(ng::chooseSize(state_, s));
          }
        }
        return TapResult::Nothing;
      case ng::View::Board:
        break;
    }
    for (int b = 0; b < BUTTONS; b++) {
      if (!ui::isInBox(x, y, buttonLeft(b), BUTTON_TOP, BUTTON_WIDTH, ui::PILL_HEIGHT)) continue;
      switch (static_cast<Button>(b)) {
        case MODE:
          return apply(ng::toggleMode(state_));
        case UNDO:
          return apply(ng::undoTap(state_));
        case CLEAR:
          return apply(ng::askClear(state_));
        case NEXT:
          return apply(ng::stepPuzzle(state_, 1));
        case SIZE:
          return apply(ng::openSizes(state_));
      }
    }
    return apply(ng::tapAt(state_, cellAt(layoutFor(state_.size), x, y), false));
  }

  // The cell under a finger is marked while a hold builds, as Minesweeper's are.
  TapResult onPress(int x, int y) override {
    if (state_.view != ng::View::Board || ng::solved(state_.board)) return TapResult::Nothing;
    const int cell = cellAt(layoutFor(state_.size), x, y);
    if (cell < 0) return TapResult::Nothing;
    pressed_ = cell;
    return TapResult::Redraw;
  }

  // A hold always marks an X, whatever FILL / X says, as in the source.
  TapResult onHold(int x, int y) override {
    if (state_.view != ng::View::Board) return TapResult::Nothing;
    pressed_ = -1;
    return apply(ng::tapAt(state_, cellAt(layoutFor(state_.size), x, y), true));
  }

  TapResult onRelease() override {
    if (pressed_ < 0) return TapResult::Nothing;
    pressed_ = -1;
    return TapResult::Redraw;
  }

  // The source's side buttons: the previous and next puzzle.
  TapResult onUpPressed() override { return apply(ng::stepPuzzle(state_, -1)); }
  TapResult onDownPressed() override { return apply(ng::stepPuzzle(state_, 1)); }

  void draw(uint8_t* frame) override {
    switch (state_.view) {
      case ng::View::ConfirmClear:
        drawConfirm(frame);
        return;
      case ng::View::Sizes:
        drawSizes(frame);
        return;
      case ng::View::Board:
        break;
    }
    ui::drawBand(frame, name(), nullptr, true);
    const int n = ng::sizeN(state_);
    const bool won = ng::solved(state_.board);
    char text[48];
    if (won) {
      std::snprintf(text, sizeof(text), "SOLVED!   %dX%d   %d/%d", n, n, state_.place[state_.size] + 1, ng::kBankPuzzles);
      fillRect(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, true);
    } else {
      std::snprintf(text, sizeof(text), "%dX%d   %d/%d   SOLVED %d", n, n, state_.place[state_.size] + 1, ng::kBankPuzzles,
                    ng::doneCount(state_, state_.size));
      drawRectOutline(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, 2);
    }
    drawTextInBox(frame, Font::Size20, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, text, !won);

    drawBoard(frame);
    drawTextInBox(frame, Font::Size20, MARGIN, MESSAGE_TOP, INNER_WIDTH, MESSAGE_HEIGHT, messageText());

    char sizeLabel[8];
    std::snprintf(sizeLabel, sizeof(sizeLabel), "%dX%d", n, n);
    const char* labels[BUTTONS] = {state_.cross ? "X" : "FILL", "UNDO", "CLEAR", "NEXT", sizeLabel};
    for (int b = 0; b < BUTTONS; b++) {
      const bool can = b == UNDO ? ng::canUndo(state_) : b == CLEAR ? ng::canClear(state_) : true;
      if (can) {
        ui::drawPill(frame, buttonLeft(b), BUTTON_TOP, BUTTON_WIDTH, labels[b], true);
      } else {
        ui::drawDimmedPill(frame, buttonLeft(b), BUTTON_TOP, BUTTON_WIDTH, labels[b]);
      }
    }
  }

  bool saveFailed() const override { return saveFailed_; }
  void retrySave() override { persist(); }

 private:
  static constexpr int TWO_WIDTH = (INNER_WIDTH - GAP) / 2;
  static int buttonLeftOfTwo(int k) { return MARGIN + k * (TWO_WIDTH + GAP); }
  static int sizeRowTop(int s) { return ui::CONTENT_TOP + s * (ui::ROW_HEIGHT + ui::ROW_GAP); }

  TapResult apply(ng::Change change) {
    switch (change) {
      case ng::Change::None:
        return TapResult::Nothing;
      case ng::Change::View:
        return TapResult::Redraw;
      case ng::Change::Move:
        persist();
        return TapResult::Redraw;
      case ng::Change::NewPuzzle:
      case ng::Change::Solved:
        persist();
        return TapResult::RedrawFull;
    }
    return TapResult::Nothing;
  }

  const char* messageText() const {
    switch (state_.message) {
      case ng::Message::Help:
        return state_.cross ? "TAP: X OR CLEAR. HOLD: X." : "TAP: FILL OR CLEAR. HOLD: X.";
      case ng::Message::FillMode:
        return "TAPS FILL CELLS.";
      case ng::Message::CrossMode:
        return "TAPS MARK CELLS EMPTY (X).";
      case ng::Message::Solved:
        return "SOLVED! NEXT FOR ANOTHER.";
    }
    return "";
  }

  void drawBoard(uint8_t* frame) {
    const ng::Board& b = state_.board;
    const Layout& l = layoutFor(state_.size);
    const int n = l.n;
    const int c = l.cell;
    const int side = n * c;
    const int lastRow = b.last >= 0 ? b.last / n : -1;
    const int lastCol = b.last >= 0 ? b.last % n : -1;

    // Row clues right-aligned beside the grid, struck through once the row matches.
    for (int r = 0; r < n; r++) {
      const std::string s = clueText(b.puzzle.rows[r]);
      const int top = l.gridTop + r * c;
      const int w = textWidth(l.clueFont, s.c_str());
      const int right = l.gridLeft - CLUE_PAD;
      ui::drawTextLeft(frame, l.clueFont, right - w, top, c, s.c_str());
      if (ng::rowDone(b, r)) fillRect(frame, right - w - 3, top + c / 2 - 1, w + 6, 2, true);
      if (r == lastRow) drawRectOutline(frame, l.gridLeft - l.clueWidth, top + 1, l.clueWidth - 3, c - 2, 2);
    }
    // Column clues stacked above, the last number next to the grid.
    for (int col = 0; col < n; col++) {
      const ng::Clue& clue = b.puzzle.cols[col];
      const int left = l.gridLeft + col * c;
      const int count = static_cast<int>(clue.size());
      for (int k = 0; k < count; k++) {
        const int top = l.gridTop - CLUE_PAD / 2 - (count - k) * l.clueStep;
        drawTextInBox(frame, l.clueFont, left, top, c, l.clueStep, std::to_string(clue[static_cast<size_t>(k)]).c_str());
      }
      if (ng::colDone(b, col)) fillRect(frame, left + c / 2 - 1, l.gridTop - CLUE_PAD / 2 - count * l.clueStep, 2, count * l.clueStep, true);
      if (col == lastCol) drawRectOutline(frame, left + 1, l.gridTop - l.clueHeight, c - 2, l.clueHeight - 3, 2);
    }

    // Grid lines, every fifth and the outside thicker.
    for (int k = 0; k <= n; k++) {
      const int w = (k % 5 == 0 || k == n) ? THICK : ui::HAIRLINE;
      const int off = w == THICK ? 1 : 0;
      fillRect(frame, l.gridLeft + k * c - off, l.gridTop - 1, w, side + 2, true);
      fillRect(frame, l.gridLeft - 1, l.gridTop + k * c - off, side + 2, w, true);
    }

    // Filled cells are squares a little inside the cell: less black ghosts less.
    const int inset = c / 6 > 3 ? c / 6 : 3;
    for (int i = 0; i < n * n; i++) {
      const int x = l.gridLeft + (i % n) * c;
      const int y = l.gridTop + (i / n) * c;
      if (b.cells[i] == ng::Filled) {
        fillRect(frame, x + inset, y + inset, c - 2 * inset + 1, c - 2 * inset + 1, true);
      } else if (b.cells[i] == ng::Crossed) {
        const int a = inset + 2;
        drawCross(frame, x + a, y + a, c - 2 * a);
      }
      if (i == pressed_) drawRectOutline(frame, x + 3, y + 3, c - 5, c - 5, 2);
    }
  }

  void drawConfirm(uint8_t* frame) {
    ui::drawBand(frame, "CLEAR", nullptr, true);
    const int n = ng::sizeN(state_);
    char text[96];
    std::snprintf(text, sizeof(text), "CLEAR EVERY CELL OF %dX%d PUZZLE %d? IT CAN'T BE UNDONE.", n, n, state_.place[state_.size] + 1);
    const int h = wrappedHeight(Font::Size20, INNER_WIDTH, text);
    drawWrapped(frame, Font::Size20, MARGIN, ui::CONTENT_TOP + (BUTTON_TOP - ui::CONTENT_TOP - h) / 2, INNER_WIDTH, text, Align::Center);
    ui::drawPill(frame, buttonLeftOfTwo(0), BUTTON_TOP, TWO_WIDTH, "KEEP", false);
    ui::drawPill(frame, buttonLeftOfTwo(1), BUTTON_TOP, TWO_WIDTH, "CLEAR", true);
  }

  // Each size with how many you've solved, the one you're on framed.
  void drawSizes(uint8_t* frame) {
    ui::drawBand(frame, "BOARD SIZE", nullptr, true);
    const int width = SCREEN_WIDTH - 2 * ui::MARGIN;
    for (int s = 0; s < ng::kSizes; s++) {
      const int top = sizeRowTop(s);
      char label[8];
      std::snprintf(label, sizeof(label), "%dX%d", ng::kSizeList[s], ng::kSizeList[s]);
      ui::drawRow(frame, top, label, nullptr);
      if (s == state_.size) drawRectOutline(frame, ui::MARGIN, top, width, ui::ROW_HEIGHT, ui::FRAME);
      char text[24];
      std::snprintf(text, sizeof(text), "SOLVED %d/%d", ng::doneCount(state_, s), ng::kBankPuzzles);
      const int w = textWidth(Font::Size14, text);
      ui::drawTextLeft(frame, Font::Size14, ui::MARGIN + width - 14 - w, top, ui::ROW_HEIGHT, text);
    }
  }

  void load() {
    if (!storage::ready()) return;
    uint8_t data[ng::SAVE_BYTES];
    uint8_t version = 0;
    size_t size = 0;
    if (storage::load(SAVE_FILE, version, data, sizeof(data), size) && !ng::unpackState(version, data, size, state_)) {
      Serial.println("Ignoring saved Nonograms state");
    }
    const storage::Blob blob = storage::loadBlob(GRIDS_BLOB, size);
    if (blob && !ng::unpackGrids(blob.get(), size, state_)) Serial.println("Ignoring saved Nonograms grids");
  }

  // Both, on every change: the half-done grids hold the cells on the board.
  void persist() {
    if (!storage::ready()) {
      saveFailed_ = false;
      return;
    }
    uint8_t data[ng::SAVE_BYTES];
    const size_t size = ng::packState(state_, data);
    const std::vector<uint8_t> grids = ng::packGrids(state_);
    const bool stateSaved = storage::save(SAVE_FILE, ng::SAVE_VERSION, data, size);
    const bool gridsSaved = storage::saveBlob(GRIDS_BLOB, grids.data(), grids.size());
    saveFailed_ = !stateSaved || !gridsSaved;
  }

  ng::State state_{};
  int pressed_ = -1;
  bool loaded_ = false;
  bool saveFailed_ = false;
};

}  // namespace

Game& nonogramGame() {
  static Nonograms game;
  return game;
}
