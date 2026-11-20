#pragma once

#include <cstdint>

#include "oneclue_cards.h"

// The deck as a shuffled order and how far through it the players are. No screen
// or card, so it can be tested off the device. The rules take a seed and never
// reach for the hardware's random numbers themselves.
namespace oneclue {

struct Deck {
  uint8_t order[kCards] = {};
  uint8_t drawn = 0;  // cards shown so far, 1..kCards once shuffled
};

// Fisher-Yates with xorshift32, so a seed always gives the same order in tests.
inline void shuffle(Deck& deck, uint32_t seed) {
  uint32_t state = seed != 0 ? seed : 0x9E3779B9u;  // xorshift never leaves 0
  for (int i = 0; i < kCards; i++) deck.order[i] = static_cast<uint8_t>(i);
  for (int i = kCards - 1; i > 0; i--) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    const int j = static_cast<int>(state % static_cast<uint32_t>(i + 1));
    const uint8_t swap = deck.order[i];
    deck.order[i] = deck.order[j];
    deck.order[j] = swap;
  }
  deck.drawn = 1;
}

inline bool finished(const Deck& deck) { return deck.drawn >= kCards; }

// The card on the table, as its index in CARDS.
inline int current(const Deck& deck) { return deck.order[deck.drawn - 1]; }

// Turns over the next card. False, and nothing changes, once every card is used.
inline bool next(Deck& deck) {
  if (finished(deck)) return false;
  deck.drawn++;
  return true;
}

}  // namespace oneclue
