#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "kakuro_bank.h"

// Kakuro's rules, translated from the Lua of touch-licorice-apps' apps/kakuro (src/main.lua, MIT,
// LICENSE-licorice.txt), with no screen or card so they can be tested off the device. Fill the
// white cells with digits 1 to 9: each run of white cells, across or down, adds up to the clue at
// its start, and no digit is used twice in a run. A bank line is the solution; the clues are
// worked out from it.
namespace kakuro {

constexpr int kLevels = 3;
constexpr int kMaxN = 8;
constexpr int kMaxCells = kMaxN * kMaxN;

struct Level {
  const char* const* lines;
  int n;
  const char* label;  // the capsule's
  const char* menu;   // the level page's row
};

inline const Level& level(int k) {
  static const Level kList[kLevels] = {
      {kEasy, 6, "EASY 6X6", "EASY 6X6"},
      {kMedium, 7, "MEDIUM 7X7", "MEDIUM 7X7"},
      {kHard, 8, "HARD 8X8", "HARD 8X8"},
  };
  return kList[k];
}

struct Run {
  std::vector<uint8_t> cells;
  int sum = 0;
  bool across = true;
};

struct Puzzle {
  int n = 6;
  uint8_t solution[kMaxCells] = {};  // 0 for a block
  std::vector<Run> runs;
  int across[kMaxCells];  // the run each white cell is in, or -1
  int down[kMaxCells];
  int clueAcross[kMaxCells];  // a block's clues, or 0
  int clueDown[kMaxCells];
  bool outside[kMaxCells] = {};  // an empty block joined to the edge: not drawn
};

inline bool white(const Puzzle& p, int i) { return p.solution[i] != 0; }

// A bank line into the puzzle, its runs and clues. False if the line isn't one.
inline bool parse(const char* line, int n, Puzzle& p) {
  p.n = n;
  p.runs.clear();
  for (int i = 0; i < kMaxCells; i++) {
    p.solution[i] = 0;
    p.across[i] = p.down[i] = -1;
    p.clueAcross[i] = p.clueDown[i] = 0;
    p.outside[i] = false;
  }
  for (int i = 0; i < n * n; i++) {
    const char c = line[i];
    if (c == '#') continue;
    if (c < '1' || c > '9') return false;
    p.solution[i] = static_cast<uint8_t>(c - '0');
  }
  if (line[n * n] != 0) return false;
  for (int i = 0; i < n * n; i++) {
    if (white(p, i)) continue;
    for (int dir = 0; dir < 2; dir++) {
      const bool across = dir == 0;
      Run run;
      run.across = across;
      int r = i / n + (across ? 0 : 1);
      int c = i % n + (across ? 1 : 0);
      while (r < n && c < n && white(p, r * n + c)) {
        run.cells.push_back(static_cast<uint8_t>(r * n + c));
        run.sum += p.solution[r * n + c];
        r += across ? 0 : 1;
        c += across ? 1 : 0;
      }
      if (run.cells.size() < 2) continue;
      const int index = static_cast<int>(p.runs.size());
      for (uint8_t x : run.cells) (across ? p.across : p.down)[x] = index;
      (across ? p.clueAcross : p.clueDown)[i] = run.sum;
      p.runs.push_back(run);
    }
  }
  // Empty blocks reached from the edge through other empty blocks aren't drawn, so the puzzle
  // stands on blank paper; an enclosed one keeps its box, or it would look like a cell to fill.
  auto empty = [&](int i) { return !white(p, i) && p.clueAcross[i] == 0 && p.clueDown[i] == 0; };
  std::vector<int> stack;
  for (int i = 0; i < n * n; i++) {
    const int r = i / n;
    const int c = i % n;
    if ((r == 0 || c == 0 || r == n - 1 || c == n - 1) && empty(i)) {
      p.outside[i] = true;
      stack.push_back(i);
    }
  }
  while (!stack.empty()) {
    const int i = stack.back();
    stack.pop_back();
    const int r = i / n;
    const int c = i % n;
    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};
    for (int k = 0; k < 4; k++) {
      const int rr = r + dr[k];
      const int cc = c + dc[k];
      if (rr < 0 || cc < 0 || rr >= n || cc >= n) continue;
      const int j = rr * n + cc;
      if (empty(j) && !p.outside[j]) {
        p.outside[j] = true;
        stack.push_back(j);
      }
    }
  }
  return true;
}

// The ways to make a run, as digit strings like "79" for 16 in 2, smallest first.
inline std::vector<std::string> combos(int length, int sum) {
  std::vector<std::string> out;
  std::string acc;
  auto rec = [&](auto&& self, int start, int left, int total) -> void {
    if (left == 0) {
      if (total == sum) out.push_back(acc);
      return;
    }
    for (int d = start; d <= 9 && total + d <= sum; d++) {
      acc.push_back(static_cast<char>('0' + d));
      self(self, d + 1, left - 1, total + d);
      acc.pop_back();
    }
  };
  rec(rec, 1, length, 0);
  return out;
}

struct Board {
  Puzzle puzzle;
  uint8_t entry[kMaxCells] = {};  // 0 empty, else the digit
  bool hinted[kMaxCells] = {};
  int selected = -1;
};

inline void clearEntries(Board& b) {
  for (int i = 0; i < kMaxCells; i++) {
    b.entry[i] = 0;
    b.hinted[i] = false;
  }
}

inline bool solved(const Board& b) {
  const int cells = b.puzzle.n * b.puzzle.n;
  for (int i = 0; i < cells; i++) {
    if (white(b.puzzle, i) && b.entry[i] != b.puzzle.solution[i]) return false;
  }
  return true;
}

// Cells that break a rule: a digit twice in a run, or a run over its sum or full and wrong.
inline void conflicts(const Board& b, bool bad[kMaxCells]) {
  for (int i = 0; i < kMaxCells; i++) bad[i] = false;
  for (const Run& run : b.puzzle.runs) {
    int seen[10];
    for (int d = 0; d < 10; d++) seen[d] = -1;
    int total = 0;
    bool full = true;
    for (uint8_t x : run.cells) {
      const int v = b.entry[x];
      if (v == 0) {
        full = false;
        continue;
      }
      if (seen[v] >= 0) {
        bad[x] = true;
        bad[seen[v]] = true;
      } else {
        seen[v] = x;
      }
      total += v;
    }
    if ((full && total != run.sum) || total > run.sum) {
      for (uint8_t x : run.cells) {
        if (b.entry[x]) bad[x] = true;
      }
    }
  }
}

inline int firstOpen(const Board& b) {
  const int cells = b.puzzle.n * b.puzzle.n;
  int first = -1;
  for (int i = 0; i < cells; i++) {
    if (!white(b.puzzle, i)) continue;
    if (first < 0) first = i;
    if (b.entry[i] == 0) return i;
  }
  return first;
}

inline int hintCount(const Board& b) {
  int n = 0;
  for (int i = 0; i < kMaxCells; i++) n += b.hinted[i];
  return n;
}

}  // namespace kakuro
