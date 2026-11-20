#include "polyominopanic_save.h"

namespace polyominopanic {

namespace {

void putDeck(const Deck& deck, uint8_t* out) {
  for (int i = 0; i < 4; i++) out[i] = static_cast<uint8_t>(deck.seed >> (8 * i));
  out[4] = deck.drawn;
}

Deck getDeck(const uint8_t* in) {
  Deck deck;
  deck.seed = 0;
  for (int i = 0; i < 4; i++) deck.seed |= static_cast<uint32_t>(in[i]) << (8 * i);
  deck.drawn = in[4];
  return deck;
}

}  // namespace

size_t pack(const State& state, uint8_t* out) {
  out[0] = static_cast<uint8_t>(state.phase);
  putDeck(state.starter, out + 1);
  putDeck(state.game, out + 6);
  return SAVE_SIZE;
}

bool unpack(uint8_t version, const uint8_t* in, size_t size, State& state) {
  if (version != SAVE_VERSION || size != SAVE_SIZE) return false;
  if (in[0] > static_cast<uint8_t>(Phase::Play)) return false;
  State loaded;
  loaded.phase = static_cast<Phase>(in[0]);
  loaded.starter = getDeck(in + 1);
  loaded.game = getDeck(in + 6);
  if (loaded.starter.seed == 0 || loaded.game.seed == 0) return false;
  if (loaded.starter.drawn > kStarterCount) return false;
  const uint8_t drawn = loaded.game.drawn;
  if (drawn > kGameCount || drawn % kPerRound != 0) return false;
  // The game deck is untouched before play, and has a pair out during it.
  if (loaded.phase == Phase::Starter ? drawn != 0 : drawn == 0) return false;
  state = loaded;
  return true;
}

}  // namespace polyominopanic
