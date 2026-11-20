#pragma once

#include <cstdint>

// Last Piece's rules, with no screen or card: chess pieces on a 4x4 board, where every move must
// capture and a challenge is solved when one piece is left. Pieces move as in chess. A pawn takes
// one square diagonally up the board, and there's no promotion and no check, so a king can be
// taken like anything else. The capture rules are Grid Gambit's, copied here because a game
// can't include another's files.
namespace lastpiece {

constexpr int kSide = 4;
constexpr int kCells = kSide * kSide;
constexpr int kNoCell = -1;
constexpr int kMaxPieces = 10;
constexpr int kMaxMoves = kMaxPieces - 1;

enum Piece : uint8_t { None, King, Queen, Rook, Bishop, Knight, Pawn };
constexpr int kPieceTypes = 7;  // None included

// A board's cells, row 0 at the top: cell = row * 4 + col.
struct Board {
  uint8_t cell[kCells] = {};
};

inline bool sameBoard(const Board& a, const Board& b) {
  for (int i = 0; i < kCells; i++) {
    if (a.cell[i] != b.cell[i]) return false;
  }
  return true;
}

inline int rowOf(int i) { return i / kSide; }
inline int colOf(int i) { return i % kSide; }
inline bool onBoard(int r, int c) { return r >= 0 && r < kSide && c >= 0 && c < kSide; }

inline int pieceCount(const Board& b) {
  int n = 0;
  for (int i = 0; i < kCells; i++) n += b.cell[i] != None;
  return n;
}

namespace detail {
constexpr int kKingSteps[8][2] = {{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}};
constexpr int kKnightSteps[8][2] = {{-2, -1}, {-2, 1}, {-1, -2}, {-1, 2}, {1, -2}, {1, 2}, {2, -1}, {2, 1}};
// Rook directions first, then bishop directions, so a queen uses all eight.
constexpr int kLines[8][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}, {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};

inline void lineRange(uint8_t piece, int& first, int& last) {
  first = piece == Bishop ? 4 : 0;
  last = piece == Rook ? 4 : 8;
}
}  // namespace detail

// The squares the piece on `from` can capture, written to `out` (room for 8). Returns how many.
// An empty square captures nothing.
inline int capturesFrom(const Board& b, int from, int* out) {
  const uint8_t p = b.cell[from];
  const int r = rowOf(from);
  const int c = colOf(from);
  int n = 0;
  auto take = [&](int rr, int cc) {
    if (onBoard(rr, cc) && b.cell[rr * kSide + cc] != None) out[n++] = rr * kSide + cc;
  };
  switch (p) {
    case None:
      break;
    case King:
      for (const auto& s : detail::kKingSteps) take(r + s[0], c + s[1]);
      break;
    case Knight:
      for (const auto& s : detail::kKnightSteps) take(r + s[0], c + s[1]);
      break;
    case Pawn:
      take(r - 1, c - 1);
      take(r - 1, c + 1);
      break;
    default: {
      int first, last;
      detail::lineRange(p, first, last);
      for (int d = first; d < last; d++) {
        int rr = r + detail::kLines[d][0];
        int cc = c + detail::kLines[d][1];
        while (onBoard(rr, cc) && b.cell[rr * kSide + cc] == None) {
          rr += detail::kLines[d][0];
          cc += detail::kLines[d][1];
        }
        take(rr, cc);
      }
      break;
    }
  }
  return n;
}

inline bool canCapture(const Board& b, int from, int to) {
  if (from < 0 || from >= kCells || to < 0 || to >= kCells) return false;
  int t[8];
  const int n = capturesFrom(b, from, t);
  for (int k = 0; k < n; k++) {
    if (t[k] == to) return true;
  }
  return false;
}

inline bool anyCapture(const Board& b) {
  int t[8];
  for (int i = 0; i < kCells; i++) {
    if (capturesFrom(b, i, t) > 0) return true;
  }
  return false;
}

// Moves the piece on `from` onto `to`, taking what's there. Returns the piece taken, or None if
// the move isn't a capture, which leaves the board alone.
inline uint8_t capture(Board& b, int from, int to) {
  if (!canCapture(b, from, to)) return None;
  const uint8_t taken = b.cell[to];
  b.cell[to] = b.cell[from];
  b.cell[from] = None;
  return taken;
}

inline bool isWon(const Board& b) { return pieceCount(b) == 1; }
inline bool isLost(const Board& b) { return pieceCount(b) > 1 && !anyCapture(b); }

// A cache for solvable(), in memory the caller owns: the device takes it from the heap, and the
// tests from a vector. Each slot holds a board's code (3 bits a cell) above one bit saying
// whether it can be won, and 0 is empty. Any power-of-two size works, since a full table only
// overwrites; with no slots at all the search still works, only slower. A board's answer never
// changes, so the cache is never cleared.
struct Memo {
  uint64_t* slots = nullptr;
  uint32_t mask = 0;  // slot count - 1
};

namespace detail {

inline uint64_t code(const Board& b) {
  uint64_t k = 0;
  for (int i = 0; i < kCells; i++) k = k << 3 | b.cell[i];
  return k;
}

inline uint32_t slotOf(uint64_t key, uint32_t mask) {
  return static_cast<uint32_t>((key * 0x9E3779B97F4A7C15ull) >> 40) & mask;
}

}  // namespace detail

// Whether some order of captures ends on one piece. Ten pieces is quick with the cache, because
// positions repeat: the same pieces end up on the same squares by different orders.
inline bool solvable(Board& b, Memo& memo) {
  const int pieces = pieceCount(b);
  if (pieces <= 1) return pieces == 1;
  const uint64_t key = detail::code(b);
  const uint32_t slot = memo.slots ? detail::slotOf(key, memo.mask) : 0;
  if (memo.slots && memo.slots[slot] >> 1 == key) return memo.slots[slot] & 1;
  bool win = false;
  int t[8];
  for (int from = 0; from < kCells && !win; from++) {
    const int n = capturesFrom(b, from, t);
    for (int k = 0; k < n && !win; k++) {
      const uint8_t mover = b.cell[from];
      const uint8_t taken = b.cell[t[k]];
      b.cell[t[k]] = mover;
      b.cell[from] = None;
      win = solvable(b, memo);
      b.cell[from] = mover;
      b.cell[t[k]] = taken;
    }
  }
  if (memo.slots) memo.slots[slot] = key << 1 | (win ? 1 : 0);
  return win;
}

// The first capture, in cell order, that still leaves a win. Returns false if there's none.
inline bool bestMove(const Board& start, Memo& memo, int& from, int& to) {
  Board b = start;
  int t[8];
  for (int f = 0; f < kCells; f++) {
    const int n = capturesFrom(b, f, t);
    for (int k = 0; k < n; k++) {
      const uint8_t mover = b.cell[f];
      const uint8_t taken = b.cell[t[k]];
      b.cell[t[k]] = mover;
      b.cell[f] = None;
      const bool win = solvable(b, memo);
      b.cell[f] = mover;
      b.cell[t[k]] = taken;
      if (win) {
        from = f;
        to = t[k];
        return true;
      }
    }
  }
  return false;
}

}  // namespace lastpiece
