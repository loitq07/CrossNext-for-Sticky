#pragma once

#include <cstdint>

// Grid Gambit's rules, with no screen or card: chess pieces on a 4x4 board, where every move
// must capture and the puzzle is solved when one piece is left. Pieces move as in chess. A pawn
// takes one square diagonally up the board, and there's no promotion and no check, so a king
// can be taken like anything else.
//
// Puzzles are made by playing the game backwards from one piece: each step "un-takes", moving
// a piece back to a square it could have captured from and leaving a new piece where it was.
// Every puzzle made this way can be solved. Each step tries several un-takes and keeps the one
// that leaves random play least likely to win, which is what makes a puzzle hard.
namespace gridgambit {

constexpr int kSide = 4;
constexpr int kCells = kSide * kSide;
constexpr int kNoCell = -1;
constexpr int kMaxPieces = 8;
constexpr int kMaxMoves = kMaxPieces - 1;
constexpr int kLevels = 4;

enum Piece : uint8_t { None, King, Queen, Rook, Bishop, Knight, Pawn };
constexpr int kPieceTypes = 7;  // None included

// The most of each piece a puzzle may hold, as in a chess set with two pawns.
constexpr uint8_t kPieceCap[kPieceTypes] = {0, 1, 1, 2, 2, 2, 2};

// A board's cells, row 0 at the top: cell = row * 4 + col.
struct Board {
  uint8_t cell[kCells] = {};
};

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

// The squares the piece on `from` can capture, written to `out` (room for 8). Returns how
// many. An empty square captures nothing.
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

// Moves the piece on `from` onto `to`, taking what's there. Returns the piece taken, or None
// if the move isn't a capture, which leaves the board alone.
inline uint8_t capture(Board& b, int from, int to) {
  if (!canCapture(b, from, to)) return None;
  const uint8_t taken = b.cell[to];
  b.cell[to] = b.cell[from];
  b.cell[from] = None;
  return taken;
}

inline bool isWon(const Board& b) { return pieceCount(b) == 1; }
inline bool isLost(const Board& b) { return pieceCount(b) > 1 && !anyCapture(b); }

// How many orders of captures end on one piece, counting at most `limit`. The board is small
// enough that this is quick even from eight pieces.
inline int countWins(Board& b, int limit) {
  if (pieceCount(b) == 1) return 1;
  int wins = 0;
  int t[8];
  for (int from = 0; from < kCells && wins < limit; from++) {
    const int n = capturesFrom(b, from, t);
    for (int k = 0; k < n && wins < limit; k++) {
      const uint8_t mover = b.cell[from];
      const uint8_t taken = b.cell[t[k]];
      b.cell[t[k]] = mover;
      b.cell[from] = None;
      wins += countWins(b, limit - wins);
      b.cell[from] = mover;
      b.cell[t[k]] = taken;
    }
  }
  return wins;
}

// A small random source, so a seed always makes the same puzzle and tests can repeat one.
struct Rng {
  uint32_t s;
  explicit Rng(uint32_t seed) : s(seed ? seed : 0x9E3779B9u) {}
  uint32_t next() {
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
  }
  int below(int n) { return static_cast<int>(next() % static_cast<uint32_t>(n)); }
};

// A cache for winChance(), in memory the caller owns: the device keeps it in PSRAM, and the
// tests in a vector. Each slot holds a board's code (3 bits a cell) above its chance in 16
// bits, and 0 is empty. Any size that's a power of two works, since a full table only
// overwrites; a bigger one is quicker.
struct Memo {
  uint64_t* slots = nullptr;
  uint32_t mask = 0;  // slot count - 1
  void clear() {
    for (uint32_t i = 0; i <= mask; i++) slots[i] = 0;
  }
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

constexpr int kProbe = 8;

}  // namespace detail

// The chance of ending on one piece by picking every capture at random, from 0 to 65535. Lower
// is harder: random play rarely stumbles on the answer.
inline uint32_t winChance(Board& b, Memo& memo) {
  const int pieces = pieceCount(b);
  if (pieces == 1) return 65535;
  const uint64_t key = detail::code(b);
  const uint32_t home = detail::slotOf(key, memo.mask);
  for (int k = 0; k < detail::kProbe; k++) {
    const uint64_t slot = memo.slots[(home + k) & memo.mask];
    if (slot == 0) break;
    if (slot >> 16 == key) return static_cast<uint32_t>(slot & 0xFFFF);
  }
  uint32_t sum = 0;
  int moves = 0;
  int t[8];
  for (int from = 0; from < kCells; from++) {
    const int n = capturesFrom(b, from, t);
    for (int j = 0; j < n; j++) {
      const uint8_t mover = b.cell[from];
      const uint8_t taken = b.cell[t[j]];
      b.cell[t[j]] = mover;
      b.cell[from] = None;
      sum += winChance(b, memo);
      b.cell[from] = mover;
      b.cell[t[j]] = taken;
      moves++;
    }
  }
  const uint32_t chance = moves ? (sum + moves / 2) / moves : 0;
  // The first empty slot, or the home slot if all are taken: it's a cache, so losing an
  // entry only costs time.
  uint32_t put = home;
  for (int k = 0; k < detail::kProbe; k++) {
    if (memo.slots[(home + k) & memo.mask] == 0) {
      put = (home + k) & memo.mask;
      break;
    }
  }
  memo.slots[put] = key << 16 | chance;
  return chance;
}

// Each level's piece count, how many un-takes each step of making a puzzle tries (more tries
// find a harder one), and the most a finished puzzle may let random play win, in percent. A
// puzzle over that is made again, a few times at most.
struct LevelDef {
  const char* name;
  uint8_t minPieces;
  uint8_t maxPieces;
  uint8_t tries;
  uint8_t maxChancePercent;
};
constexpr LevelDef kLevelList[kLevels] = {
    {"BEGINNER", 4, 4, 8, 25},
    {"INTERMEDIATE", 5, 6, 15, 15},
    {"ADVANCED", 6, 7, 25, 9},
    {"EXPERT", 8, 8, 40, 5},
};
constexpr int kRemakes = 6;

namespace detail {

// The squares the piece on `to` could have captured it from: empty squares from which it
// reaches `to`, with an empty path between. Written to `out` (room for 15).
inline int origins(const Board& b, int to, int* out) {
  const uint8_t p = b.cell[to];
  const int r = rowOf(to);
  const int c = colOf(to);
  int n = 0;
  auto from = [&](int rr, int cc) {
    if (onBoard(rr, cc) && b.cell[rr * kSide + cc] == None) out[n++] = rr * kSide + cc;
  };
  switch (p) {
    case None:
      break;
    case King:
      for (const auto& s : kKingSteps) from(r + s[0], c + s[1]);
      break;
    case Knight:
      for (const auto& s : kKnightSteps) from(r + s[0], c + s[1]);
      break;
    case Pawn:  // a pawn takes upwards, so it came from the row below
      from(r + 1, c - 1);
      from(r + 1, c + 1);
      break;
    default: {
      int first, last;
      lineRange(p, first, last);
      for (int d = first; d < last; d++) {
        int rr = r + kLines[d][0];
        int cc = c + kLines[d][1];
        while (onBoard(rr, cc) && b.cell[rr * kSide + cc] == None) {
          out[n++] = rr * kSide + cc;
          rr += kLines[d][0];
          cc += kLines[d][1];
        }
      }
      break;
    }
  }
  return n;
}

inline int countOf(const Board& b, uint8_t piece) {
  int n = 0;
  for (int i = 0; i < kCells; i++) n += b.cell[i] == piece;
  return n;
}

// One random un-take: a piece steps back to a square it could have captured from, and a new
// piece goes where it was. Returns false if none was found.
inline bool untake(Rng& rng, Board& b) {
  for (int tries = 0; tries < 40; tries++) {
    int at[kCells];
    int on = 0;
    for (int i = 0; i < kCells; i++) {
      if (b.cell[i] != None) at[on++] = i;
    }
    const int to = at[rng.below(on)];
    int from[15];
    const int n = origins(b, to, from);
    if (n == 0) continue;
    const uint8_t left = static_cast<uint8_t>(1 + rng.below(kPieceTypes - 1));
    if (countOf(b, left) >= kPieceCap[left]) continue;
    const int f = from[rng.below(n)];
    b.cell[f] = b.cell[to];
    b.cell[to] = left;
    return true;
  }
  return false;
}

}  // namespace detail

// Makes a puzzle a little at a time, so the screen can keep answering: each step() tries one
// un-take and grades it. Every puzzle it makes can be solved, since it was played backwards
// from one piece.
class Maker {
 public:
  void start(Rng& rng, int level) {
    def_ = &kLevelList[level];
    pieces_ = def_->minPieces + rng.below(def_->maxPieces - def_->minPieces + 1);
    restarts_ = 0;
    remakes_ = 0;
    restart(rng);
  }

