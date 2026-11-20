#include "crowncrest_save.h"

namespace crowncrest {

size_t pack(const Turn& turn, uint8_t* out) {
  for (int d = 0; d < kDice; d++) out[d] = turn.dice.face[d];
  out[kDice] = turn.roller;
  out[kDice + 1] = turn.picks;
  for (int i = 0; i < kPicks; i++) out[kDice + 2 + i] = i < turn.picks ? turn.order[i] : 0;
  for (int b = 0; b < 4; b++) out[kDice + 2 + kPicks + b] = static_cast<uint8_t>(turn.rolls >> (8 * b));
  return SAVE_BYTES;
}

bool unpack(uint8_t version, const uint8_t* in, size_t size, Turn& turn) {
  if (version != SAVE_VERSION || size != SAVE_BYTES) return false;
  const bool blank = in[0] == 0;
  for (int d = 0; d < kDice; d++) {
    if (blank ? in[d] != 0 : in[d] < 1 || in[d] > kFaces) return false;
  }
  const uint8_t roller = in[kDice];
  const uint8_t picks = in[kDice + 1];
  if (roller >= kPlayers || picks > kPicks) return false;
  if (blank && (roller != 0 || picks != 0)) return false;
  bool taken[kDice] = {};
  for (int i = 0; i < picks; i++) {
    const uint8_t die = in[kDice + 2 + i];
    if (die >= kDice || taken[die]) return false;
    taken[die] = true;
  }
  uint32_t rolls = 0;
  for (int b = 0; b < 4; b++) rolls |= static_cast<uint32_t>(in[kDice + 2 + kPicks + b]) << (8 * b);
  if (blank ? rolls != 0 : rolls < 1 || rolls > kMaxRolls) return false;

  Turn loaded;
  for (int d = 0; d < kDice; d++) loaded.dice.face[d] = in[d];
  loaded.roller = roller;
  loaded.picks = picks;
  for (int i = 0; i < picks; i++) loaded.order[i] = in[kDice + 2 + i];
  loaded.rolls = rolls;
  turn = loaded;
  return true;
}

}  // namespace crowncrest
