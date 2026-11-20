#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

// Suguru's rules, translated from the Lua of touch-licorice-apps' apps/suguru (index.lua,
// MIT, LICENSE-licorice.txt). No screen or card, so they can be tested off the device.
//
// The grid is split into regions of one to five cells. A region of n cells holds 1 to n,
// each once, and touching cells, diagonals included, never hold the same number. The
// givens are fixed. Solved means every cell is filled and no rule is broken; the banks
// have one solution each, so that is the solution.
namespace suguru {

constexpr int kMinSize = 4;
constexpr int kMaxSize = 9;
constexpr int kMaxCells = kMaxSize * kMaxSize;
constexpr int kMaxDigit = 5;

// One puzzle: region letters and given digits, row by row, as '0'..'5' digits stored as 0..5.
struct Puzzle {
  int n = 0;
  char regions[kMaxCells] = {};
  uint8_t givens[kMaxCells] = {};
};

inline bool isRegionChar(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

// One bank line into a puzzle: n*n region letters, then n*n digits. False, and the
// puzzle left alone, for anything else (parse_line).
inline bool parseLine(const char* line, Puzzle& out) {
  if (!line) return false;
  size_t length = std::strlen(line);
  while (length > 0 && (line[length - 1] == '\r' || line[length - 1] == '\n')) length--;
  if (length % 2 != 0) return false;
  const size_t half = length / 2;
  int n = 0;
  while (n * n < static_cast<int>(half)) n++;
  if (n < kMinSize || n > kMaxSize || static_cast<size_t>(n * n) != half) return false;
  Puzzle p;
  p.n = n;
  for (size_t i = 0; i < half; i++) {
    const char r = line[i];
    const char d = line[half + i];
    if (!isRegionChar(r) || d < '0' || d > '0' + kMaxDigit) return false;
    p.regions[i] = r;
    p.givens[i] = static_cast<uint8_t>(d - '0');
  }
  out = p;
  return true;
}

// How many cells share cell i's region.
inline int regionSize(const Puzzle& p, int i) {
  int size = 0;
  for (int j = 0; j < p.n * p.n; j++) {
    if (p.regions[j] == p.regions[i]) size++;
  }
  return size;
}

// bad[i] is true when the number in cell i breaks a rule: bigger than its region,
// repeated in its region, or equal to a touching cell, diagonals too (conflicts).
// Returns how many cells are bad.
inline int conflicts(const Puzzle& p, const uint8_t* cells, bool* bad) {
  const int n = p.n;
  const int count = n * n;
  for (int i = 0; i < count; i++) bad[i] = false;
  for (int i = 0; i < count; i++) {
    const uint8_t v = cells[i];
    if (v == 0) continue;
    if (v > regionSize(p, i)) bad[i] = true;
    for (int j = i + 1; j < count; j++) {
      if (cells[j] == v && p.regions[j] == p.regions[i]) bad[i] = bad[j] = true;
    }
    const int row = i / n;
    const int col = i % n;
    for (int dr = -1; dr <= 1; dr++) {
      for (int dc = -1; dc <= 1; dc++) {
        const int r = row + dr;
        const int c = col + dc;
        if ((dr == 0 && dc == 0) || r < 0 || r >= n || c < 0 || c >= n) continue;
        if (cells[r * n + c] == v) bad[i] = bad[r * n + c] = true;
      }
    }
  }
  int total = 0;
  for (int i = 0; i < count; i++) total += bad[i] ? 1 : 0;
  return total;
}

inline bool solved(const Puzzle& p, const uint8_t* cells) {
  for (int i = 0; i < p.n * p.n; i++) {
    if (cells[i] == 0) return false;
  }
  bool bad[kMaxCells];
  return conflicts(p, cells, bad) == 0;
}

}  // namespace suguru
