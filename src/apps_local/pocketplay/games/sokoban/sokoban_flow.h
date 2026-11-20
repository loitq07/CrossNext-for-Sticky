#pragma once

#include <cstdint>

#include "sokoban_levels.h"
#include "sokoban_rules.h"

// What a move, tap or button does to a game of Sokoban, with no screen or card, so the whole
// flow can be tested off the device. Each action says what changed, which the screen turns
// into Nothing, Redraw or RedrawFull, and which saves it needs.
namespace sokoban {

constexpr int kTotalLevels = [] {
  int total = 0;
  for (const PackDef& p : kPacks) total += p.count;
  return total;
}();

// Where pack p's levels start in the list of best scores.
inline int firstLevel(int pack) {
  int first = 0;
  for (int p = 0; p < pack; p++) first += kPacks[p].count;
  return first;
}

enum class Mode : uint8_t { Play, ConfirmReset, Packs };

// The line under the board: the level's title as it starts, nothing once you move, or
// what the last move did.
enum class Message : uint8_t { Title, None, Solved, NewBest, HistoryFull };

enum class Change : uint8_t {
  None,      // nothing to redraw
  View,      // only what's shown changed: a message, the dialog or the pack list
  Move,      // the board changed
  NewLevel,  // another level, or this one from the start
  Solved,    // this move solved the level
};

struct State {
  uint8_t pack = 0;
  uint8_t place[kPackCount] = {};  // each pack's level, 0-based
  Game game;
  Mode mode = Mode::Play;
  Message message = Message::Title;
  uint16_t best[kTotalLevels] = {};  // fewest moves per level, 0 = not solved
  bool bestChanged = false;          // set when a solve beats the record, cleared once saved
};

inline const LevelDef& currentLevel(const State& s) { return kPacks[s.pack].levels[s.place[s.pack]]; }
inline uint16_t currentBest(const State& s) { return s.best[firstLevel(s.pack) + s.place[s.pack]]; }
inline bool isSolved(const State& s) { return solved(s.game.board); }

inline void loadLevel(State& s) {
  start(currentLevel(s), s.game);
  s.mode = Mode::Play;
  s.message = Message::Title;
}

inline bool canUndo(const State& s) { return s.mode == Mode::Play && s.game.count > 0; }
inline bool canReset(const State& s) { return s.mode == Mode::Play && s.game.count > 0; }

inline Change go(State& s, Dir d) {
  if (s.mode != Mode::Play || isSolved(s)) return Change::None;
  if (s.game.count >= kMaxHistory) {
    if (s.message == Message::HistoryFull) return Change::None;
    s.message = Message::HistoryFull;
    return Change::View;
  }
  if (!move(s.game, d)) return Change::None;
  s.message = Message::None;
  if (!isSolved(s)) return Change::Move;
  uint16_t& best = s.best[firstLevel(s.pack) + s.place[s.pack]];
  if (best == 0 || s.game.moves < best) {
    s.message = best == 0 ? Message::Solved : Message::NewBest;
    best = s.game.moves;
    s.bestChanged = true;
  } else {
    s.message = Message::Solved;
  }
  return Change::Solved;
}

// The way a swipe or a tap points: along whichever axis it moved further, across when
// they're equal, as the source has it. dx and dy are in screen pixels, down positive.
inline Dir dirOf(int dx, int dy) {
  const int ax = dx < 0 ? -dx : dx;
  const int ay = dy < 0 ? -dy : dy;
  if (ax >= ay) return dx < 0 ? Dir::Left : Dir::Right;
  return dy < 0 ? Dir::Up : Dir::Down;
}

inline Change undoMove(State& s) {
  if (!canUndo(s)) return Change::None;
  undo(s.game);
  s.message = Message::None;
  return Change::Move;
}

// RESET asks first, as in the source.
inline Change askReset(State& s) {
  if (!canReset(s)) return Change::None;
  s.mode = Mode::ConfirmReset;
  return Change::View;
}

inline Change answerReset(State& s, bool yes) {
  if (s.mode != Mode::ConfirmReset) return Change::None;
  if (yes) {
    loadLevel(s);
    return Change::NewLevel;
  }
  s.mode = Mode::Play;
  return Change::View;
}

// PREV and NEXT: the pack wraps at both ends.
inline Change stepLevel(State& s, int delta) {
  if (s.mode != Mode::Play) return Change::None;
  const int count = kPacks[s.pack].count;
  s.place[s.pack] = static_cast<uint8_t>((s.place[s.pack] + delta % count + count) % count);
  loadLevel(s);
  return Change::NewLevel;
}

// The Up and Down buttons step the player, and once a level is solved Down goes on to
// the next one.
inline Change upButton(State& s) { return go(s, Dir::Up); }
inline Change downButton(State& s) {
  if (s.mode == Mode::Play && isSolved(s)) return stepLevel(s, 1);
  return go(s, Dir::Down);
}

inline Change openPacks(State& s) {
  if (s.mode != Mode::Play) return Change::None;
  s.mode = Mode::Packs;
  return Change::View;
}

inline Change closePacks(State& s) {
  if (s.mode != Mode::Packs) return Change::None;
  s.mode = Mode::Play;
  return Change::View;
}

// Choosing the pack you're in goes back to the level as it was; another pack opens at the
// level you were on there.
inline Change choosePack(State& s, int pack) {
  if (s.mode != Mode::Packs || pack < 0 || pack >= kPackCount) return Change::None;
  if (pack == s.pack) return closePacks(s);
  s.pack = static_cast<uint8_t>(pack);
  loadLevel(s);
  return Change::NewLevel;
}

}  // namespace sokoban
