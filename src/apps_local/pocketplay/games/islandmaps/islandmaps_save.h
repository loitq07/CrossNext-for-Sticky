#pragma once

#include <cstddef>
#include <cstdint>

// The island on screen, so the game reopens where it was: one byte, its place in kIslands.
namespace islandmaps {

constexpr uint8_t SAVE_VERSION = 1;
constexpr size_t SAVE_BYTES = 1;

size_t pack(int index, uint8_t* out);

// Returns false and leaves index alone unless the save is one byte naming an island in the list.
bool unpack(uint8_t version, const uint8_t* in, size_t size, int& index);

}  // namespace islandmaps
