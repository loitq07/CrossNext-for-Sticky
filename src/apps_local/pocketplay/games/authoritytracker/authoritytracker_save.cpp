#include "authoritytracker_save.h"

namespace authoritytracker {

size_t pack(const State& state, uint8_t* out) {
  for (int p = 0; p < kPlayers; p++) {
    out[2 * p] = static_cast<uint8_t>(state.score[p] >> 8);
    out[2 * p + 1] = static_cast<uint8_t>(state.score[p] & 0xFF);
  }
  return SAVE_BYTES;
}

bool unpack(uint8_t version, const uint8_t* in, size_t size, State& state) {
  if (version != SAVE_VERSION || size != SAVE_BYTES) return false;
  int scores[kPlayers];
  for (int p = 0; p < kPlayers; p++) {
    scores[p] = (in[2 * p] << 8) | in[2 * p + 1];
    if (scores[p] > kMax) return false;
  }
  for (int p = 0; p < kPlayers; p++) state.score[p] = scores[p];
  return true;
}

}  // namespace authoritytracker
