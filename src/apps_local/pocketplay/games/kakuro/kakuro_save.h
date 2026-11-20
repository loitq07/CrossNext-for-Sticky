#pragma once

#include <cstddef>
#include <cstdint>

#include "kakuro_flow.h"

// Kakuro as bytes, with no screen or card: the level, your place in every level, which puzzles
// you've solved, and the digits of the puzzle on screen.
namespace kakuro {

// Level, place per level, each level's solved puzzles a bit each, then a byte per cell of the
// board: 0 empty (and every block), 1-9 a digit, or 16 plus the digit for one from HINT.
constexpr uint8_t SAVE_VERSION = 1;
constexpr size_t SAVE_HEADER = 1 + kLevels + kLevels * kDoneBytes;
constexpr size_t SAVE_MAX = SAVE_HEADER + kMaxCells;
constexpr uint8_t HINTED = 16;

size_t pack(const State& state, uint8_t* out);

// Returns false and leaves the state alone unless the bytes name a real level and places, and
// hold digits only in white cells, with every hinted digit the right one.
bool unpack(uint8_t version, const uint8_t* in, size_t size, State& state);

}  // namespace kakuro
