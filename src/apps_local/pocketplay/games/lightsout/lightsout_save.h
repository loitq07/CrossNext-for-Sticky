#pragma once

#include <cstddef>
#include <cstdint>

#include "lightsout_rules.h"

// Lights Out as bytes, with no screen or card. Only a puzzle in progress is saved: a
// solved one erases its save. The record and level live in a file of their own, so
// they outlast it.
namespace lightsout {

constexpr uint8_t SAVE_VERSION = 1;
constexpr size_t SAVE_HEADER = 8;  // board 4, moves 2, par 1, history count 1
constexpr size_t SAVE_MAX = SAVE_HEADER + kHistory;

size_t packGame(const Game& game, uint8_t* out);

// Returns false and leaves the game alone unless the bytes are an unsolved puzzle that
// can still be solved, with a history of real squares.
bool unpackGame(uint8_t version, const uint8_t* in, size_t size, Game& game);

struct Stats {
  uint32_t solved = 0;
  uint8_t level = kDefaultLevel;
};

constexpr uint8_t STATS_VERSION = 1;
constexpr size_t STATS_BYTES = 5;

size_t packStats(const Stats& stats, uint8_t* out);
bool unpackStats(uint8_t version, const uint8_t* in, size_t size, Stats& stats);

}  // namespace lightsout
