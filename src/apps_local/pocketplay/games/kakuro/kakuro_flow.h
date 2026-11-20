#pragma once

#include <cstdint>

#include "kakuro_rules.h"

// What each tap does to Kakuro, with no screen or card, so it can be tested off the device: the
// board, the MORE page and how to play, your place and solved puzzles in every level.
namespace kakuro {

enum class View : uint8_t { Board, More, Rules };

enum class Message : uint8_t { Help, FromHint, Hint, Solved };

// What a tap changed, for the screen to choose its refresh and whether to save.
enum class Change : uint8_t { None, View, Move, NewPuzzle, Solved };

constexpr int kDoneBytes = kBankPuzzles / 8;  // 25, with nothing spare

struct State {
  uint8_t level = 0;
  uint8_t place[kLevels] = {};
  uint8_t done[kLevels][kDoneBytes] = {};
  Board board;
  View view = View::Board;
  Message message = Message::Help;
  int hintDigit = 0;
};

inline bool isDone(const State& s, int lvl, int puzzle) { return (s.done[lvl][puzzle / 8] >> (puzzle % 8)) & 1; }

inline int doneCount(const State& s, int lvl) {
  int n = 0;
  for (int p = 0; p < kBankPuzzles; p++) n += isDone(s, lvl, p);
  return n;
}

// The puzzle at your place in the level, empty. Only the puzzle on screen keeps its digits,
// as in the source.
inline void loadPuzzle(State& s) {
  const Level& l = level(s.level);
  parse(l.lines[s.place[s.level]], l.n, s.board.puzzle);
  clearEntries(s.board);
  s.board.selected = firstOpen(s.board);
  s.view = View::Board;
  s.message = Message::Help;
}

// After the digits are put back from a save.
inline void settle(State& s) {
  if (solved(s.board)) {
    s.board.selected = -1;
    s.message = Message::Solved;
  } else {
    s.board.selected = firstOpen(s.board);
    s.message = Message::Help;
  }
}

inline Change finish(State& s) {
  if (!solved(s.board)) return Change::Move;
  s.board.selected = -1;
  s.done[s.level][s.place[s.level] / 8] |= static_cast<uint8_t>(1 << (s.place[s.level] % 8));
  s.message = Message::Solved;
  return Change::Solved;
}

inline Change selectCell(State& s, int i) {
  const Board& b = s.board;
  if (s.view != View::Board || solved(b) || i < 0 || i >= b.puzzle.n * b.puzzle.n || !white(b.puzzle, i) || i == b.selected) {
    return Change::None;
  }
  s.board.selected = i;
  s.message = Message::Help;
  return Change::View;
}

inline bool canEnter(const State& s) { return s.view == View::Board && !solved(s.board) && s.board.selected >= 0; }

// A digit key: into the selected cell. A digit from HINT stays put.
inline Change putDigit(State& s, int d) {
  if (!canEnter(s) || d < 1 || d > 9) return Change::None;
  Board& b = s.board;
  const int x = b.selected;
  if (b.hinted[x]) {
    if (s.message == Message::FromHint) return Change::None;
    s.message = Message::FromHint;
    return Change::View;
  }
  if (b.entry[x] == d) return Change::None;
  b.entry[x] = static_cast<uint8_t>(d);
  s.message = Message::Help;
  return finish(s);
}

inline bool canErase(const State& s) { return canEnter(s) && s.board.entry[s.board.selected] != 0; }

inline Change erase(State& s) {
  if (!canErase(s)) return Change::None;
  Board& b = s.board;
  if (b.hinted[b.selected]) {
    if (s.message == Message::FromHint) return Change::None;
    s.message = Message::FromHint;
    return Change::View;
  }
  b.entry[b.selected] = 0;
  s.message = Message::Help;
  return Change::Move;
}

inline bool canHint(const State& s) { return s.view == View::Board && !solved(s.board); }

// HINT fills the selected cell if it's wrong or empty, else the first that is.
inline Change hint(State& s) {
  if (!canHint(s)) return Change::None;
  Board& b = s.board;
  const Puzzle& p = b.puzzle;
  int x = -1;
  if (b.selected >= 0 && b.entry[b.selected] != p.solution[b.selected]) x = b.selected;
  for (int i = 0; x < 0 && i < p.n * p.n; i++) {
    if (white(p, i) && b.entry[i] != p.solution[i]) x = i;
  }
  if (x < 0) return Change::None;
  b.entry[x] = p.solution[x];
  b.hinted[x] = true;
  b.selected = x;
  s.message = Message::Hint;
  s.hintDigit = p.solution[x];
  return finish(s);
}

// NEXT, PREVIOUS and the Up and Down buttons: round at the ends of the bank.
inline Change stepPuzzle(State& s, int delta) {
  if (s.view == View::Rules) return Change::None;
  s.place[s.level] = static_cast<uint8_t>(((s.place[s.level] + delta) % kBankPuzzles + kBankPuzzles) % kBankPuzzles);
  loadPuzzle(s);
  return Change::NewPuzzle;
}

inline Change openMore(State& s) {
  if (s.view != View::Board) return Change::None;
  s.view = View::More;
  return Change::View;
}

inline Change openRules(State& s) {
  s.view = View::Rules;
  return Change::View;
}

inline Change closePage(State& s) {
  if (s.view == View::Board) return Change::None;
  s.view = View::Board;
  return Change::View;
}

inline Change chooseLevel(State& s, int lvl) {
  if (s.view != View::More || lvl < 0 || lvl >= kLevels) return Change::None;
  if (lvl == s.level) return closePage(s);
  s.level = static_cast<uint8_t>(lvl);
  loadPuzzle(s);
  return Change::NewPuzzle;
}

inline Change startOver(State& s) {
  loadPuzzle(s);
  return Change::NewPuzzle;
}

}  // namespace kakuro
