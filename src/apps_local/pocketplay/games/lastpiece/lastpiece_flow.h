#pragma once

#include <cstdint>

#include "lastpiece_pack.h"
#include "lastpiece_rules.h"

// What a tap or button does to a game of Last Piece, with no screen or card, so the whole flow
// can be tested off the device. Each action says what changed, which the screen turns into
// Nothing, Redraw or RedrawFull. The challenges come from the card, so the state keeps the board
// it started from and checks it against the card each time the files are read.
namespace lastpiece {

constexpr int kDifficulties = 4;
constexpr const char* kDifficultyNames[kDifficulties] = {"BEGINNER", "INTERMEDIATE", "ADVANCED", "EXPERT"};
constexpr int kSolvedBytes = (kMaxChallenges + 7) / 8;

enum class Message : uint8_t { Help, Selected, CannotCapture, Hint, NoWin };

enum class Change : uint8_t {
  None,  // nothing to redraw
  View,  // only the selection or a message changed
  Move,  // a capture, or one taken back
  Full,  // another challenge, a capture that ends one, or an undo out of the end
};

struct State {
  uint8_t difficulty = 0;
  uint8_t challenge[kDifficulties] = {};          // the one you're on at each difficulty, from 0
  uint8_t solved[kDifficulties][kSolvedBytes] = {};  // a bit for each challenge solved
  Board start;                                    // the board the moves were played from
  uint8_t moves[kMaxMoves] = {};                  // each capture as from << 4 | to
  uint8_t moveCount = 0;
  // Worked out from the above, never saved.
  Board board;
  uint8_t taken[kMaxMoves] = {};  // what each capture took, in order
  int sel = kNoCell;
  int hintTo = kNoCell;  // the piece a hint says to take, bracketed alone
  Message message = Message::Help;
};

// Rebuilds the board from the start and the moves. Returns false at the first move that isn't a
// capture, which leaves the moves before it in place.
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

inline bool isSolved(const State& s, int difficulty, int index) {
  return (s.solved[difficulty][index / 8] >> (index % 8)) & 1;
}

inline int solvedCount(const State& s, int difficulty, int count) {
  int n = 0;
  for (int k = 0; k < count; k++) n += isSolved(s, difficulty, k);
  return n;
}

inline void clearView(State& s) {
  s.sel = kNoCell;
  s.hintTo = kNoCell;
  s.message = Message::Help;
}

// Matches the state to the challenges just read from the card. The moves are kept only if the
// challenge you were on still has the same board, so editing a file never replays old moves on
// a different puzzle. Returns true if the saved position changed.
inline bool sync(State& s, const Pack& pack) {
  bool changed = false;
  uint8_t& index = s.challenge[s.difficulty];
  if (index >= pack.count && index != 0) {
    index = 0;
    changed = true;
  }
  const Board board = pack.count > 0 ? pack.challenges[index].board : Board{};
  if (!sameBoard(board, s.start)) {
    s.start = board;
    s.moveCount = 0;
    changed = true;
  }
  if (!replay(s)) changed = true;
  clearView(s);
  return changed;
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
  const bool hinted = s.hintTo != kNoCell;
  s.hintTo = kNoCell;
  if (s.sel != kNoCell && canCapture(s.board, s.sel, i)) {
    s.taken[s.moveCount] = capture(s.board, s.sel, i);
    s.moves[s.moveCount++] = static_cast<uint8_t>(s.sel << 4 | i);
    s.sel = kNoCell;
    s.message = Message::Help;
    if (isWon(s)) {
      const int index = s.challenge[s.difficulty];
      s.solved[s.difficulty][index / 8] = static_cast<uint8_t>(s.solved[s.difficulty][index / 8] | 1 << (index % 8));
    }
    return isOver(s) ? Change::Full : Change::Move;
  }
  if (s.board.cell[i] == None || i == s.sel) {
    if (s.sel == kNoCell) return hinted ? Change::View : Change::None;
    clearView(s);
    return Change::View;
  }
  int t[8];
  if (capturesFrom(s.board, i, t) > 0) {
    s.sel = i;
    s.message = Message::Selected;
    return Change::View;
  }
  const bool had = s.sel != kNoCell || hinted;
  s.sel = kNoCell;
  const Change c = setMessage(s, Message::CannotCapture);
  return had ? Change::View : c;
}

// A tap off the board puts down the piece picked up.
inline Change tapOff(State& s) {
  if (s.sel == kNoCell && s.hintTo == kNoCell) return Change::None;
  clearView(s);
  return Change::View;
}

inline Change undoMove(State& s) {
  if (!canUndo(s)) return Change::None;
  const bool wasOver = isOver(s);
  s.moveCount--;
  replay(s);
  clearView(s);
  return wasOver ? Change::Full : Change::Move;
}

// HINT picks up the piece to move and brackets the one to take. While your moves follow the
// solutions file, it gives that solution's next move; otherwise it searches from here.
inline Change hint(State& s, const Pack& pack, Memo& memo) {
  if (isOver(s) || pack.count == 0) return Change::None;
  const Challenge& c = pack.challenges[s.challenge[s.difficulty]];
  bool onPath = c.solutionLength > s.moveCount;
  for (int k = 0; k < s.moveCount && onPath; k++) onPath = s.moves[k] == c.solution[k];
  int from = kNoCell, to = kNoCell;
  if (onPath) {
    from = moveFrom(c.solution[s.moveCount]);
    to = moveTo(c.solution[s.moveCount]);
  } else if (!bestMove(s.board, memo, from, to)) {
    if (s.sel == kNoCell && s.hintTo == kNoCell) return setMessage(s, Message::NoWin);
    clearView(s);
    s.message = Message::NoWin;
    return Change::View;
  }
  if (s.sel == from && s.hintTo == to) return Change::None;
  s.sel = from;
  s.hintTo = to;
  s.message = Message::Hint;
  return Change::View;
}

// RESET puts every piece back where the challenge started. The solved mark stays.
inline Change resetChallenge(State& s) {
  if (!canUndo(s)) return Change::None;
  s.moveCount = 0;
  replay(s);
  clearView(s);
  return Change::Full;
}

inline void openChallenge(State& s, const Pack& pack, int index) {
  s.challenge[s.difficulty] = static_cast<uint8_t>(index);
  s.start = pack.challenges[index].board;
  s.moveCount = 0;
  replay(s);
  clearView(s);
}

// The Up and Down keys, wrapping at the ends. A challenge left part-way starts again when you come back.
inline Change step(State& s, const Pack& pack, int delta) {
  if (pack.count < 2) return Change::None;
  const int index = (s.challenge[s.difficulty] + delta + pack.count) % pack.count;
  openChallenge(s, pack, index);
  return Change::Full;
}

// A difficulty from the LEVELS list, at the challenge you were on there. Choosing the one you're
// on keeps your moves.
inline Change chooseDifficulty(State& s, const Pack& pack, int difficulty) {
  if (difficulty < 0 || difficulty >= kDifficulties || pack.count == 0) return Change::None;
  if (difficulty != s.difficulty) {
    s.difficulty = static_cast<uint8_t>(difficulty);
    s.start = Board{};
  }
  sync(s, pack);
  return Change::Full;
}

}  // namespace lastpiece
