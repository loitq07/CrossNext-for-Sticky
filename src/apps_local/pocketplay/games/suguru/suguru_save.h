#pragma once

#include <cstddef>
#include <cstdint>

#include "suguru_flow.h"

// Suguru as bytes, with no screen or card. The save is your place in every bank and
// the grid of the puzzle on the board. Regions and givens aren't saved: they come from
// the bank. The selected cell isn't saved either.
namespace suguru {

constexpr uint8_t SAVE_VERSION = 1;
constexpr size_t SAVE_HEADER = 2 + kBanks;  // bank, place per bank, check
constexpr size_t SAVE_MAX = SAVE_HEADER + kMaxCells;

size_t pack(const State& state, uint8_t* out);

// Returns false and leaves the state alone unless the bytes name a real bank and place,
// and hold a grid of that puzzle's size that keeps every given.
bool unpack(uint8_t version, const uint8_t* in, size_t size, State& state);

}  // namespace suguru
