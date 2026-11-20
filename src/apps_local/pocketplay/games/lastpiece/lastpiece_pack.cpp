#include "lastpiece_pack.h"

namespace lastpiece {

namespace {

// One line at a time, with its line ending and the spaces around it taken off.
struct Lines {
  const uint8_t* text;
  size_t size;
  size_t at = 0;

  bool next(const uint8_t*& line, size_t& length) {
    if (at >= size) return false;
    size_t end = at;
    while (end < size && text[end] != '\n') end++;
    size_t first = at;
    size_t last = end;
    at = end + 1;
    auto space = [](uint8_t c) { return c == ' ' || c == '\t' || c == '\r'; };
    while (first < last && space(text[first])) first++;
    while (last > first && space(text[last - 1])) last--;
    line = text + first;
    length = last - first;
    return true;
  }
};

uint8_t pieceFor(uint8_t c) {
  if (c >= 'a' && c <= 'z') c = static_cast<uint8_t>(c - 'a' + 'A');
  switch (c) {
    case '.':
      return None;
    case 'K':
      return King;
    case 'Q':
      return Queen;
    case 'R':
      return Rook;
    case 'B':
      return Bishop;
    case 'N':
      return Knight;
    case 'P':
      return Pawn;
  }
  return kPieceTypes;  // not a cell
}

// A row of 4 cells, with spaces or tabs between them or none. Returns false for anything else.
bool parseRow(const uint8_t* line, size_t length, uint8_t* row) {
  int n = 0;
  for (size_t i = 0; i < length; i++) {
    if (line[i] == ' ' || line[i] == '\t') continue;
    const uint8_t p = pieceFor(line[i]);
    if (p == kPieceTypes || n == kSide) return false;
    row[n++] = p;
  }
  return n == kSide;
}

// The challenge being read, from its first row to the blank line or end of file after it.
struct Reading {
  Board board;
  int rows = 0;
  bool bad = false;
  bool started() const { return rows > 0 || bad; }
};

void finish(Reading& r, int& number, Pack& pack, Memo& memo) {
  if (!r.started()) return;
  // The number counts every challenge, so solutions still line up after a bad one.
  if (number < 255) number++;
  const int pieces = pieceCount(r.board);
  Board check = r.board;
  if (r.bad || r.rows != kSide || pieces < 2 || pieces > kMaxPieces || !solvable(check, memo)) {
    if (pack.skipped < 255) pack.skipped++;
  } else if (pack.count >= kMaxChallenges || number >= 255) {
    if (pack.overLimit < 255) pack.overLimit++;
  } else {
    Challenge& c = pack.challenges[pack.count++];
    c.board = r.board;
    c.number = static_cast<uint8_t>(number);
    c.solutionLength = 0;
  }
  r = Reading{};
}

bool isDigit(uint8_t c) { return c >= '0' && c <= '9'; }

// A cell as "row,col", each 0 to 3.
bool parseCell(const uint8_t* s, size_t length, size_t& at, int& cell) {
  if (at + 3 > length || !isDigit(s[at]) || s[at + 1] != ',' || !isDigit(s[at + 2])) return false;
  const int r = s[at] - '0';
  const int c = s[at + 2] - '0';
  if (!onBoard(r, c)) return false;
  cell = r * kSide + c;
  at += 3;
  return true;
}

// A solution line: "NN:" and then captures as "r,c->r,c", with any spaces between them.
bool parseSolutionLine(const uint8_t* line, size_t length, int& number, uint8_t* moves, int& moveCount) {
  // The spaces are taken out first, so "3,0 -> 2,1" reads the same as "3,0->2,1".
  uint8_t s[256];
  size_t n = 0;
  for (size_t i = 0; i < length; i++) {
    if (line[i] == ' ' || line[i] == '\t') continue;
    if (n == sizeof(s)) return false;
    s[n++] = line[i];
  }
  size_t at = 0;
  number = 0;
  while (at < n && isDigit(s[at])) {
    number = number * 10 + (s[at] - '0');
    if (number > 255) return false;
    at++;
  }
  if (at == 0 || at >= n || s[at] != ':') return false;
  at++;
  moveCount = 0;
  while (at < n) {
    int from, to;
    if (!parseCell(s, n, at, from)) return false;
    if (at + 2 > n || s[at] != '-' || s[at + 1] != '>') return false;
    at += 2;
    if (!parseCell(s, n, at, to)) return false;
    if (moveCount == kMaxMoves) return false;
    moves[moveCount++] = static_cast<uint8_t>(from << 4 | to);
  }
  return moveCount > 0;
}

}  // namespace

void parsePack(const uint8_t* text, size_t size, Pack& pack, Memo& memo) {
  // Field by field, not `pack = Pack{}`, which would build a 3 KB copy on the stack.
  pack.count = 0;
  pack.skipped = 0;
  pack.overLimit = 0;
  pack.badSolutions = 0;
  Lines lines{text, size};
  Reading r;
  int number = 0;
  const uint8_t* line;
  size_t length;
  while (lines.next(line, length)) {
    if (length == 0) {
      finish(r, number, pack, memo);
      continue;
    }
    if (line[0] == '#') continue;
    if (r.bad) continue;
    if (r.rows == kSide || !parseRow(line, length, r.board.cell + r.rows * kSide)) {
      r.bad = true;
      continue;
    }
    r.rows++;
  }
  finish(r, number, pack, memo);
}

void parseSolutions(const uint8_t* text, size_t size, Pack& pack) {
  Lines lines{text, size};
  const uint8_t* line;
  size_t length;
  while (lines.next(line, length)) {
    if (length == 0 || line[0] == '#') continue;
    int number = 0;
    uint8_t moves[kMaxMoves];
    int moveCount = 0;
    Challenge* challenge = nullptr;
    if (parseSolutionLine(line, length, number, moves, moveCount)) {
      for (int k = 0; k < pack.count; k++) {
        if (pack.challenges[k].number == number) challenge = &pack.challenges[k];
      }
    }
    // A second solution for the same challenge is dropped too: one of the two is a typo.
    bool ok = challenge != nullptr && challenge->solutionLength == 0;
    if (ok) {
      Board b = challenge->board;
      for (int k = 0; k < moveCount && ok; k++) ok = capture(b, moveFrom(moves[k]), moveTo(moves[k])) != None;
      ok = ok && isWon(b);
    }
    if (!ok) {
      if (pack.badSolutions < 255) pack.badSolutions++;
      continue;
    }
    for (int k = 0; k < moveCount; k++) challenge->solution[k] = moves[k];
    challenge->solutionLength = static_cast<uint8_t>(moveCount);
  }
}

}  // namespace lastpiece
