#pragma once

#include <cstddef>
#include <cstdint>

#include "gridgambit_flow.h"

// Grid Gambit as bytes, with no screen or card: the level, the solved count at each level, the
// puzzle as it was set, and every capture as the cell it left and the cell it took. The board
// now isn't stored: replaying the captures finds it, and checks every one could be made.
namespace gridgambit {

constexpr uint8_t SAVE_VERSION = 1;
// level, counted, solved (2 bytes each), the start board (a nibble a cell), capture count
constexpr size_t SAVE_HEADER = 1 + 1 + 2 * kLevels + kCells / 2 + 1;
constexpr size_t SAVE_MAX = SAVE_HEADER + kMaxMoves;

size_t pack(const State& state, uint8_t* out);

// Returns false and leaves the state alone unless the level exists, every cell holds a piece
// code, the puzzle has 2 to 9 pieces, and every capture plays.
bool unpack(uint8_t version, const uint8_t* in, size_t size, State& state);

}  // namespace gridgambit
