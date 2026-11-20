#include "diceroller_save.h"

namespace diceroller {

size_t pack(const Roll& roll, uint8_t* out) {
  for (int d = 0; d < kDice; d++) out[d] = roll.dice[d];
  for (int b = 0; b < 4; b++) out[kDice + b] = static_cast<uint8_t>(roll.rolls >> (8 * b));
  return SAVE_BYTES;
}

bool unpack(uint8_t version, const uint8_t* in, size_t size, Roll& roll) {
  if (version != SAVE_VERSION || size != SAVE_BYTES) return false;
  for (int d = 0; d < kDice; d++) {
    if (in[d] < 1 || in[d] > kFaces) return false;
  }
  uint32_t rolls = 0;
  for (int b = 0; b < 4; b++) rolls |= static_cast<uint32_t>(in[kDice + b]) << (8 * b);
  if (rolls < 1 || rolls > kMaxRolls) return false;
  for (int d = 0; d < kDice; d++) roll.dice[d] = in[d];
  roll.rolls = rolls;
  return true;
}

}  // namespace diceroller
