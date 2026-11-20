#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

// Chess Puzzles' rules, translated from touch-licorice-apps' apps/chess/src/main.lua (MIT,
// LICENSE-licorice.txt). The player's move is checked against the puzzle's solution, so there's no
// move generator: a move is only applied, with castling, en passant and promotion.
namespace chesspuzzles {

constexpr int kSquares = 64;
constexpr int kNoSquare = -1;
constexpr int kMaxMoves = 16;  // the banks' longest line is 10

// The pieces as FEN letters, upper case white and lower case black, '\0' empty. Square 0 is a1,
// 7 is h1 and 63 is h8.
struct Board {
  char sq[kSquares] = {};
  char turn = 'w';
  int lastFrom = kNoSquare;  // the last move, marked on the board
  int lastTo = kNoSquare;
};

inline int fileOf(int s) { return s % 8; }
inline int rankOf(int s) { return s / 8; }
inline bool isWhite(char p) { return p >= 'A' && p <= 'Z'; }
inline char lower(char c) { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c; }
inline char upper(char c) { return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c; }

// 'a1' -> 0, or kNoSquare for anything else (sq).
inline int squareOf(const char* name) {
  const char f = name[0], r = name[1];
  if (f < 'a' || f > 'h' || r < '1' || r > '8') return kNoSquare;
  return (r - '1') * 8 + (f - 'a');
}

inline bool isPiece(char c) { return c && std::strchr("KQRBNPkqrbnp", c) != nullptr; }

// The board and side to move from a FEN (set_fen). Returns false for a FEN it can't read.
inline bool setFen(Board& b, const char* fen) {
  b = Board{};
  int r = 7, f = 0;
  const char* p = fen;
  for (; *p && *p != ' '; p++) {
    const char ch = *p;
    if (ch == '/') {
      r--;
      f = 0;
    } else if (ch >= '1' && ch <= '8') {
      f += ch - '0';
    } else if (isPiece(ch) && r >= 0 && f < 8) {
      b.sq[r * 8 + f] = ch;
      f++;
    } else {
      return false;
    }
    if (r < 0 || f > 8) return false;
  }
  if (*p != ' ' || (p[1] != 'w' && p[1] != 'b')) return false;
  b.turn = p[1];
  return true;
}

// A UCI move's squares and promotion, such as "e7e8q". Returns false if it isn't one.
struct Move {
  int from = kNoSquare;
  int to = kNoSquare;
  char promo = '\0';
};

inline bool parseMove(const char* uci, size_t len, Move& m) {
  if (len != 4 && len != 5) return false;
  m.from = squareOf(uci);
  m.to = squareOf(uci + 2);
  m.promo = len == 5 ? uci[4] : '\0';
  if (m.from == kNoSquare || m.to == kNoSquare) return false;
  return m.promo == '\0' || std::strchr("qrbn", m.promo) != nullptr;
}

// Makes a move (apply): castling moves the rook too, en passant takes the pawn beside, and a
// promotion becomes its piece. Returns false, changing nothing, if `from` is empty.
inline bool applyMove(Board& b, const Move& m) {
  const char p = b.sq[m.from];
  if (!p) return false;
  const int df = fileOf(m.to) - fileOf(m.from);
  if (lower(p) == 'k' && (df == 2 || df == -2)) {
    const int rookFrom = df > 0 ? m.from + 3 : m.from - 4;
    const int rookTo = df > 0 ? m.from + 1 : m.from - 1;
    b.sq[rookTo] = b.sq[rookFrom];
    b.sq[rookFrom] = '\0';
  }
  if (lower(p) == 'p' && df != 0 && !b.sq[m.to]) {
    b.sq[m.to + (isWhite(p) ? -8 : 8)] = '\0';
  }
  b.sq[m.to] = p;
  b.sq[m.from] = '\0';
  if (m.promo) b.sq[m.to] = isWhite(p) ? upper(m.promo) : m.promo;
  b.turn = b.turn == 'w' ? 'b' : 'w';
  b.lastFrom = m.from;
  b.lastTo = m.to;
  return true;
}

// One bank line, "rating|FEN|moves|themes", read where it lies: the FEN and themes point into
// the line.
struct Puzzle {
  int rating = 0;
  const char* fen = nullptr;
  Move moves[kMaxMoves];
  int moveCount = 0;
  const char* themes = "";
  size_t themesLen = 0;
};

// Reads a line (read_puzzle). Returns false unless every part is there, the FEN reads and every
// move plays from it.
inline bool parsePuzzle(const char* line, Puzzle& out) {
  Puzzle p;
  const char* s = line;
  if (*s < '0' || *s > '9') return false;
  while (*s >= '0' && *s <= '9') p.rating = p.rating * 10 + (*s++ - '0');
  if (*s++ != '|') return false;
  p.fen = s;
  const char* bar = std::strchr(s, '|');
  if (!bar) return false;
  s = bar + 1;
  while (*s && *s != '|') {
    while (*s == ' ') s++;
    const char* start = s;
    while (*s && *s != ' ' && *s != '|') s++;
    if (s == start) break;
    if (p.moveCount == kMaxMoves || !parseMove(start, static_cast<size_t>(s - start), p.moves[p.moveCount])) return false;
    p.moveCount++;
  }
  if (p.moveCount < 2) return false;
  if (*s == '|') {
    p.themes = s + 1;
    p.themesLen = std::strlen(p.themes);
  }
  Board b;
  if (!setFen(b, p.fen)) return false;
  for (int k = 0; k < p.moveCount; k++) {
    if (!applyMove(b, p.moves[k])) return false;
  }
  out = p;
  return true;
}

// The themes as words (theme_words): "discoveredAttack mateIn2" -> "DISCOVERED ATTACK, MATE IN 2".
// The source writes them in lower case; the font here has only capitals.
inline void themeWords(const char* themes, size_t len, char* out, size_t size) {
  size_t at = 0;
  auto put = [&](char c) {
    if (at + 1 < size) out[at++] = c;
  };
  bool first = true;
  size_t i = 0;
  while (i < len) {
    while (i < len && themes[i] == ' ') i++;
    if (i >= len) break;
    if (!first) {
      put(',');
      put(' ');
    }
    first = false;
    char prev = '\0';
    for (; i < len && themes[i] != ' '; i++) {
      const char c = themes[i];
      const bool lowerToUpper = prev >= 'a' && prev <= 'z' && c >= 'A' && c <= 'Z';
      const bool letterToDigit = ((prev >= 'a' && prev <= 'z') || (prev >= 'A' && prev <= 'Z')) && c >= '0' && c <= '9';
      if (lowerToUpper || letterToDigit) put(' ');
      put(upper(c));
      prev = c;
    }
  }
  out[at] = '\0';
}

}  // namespace chesspuzzles
