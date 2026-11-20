#pragma once

#include <cstdint>

#include "chesspuzzles_banks.h"
#include "chesspuzzles_rules.h"

// What a tap or button does in Chess Puzzles, with no screen or card, so the whole flow can be
// tested off the device: main.lua's start, play_mine, try_move, show_hint and tap. Each action
// says what changed, which the screen turns into Nothing, Redraw or RedrawFull.
namespace chesspuzzles {

enum class Message : uint8_t { Prompt, Good, NotTheMove, HintPiece, HintSquare, Solved, Unreadable };

enum class Change : uint8_t {
  None,  // nothing to redraw
  View,  // only the selection, a hint or a message changed
  Move,  // a move made, or a wrong one counted
  Full,  // a new puzzle, or the last move of one
};

struct State {
  uint8_t bank = 2;                // Easy, as the source opens
  uint16_t index[kBanks] = {};     // the puzzle each bank is on, from 0
  uint16_t solved[kBanks] = {};    // puzzles solved in each bank
  uint16_t clean[kBanks] = {};     // of those, solved with no wrong move and no SHOW
  uint8_t ply = 0;                 // the next of the puzzle's moves to be made
  uint8_t mistakes = 0;
  bool shown = false;              // SHOW was used on this puzzle
  bool done = false;
  // Worked out from the above, never saved.
  bool ok = false;                 // the puzzle read
  Puzzle puzzle;
  Board board;
  char me = 'w';                   // the side the player plays
  int sel = kNoSquare;
  uint8_t hint = 0;                // 1 frames the piece, 2 the piece and its square
  Message message = Message::Prompt;
};

inline const BankDef& bankDef(const State& s) { return kBankList[s.bank]; }

// Opens puzzle i of bank k (start), wrapping round the bank. The opponent's move is made, and
// with keepPly a puzzle left half way is replayed to where it was.
inline bool start(State& s, int k, int i, int keepPly = 0) {
  s.bank = static_cast<uint8_t>(k);
  const int n = kBankList[k].count;
  i = ((i % n) + n) % n;
  s.index[k] = static_cast<uint16_t>(i);
  s.sel = kNoSquare;
  s.hint = 0;
  s.mistakes = 0;
  s.shown = false;
  s.done = false;
  s.message = Message::Prompt;
  s.ok = parsePuzzle(kBankList[k].lines[i], s.puzzle) && setFen(s.board, s.puzzle.fen);
  if (!s.ok) {
    s.message = Message::Unreadable;
    s.ply = 0;
    return false;
  }
  s.me = s.board.turn == 'w' ? 'b' : 'w';
  applyMove(s.board, s.puzzle.moves[0]);
  s.ply = 1;
  while (s.ply < keepPly && s.ply < s.puzzle.moveCount) applyMove(s.board, s.puzzle.moves[s.ply++]);
  return true;
}

inline bool mine(const State& s, char p) { return p && isWhite(p) == (s.me == 'w'); }
inline bool canHelp(const State& s) { return s.ok && !s.done; }

inline void finish(State& s) {
  s.done = true;
  if (s.solved[s.bank] < 0xFFFF) s.solved[s.bank]++;
  if (s.mistakes == 0 && !s.shown && s.clean[s.bank] < 0xFFFF) s.clean[s.bank]++;
  s.message = Message::Solved;
}

// The player's move matched, or was shown: play it, then the reply (play_mine).
inline Change playMine(State& s) {
  applyMove(s.board, s.puzzle.moves[s.ply++]);
  if (s.ply < s.puzzle.moveCount) applyMove(s.board, s.puzzle.moves[s.ply++]);
  s.sel = kNoSquare;
  s.hint = 0;
  if (s.ply >= s.puzzle.moveCount) {
    finish(s);
    return Change::Full;
  }
  s.message = Message::Good;
  return Change::Move;
}

// A tap on a square (tap): one of your pieces is picked up, or put down if it was; with one
// picked up, any other square is the move, right or wrong.
inline Change tapSquare(State& s, int sq) {
  if (!canHelp(s) || sq < 0 || sq >= kSquares) return Change::None;
  if (mine(s, s.board.sq[sq])) {
    s.sel = s.sel == sq ? kNoSquare : sq;
    s.message = Message::Prompt;
    return Change::View;
  }
  if (s.sel == kNoSquare) return Change::None;
  const Move& want = s.puzzle.moves[s.ply];
  if (s.sel == want.from && sq == want.to) return playMine(s);
  if (s.mistakes < 0xFF) s.mistakes++;
  s.sel = kNoSquare;
  s.message = Message::NotTheMove;
  return Change::Move;
}

// A tap off the board puts down the piece picked up.
inline Change tapOff(State& s) {
  if (s.sel == kNoSquare) return Change::None;
  s.sel = kNoSquare;
  return Change::View;
}

// HINT frames the piece to move, and again its square too (show_hint).
inline Change showHint(State& s) {
  if (!canHelp(s) || s.hint == 2) return Change::None;
  s.hint++;
  s.message = s.hint == 1 ? Message::HintPiece : Message::HintSquare;
  return Change::View;
}

// SHOW makes the move for you; the solve then isn't counted as first try.
inline Change showMove(State& s) {
  if (!canHelp(s)) return Change::None;
  s.shown = true;
  return playMine(s);
}

// NEXT and the Down button: the next puzzle, wrapping round the bank.
inline Change next(State& s) {
  start(s, s.bank, s.index[s.bank] + 1);
  return Change::Full;
}

// The Up button: the puzzle before, stopping at the first.
inline Change previous(State& s) {
  if (s.index[s.bank] == 0) return Change::None;
  start(s, s.bank, s.index[s.bank] - 1);
  return Change::Full;
}

// A bank from the list, at the puzzle it was on, from the start.
inline Change pickBank(State& s, int k) {
  start(s, k, s.index[k]);
  return Change::Full;
}

}  // namespace chesspuzzles
