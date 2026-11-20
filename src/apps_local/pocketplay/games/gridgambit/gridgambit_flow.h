#pragma once

#include <cstdint>

#include "gridgambit_rules.h"

// What a tap or button does to a game of Grid Gambit, with no screen or card, so the whole flow
// can be tested off the device. Each action says what changed, which the screen turns into
// Nothing, Redraw or RedrawFull.
namespace gridgambit {

enum class Message : uint8_t { Help, Selected, CannotCapture };

enum class Change : uint8_t {
  None,  // nothing to redraw
  View,  // only the selection or a message changed
  Move,  // a capture, or one taken back
  Full,  // a new puzzle, a capture that ends it, or an undo out of the end
};

struct State {
  Board start;                        // the puzzle as it was set
  uint8_t moves[kMaxMoves] = {};      // each capture as from << 4 | to
  uint8_t moveCount = 0;
  uint8_t level = 0;
  uint16_t solved[kLevels] = {};      // puzzles solved at each level
  bool counted = false;               // this puzzle's solve is already in `solved`
  // Worked out from the above by replay(), never saved.
  Board board;
  uint8_t taken[kMaxMoves] = {};      // what each capture took, in order
  int sel = kNoCell;
  Message message = Message::Help;
};

inline int moveFrom(uint8_t m) { return m >> 4; }
inline int moveTo(uint8_t m) { return m & 0x0F; }

// Rebuilds the board from the start and the moves. Returns false at the first move that isn't
// a capture, which leaves the moves before it in place.
inline bool replay(State& s) {
  s.board = s.start;
  for (int k = 0; k < s.moveCount; k++) {
    const uint8_t t = capture(s.board, moveFrom(s.moves[k]), moveTo(s.moves[k]));
    if (t == None) {
      s.moveCount = static_cast<uint8_t>(k);
      return false;
    }
    s.taken[k] = t;
  }
  return true;
}

inline bool isWon(const State& s) { return isWon(s.board); }
inline bool isLost(const State& s) { return isLost(s.board); }
inline bool isOver(const State& s) { return isWon(s) || isLost(s); }
inline bool canUndo(const State& s) { return s.moveCount > 0; }
inline int startPieces(const State& s) { return pieceCount(s.start); }

inline void setPuzzle(State& s, const Board& b) {
  s.start = b;
  s.moveCount = 0;
  s.counted = false;
  s.sel = kNoCell;
  s.message = Message::Help;
  replay(s);
}

inline Change setMessage(State& s, Message m) {
  if (s.message == m) return Change::None;
  s.message = m;
  return Change::View;
}

// A tap on cell i: a piece that can capture is picked up, or put down if it was; a bracketed
// piece is taken by the one picked up; a piece that can't capture says so.
inline Change tapCell(State& s, int i) {
  if (isOver(s) || i < 0 || i >= kCells) return Change::None;
  if (s.sel != kNoCell && canCapture(s.board, s.sel, i)) {
    s.taken[s.moveCount] = capture(s.board, s.sel, i);
    s.moves[s.moveCount++] = static_cast<uint8_t>(s.sel << 4 | i);
    s.sel = kNoCell;
    s.message = Message::Help;
    if (isWon(s) && !s.counted) {
      if (s.solved[s.level] < 0xFFFF) s.solved[s.level]++;
      s.counted = true;
    }
    return isOver(s) ? Change::Full : Change::Move;
  }
  if (s.board.cell[i] == None || i == s.sel) {
    if (s.sel == kNoCell) return Change::None;
    s.sel = kNoCell;
    s.message = Message::Help;
    return Change::View;
  }
  int t[8];
  if (capturesFrom(s.board, i, t) > 0) {
    s.sel = i;
    s.message = Message::Selected;
    return Change::View;
  }
  const bool had = s.sel != kNoCell;
  s.sel = kNoCell;
  const Change c = setMessage(s, Message::CannotCapture);
  return had ? Change::View : c;
}

// A tap off the board puts down the piece picked up.
inline Change tapOff(State& s) {
  if (s.sel == kNoCell) return Change::None;
  s.sel = kNoCell;
  s.message = Message::Help;
  return Change::View;
}

inline Change undoMove(State& s) {
  if (!canUndo(s)) return Change::None;
  const bool wasOver = isOver(s);
  s.moveCount--;
  replay(s);
  s.sel = kNoCell;
  s.message = Message::Help;
  return wasOver ? Change::Full : Change::Move;
}

// NEW and the level button: the screen makes the puzzle and hands it here.
inline Change newPuzzle(State& s, const Board& b) {
  setPuzzle(s, b);
  return Change::Full;
}

inline int nextLevel(const State& s) { return (s.level + 1) % kLevels; }

}  // namespace gridgambit
