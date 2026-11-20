#pragma once

#include <cstddef>
#include <cstdint>

#include "lastpiece_flow.h"

// Last Piece as bytes, with no screen or card: the difficulty, the challenge you're on at each,
// which challenges are solved, the board the moves started from, and every capture as the cell
// it left and the cell it took. The board now isn't stored: replaying the captures finds it,
// and checks every one could be made. The starting board is kept so a challenge edited on the
// card isn't replayed with old moves (sync() in the flow).
namespace lastpiece {

constexpr uint8_t SAVE_VERSION = 1;
// difficulty, challenge at each, solved bits, the start board (a nibble a cell), capture count
constexpr size_t SAVE_HEADER = 1 + kDifficulties + kDifficulties * kSolvedBytes + kCells / 2 + 1;
constexpr size_t SAVE_MAX = SAVE_HEADER + kMaxMoves;

size_t pack(const State& state, uint8_t* out);

// Returns false and leaves the state alone unless the difficulty exists, every challenge index
// and solved bit is in range, every cell holds a piece code, and every capture plays.
bool unpack(uint8_t version, const uint8_t* in, size_t size, State& state);

}  // namespace lastpiece
