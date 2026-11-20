#pragma once

#include <cstddef>
#include <cstdint>

#include "dungeon_rules.h"

// The board in hand and the campaign record as bytes, with no screen or card, so
// it can be tested off the device. Packed byte by byte, least significant first:
//
//   [0]      the dungeon in hand, 1..64
//   [1..8]   its walls
//   [9..16]  its floor notes
//   [17..24] solved dungeons 0..63 (bit 0, the tutorial, is never set)
//   [25..32] solved dungeon 64, in bit 0
namespace dungeon {

constexpr uint8_t SAVE_VERSION = 1;
constexpr size_t SAVE_BYTES = 33;

size_t packGame(const Board& board, const Progress& progress, uint8_t* out);

// Returns false and changes nothing unless the bytes name a playable dungeon and
// a record with no bits outside the campaign. Marks standing on a monster or a
// chest are dropped by Board::restore(), not refused.
bool unpackGame(uint8_t version, const uint8_t* in, size_t size, Board& board, Progress& progress);

}  // namespace dungeon
