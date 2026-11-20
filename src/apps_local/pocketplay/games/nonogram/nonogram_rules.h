#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "nonogram_bank.h"

// Nonograms' rules, translated from the Lua of touch-licorice-apps' apps/nonogram (src/main.lua,
// MIT, LICENSE-licorice.txt), with no screen or card so they can be tested off the device. Fill
// the cells so each row and column shows its clue: the lengths of its runs of filled cells, in
// order. Every bank puzzle can be solved line by line, so it has one solution.
namespace nonogram {

constexpr int kSizes = 4;
constexpr int kSizeList[kSizes] = {5, 8, 10, 12};
constexpr int kMaxN = 12;
constexpr int kMaxCells = kMaxN * kMaxN;
// The most numbers a row or column clue has in each size's bank, so its clues fit the screen.
constexpr int kCap[kSizes] = {3, 4, 4, 5};

enum Cell : uint8_t { Unknown, Filled, Crossed };

inline const char* const* bankFor(int sizeIndex) {
  static const char* const* const kBanks[kSizes] = {kBank5, kBank8, kBank10, kBank12};
  return kBanks[sizeIndex];
}

using Clue = std::vector<uint8_t>;

// The runs of a line, `get(k)` saying whether its k-th cell is filled.
template <typename Get>
Clue clueOf(int n, Get get) {
  Clue out;
  int run = 0;
  for (int k = 0; k < n; k++) {
    if (get(k)) {
      run++;
    } else if (run > 0) {
      out.push_back(static_cast<uint8_t>(run));
      run = 0;
    }
  }
  if (run > 0) out.push_back(static_cast<uint8_t>(run));
  return out;
}

struct Puzzle {
  int n = 10;
  bool solution[kMaxCells] = {};
  Clue rows[kMaxN];
  Clue cols[kMaxN];
};

inline int hexValue(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

// The bank's hex into cells, row by row, and the clues from them.
inline void decode(const char* hex, int n, Puzzle& p) {
  p.n = n;
  for (int i = 0; i < kMaxCells; i++) p.solution[i] = false;
  int bit = 0;
  for (const char* c = hex; *c && bit < n * n; c++) {
    const int v = hexValue(*c);
    for (int b = 3; b >= 0 && bit < n * n; b--, bit++) p.solution[bit] = v >= 0 && ((v >> b) & 1);
  }
  for (int r = 0; r < n; r++) p.rows[r] = clueOf(n, [&](int k) { return p.solution[r * n + k]; });
  for (int c = 0; c < n; c++) p.cols[c] = clueOf(n, [&](int k) { return p.solution[k * n + c]; });
}

constexpr int kMaxUndo = 1000;

struct Board {
  Puzzle puzzle;
  uint8_t cells[kMaxCells] = {};
  struct Step {
    uint8_t cell, was;
  };
  std::vector<Step> history;  // for UNDO, this puzzle only, as in the source
  int last = -1;              // the cell last tapped, whose row and column clues are framed
};

inline int cellCount(const Board& b) { return b.puzzle.n * b.puzzle.n; }

inline bool rowDone(const Board& b, int r) {
  const int n = b.puzzle.n;
  return clueOf(n, [&](int k) { return b.cells[r * n + k] == Filled; }) == b.puzzle.rows[r];
}

inline bool colDone(const Board& b, int c) {
  const int n = b.puzzle.n;
  return clueOf(n, [&](int k) { return b.cells[k * n + c] == Filled; }) == b.puzzle.cols[c];
}

// Solved when the filled cells are the picture; crosses and blanks count alike.
inline bool solved(const Board& b) {
  for (int i = 0; i < cellCount(b); i++) {
    if ((b.cells[i] == Filled) != b.puzzle.solution[i]) return false;
  }
  return true;
}

inline bool blank(const Board& b) {
  for (int i = 0; i < cellCount(b); i++) {
    if (b.cells[i] != Unknown) return false;
  }
  return true;
}

inline void clearCells(Board& b) {
  for (int i = 0; i < kMaxCells; i++) b.cells[i] = Unknown;
  b.history.clear();
  b.last = -1;
}

// A tap: fill (or cross, with `cross`), or clear if it already is that. False if solved.
inline bool tapCell(Board& b, int i, bool cross) {
  if (i < 0 || i >= cellCount(b) || solved(b)) return false;
  const uint8_t want = cross ? Crossed : Filled;
  const uint8_t next = b.cells[i] == want ? static_cast<uint8_t>(Unknown) : want;
  b.last = i;
  if (b.history.size() >= static_cast<size_t>(kMaxUndo)) b.history.erase(b.history.begin());
  b.history.push_back({static_cast<uint8_t>(i), b.cells[i]});
  b.cells[i] = next;
  return true;
}

// A solved board stays as it is, as the picture's reward: CLEAR starts it again.
inline bool undo(Board& b) {
  if (b.history.empty() || solved(b)) return false;
  const Board::Step s = b.history.back();
  b.history.pop_back();
  b.cells[s.cell] = s.was;
  b.last = s.cell;
  return true;
}

}  // namespace nonogram
