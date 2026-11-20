#pragma once

#include <cstdint>

// The two decks' shapes, read off the game's card images: '#' is a square, '.' is empty, and '|'
// starts the next row. Each deck is in the order of its card files. No screen or card, so it
// can be tested off the device.
namespace polyominopanic {

struct Shape {
  const char* rows;
};

constexpr int kMaxWidth = 5;
constexpr int kMaxHeight = 4;

// starting/card_41..53, every one 8 squares.
constexpr Shape kStarter[] = {
    {"###|#.#|###"},          // 41 o ring
    {"#..#|####|#..#"},       // 42 double t
    {"##..|####|..##"},       // 43 z wave
    {"####|#..#|#..#"},       // 44 deep u
    {"###|###|#..|#.."},      // 45 flag
    {".##.|####|.##."},       // 46 offset cross
    {"##..|.##.|..##|..##"},  // 47 grand stairs
    {"###|###|.##"},          // 48 notched square
    {"#.#.#|#####"},          // 49 trident
    {".##|.#.|###|##."},      // 50 s hook
    {"####|.##.|.##."},       // 51 anchor
    {".##|##.|.##|##."},      // 52 pinwheel
    {"####|#.##|#..."},       // 53 twin arch
};

// cards/card_01..40, sizes 1 to 7.
constexpr Shape kGame[] = {
    {"#"},              // 01
    {"#"},              // 02
    {"#"},              // 03
    {"#|#"},            // 04
    {"#|#"},            // 05
    {"#|#"},            // 06
    {"#|#|#"},          // 07
    {"#|#|#"},          // 08
    {"#.|##"},          // 09
    {"#.|##"},          // 10
    {"####"},           // 11
    {"###|#.."},        // 12
    {"##|##"},          // 13
    {"##.|.##"},        // 14
    {"###|.#."},        // 15
    {"###.|..##"},      // 16
    {"###|#.#"},        // 17
    {"###|##."},        // 18
    {"####|.#.."},      // 19
    {".#.|###|.#."},    // 20
    {"#..|#..|###"},    // 21
    {"###|.#.|.#."},    // 22
    {"#..|##.|.##"},    // 23
    {"#..|###|..#"},    // 24
    {"####|.##."},      // 25
    {"###|###"},        // 26
    {"#####|..#.."},    // 27
    {"##|.#|.#|##"},    // 28
    {".#.|###|##."},    // 29
    {".#..|####|.#.."}, // 30
    {".#.|###|#.#"},    // 31
    {"#..|##.|###"},    // 32
    {"#.#|###|#.#"},    // 33
    {".#.#.|#####"},    // 34
    {"##..|.##.|..##|...#"},  // 35
    {"..#.|####|..#.|..#."},  // 36
    {"###|#.#|#.#"},    // 37
    {".##|###|.##"},    // 38
    {"..##|###.|..##"}, // 39
    {".##|###|##."},    // 40
};

constexpr int kStarterCount = sizeof(kStarter) / sizeof(kStarter[0]);
constexpr int kGameCount = sizeof(kGame) / sizeof(kGame[0]);

inline int width(const Shape& s) {
  int widest = 0, run = 0;
  for (const char* p = s.rows;; p++) {
    if (*p == '|' || *p == '\0') {
      if (run > widest) widest = run;
      run = 0;
      if (*p == '\0') break;
    } else {
      run++;
    }
  }
  return widest;
}

inline int height(const Shape& s) {
  int rows = 1;
  for (const char* p = s.rows; *p; p++) rows += *p == '|';
  return rows;
}

// Whether the square at column x, row y (from the top left) is part of the shape.
inline bool filled(const Shape& s, int x, int y) {
  if (x < 0 || y < 0) return false;
  const char* p = s.rows;
  for (int row = 0; row < y; row++) {
    while (*p && *p != '|') p++;
    if (!*p) return false;
    p++;
  }
  for (int col = 0; col < x; col++) {
    if (!*p || *p == '|') return false;
    p++;
  }
  return *p == '#';
}

inline int squares(const Shape& s) {
  int n = 0;
  for (const char* p = s.rows; *p; p++) n += *p == '#';
  return n;
}

}  // namespace polyominopanic
