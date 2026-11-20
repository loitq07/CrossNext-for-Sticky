#pragma once

#include <cstdint>

#include "polyominopanic_cards.h"

// The two decks as shuffled orders and how far into each the players are. An order is rebuilt
// from its seed, so a save holds a few bytes. No screen or card, so it can be tested off the device.
namespace polyominopanic {

constexpr int kPerRound = 2;
constexpr int kRounds = kGameCount / kPerRound;  // 20
static_assert(kGameCount % kPerRound == 0, "every round flips a whole pair");

struct Deck {
  uint32_t seed = 1;  // never 0: xorshift never leaves 0
  uint8_t drawn = 0;  // cards turned over so far
};

enum class Phase : uint8_t { Starter, Play };

struct State {
  Phase phase = Phase::Starter;
  Deck starter;
  Deck game;
};

// What an action changed, so the screen knows how much to refresh.
enum class Change { None, Redraw, Full };

inline Deck shuffled(uint32_t seed) {
  Deck deck;
  deck.seed = seed != 0 ? seed : 0x9E3779B9u;
  return deck;
}

// Fisher-Yates with xorshift32, so a seed always gives the same order. out holds count cards.
inline void order(const Deck& deck, int count, uint8_t* out) {
  for (int i = 0; i < count; i++) out[i] = static_cast<uint8_t>(i);
  uint32_t state = deck.seed;
  for (int i = count - 1; i > 0; i--) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    const int j = static_cast<int>(state % static_cast<uint32_t>(i + 1));
    const uint8_t swap = out[i];
    out[i] = out[j];
    out[j] = swap;
  }
}

// The card at position n of a deck's order.
inline int cardAt(const Deck& deck, int count, int n) {
  uint8_t cards[kGameCount > kStarterCount ? kGameCount : kStarterCount];
  order(deck, count, cards);
  return cards[n];
}

inline bool starterEmpty(const State& s) { return s.starter.drawn >= kStarterCount; }
inline bool over(const State& s) { return s.phase == Phase::Play && s.game.drawn >= kGameCount; }
inline int roundNumber(const State& s) { return s.game.drawn / kPerRound; }
inline int cardsLeft(const State& s) { return kGameCount - s.game.drawn; }

// The starter card on show, or -1 before the first deal.
inline int starterCard(const State& s) {
  if (s.starter.drawn == 0) return -1;
  return cardAt(s.starter, kStarterCount, s.starter.drawn - 1);
}

// The round's two cards, 0 or 1, or -1 before play.
inline int gameCard(const State& s, int which) {
  if (s.phase != Phase::Play || s.game.drawn < kPerRound) return -1;
  return cardAt(s.game, kGameCount, s.game.drawn - kPerRound + which);
}

// Turns over the next starter card. Nothing once all are out.
inline Change deal(State& s) {
  if (s.phase != Phase::Starter || starterEmpty(s)) return Change::None;
  s.starter.drawn++;
  return Change::Redraw;
}

// Puts every starter card back in a new order. Nothing when none is out.
inline Change shuffleStarter(State& s, uint32_t seed) {
  if (s.phase != Phase::Starter || s.starter.drawn == 0) return Change::None;
  s.starter = shuffled(seed);
  return Change::Redraw;
}

// Leaves the starter deck and flips the first pair.
inline Change startPlay(State& s) {
  if (s.phase != Phase::Starter) return Change::None;
  s.phase = Phase::Play;
  s.game.drawn = kPerRound;
  return Change::Full;
}

// Discards the pair and flips the next. The last pair ends the game, with a full refresh.
inline Change nextRound(State& s) {
  if (s.phase != Phase::Play || over(s)) return Change::None;
  s.game.drawn += kPerRound;
  return over(s) ? Change::Full : Change::Redraw;
}

// Both decks shuffled afresh, back at the starter deck.
inline Change newGame(State& s, uint32_t starterSeed, uint32_t gameSeed) {
  s.phase = Phase::Starter;
  s.starter = shuffled(starterSeed);
  s.game = shuffled(gameSeed);
  return Change::Full;
}

}  // namespace polyominopanic
