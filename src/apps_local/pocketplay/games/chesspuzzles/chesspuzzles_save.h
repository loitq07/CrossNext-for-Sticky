#pragma once

#include <cstddef>
#include <cstdint>

#include "chesspuzzles_flow.h"

// Chess Puzzles as bytes, with no screen or card: the bank, and for each bank the puzzle it's on
// and how many were solved, then how far into the open puzzle you are. The board isn't stored:
// replaying the puzzle's moves finds it.
namespace chesspuzzles {

constexpr uint8_t SAVE_VERSION = 1;
// bank, then index, solved and clean per bank (2 bytes each), then ply, mistakes, shown, done
constexpr size_t SAVE_SIZE = 1 + 6 * kBanks + 4;

size_t pack(const State& state, uint8_t* out);

// Returns false and leaves the state alone unless every number fits its bank and the open puzzle
// is somewhere it can be.
bool unpack(uint8_t version, const uint8_t* in, size_t size, State& state);

}  // namespace chesspuzzles
