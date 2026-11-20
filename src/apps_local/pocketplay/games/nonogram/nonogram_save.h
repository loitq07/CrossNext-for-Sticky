#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "nonogram_flow.h"

// Nonograms as bytes, with no screen or card. Two saves: the small one has your size, your
// place in every size, FILL or X, and the puzzles you've solved; a blob has the puzzles left
// half done. Undo isn't saved, as in the source.
namespace nonogram {

// Size, place per size, X mode, then each size's solved puzzles a bit each.
constexpr uint8_t SAVE_VERSION = 1;
constexpr size_t SAVE_BYTES = 1 + kSizes + 1 + kSizes * kDoneBytes;

size_t packState(const State& state, uint8_t* out);

// Returns false and leaves the state alone unless every value is in range.
bool unpackState(uint8_t version, const uint8_t* in, size_t size, State& state);

// Version, then for each size: how many kept, then each one's puzzle and its cells at two bits
// a cell, four to a byte: 0 blank, 1 filled, 2 an X.
constexpr uint8_t GRIDS_VERSION = 1;

std::vector<uint8_t> packGrids(const State& state);

// Returns false and leaves the state alone unless every kept grid could have been left half
// done: a real puzzle, once, with some cells marked and not solved.
bool unpackGrids(const uint8_t* in, size_t size, State& state);

}  // namespace nonogram
