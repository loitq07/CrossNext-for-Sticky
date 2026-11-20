#pragma once

#include <cstdint>

#include "suguru_banks.h"
#include "suguru_rules.h"

// What a tap or button does to a game of Suguru, with no screen or card, so the whole
// flow can be tested off the device. Each action says what changed, which the screen
// turns into Nothing, Redraw or RedrawFull.
namespace suguru {

struct Bank {
  const char* label;
  int size;
  const char* const* lines;
};

constexpr int kBanks = 4;
constexpr Bank kBankList[kBanks] = {
    {"5X5 EASY", 5, EASY_5},
    {"6X6 MEDIUM", 6, MEDIUM_6},
    {"7X7 HARD", 7, HARD_7},
    {"8X8 EXPERT", 8, EXPERT_8},
};

constexpr int kNoCell = -1;

enum class Message : uint8_t { Help, CellFirst, Given };

enum class Change : uint8_t {
  None,       // nothing to redraw
  View,       // only the selection or a message changed: redraw, nothing to save
  Move,       // the grid or a setting changed
  NewPuzzle,  // another puzzle is on the board
  Solved,     // this move solved the puzzle
};

struct State {
  uint8_t bank = 0;
  uint8_t place[kBanks] = {};  // each bank's puzzle, 0-based
  bool check = true;           // cross the cells that break a rule
  Puzzle puzzle;
  uint8_t cells[kMaxCells] = {};
  int sel = kNoCell;
  bool solved = false;
  Message message = Message::Help;
};

// The puzzle at the state's bank and place, with only its givens filled. Every bank
// line parses (tests/suguru/ checks), so the empty grid is only a safety net.
inline void loadPuzzle(State& s) {
  const Bank& b = kBankList[s.bank];
  if (!parseLine(b.lines[s.place[s.bank]], s.puzzle)) {
    s.puzzle = Puzzle{};
    s.puzzle.n = b.size;
    for (int i = 0; i < b.size * b.size; i++) s.puzzle.regions[i] = 'A';
  }
  for (int i = 0; i < kMaxCells; i++) s.cells[i] = s.puzzle.givens[i];
  s.sel = kNoCell;
  s.solved = false;
  s.message = Message::Help;
}

inline int cellCount(const State& s) { return s.puzzle.n * s.puzzle.n; }
inline bool isGiven(const State& s, int i) { return s.puzzle.givens[i] != 0; }

// Whether the pad can write into the selected cell, and whether ERASE can clear it.
inline bool canEnter(const State& s) { return !s.solved && s.sel != kNoCell && !isGiven(s, s.sel); }
inline bool canErase(const State& s) { return canEnter(s) && s.cells[s.sel] != 0; }

inline Change setMessage(State& s, Message m) {
  if (s.message == m) return Change::None;
  s.message = m;
  return Change::View;
}

// Tapping a cell selects it, given or not, and clears any message.
inline Change select(State& s, int i) {
  if (i < 0 || i >= cellCount(s)) return Change::None;
  if (s.sel == i && s.message == Message::Help) return Change::None;
  s.sel = i;
  s.message = Message::Help;
  return Change::View;
}

// d is 1..5 from the pad, or 0 from ERASE (set_cell).
inline Change setCell(State& s, uint8_t d) {
  if (s.solved) return Change::None;
  if (s.sel == kNoCell) return d == 0 ? Change::None : setMessage(s, Message::CellFirst);
  if (isGiven(s, s.sel)) return setMessage(s, Message::Given);
  if (s.cells[s.sel] == d) return Change::None;
  s.cells[s.sel] = d;
  s.message = Message::Help;
  if (solved(s.puzzle, s.cells)) {
    s.solved = true;
    return Change::Solved;
  }
  return Change::Move;
}

inline Change toggleCheck(State& s) {
  s.check = !s.check;
  return Change::Move;
}

// NEXT and the Up and Down buttons: the bank wraps at both ends (step_puzzle).
inline Change step(State& s, int delta) {
  const int place = (s.place[s.bank] + delta % kBankPuzzles + kBankPuzzles) % kBankPuzzles;
  s.place[s.bank] = static_cast<uint8_t>(place);
  loadPuzzle(s);
  return Change::NewPuzzle;
}

// BANK: the next bank, at the puzzle it was left on.
inline Change nextBank(State& s) {
  s.bank = static_cast<uint8_t>((s.bank + 1) % kBanks);
  loadPuzzle(s);
  return Change::NewPuzzle;
}

}  // namespace suguru
