#pragma once

#include <cstddef>
#include <cstdint>

#include "authoritytracker_rules.h"

// Both scores as bytes, so a wake shows the same game: two bytes a score, high byte
// first.
namespace authoritytracker {

constexpr uint8_t SAVE_VERSION = 1;
constexpr size_t SAVE_BYTES = 2 * kPlayers;

size_t pack(const State& state, uint8_t* out);

// Returns false and leaves the state alone unless every score is 0 to kMax.
bool unpack(uint8_t version, const uint8_t* in, size_t size, State& state);

}  // namespace authoritytracker
