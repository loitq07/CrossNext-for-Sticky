#pragma once

// Tic Tac Toe, the rules. Freestanding: no renderer, no Activity, no storage.
//
// Three by three, X opens, players alternate. Three of yours in a line wins. A
// full board with no line is a draw.
//
// Cells are numbered 0 to 8, left to right and top to bottom, as they are drawn.

#include <cstdint>

namespace tictactoe {

constexpr int kSize = 3;
constexpr int kCells = kSize * kSize;
constexpr int kLine = 3;
constexpr int kLineCount = 8;

constexpr uint8_t kEmpty = 0;
constexpr uint8_t kX = 1;
constexpr uint8_t kO = 2;

enum class Outcome : uint8_t {
  Running,
  XWins,
  OWins,
  Draw,
};

struct Game {
  uint8_t cell[kCells];
  uint8_t turn;
  Outcome outcome;
};

constexpr int8_t kLines[kLineCount][kLine] = {
    {0, 1, 2}, {3, 4, 5}, {6, 7, 8},  // rows
    {0, 3, 6}, {1, 4, 7}, {2, 5, 8},  // columns
    {0, 4, 8}, {2, 4, 6},             // diagonals
};

inline uint8_t other(const uint8_t side) { return side == kX ? kO : kX; }

inline void start(Game& game) {
  for (int i = 0; i < kCells; ++i) game.cell[i] = kEmpty;
  game.turn = kX;
  game.outcome = Outcome::Running;
}

inline bool over(const Game& game) { return game.outcome != Outcome::Running; }

inline bool canPlace(const Game& game, const int cell) {
  if (over(game) || cell < 0 || cell >= kCells) return false;
  return game.cell[cell] == kEmpty;
}

// Whether `side` holds a whole line, and which. `out` receives the three cells
// when it is not null, and is untouched when there is no line.
inline bool winningLine(const Game& game, const uint8_t side, int* out) {
  for (const auto& line : kLines) {
    if (game.cell[line[0]] != side || game.cell[line[1]] != side || game.cell[line[2]] != side) continue;
    if (out != nullptr) {
      for (int i = 0; i < kLine; ++i) out[i] = line[i];
    }
    return true;
  }
  return false;
}

inline bool boardFull(const Game& game) {
  for (const uint8_t cell : game.cell) {
    if (cell == kEmpty) return false;
  }
  return true;
}

// The only way a mark reaches the board. Returns false and changes nothing when
// the move is not legal. The line is looked for before the full board, because
// a ninth mark can do both and then it is a win. A finished game keeps its turn,
// so `turn` still names the winner.
inline bool place(Game& game, const int cell) {
  if (!canPlace(game, cell)) return false;
  game.cell[cell] = game.turn;
  if (winningLine(game, game.turn, nullptr)) {
    game.outcome = game.turn == kX ? Outcome::XWins : Outcome::OWins;
    return true;
  }
  if (boardFull(game)) {
    game.outcome = Outcome::Draw;
    return true;
  }
  game.turn = other(game.turn);
  return true;
}

}  // namespace tictactoe
