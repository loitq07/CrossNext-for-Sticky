#pragma once

#include <cstddef>
#include <cstdint>

#include "diceroller_rules.h"

// The last roll and the count as bytes, so a wake shows the same dice: one byte a die, 1 to
// 6, then the count in 4 bytes, low byte first. Only a rolled state is saved: a reset erases
// the save instead.
namespace diceroller {

constexpr uint8_t SAVE_VERSION = 2;  // 1 had the dice only
constexpr size_t SAVE_BYTES = kDice + 4;

size_t pack(const Roll& roll, uint8_t* out);

// Returns false and leaves the roll alone unless every die is 1 to 6 and the count is 1 to
// kMaxRolls.
bool unpack(uint8_t version, const uint8_t* in, size_t size, Roll& roll);

}  // namespace diceroller
