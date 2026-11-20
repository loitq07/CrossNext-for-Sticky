#include "tictactoe.h"

#include <Arduino.h>

#include "core/graphics.h"
#include "core/storage.h"
#include "core/text.h"
#include "tictactoe_icon.h"
#include "tictactoe_rules.h"
#include "tictactoe_save.h"
#include "ui/ui.h"

namespace ttt = tictactoe;

namespace {

constexpr const char* SAVE_FILE = "tictactoe";

// CrossPlay's layout: the status in a capsule under the header, the board, the rules,
// and NEW GAME along the bottom.
constexpr int MARGIN = 16;
constexpr int INNER_WIDTH = SCREEN_WIDTH - 2 * MARGIN;
constexpr int STATUS_TOP = ui::CONTENT_TOP;
constexpr int STATUS_HEIGHT = ui::PILL_HEIGHT;
constexpr int STATUS_BORDER = 2;
constexpr int BOARD_LEFT = MARGIN;
constexpr int BOARD_TOP = STATUS_TOP + STATUS_HEIGHT + 12;
constexpr int CELL_SIZE = INNER_WIDTH / ttt::kSize;
constexpr int BOARD_SIZE = CELL_SIZE * ttt::kSize;
constexpr int GRID_WEIGHT = 4;
constexpr int MARK_WEIGHT = CELL_SIZE / 11;
constexpr int O_RADIUS = CELL_SIZE * 3 / 10;
constexpr int X_REACH = O_RADIUS * 4 / 5;

constexpr int RULES_TOP = BOARD_TOP + BOARD_SIZE + 8;
constexpr int BUTTON_HEIGHT = ui::PILL_HEIGHT;
constexpr int BUTTON_TOP = SCREEN_HEIGHT - MARGIN - BUTTON_HEIGHT;

constexpr const char* RULES[] = {
    "X GOES FIRST. TAP AN EMPTY SQUARE.",
    "THREE IN A LINE WINS.",
    "A FULL BOARD WITH NO LINE IS A DRAW.",
};

const char* statusText(const ttt::Game& game) {
  switch (game.outcome) {
    case ttt::Outcome::Running:
      return game.turn == ttt::kX ? "X TO MOVE" : "O TO MOVE";
    case ttt::Outcome::XWins:
      return "X WINS";
    case ttt::Outcome::OWins:
      return "O WINS";
    case ttt::Outcome::Draw:
      return "A DRAW";
  }
  return "";
}

// There's no line drawing, so each stroke is a square brush stepped along the
// diagonal, which keeps it as heavy as a straight line would be.
void drawX(uint8_t* frame, int centerX, int centerY, bool black) {
  for (int step = -X_REACH; step <= X_REACH; step++) {
    const int left = centerX + step - MARK_WEIGHT / 2;
    fillRect(frame, left, centerY + step - MARK_WEIGHT / 2, MARK_WEIGHT, MARK_WEIGHT, black);
    fillRect(frame, left, centerY - step - MARK_WEIGHT / 2, MARK_WEIGHT, MARK_WEIGHT, black);
  }
}

void drawGrid(uint8_t* frame) {
  for (int i = 1; i < ttt::kSize; i++) {
    const int offset = i * CELL_SIZE - GRID_WEIGHT / 2;
    fillRect(frame, BOARD_LEFT + offset, BOARD_TOP, GRID_WEIGHT, BOARD_SIZE, true);
    fillRect(frame, BOARD_LEFT, BOARD_TOP + offset, BOARD_SIZE, GRID_WEIGHT, true);
  }
}

// The winning three are inverted, not crossed out: a line through three X marks on a
// diagonal runs along one stroke of each and can't be seen.
void drawCell(uint8_t* frame, int cell, uint8_t mark, bool won) {
  const int left = BOARD_LEFT + (cell % ttt::kSize) * CELL_SIZE;
  const int top = BOARD_TOP + (cell / ttt::kSize) * CELL_SIZE;
  if (won) fillRect(frame, left + GRID_WEIGHT, top + GRID_WEIGHT, CELL_SIZE - 2 * GRID_WEIGHT, CELL_SIZE - 2 * GRID_WEIGHT, true);

  const int centerX = left + CELL_SIZE / 2;
  const int centerY = top + CELL_SIZE / 2;
  if (mark == ttt::kX) {
    drawX(frame, centerX, centerY, !won);
  } else if (mark == ttt::kO) {
    drawRing(frame, centerX, centerY, O_RADIUS, MARK_WEIGHT, !won);
  }
}

bool boardIsEmpty(const ttt::Game& game) {
  for (const uint8_t cell : game.cell) {
    if (cell != ttt::kEmpty) return false;
  }
  return true;
}

class TicTacToe final : public Game {
 public:
  const char* name() const override { return "TIC TAC TOE"; }
  const uint8_t* icon() const override { return tictactoeIcon::ICON; }

