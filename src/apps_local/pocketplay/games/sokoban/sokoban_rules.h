#pragma once

#include <cstdint>

#include "sokoban_levels.h"

// Sokoban's rules, translated from the Lua of touch-licorice-apps' apps/sokoban (index.lua,
// MIT, LICENSE-licorice.txt), itself after CPR-vCodex's SokobanGame. No screen or card, so they
// can be tested off the device.
//
// The player moves one square at a time. A box in the way is pushed when the square behind it
// is free; a wall, or a second box, stops the move. The level is solved when no box is off a
// goal. Undo takes back one move, with its push.
namespace sokoban {

constexpr int kMaxRows = 20;
constexpr int kMaxCols = 32;
constexpr int kMaxCells = kMaxRows * kMaxCols;
static_assert(kLargestRows <= kMaxRows && kLargestCols <= kMaxCols, "a level is bigger than the board");

// Moves Undo can take back, and the save replays. Past it the player has to undo or restart,
// which no real solution comes near: the longest Sasquatch solutions are a few hundred moves.
constexpr int kMaxHistory = 8000;

enum class Dir : uint8_t { Left, Right, Up, Down };

// '#' wall, ' ' floor, '.' goal, '$' box, '*' box on a goal. The player is kept apart.
struct Board {
  int rows = 0;
  int cols = 0;
  char cells[kMaxCells] = {};
  int player = 0;
};

struct Game {
  Board board;
  uint16_t moves = 0;
  uint16_t pushes = 0;
  // One entry a move: the direction in bits 0-1, and bit 2 set when it pushed a box.
  uint8_t history[kMaxHistory] = {};
  uint16_t count = 0;
};

inline bool isBox(char c) { return c == '$' || c == '*'; }

inline void build(const LevelDef& def, Board& out) {
  out.rows = def.rows;
  out.cols = def.cols;
  out.player = 0;
  for (int i = 0; i < def.rows * def.cols; i++) {
    char c = def.cells[i];
    if (c == '@') {
      out.player = i;
      c = ' ';
    } else if (c == '+') {
      out.player = i;
      c = '.';
    }
    out.cells[i] = c;
  }
}

inline void start(const LevelDef& def, Game& game) {
  build(def, game.board);
  game.moves = 0;
  game.pushes = 0;
  game.count = 0;
}

// The square one step from i, or -1 off the board, which counts as wall.
inline int neighbour(const Board& b, int i, Dir d) {
  int r = i / b.cols;
  int c = i % b.cols;
  switch (d) {
    case Dir::Left: c--; break;
    case Dir::Right: c++; break;
    case Dir::Up: r--; break;
    case Dir::Down: r++; break;
  }
  if (r < 0 || r >= b.rows || c < 0 || c >= b.cols) return -1;
  return r * b.cols + c;
}

inline char at(const Board& b, int i) { return i < 0 ? '#' : b.cells[i]; }

inline bool solved(const Board& b) {
  for (int i = 0; i < b.rows * b.cols; i++) {
    if (b.cells[i] == '$') return false;
  }
  return true;
}

inline Dir opposite(Dir d) {
  switch (d) {
    case Dir::Left: return Dir::Right;
    case Dir::Right: return Dir::Left;
    case Dir::Up: return Dir::Down;
    case Dir::Down: return Dir::Up;
  }
  return d;
}

enum class Step : uint8_t { Blocked, Moved, Pushed };

// One step on the board alone (step).
inline Step step(Board& b, Dir d) {
  const int next = neighbour(b, b.player, d);
  const char dest = at(b, next);
  if (dest == '#') return Step::Blocked;
  bool pushed = false;
  if (isBox(dest)) {
    const int behind = neighbour(b, next, d);
    const char there = at(b, behind);
    if (there == '#' || isBox(there)) return Step::Blocked;
    b.cells[behind] = there == '.' ? '*' : '$';
    b.cells[next] = dest == '*' ? '.' : ' ';
    pushed = true;
  }
  b.player = next;
  return pushed ? Step::Pushed : Step::Moved;
}

// Takes one step back (unstep): the player returns, and a pushed box comes with them.
inline void unstep(Board& b, Dir d, bool pushed) {
  const int from = b.player;
  const int back = neighbour(b, from, opposite(d));
  if (pushed) {
    const int box = neighbour(b, from, d);
    b.cells[box] = b.cells[box] == '*' ? '.' : ' ';
    b.cells[from] = b.cells[from] == '.' ? '*' : '$';
  }
  b.player = back;
}

// A move by the player, kept for Undo. False when blocked, solved or the history is full.
inline bool move(Game& g, Dir d) {
  if (solved(g.board) || g.count >= kMaxHistory) return false;
  const Step s = step(g.board, d);
  if (s == Step::Blocked) return false;
  g.moves++;
  if (s == Step::Pushed) g.pushes++;
  g.history[g.count++] = static_cast<uint8_t>(static_cast<uint8_t>(d) | (s == Step::Pushed ? 4 : 0));
  return true;
}

inline bool undo(Game& g) {
  if (g.count == 0) return false;
  const uint8_t entry = g.history[--g.count];
  const bool pushed = (entry & 4) != 0;
  unstep(g.board, static_cast<Dir>(entry & 3), pushed);
  g.moves--;
  if (pushed) g.pushes--;
  return true;
}

}  // namespace sokoban
