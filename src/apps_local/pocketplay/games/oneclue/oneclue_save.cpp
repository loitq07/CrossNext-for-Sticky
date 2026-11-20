#include "oneclue_save.h"

namespace oneclue {

size_t packDeck(const Deck& deck, uint8_t* out) {
  for (int i = 0; i < kCards; i++) out[i] = deck.order[i];
  out[kCards] = deck.drawn;
  return SAVE_BYTES;
}

bool unpackDeck(uint8_t version, const uint8_t* in, size_t size, Deck& deck) {
  if (version != SAVE_VERSION || size != SAVE_BYTES) return false;

  Deck loaded;
  bool seen[kCards] = {};
  for (int i = 0; i < kCards; i++) {
    const uint8_t card = in[i];
    if (card >= kCards || seen[card]) return false;
    seen[card] = true;
    loaded.order[i] = card;
  }

  const uint8_t drawn = in[kCards];
  if (drawn < 1 || drawn > kCards) return false;
  loaded.drawn = drawn;

  deck = loaded;
  return true;
}

}  // namespace oneclue