  // Memory is the truth once loaded, so a game also survives leaving and coming back
  // without a card. A finished game doesn't: coming back starts a new one.
  void open() override {
    if (!loaded_) {
      loaded_ = true;
      ttt::start(game_);
      load();
    } else if (ttt::over(game_)) {
      ttt::start(game_);
    }
  }

  TapResult onTap(int x, int y) override {
    if (ui::isOnBack(x, y)) return TapResult::Leave;

    if (ui::isInBox(x, y, MARGIN, BUTTON_TOP, INNER_WIDTH, BUTTON_HEIGHT)) {
      if (boardIsEmpty(game_)) return TapResult::Nothing;
      ttt::start(game_);
      persist();
      return TapResult::RedrawFull;
    }

    if (!ui::isInBox(x, y, BOARD_LEFT, BOARD_TOP, BOARD_SIZE, BOARD_SIZE)) return TapResult::Nothing;
    const int column = (x - BOARD_LEFT) / CELL_SIZE;
    const int row = (y - BOARD_TOP) / CELL_SIZE;
    if (!ttt::place(game_, row * ttt::kSize + column)) return TapResult::Nothing;

    persist();
    return ttt::over(game_) ? TapResult::RedrawFull : TapResult::Redraw;
  }

  void draw(uint8_t* frame) override {
    ui::drawBand(frame, name(), nullptr, true);

    // Outlined while the game runs, and solid once it's over: a fill that appears
    // once, with the full refresh, is the payoff rather than a cost.
    const bool over = ttt::over(game_);
    if (over) {
      fillRect(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, true);
    } else {
      drawRectOutline(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, STATUS_BORDER);
    }
    drawTextInBox(frame, Font::Size30, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, statusText(game_), !over);

    int line[ttt::kLine] = {-1, -1, -1};
    if (game_.outcome == ttt::Outcome::XWins) ttt::winningLine(game_, ttt::kX, line);
    if (game_.outcome == ttt::Outcome::OWins) ttt::winningLine(game_, ttt::kO, line);

    drawGrid(frame);
    for (int cell = 0; cell < ttt::kCells; cell++) {
      const bool won = cell == line[0] || cell == line[1] || cell == line[2];
      drawCell(frame, cell, game_.cell[cell], won);
    }

    int top = RULES_TOP;
    for (const char* rule : RULES) {
      ui::drawTextLeft(frame, Font::Size14, MARGIN, top, lineHeight(Font::Size14), rule);
      top += lineHeight(Font::Size14);
    }

    ui::drawPill(frame, MARGIN, BUTTON_TOP, INNER_WIDTH, "NEW GAME", true);
  }

  bool saveFailed() const override { return saveFailed_; }

  void retrySave() override { persist(); }

 private:
  void load() {
    if (!storage::ready()) return;

    uint8_t data[ttt::SAVE_BYTES];
    uint8_t version = 0;
    size_t size = 0;
    if (storage::load(SAVE_FILE, version, data, sizeof(data), size) && !ttt::unpackGame(version, data, size, game_)) {
      Serial.println("Ignoring saved Tic Tac Toe game");
    }
  }

  // Only a game in progress has a save. Otherwise it would be offered for resume
  // after it has ended.
  void persist() {
    if (!storage::ready()) {
      saveFailed_ = false;
      return;
    }

    if (ttt::over(game_) || boardIsEmpty(game_)) {
      saveFailed_ = !storage::erase(SAVE_FILE);
      return;
    }

    uint8_t data[ttt::SAVE_BYTES];
    const size_t size = ttt::packGame(game_, data);
    const uint32_t startUs = micros();
    saveFailed_ = !storage::save(SAVE_FILE, ttt::SAVE_VERSION, data, size);
    Serial.printf("Saved %s in %lu us\n", SAVE_FILE, static_cast<unsigned long>(micros() - startUs));
  }

  ttt::Game game_{};
  bool loaded_ = false;
  bool saveFailed_ = false;
};

}  // namespace

Game& ticTacToe() {
  static TicTacToe game;
  return game;
}
