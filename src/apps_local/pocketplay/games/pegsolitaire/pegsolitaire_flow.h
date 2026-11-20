#pragma once

#include <cstdint>

#include "pegsolitaire_rules.h"

// What a tap or button does to a game of Peg Solitaire, with no screen or card, so the whole
// flow can be tested off the device. Each action says what changed, which the screen turns into
// Nothing, Redraw or RedrawFull.
namespace pegsolitaire {

enum class Message : uint8_t { Help, Selected, CannotJump, PickMarked };

enum class Change : uint8_t {
  None,     // nothing to redraw
  View,     // only the selection or a message changed
  Move,     // a jump, or one taken back
  NewGame,  // a fresh board
  Over,     // this jump left no more
};

struct State {
  Game game;
  int sel = kNoCell;
  Message message = Message::Help;
  uint8_t best[kBoards] = {};  // fewest pegs left on each board, 0 = never finished
  bool bestChanged = false;    // set by a finish that beats the best, cleared once saved
};

inline bool isOver(const State& s) { return !anyMove(s.game); }
inline bool canUndo(const State& s) { return s.game.jumps > 0; }
inline bool canNew(const State& s) { return s.game.jumps > 0; }

inline void newGame(State& s, int board) {
  reset(s.game, board);
  s.sel = kNoCell;
  s.message = Message::Help;
}

inline Change setMessage(State& s, Message m) {
  if (s.message == m) return Change::None;
  s.message = m;
  return Change::View;
}

// When no jump is left the game is rated and the best kept (game_over).
inline Change finish(State& s) {
  if (!isOver(s)) return Change::Move;
  const int n = pegCount(s.game);
  uint8_t& best = s.best[s.game.board];
  if (best == 0 || n < best) {
    best = static_cast<uint8_t>(n);
    s.bestChanged = true;
  }
  return Change::Over;
}

// A tap on hole i (tap_cell): a peg selects it, or lets it go if it was selected; an empty
// hole takes the selected peg's jump there.
inline Change tapHole(State& s, int i) {
  if (isOver(s) || i < 0 || i >= def(s.game).cells) return Change::None;
  Jump jumps[6];
  if (hasPeg(s.game, i)) {
    if (s.sel == i) {
      s.sel = kNoCell;
      s.message = Message::Help;
      return Change::View;
    }
    if (jumpsFrom(s.game, i, jumps) == 0) {
      const bool had = s.sel != kNoCell;
      s.sel = kNoCell;
      const Change c = setMessage(s, Message::CannotJump);
      return had ? Change::View : c;
    }
    s.sel = i;
    s.message = Message::Selected;
    return Change::View;
  }
  if (s.sel == kNoCell) return Change::None;
  if (!jump(s.game, s.sel, i)) return setMessage(s, Message::PickMarked);
  s.sel = kNoCell;
  s.message = Message::Help;
  return finish(s);
}

// A tap off the holes lets the selected peg go.
inline Change tapOff(State& s) {
  if (s.sel == kNoCell) return Change::None;
  s.sel = kNoCell;
  s.message = Message::Help;
  return Change::View;
}

inline Change undoJump(State& s) {
  if (!canUndo(s)) return Change::None;
  undo(s.game);
  s.sel = kNoCell;
  s.message = Message::Help;
  return Change::Move;
}

inline Change startAgain(State& s) {
  if (!canNew(s)) return Change::None;
  newGame(s, s.game.board);
  return Change::NewGame;
}

// The board button: the other board, from the start, as in the source.
inline Change switchBoard(State& s) {
  newGame(s, (s.game.board + 1) % kBoards);
  return Change::NewGame;
}

}  // namespace pegsolitaire
