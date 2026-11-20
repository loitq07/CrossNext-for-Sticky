#pragma once

#include <cstddef>
#include <cstdint>

#include "symboldice_rules.h"

// Symbol Dice as bytes, with no screen or card: the turn, then each die.
namespace symboldice {

constexpr uint8_t SAVE_VERSION = 1;
constexpr size_t SAVE_BYTES = 1 + kDice;

size_t pack(const Game& game, uint8_t* out);

// Returns false and leaves the game alone unless the turn is 1 to 12 and each die a symbol.
// Turn 0 is never saved: a game that hasn't been rolled has no save.
bool unpack(uint8_t version, const uint8_t* in, size_t size, Game& game);

}  // namespace symboldice
