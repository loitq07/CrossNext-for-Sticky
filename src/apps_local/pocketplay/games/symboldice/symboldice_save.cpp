#include "symboldice_save.h"

namespace symboldice {

size_t pack(const Game& game, uint8_t* out) {
  out[0] = game.turn;
  for (int d = 0; d < kDice; d++) out[1 + d] = game.dice[d];
  return SAVE_BYTES;
}

bool unpack(uint8_t version, const uint8_t* in, size_t size, Game& game) {
  if (version != SAVE_VERSION || size != SAVE_BYTES) return false;
  if (in[0] < 1 || in[0] > kTurns) return false;
  for (int d = 0; d < kDice; d++) {
    if (in[1 + d] >= kSymbols) return false;
  }
  game.turn = in[0];
  for (int d = 0; d < kDice; d++) game.dice[d] = in[1 + d];
  return true;
}

}  // namespace symboldice
