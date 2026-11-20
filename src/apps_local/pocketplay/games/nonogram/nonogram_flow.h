#pragma once

#include <cstdint>
#include <vector>

#include "nonogram_rules.h"

// What each tap does to Nonograms, with no screen or card, so it can be tested off the device:
// the board, the clear check, the size page, your place and solved puzzles in every size, and
// the puzzles left half done.
namespace nonogram {

enum class View : uint8_t { Board, ConfirmClear, Sizes };

enum class Message : uint8_t { Help, FillMode, CrossMode, Solved };

// What a tap changed, for the screen to choose its refresh and what to save.
enum class Change : uint8_t { None, View, Move, NewPuzzle, Solved };

constexpr int kDoneBytes = (kBankPuzzles + 7) / 8;  // 19
// Half-done puzzles kept per size, the newest first, as in the source.
constexpr size_t kKeepGrids = 40;

struct Kept {
  uint8_t puzzle = 0;
  std::vector<uint8_t> cells;
};

struct State {
  uint8_t size = 2;  // 10x10, the source's first size
  uint8_t place[kSizes] = {};
  bool cross = false;  // what a tap does: fill, or mark X
  uint8_t done[kSizes][kDoneBytes] = {};
  std::vector<Kept> kept[kSizes];
  Board board;
  View view = View::Board;
  Message message = Message::Help;
};

inline int sizeN(const State& s) { return kSizeList[s.size]; }

inline bool isDone(const State& s, int size, int puzzle) { return (s.done[size][puzzle / 8] >> (puzzle % 8)) & 1; }

inline int doneCount(const State& s, int size) {
  int n = 0;
  for (int p = 0; p < kBankPuzzles; p++) n += isDone(s, size, p);
  return n;
}

// The board into the kept grids: first if it's half done, gone if it's blank or solved.
inline void keepBoard(State& s) {
  std::vector<Kept>& list = s.kept[s.size];
  const uint8_t puzzle = s.place[s.size];
  for (size_t k = 0; k < list.size(); k++) {
    if (list[k].puzzle == puzzle) {
      list.erase(list.begin() + static_cast<long>(k));
      break;
    }
  }
  if (blank(s.board) || solved(s.board)) return;
  Kept kept;
  kept.puzzle = puzzle;
  kept.cells.assign(s.board.cells, s.board.cells + cellCount(s.board));
  list.insert(list.begin(), kept);
  if (list.size() > kKeepGrids) list.resize(kKeepGrids);
}

// The puzzle at your place in the current size, with its kept cells if it has some.
inline void loadPuzzle(State& s) {
  const int n = sizeN(s);
  decode(bankFor(s.size)[s.place[s.size]], n, s.board.puzzle);
  clearCells(s.board);
  for (const Kept& k : s.kept[s.size]) {
    if (k.puzzle == s.place[s.size] && k.cells.size() == static_cast<size_t>(n * n)) {
      for (int i = 0; i < n * n; i++) s.board.cells[i] = k.cells[static_cast<size_t>(i)];
    }
  }
  s.view = View::Board;
  s.message = solved(s.board) ? Message::Solved : Message::Help;
}

inline Change markSolved(State& s) {
  s.done[s.size][s.place[s.size] / 8] |= static_cast<uint8_t>(1 << (s.place[s.size] % 8));
  s.message = Message::Solved;
  keepBoard(s);
  return Change::Solved;
}

// A tap on a cell, or a hold (always an X).
inline Change tapAt(State& s, int cell, bool hold) {
  if (s.view != View::Board || !tapCell(s.board, cell, hold || s.cross)) return Change::None;
  s.message = Message::Help;
  if (solved(s.board)) return markSolved(s);
  keepBoard(s);
  return Change::Move;
}

inline bool canUndo(const State& s) { return s.view == View::Board && !s.board.history.empty() && !solved(s.board); }

inline Change undoTap(State& s) {
  if (s.view != View::Board || !undo(s.board)) return Change::None;
  s.message = Message::Help;
  keepBoard(s);
  return Change::Move;
}

// FILL / X: what a tap does, kept in the save.
inline Change toggleMode(State& s) {
  if (s.view != View::Board) return Change::None;
  s.cross = !s.cross;
  s.message = s.cross ? Message::CrossMode : Message::FillMode;
  return Change::Move;
}

// NEXT, and the Up and Down buttons: round at the ends of the bank.
inline Change stepPuzzle(State& s, int delta) {
  if (s.view != View::Board) return Change::None;
  keepBoard(s);
  s.place[s.size] = static_cast<uint8_t>(((s.place[s.size] + delta) % kBankPuzzles + kBankPuzzles) % kBankPuzzles);
  loadPuzzle(s);
  return Change::NewPuzzle;
}

// CLEAR asks first, as the source does, unless there's nothing to clear.
inline bool canClear(const State& s) { return !blank(s.board); }

inline Change askClear(State& s) {
  if (s.view != View::Board || !canClear(s)) return Change::None;
  s.view = View::ConfirmClear;
  return Change::View;
}

inline Change confirmClear(State& s) {
  if (s.view != View::ConfirmClear) return Change::None;
  clearCells(s.board);
  keepBoard(s);
  s.view = View::Board;
  s.message = Message::Help;
  return Change::NewPuzzle;
}

inline Change openSizes(State& s) {
  if (s.view != View::Board) return Change::None;
  s.view = View::Sizes;
  return Change::View;
}

inline Change chooseSize(State& s, int size) {
  if (s.view != View::Sizes || size < 0 || size >= kSizes) return Change::None;
  if (size == s.size) {
    s.view = View::Board;
    return Change::View;
  }
  keepBoard(s);
  s.size = static_cast<uint8_t>(size);
  loadPuzzle(s);
  return Change::NewPuzzle;
}

// BACK from the clear check or the size page.
inline Change closePage(State& s) {
  if (s.view == View::Board) return Change::None;
  s.view = View::Board;
  return Change::View;
}

}  // namespace nonogram
