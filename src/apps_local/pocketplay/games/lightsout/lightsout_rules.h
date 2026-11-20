#pragma once

#include <cstdint>

// Lights Out's rules, translated from the Lua of touch-licorice-apps' apps/lightsout
// (index.lua, MIT, LICENSE-licorice.txt). No screen or card, so they can be tested
// off the device. They take a seed and never reach for the hardware's random numbers.
//
// A 5x5 board of lights. Pressing a square switches it and its four neighbours; switch
// them all off. Puzzles are made by pressing random squares from the dark board, so
// every one can be solved. A solver over GF(2) gives the fewest presses, shown as the
// target, and drives the hint.
namespace lightsout {

constexpr int kSize = 5;
constexpr int kCells = kSize * kSize;
constexpr uint32_t kAllLights = (1u << kCells) - 1;
constexpr int kHistory = 255;  // presses Undo can take back; older ones drop off

struct Level {
  const char* name;
  int presses;   // random presses that make the puzzle
  int minimum;   // its fewest presses must be at least this
};
constexpr int kLevelCount = 3;
constexpr Level kLevels[kLevelCount] = {{"EASY", 4, 3}, {"NORMAL", 8, 6}, {"HARD", 13, 10}};
constexpr int kDefaultLevel = 1;

struct Game {
  uint32_t board = 0;  // bit i (row-major) set = light i on
  uint16_t moves = 0;
  uint8_t par = 0;     // the fewest presses the puzzle could be solved in
  uint8_t history[kHistory] = {};
  uint8_t historyCount = 0;
};

inline uint32_t pressMask(int i) {
  const int r = i / kSize;
  const int c = i % kSize;
  uint32_t m = 1u << i;
  if (r > 0) m |= 1u << (i - kSize);
  if (r < kSize - 1) m |= 1u << (i + kSize);
  if (c > 0) m |= 1u << (i - 1);
  if (c < kSize - 1) m |= 1u << (i + 1);
  return m;
}

inline int popcount(uint32_t x) {
  int n = 0;
  while (x != 0) {
    x &= x - 1;
    n++;
  }
  return n;
}

// The fewest presses that switch every light of board off, as a mask of squares.
// False when no presses can. Gaussian elimination on the press matrix (which is
// symmetric, so its rows are the press masks), then every combination of the free
// columns, of which a 5x5 board has two.
inline bool solve(uint32_t board, uint32_t& out) {
  uint32_t a[kCells];
  uint8_t b[kCells];
  for (int j = 0; j < kCells; j++) {
    a[j] = pressMask(j);
    b[j] = (board >> j) & 1;
  }
  int pivot[kCells];
  int r = 0;
  for (int col = 0; col < kCells; col++) {
    int p = -1;
    for (int k = r; k < kCells; k++) {
      if ((a[k] >> col) & 1) {
        p = k;
        break;
      }
    }
    if (p < 0) continue;
    const uint32_t ta = a[r];
    a[r] = a[p];
    a[p] = ta;
    const uint8_t tb = b[r];
    b[r] = b[p];
    b[p] = tb;
    for (int k = 0; k < kCells; k++) {
      if (k != r && ((a[k] >> col) & 1)) {
        a[k] ^= a[r];
        b[k] ^= b[r];
      }
    }
    pivot[r] = col;
    r++;
  }
  for (int k = r; k < kCells; k++) {
    if (a[k] == 0 && b[k] == 1) return false;
  }

  bool isPivot[kCells] = {};
  for (int k = 0; k < r; k++) isPivot[pivot[k]] = true;
  int freeCols[kCells];
  int freeCount = 0;
  for (int col = 0; col < kCells; col++) {
    if (!isPivot[col]) freeCols[freeCount++] = col;
  }

  bool found = false;
  uint32_t best = 0;
  for (uint32_t combo = 0; combo < (1u << freeCount); combo++) {
    uint32_t x = 0;
    for (int f = 0; f < freeCount; f++) {
      if ((combo >> f) & 1) x |= 1u << freeCols[f];
    }
    for (int k = 0; k < r; k++) {
      const int v = b[k] ^ (popcount(a[k] & ~(1u << pivot[k]) & x) & 1);
      if (v == 1) x |= 1u << pivot[k];
    }
    if (!found || popcount(x) < popcount(best)) {
      best = x;
      found = true;
    }
  }
  out = best;
  return true;
}

inline int fewestPresses(uint32_t board) {
  uint32_t x = 0;
  return solve(board, x) ? popcount(x) : -1;
}

inline bool solved(const Game& game) { return game.board == 0; }

inline uint32_t nextRandom(uint32_t& state) {
  if (state == 0) state = 0x9E3779B9u;  // xorshift never leaves 0
  state ^= state << 13;
  state ^= state >> 17;
  state ^= state << 5;
  return state;
}

// Presses distinct random squares from the dark board until the puzzle needs at
// least the level's minimum.
inline void newPuzzle(Game& game, int level, uint32_t& rng) {
  const Level& lv = kLevels[level];
  uint32_t board = 0;
  int par = 0;
  do {
    board = 0;
    uint32_t pressed = 0;
    int count = 0;
    while (count < lv.presses) {
      const int i = static_cast<int>(nextRandom(rng) % kCells);
      if (pressed & (1u << i)) continue;
      pressed |= 1u << i;
      count++;
      board ^= pressMask(i);
    }
    par = fewestPresses(board);
  } while (par < lv.minimum);
  game = Game{};
  game.board = board;
  game.par = static_cast<uint8_t>(par);
}

// False, and nothing changes, once the puzzle is solved.
inline bool press(Game& game, int i) {
  if (solved(game) || i < 0 || i >= kCells) return false;
  game.board ^= pressMask(i);
  if (game.moves < UINT16_MAX) game.moves++;
  if (game.historyCount == kHistory) {
    for (int k = 1; k < kHistory; k++) game.history[k - 1] = game.history[k];
    game.historyCount--;
  }
  game.history[game.historyCount++] = static_cast<uint8_t>(i);
  return true;
}

// Takes the last press back, and its move with it. False with nothing to take back,
// or once solved: a solved puzzle stays solved.
inline bool undo(Game& game) {
  if (solved(game) || game.historyCount == 0) return false;
  const int i = game.history[--game.historyCount];
  game.board ^= pressMask(i);
  if (game.moves > 0) game.moves--;
  return true;
}

// One square of a fewest-press solution from here, or -1.
inline int hint(const Game& game) {
  if (solved(game)) return -1;
  uint32_t x = 0;
  if (!solve(game.board, x) || x == 0) return -1;
  for (int i = 0; i < kCells; i++) {
    if ((x >> i) & 1) return i;
  }
  return -1;
}

}  // namespace lightsout
