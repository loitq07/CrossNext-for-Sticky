#pragma once

#include <cstddef>
#include <cstdint>

#include "queens_flow.h"

// Queens as bytes, with no screen or card. Two saves: your place and the marks on the board,
// and the puzzles you've solved. Regions and solutions aren't saved: they come from the bank.
// Undo isn't saved either, as in the source.
namespace queens {

// Bank, place per bank, dots, then the marks, two bits a cell, four to a byte: 0 empty, 1 a
// cross, 2 a piece, 3 a piece from a hint.
constexpr uint8_t SAVE_VERSION = 1;
constexpr size_t SAVE_HEADER = 1 + kBanks + 1;
constexpr size_t SAVE_MAX = SAVE_HEADER + (kMaxCells + 3) / 4;

size_t packGame(const State& state, uint8_t* out);

// Returns false and leaves the state alone unless the bytes name a real bank and place and hold
// marks for that puzzle's size.
bool unpackGame(uint8_t version, const uint8_t* in, size_t size, State& state);

constexpr uint8_t DONE_VERSION = 1;
constexpr size_t DONE_BYTES = kBanks * kDoneBytes;

size_t packDone(const State& state, uint8_t* out);
bool unpackDone(uint8_t version, const uint8_t* in, size_t size, State& state);

}  // namespace queens
