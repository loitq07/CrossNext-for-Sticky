#pragma once

#include <cstddef>
#include <cstdint>

#include "polyominopanic_deck.h"

// Both decks as bytes, so sleep or a flat battery keeps the round: the phase, then each deck's
// seed (4 bytes, low first) and how many cards it has turned over.
namespace polyominopanic {

constexpr uint8_t SAVE_VERSION = 1;
constexpr size_t SAVE_SIZE = 1 + 2 * 5;

size_t pack(const State& state, uint8_t* out);

// Returns false and leaves the state alone unless the version, phase, seeds and counts are all
// ones the game could have made.
bool unpack(uint8_t version, const uint8_t* in, size_t size, State& state);

}  // namespace polyominopanic
