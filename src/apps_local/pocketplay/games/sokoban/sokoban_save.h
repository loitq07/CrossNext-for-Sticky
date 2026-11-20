#pragma once

#include <cstddef>
#include <cstdint>

#include "sokoban_flow.h"

// Sokoban as bytes, with no screen or card. Both files are blobs, because a long game's moves
// and the record of every level are more than a save holds. Each starts with its version.
namespace sokoban {

// The pack you're in, your level in every pack, and the moves of the level on the board,
// two bits each, four to a byte. Pushes aren't stored: replaying the moves finds them.
constexpr uint8_t GAME_VERSION = 1;
constexpr size_t GAME_HEADER = 1 + 1 + kPackCount + 2;  // version, pack, places, move count
constexpr size_t GAME_MAX = GAME_HEADER + (kMaxHistory + 3) / 4;

size_t packGame(const State& state, uint8_t* out);

// Replays the moves on the level they name. Returns false and leaves the state alone unless
// every byte is in range and every move can be made.
bool unpackGame(const uint8_t* in, size_t size, State& state);

// The fewest moves for every level, two bytes each, low byte first, 0 for not solved.
constexpr uint8_t BEST_VERSION = 1;
constexpr size_t BEST_BYTES = 1 + 2 * kTotalLevels;

size_t packBest(const State& state, uint8_t* out);
bool unpackBest(const uint8_t* in, size_t size, State& state);

}  // namespace sokoban