  // One try. Returns true once the puzzle is made, and board() holds it.
  bool step(Rng& rng, Memo& memo) {
    if (done_) return true;
    Board c = board_;
    if (detail::untake(rng, c)) {
      memo.clear();
      const uint32_t chance = winChance(c, memo);
      if (!found_ || chance < bestChance_) {
        best_ = c;
        bestChance_ = chance;
        found_ = true;
      }
    }
    if (++tries_ < def_->tries) return false;
    tries_ = 0;
    if (!found_) {  // stuck: begin again from one piece
      if (++restarts_ > 50) return giveUp();
      restart(rng);
      return false;
    }
    board_ = best_;
    found_ = false;
    if (pieceCount(board_) < pieces_) return false;
    // Too easy: make it again, unless it has been made enough times already.
    if (bestChance_ * 100 > def_->maxChancePercent * 65535u && ++remakes_ <= kRemakes) {
      restart(rng);
      return false;
    }
    done_ = true;
    return true;
  }

  bool done() const { return done_; }
  const Board& board() const { return board_; }

 private:
  void restart(Rng& rng) {
    board_ = Board{};
    board_.cell[rng.below(kCells)] = static_cast<uint8_t>(1 + rng.below(kPieceTypes - 1));
    found_ = false;
    tries_ = 0;
    done_ = false;
  }

  // Never seen; a rook and a pawn it can take, so a puzzle always comes out.
  bool giveUp() {
    board_ = Board{};
    board_.cell[12] = Rook;
    board_.cell[0] = Pawn;
    done_ = true;
    return true;
  }

  const LevelDef* def_ = &kLevelList[0];
  int pieces_ = 4;
  Board board_;
  Board best_;
  uint32_t bestChance_ = 0;
  bool found_ = false;
  int tries_ = 0;
  int restarts_ = 0;
  int remakes_ = 0;
  bool done_ = true;
};

// A whole puzzle at once, for the tests and the preview.
inline Board generate(Rng& rng, int level, Memo& memo) {
  Maker m;
  m.start(rng, level);
  while (!m.step(rng, memo)) {
  }
  return m.board();
}

}  // namespace gridgambit
