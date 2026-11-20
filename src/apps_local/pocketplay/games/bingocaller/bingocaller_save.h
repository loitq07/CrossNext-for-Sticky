#pragma once

#include <cstddef>
#include <cstdint>

#include "bingocaller_rules.h"

// The game in play as bytes, so sleep or a flat battery keeps every call: the mode, the count,
// then each call in order, one byte each.
namespace bingocaller {

constexpr uint8_t SAVE_VERSION = 1;
constexpr size_t SAVE_HEADER = 2;
constexpr size_t MAX_SAVE_BYTES = SAVE_HEADER + kMaxBalls;  // 92

size_t pack(const State& state, uint8_t* out);

// Returns false and leaves the state alone unless the mode is known, the size matches the count,
// and every call is a number in range that hasn't come before.
bool unpack(uint8_t version, const uint8_t* in, size_t size, State& state);

}  // namespace bingocaller
