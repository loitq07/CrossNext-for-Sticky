#pragma once

#include <cstdint>

#include "queens_banks.h"
#include "queens_rules.h"

// What a tap or button does to a game of Queens, with no screen or card, so the whole flow can
// be tested off the device. Each action says what changed, which the screen turns into Nothing,
// Redraw or RedrawFull, and which saves it needs.
namespace queens {

struct Bank {
  const char* label;  // as the status capsule shows it
  const char* menu;   // the size list's, short enough to leave room for the count
  const char* title;  // the band's
  int size;
  int pieces;
  const char* const* lines;
};

constexpr int kBanks = 6;
constexpr Bank kBankList[kBanks] = {
    {"QUEENS 6X6", "QUEENS 6X6", "QUEENS", 6, 1, QUEENS_6},
    {"QUEENS 7X7", "QUEENS 7X7", "QUEENS", 7, 1, QUEENS_7},
    {"QUEENS 8X8", "QUEENS 8X8", "QUEENS", 8, 1, QUEENS_8},
    {"QUEENS 9X9", "QUEENS 9X9", "QUEENS", 9, 1, QUEENS_9},
    {"STAR BATTLE 9X9", "STARS 9X9", "STAR BATTLE", 9, 2, STARS_9},
    {"STAR BATTLE 10X10", "STARS 10X10", "STAR BATTLE", 10, 2, STARS_10},
};

constexpr int kDoneBytes = (kBankPuzzles + 7) / 8;

enum class Mode : uint8_t { Board, More, Rules };

// The line under the board.
enum class Message : uint8_t {
  Help,
  Touch,
  Row,
  Column,
  Region,
  HintLocked,
  HintWrongPiece,
  HintCross,
  HintPlaced,
  Solved,
};

enum class Change : uint8_t {
  None,       // nothing to redraw
  View,       // only what's shown changed: a message or another page
  Move,       // the marks or a setting changed
  NewPuzzle,  // another puzzle, or this one from the start
  Solved,     // this tap solved the puzzle
};

struct State {
  uint8_t bank = 0;
  uint8_t place[kBanks] = {};  // each bank's puzzle, 0-based
  bool dots = true;            // dot the cells the pieces already rule out
  Game game;
  Mode mode = Mode::Board;
  Message message = Message::Help;
  uint8_t done[kBanks][kDoneBytes] = {};  // the puzzles solved, a bit each
  bool doneChanged = false;               // set by a solve, cleared once saved
};

inline bool isSolved(const State& s) { return solved(s.game); }
inline bool isDone(const State& s, int bank, int puzzle) { return (s.done[bank][puzzle / 8] >> (puzzle % 8)) & 1; }
inline int doneCount(const State& s, int bank) {
  int count = 0;
  for (int k = 0; k < kBankPuzzles; k++) count += isDone(s, bank, k) ? 1 : 0;
  return count;
}

// The puzzle at the state's bank and place, with no marks. Every bank line parses (tests/queens/
// checks), so the fallback is only a safety net.
inline void loadPuzzle(State& s) {
  const Bank& b = kBankList[s.bank];
  Puzzle p;
  if (!parseLine(b.lines[s.place[s.bank]], p)) {
    p = Puzzle{};
    p.n = b.size;
    p.k = b.pieces;
  }
  start(p, s.game);
  s.mode = Mode::Board;
  s.message = Message::Help;
}

inline bool canUndo(const State& s) { return s.mode == Mode::Board && !isSolved(s) && s.game.undoCount > 0; }
inline bool canHint(const State& s) { return s.mode == Mode::Board && !isSolved(s); }

inline Change setMessage(State& s, Message m) {
  if (s.message == m) return Change::None;
  s.message = m;
  return Change::View;
}

// After any change to the marks: what the line says, and whether it's solved (finish).
inline Change settle(State& s, Message otherwise) {
  if (isSolved(s)) {
    uint8_t& byte = s.done[s.bank][s.place[s.bank] / 8];
    const uint8_t bit = static_cast<uint8_t>(1 << (s.place[s.bank] % 8));
    if (!(byte & bit)) {
      byte |= bit;
      s.doneChanged = true;
    }
    s.message = Message::Solved;
    return Change::Solved;
  }
  s.message = otherwise;
  return Change::Move;
}

inline Message brokenMessage(Broken b) {
  switch (b) {
    case Broken::Touch: return Message::Touch;
    case Broken::Row: return Message::Row;
    case Broken::Column: return Message::Column;
    case Broken::Region: return Message::Region;
    case Broken::None: break;
  }
  return Message::Help;
}

// A tap on a cell: nothing -> cross -> piece -> nothing (cycle).
inline Change tapCell(State& s, int i) {
  if (s.mode != Mode::Board || isSolved(s) || i < 0 || i >= s.game.puzzle.n * s.game.puzzle.n) return Change::None;
  if (s.game.hinted[i]) return setMessage(s, Message::HintLocked);
  setMark(s.game, i, nextMark(s.game.marks[i]));
  bool bad[kMaxCells];
  return settle(s, brokenMessage(conflicts(s.game, bad)));
}

inline Change undoTap(State& s) {
  if (!canUndo(s)) return Change::None;
  takeBack(s.game);
  s.message = Message::Help;
  return Change::Move;
}

inline Change hintTap(State& s) {
  if (!canHint(s)) return Change::None;
  switch (hint(s.game)) {
    case Hint::RemovedPiece: return settle(s, Message::HintWrongPiece);
    case Hint::RemovedCross: return settle(s, Message::HintCross);
    case Hint::Placed: return settle(s, Message::HintPlaced);
    case Hint::None: break;
  }
  return Change::None;
}

// NEXT, PREVIOUS and the Up and Down buttons: the bank wraps at both ends.
inline Change stepPuzzle(State& s, int delta) {
  if (s.mode == Mode::Rules) return Change::None;
  s.place[s.bank] = static_cast<uint8_t>((s.place[s.bank] + delta % kBankPuzzles + kBankPuzzles) % kBankPuzzles);
  loadPuzzle(s);
  return Change::NewPuzzle;
}

inline Change openMore(State& s) {
  if (s.mode != Mode::Board) return Change::None;
  s.mode = Mode::More;
  return Change::View;
}

inline Change openRules(State& s) {
  if (s.mode == Mode::Rules) return Change::None;
  s.mode = Mode::Rules;
  return Change::View;
}

// BACK on the menu or the rules goes back to the board.
inline Change closePage(State& s) {
  if (s.mode == Mode::Board) return Change::None;
  s.mode = Mode::Board;
  return Change::View;
}

// A size from the menu: the bank you're in goes back to the board as it was; another opens at
// the puzzle you were on there.
inline Change chooseBank(State& s, int bank) {
  if (s.mode != Mode::More || bank < 0 || bank >= kBanks) return Change::None;
  if (bank == s.bank) return closePage(s);
  s.bank = static_cast<uint8_t>(bank);
  loadPuzzle(s);
  return Change::NewPuzzle;
}

// START OVER clears every mark, as the source's does, without asking: it's already a second tap.
inline Change restart(State& s) {
  if (s.mode != Mode::More) return Change::None;
  loadPuzzle(s);
  return Change::NewPuzzle;
}

inline Change toggleDots(State& s) {
  if (s.mode != Mode::More) return Change::None;
  s.dots = !s.dots;
  s.mode = Mode::Board;
  return Change::Move;
}

}  // namespace queens
