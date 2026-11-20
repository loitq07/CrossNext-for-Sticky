#pragma once

#include <cstddef>
#include <cstdint>

#include "pegsolitaire_flow.h"

// Peg Solitaire as bytes, with no screen or card: the board, the best on each board, and every
// jump as the hole it left and the hole it landed in. The pegs aren't stored: replaying the
// jumps from the start finds them, and checks every jump could be made.
namespace pegsolitaire {

constexpr uint8_t SAVE_VERSION = 1;
constexpr size_t SAVE_HEADER = 1 + kBoards + 1;  // board, best per board, jump count
constexpr size_t SAVE_MAX = SAVE_HEADER + 2 * kMaxJumps;

size_t pack(const State& state, uint8_t* out);

// Returns false and leaves the state alone unless the board exists, each best is a peg count
// the board can end with, and every jump plays.
bool unpack(uint8_t version, const uint8_t* in, size_t size, State& state);

}  // namespace pegsolitaire
