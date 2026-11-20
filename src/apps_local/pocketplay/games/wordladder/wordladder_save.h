#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "wordladder_flow.h"

// Word Ladder's save as bytes, with no screen or card: the ladder of every day played, in one
// blob. The puzzles aren't saved, since each day's comes from the list.
namespace wordladder {

// Version, then a day count (two bytes), then for each day in order: its distance from
// 1 November 2026 (two bytes, signed), the step count, 1 if solved, the HINT bits (a byte per
// eight steps), and each step's letters.
constexpr uint8_t SAVE_VERSION = 1;

std::vector<uint8_t> packDays(const State& state);

// Returns false and leaves the state alone unless every day is a ladder that could have been
// played: listed words, each one letter from the last, none twice, and solved exactly when the
// last step is the target.
bool unpackDays(const uint8_t* in, size_t size, State& state);

}  // namespace wordladder
