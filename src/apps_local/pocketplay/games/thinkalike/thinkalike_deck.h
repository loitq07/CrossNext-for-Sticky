#pragma once

#include <cstdint>
#include <vector>

// The questions as a shuffled order and how far through it the players are. The
// order is rebuilt from its seed, so a save holds a few bytes however many
// questions the file has. No screen or card, so it can be tested off the device.
namespace thinkalike {

struct Deck {
  uint32_t seed = 0;   // never 0 once shuffled
  uint16_t count = 0;  // questions in the file it was shuffled for
  uint16_t drawn = 0;  // questions shown so far, 1..count once shuffled
};

inline Deck shuffled(uint32_t seed, uint16_t count) {
  Deck deck;
  deck.seed = seed != 0 ? seed : 0x9E3779B9u;  // xorshift never leaves 0
  deck.count = count;
  deck.drawn = count > 0 ? 1 : 0;
  return deck;
}

// Fisher-Yates with xorshift32, as One Clue's deck, so a seed always gives the
// same order.
inline std::vector<uint16_t> order(const Deck& deck) {
  std::vector<uint16_t> out(deck.count);
  for (int i = 0; i < deck.count; i++) out[i] = static_cast<uint16_t>(i);
  uint32_t state = deck.seed;
  for (int i = deck.count - 1; i > 0; i--) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    const int j = static_cast<int>(state % static_cast<uint32_t>(i + 1));
    const uint16_t swap = out[i];
    out[i] = out[j];
    out[j] = swap;
  }
  return out;
}

inline bool finished(const Deck& deck) { return deck.drawn >= deck.count; }

// Shows the next question. False, and nothing changes, once every one is used.
inline bool next(Deck& deck) {
  if (finished(deck)) return false;
  deck.drawn++;
  return true;
}

}  // namespace thinkalike
