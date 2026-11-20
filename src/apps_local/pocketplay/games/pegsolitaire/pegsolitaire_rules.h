#pragma once

#include <cstdint>

// Peg Solitaire's rules, translated from the Lua of touch-licorice-apps' apps/pegsolitaire
// (src/main.lua, MIT, LICENSE-licorice.txt). No screen or card, so they can be tested off the
// device.
//
// Jump a peg over a neighbour into an empty hole; the jumped peg is removed. End with as few
// pegs as possible: one is a win, and one in the centre of the English board the classic
// perfect game. English is 33 holes in a cross, the centre empty, with jumps along rows and
// columns. Triangle is 15 holes, the top empty, with jumps in the six directions of the grid.
namespace pegsolitaire {

constexpr int kMaxCells = 33;
constexpr int kMaxJumps = kMaxCells - 1;  // each jump takes a peg
constexpr int kNoCell = -1;

struct BoardDef {
  const char* name;
  int cells;
  int8_t row[kMaxCells];
  int8_t col[kMaxCells];
  int dirCount;
  int8_t dirs[6][2];
  int start;   // the hole left empty at the start
  int centre;  // the hole a perfect finish ends in, or kNoCell
};

constexpr BoardDef makeEnglish() {
  BoardDef b{"ENGLISH", 0, {}, {}, 4, {{0, 1}, {0, -1}, {1, 0}, {-1, 0}, {0, 0}, {0, 0}}, 0, 0};
  for (int r = 1; r <= 7; r++) {
    for (int c = 1; c <= 7; c++) {
      if ((r >= 3 && r <= 5) || (c >= 3 && c <= 5)) {
        if (r == 4 && c == 4) {
          b.start = b.cells;
          b.centre = b.cells;
        }
        b.row[b.cells] = static_cast<int8_t>(r);
        b.col[b.cells] = static_cast<int8_t>(c);
        b.cells++;
      }
    }
  }
  return b;
}

constexpr BoardDef makeTriangle() {
  BoardDef b{"TRIANGLE", 0, {}, {}, 6, {{0, 1}, {0, -1}, {1, 0}, {-1, 0}, {1, 1}, {-1, -1}}, 0, kNoCell};
  for (int r = 1; r <= 5; r++) {
    for (int c = 1; c <= r; c++) {
      b.row[b.cells] = static_cast<int8_t>(r);
      b.col[b.cells] = static_cast<int8_t>(c);
      b.cells++;
    }
  }
  return b;
}

constexpr int kBoards = 2;
constexpr BoardDef kBoardList[kBoards] = {makeEnglish(), makeTriangle()};

inline int cellAt(const BoardDef& b, int r, int c) {
  for (int i = 0; i < b.cells; i++) {
    if (b.row[i] == r && b.col[i] == c) return i;
  }
  return kNoCell;
}

struct Jump {
  uint8_t from;
  uint8_t over;
  uint8_t to;
};

struct Game {
  uint8_t board = 0;
  uint64_t pegs = 0;  // bit i: a peg in hole i
  Jump history[kMaxJumps] = {};
  int jumps = 0;
};

inline const BoardDef& def(const Game& g) { return kBoardList[g.board]; }
inline bool hasPeg(const Game& g, int i) { return i >= 0 && ((g.pegs >> i) & 1); }

inline void reset(Game& g, int board) {
  g.board = static_cast<uint8_t>(board);
  const BoardDef& b = def(g);
  g.pegs = 0;
  for (int i = 0; i < b.cells; i++) {
    if (i != b.start) g.pegs |= uint64_t{1} << i;
  }
  g.jumps = 0;
}

inline int pegCount(const Game& g) {
  int n = 0;
  for (uint64_t p = g.pegs; p; p &= p - 1) n++;
  return n;
}

// The jumps the peg in hole i can make, into out (up to six); returns how many (jumps_from).
inline int jumpsFrom(const Game& g, int i, Jump* out) {
  if (!hasPeg(g, i)) return 0;
  const BoardDef& b = def(g);
  int n = 0;
  for (int d = 0; d < b.dirCount; d++) {
    const int over = cellAt(b, b.row[i] + b.dirs[d][0], b.col[i] + b.dirs[d][1]);
    const int to = cellAt(b, b.row[i] + 2 * b.dirs[d][0], b.col[i] + 2 * b.dirs[d][1]);
    if (to == kNoCell || over == kNoCell || !hasPeg(g, over) || hasPeg(g, to)) continue;
    out[n++] = {static_cast<uint8_t>(i), static_cast<uint8_t>(over), static_cast<uint8_t>(to)};
  }
  return n;
}

inline bool anyMove(const Game& g) {
  Jump j[6];
  for (int i = 0; i < def(g).cells; i++) {
    if (jumpsFrom(g, i, j) > 0) return true;
  }
  return false;
}

// Jumps the peg in `from` into `to`, if that's one of its jumps.
inline bool jump(Game& g, int from, int to) {
  Jump j[6];
  const int n = jumpsFrom(g, from, j);
  for (int k = 0; k < n; k++) {
    if (j[k].to != to) continue;
    g.pegs &= ~(uint64_t{1} << j[k].from);
    g.pegs &= ~(uint64_t{1} << j[k].over);
    g.pegs |= uint64_t{1} << j[k].to;
    g.history[g.jumps++] = j[k];
    return true;
  }
  return false;
}

inline bool undo(Game& g) {
  if (g.jumps == 0) return false;
  const Jump j = g.history[--g.jumps];
  g.pegs &= ~(uint64_t{1} << j.to);
  g.pegs |= uint64_t{1} << j.over;
  g.pegs |= uint64_t{1} << j.from;
  return true;
}

enum class Rating : uint8_t { Perfect, Excellent, VeryGood, Good, TryAgain };

// How a finished game went (rating).
inline Rating rate(const Game& g) {
  const int n = pegCount(g);
  const int centre = def(g).centre;
  if (n == 1 && centre != kNoCell && hasPeg(g, centre)) return Rating::Perfect;
  if (n == 1) return Rating::Excellent;
  if (n == 2) return Rating::VeryGood;
  if (n <= 4) return Rating::Good;
  return Rating::TryAgain;
}

}  // namespace pegsolitaire
