#pragma once

#include <cstddef>
#include <cstdint>

#include "thinkalike_deck.h"

// The deck as bytes: its seed, the question count, how many have been shown, and
// the question file's checksum, so an edited file starts a fresh shuffle.
namespace thinkalike {

constexpr uint8_t SAVE_VERSION = 1;
constexpr size_t SAVE_BYTES = 12;

struct Saved {
  Deck deck;
  uint32_t fileSum = 0;
};

size_t pack(const Saved& saved, uint8_t* out);

// Returns false and leaves `saved` alone unless every value is in range.
bool unpack(uint8_t version, const uint8_t* in, size_t size, Saved& saved);

}  // namespace thinkalike
