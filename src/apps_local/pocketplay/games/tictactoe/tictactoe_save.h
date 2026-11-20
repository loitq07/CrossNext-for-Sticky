#pragma once

#include <cstddef>
#include <cstdint>

#include "tictactoe_rules.h"

// A game in progress as bytes, with no screen or card, so it can be tested off the
// device. Only a running game is ever saved: a finished one erases its save.
namespace tictactoe {

constexpr uint8_t SAVE_VERSION = 1;
constexpr size_t SAVE_BYTES = kCells + 1;

size_t packGame(const Game& game, uint8_t* out);

// Returns false and leaves the game alone unless the bytes are a position that real
// play can reach and that isn't already finished.
bool unpackGame(uint8_t version, const uint8_t* in, size_t size, Game& game);

}  // namespace tictactoe
