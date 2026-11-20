#pragma once

#include <cstdint>

// Symbol Dice: two dice of six symbols, for a roll-and-write game played on paper. Each roll
// throws both dice together and is one turn, and a game is 12 turns. No screen or card, so it
// can be tested off the device. A roll takes a random number and never reaches for the
// hardware's itself.
namespace symboldice {

enum Symbol : uint8_t { Triangle, Cross, Circle, Hash, Lines, Slash };
constexpr int kSymbols = 6;
constexpr int kDice = 2;
constexpr int kTurns = 12;

constexpr const char* kNames[kSymbols] = {"TRIANGLE", "CROSS", "CIRCLE", "HASH", "THREE LINES", "SLASH"};

struct Game {
  uint8_t turn = 0;  // turns taken: 0 before the first roll, kTurns once the game is over
  uint8_t dice[kDice] = {};
};

inline bool over(const Game& g) { return g.turn >= kTurns; }
inline bool started(const Game& g) { return g.turn > 0; }

// Both dice from one random number: the low part for the first, the next for the second, so
// all 36 pairs are equally likely. False, and nothing changed, once the game is over.
inline bool roll(Game& g, uint32_t random) {
  if (over(g)) return false;
  g.dice[0] = static_cast<uint8_t>(random % kSymbols);
  g.dice[1] = static_cast<uint8_t>((random / kSymbols) % kSymbols);
  g.turn++;
  return true;
}

inline void reset(Game& g) { g = Game{}; }

}  // namespace symboldice
