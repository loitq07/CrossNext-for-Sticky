#pragma once

#include <cstddef>
#include <cstdint>

#include "crowncrest_rules.h"

// The turn as bytes, so sleep or a flat battery brings back the same dice and picks: one face a
// die, the roller, how many picks are made, the die each pick took, then the roll count in 4
// bytes, low byte first.
namespace crowncrest {

constexpr uint8_t SAVE_VERSION = 3;  // 2 had no roll count
constexpr size_t SAVE_BYTES = kDice + 2 + kPicks + 4;

size_t pack(const Turn& turn, uint8_t* out);

// Returns false and leaves the turn alone unless every die shows a face from 1 to 6 (or all are
// blank before the first roll), the roller is A or B, the picks are different dice, and the
// count is 0 before the first roll and 1 to kMaxRolls after it.
bool unpack(uint8_t version, const uint8_t* in, size_t size, Turn& turn);

}  // namespace crowncrest
