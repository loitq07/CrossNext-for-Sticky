#pragma once

#include <cstdint>
#include <cstring>

// Queens' rules, translated from the Lua of touch-licorice-apps' apps/queens (src/main.lua, MIT,
// LICENSE-licorice.txt). No screen or card, so they can be tested off the device.
//
// The board is split into regions. Place one queen (Queens) or two stars (Star Battle) in every
// row, every column and every region; no two may touch, not even diagonally. A tap cycles a cell
// through nothing, a cross (no piece here) and a piece. The puzzle is solved when the pieces are
// exactly the solution's, which the banks make the only one.
namespace queens {

constexpr int kMaxSize = 10;
constexpr int kMaxCells = kMaxSize * kMaxSize;
constexpr int kMaxRegions = kMaxSize;
constexpr int kMaxUndo = 300;

enum Mark : uint8_t { Empty, Cross, Piece };

struct Puzzle {
  int n = 0;
  int k = 0;  // pieces in each row, column and region
  uint8_t region[kMaxCells] = {};
  bool solution[kMaxCells] = {};
};

// One bank line into a puzzle (parse). False, and the puzzle left alone, for anything else.
inline bool parseLine(const char* line, Puzzle& out) {
  if (!line) return false;
  const int length = static_cast<int>(std::strlen(line));
  int n = 0;
  while (n * n < length) n++;
  if (n < 4 || n > kMaxSize || n * n != length) return false;
  Puzzle p;
  p.n = n;
  int pieces = 0;
  for (int i = 0; i < length; i++) {
    const char c = line[i];
    const bool upper = c >= 'A' && c <= 'Z';
    if (!upper && !(c >= 'a' && c <= 'z')) return false;
    const int r = (upper ? c - 'A' : c - 'a');
    if (r >= kMaxRegions) return false;
    p.region[i] = static_cast<uint8_t>(r);
    p.solution[i] = upper;
    pieces += upper ? 1 : 0;
  }
  if (pieces % n != 0 || pieces == 0) return false;
  p.k = pieces / n;
  out = p;
  return true;
}

// The rule a set of marks breaks first, for the line under the board.
enum class Broken : uint8_t { None, Touch, Row, Column, Region };

struct Game {
  Puzzle puzzle;
  uint8_t marks[kMaxCells] = {};
  bool hinted[kMaxCells] = {};  // pieces a hint placed: kept, and not changed by a tap
  struct Undo {
    uint8_t cell;
    uint8_t mark;
    bool hinted;
  };
  Undo undo[kMaxUndo] = {};
  int undoCount = 0;
};

inline void start(const Puzzle& p, Game& g) {
  g.puzzle = p;
  std::memset(g.marks, 0, sizeof(g.marks));
  std::memset(g.hinted, 0, sizeof(g.hinted));
  g.undoCount = 0;
}

// Calls fn(j) for each cell touching i, diagonals too (around).
template <typename Fn>
void around(int n, int i, Fn fn) {
  const int r = i / n;
  const int c = i % n;
  for (int dr = -1; dr <= 1; dr++) {
    for (int dc = -1; dc <= 1; dc++) {
      const int rr = r + dr;
      const int cc = c + dc;
      if ((dr || dc) && rr >= 0 && rr < n && cc >= 0 && cc < n) fn(rr * n + cc);
    }
  }
}

// The units in the source's order: row 0, column 0, row 1, column 1 ..., then each region.
// Calls fn(kind, inUnit) where inUnit(i) says whether cell i belongs.
template <typename Fn>
void eachUnit(const Puzzle& p, Fn fn) {
  const int n = p.n;
  for (int r = 0; r < n; r++) {
    fn(Broken::Row, [&](int i) { return i / n == r; });
    fn(Broken::Column, [&](int i) { return i % n == r; });
  }
  bool seen[kMaxRegions] = {};
  for (int i = 0; i < n * n; i++) {
    const int g = p.region[i];
    if (seen[g]) continue;
    seen[g] = true;
    fn(Broken::Region, [&](int j) { return p.region[j] == g; });
  }
}

// Pieces that break a rule (touching, or too many in a row, column or region), and the first
// rule broken (conflicts).
inline Broken conflicts(const Game& g, bool* bad) {
  const Puzzle& p = g.puzzle;
  const int count = p.n * p.n;
  Broken why = Broken::None;
  for (int i = 0; i < count; i++) bad[i] = false;
  for (int i = 0; i < count; i++) {
    if (g.marks[i] != Piece) continue;
    around(p.n, i, [&](int j) {
      if (g.marks[j] == Piece) {
        bad[i] = true;
        if (why == Broken::None) why = Broken::Touch;
      }
    });
  }
  eachUnit(p, [&](Broken kind, auto inUnit) {
    int pieces = 0;
    for (int i = 0; i < count; i++) pieces += (inUnit(i) && g.marks[i] == Piece) ? 1 : 0;
    if (pieces <= p.k) return;
    for (int i = 0; i < count; i++) {
      if (inUnit(i) && g.marks[i] == Piece) bad[i] = true;
    }
    if (why == Broken::None) why = kind;
  });
  return why;
}

// The cells the pieces already rule out: around a piece, or in a full row, column or region
// (ruled_out).
inline void ruledOut(const Game& g, bool* out) {
  const Puzzle& p = g.puzzle;
  const int count = p.n * p.n;
  for (int i = 0; i < count; i++) out[i] = false;
  for (int i = 0; i < count; i++) {
    if (g.marks[i] == Piece) around(p.n, i, [&](int j) { out[j] = true; });
  }
  eachUnit(p, [&](Broken, auto inUnit) {
    int pieces = 0;
    for (int i = 0; i < count; i++) pieces += (inUnit(i) && g.marks[i] == Piece) ? 1 : 0;
    if (pieces < p.k) return;
    for (int i = 0; i < count; i++) {
      if (inUnit(i)) out[i] = true;
    }
  });
}

inline int placed(const Game& g) {
  int pieces = 0;
  for (int i = 0; i < g.puzzle.n * g.puzzle.n; i++) pieces += g.marks[i] == Piece ? 1 : 0;
  return pieces;
}

inline int hints(const Game& g) {
  int count = 0;
  for (int i = 0; i < g.puzzle.n * g.puzzle.n; i++) count += g.hinted[i] ? 1 : 0;
  return count;
}

inline bool solved(const Game& g) {
  for (int i = 0; i < g.puzzle.n * g.puzzle.n; i++) {
    if ((g.marks[i] == Piece) != g.puzzle.solution[i]) return false;
  }
  return true;
}

// Every change goes through here, so Undo can take it back, hint mark and all. Past
// kMaxUndo the oldest change drops off, as in the source.
inline void setMark(Game& g, int i, uint8_t mark, bool hinted = false) {
  if (g.undoCount == kMaxUndo) {
    std::memmove(g.undo, g.undo + 1, sizeof(g.undo[0]) * (kMaxUndo - 1));
    g.undoCount--;
  }
  g.undo[g.undoCount++] = {static_cast<uint8_t>(i), g.marks[i], g.hinted[i]};
  g.marks[i] = mark;
  g.hinted[i] = hinted;
}

inline bool takeBack(Game& g) {
  if (g.undoCount == 0) return false;
  const Game::Undo u = g.undo[--g.undoCount];
  g.marks[u.cell] = u.mark;
  g.hinted[u.cell] = u.hinted;
  return true;
}

// nothing -> cross -> piece -> nothing (cycle).
inline uint8_t nextMark(uint8_t m) { return m == Empty ? Cross : m == Cross ? Piece : Empty; }

enum class Hint : uint8_t { None, RemovedPiece, RemovedCross, Placed };

// Removes a wrong piece, else a cross where a piece belongs, else places a right piece in the
// region with the fewest cells still open (hint).
inline Hint hint(Game& g) {
  const Puzzle& p = g.puzzle;
  const int count = p.n * p.n;
  for (int i = 0; i < count; i++) {
    if (g.marks[i] == Piece && !p.solution[i]) {
      setMark(g, i, Empty);
      return Hint::RemovedPiece;
    }
  }
  for (int i = 0; i < count; i++) {
    if (g.marks[i] == Cross && p.solution[i]) {
      setMark(g, i, Empty);
      return Hint::RemovedCross;
    }
  }
  bool out[kMaxCells];
  ruledOut(g, out);
  int best = -1;
  int bestOpen = 0;
  eachUnit(p, [&](Broken kind, auto inUnit) {
    if (kind != Broken::Region) return;
    int open = 0;
    int missing = -1;
    for (int i = 0; i < count; i++) {
      if (!inUnit(i)) continue;
      if (g.marks[i] == Empty && !out[i]) open++;
      if (p.solution[i] && g.marks[i] != Piece && missing < 0) missing = i;
    }
    if (missing >= 0 && (best < 0 || open < bestOpen)) {
      best = missing;
      bestOpen = open;
    }
  });
  if (best < 0) return Hint::None;
  setMark(g, best, Piece, true);
  return Hint::Placed;
}

}  // namespace queens
