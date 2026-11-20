#pragma once

#include <cstddef>
#include <cstdint>

#include "oneclue_deck.h"

// The deck as bytes: the shuffled order, then how many cards have been shown.
namespace oneclue {

constexpr uint8_t SAVE_VERSION = 1;
constexpr size_t SAVE_BYTES = kCards + 1;

size_t packDeck(const Deck& deck, uint8_t* out);

// Returns false and leaves the deck alone unless the order uses every card once
// and the count is in range.
bool unpackDeck(uint8_t version, const uint8_t* in, size_t size, Deck& deck);

}  // namespace oneclue
