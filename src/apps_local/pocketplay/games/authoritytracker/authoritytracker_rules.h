#pragma once

// Authority Tracker's rules: two scores that start at 50 and move by the buttons'
// amounts. A score that reaches 0 ends the game, and the player at 0 can still add
// to it, to take back a mistaken tap. Standard library only, so it's tested on the PC.
namespace authoritytracker {

constexpr int kPlayers = 2;
constexpr int kStart = 50;
constexpr int kMax = 999;  // three digits, so the score always fits its box

struct State {
  int score[kPlayers] = {kStart, kStart};
};

inline int clampScore(int value) { return value < 0 ? 0 : (value > kMax ? kMax : value); }

// The player whose score is 0, or -1 while the game is on.
inline int loser(const State& s) {
  for (int p = 0; p < kPlayers; p++) {
    if (s.score[p] == 0) return p;
  }
  return -1;
}

inline bool atStart(const State& s) { return s.score[0] == kStart && s.score[1] == kStart; }

// Whether a button would change anything. Once a game is over, only the player at 0
// can add, and nothing else moves.
inline bool canApply(const State& s, int player, int delta) {
  if (player < 0 || player >= kPlayers || delta == 0) return false;
  const int over = loser(s);
  if (over >= 0 && (player != over || delta < 0)) return false;
  return clampScore(s.score[player] + delta) != s.score[player];
}

// Moves a score, stopping at 0 and kMax. Returns false and changes nothing if the
// button can't act.
inline bool apply(State& s, int player, int delta) {
  if (!canApply(s, player, delta)) return false;
  s.score[player] = clampScore(s.score[player] + delta);
  return true;
}

inline void reset(State& s) { s = State{}; }

}  // namespace authoritytracker
