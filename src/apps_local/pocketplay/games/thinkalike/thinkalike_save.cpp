#include "thinkalike_save.h"

#include "thinkalike_questions.h"

namespace thinkalike {

namespace {

void put32(uint8_t* out, uint32_t value) {
  for (int i = 0; i < 4; i++) out[i] = static_cast<uint8_t>(value >> (8 * i));
}

uint32_t get32(const uint8_t* in) {
  uint32_t value = 0;
  for (int i = 0; i < 4; i++) value |= static_cast<uint32_t>(in[i]) << (8 * i);
  return value;
}

}  // namespace

size_t pack(const Saved& saved, uint8_t* out) {
  put32(out, saved.deck.seed);
  out[4] = static_cast<uint8_t>(saved.deck.count);
  out[5] = static_cast<uint8_t>(saved.deck.count >> 8);
  out[6] = static_cast<uint8_t>(saved.deck.drawn);
  out[7] = static_cast<uint8_t>(saved.deck.drawn >> 8);
  put32(out + 8, saved.fileSum);
  return SAVE_BYTES;
}

bool unpack(uint8_t version, const uint8_t* in, size_t size, Saved& saved) {
  if (version != SAVE_VERSION || size != SAVE_BYTES) return false;
  Saved loaded;
  loaded.deck.seed = get32(in);
  loaded.deck.count = static_cast<uint16_t>(in[4] | (in[5] << 8));
  loaded.deck.drawn = static_cast<uint16_t>(in[6] | (in[7] << 8));
  loaded.fileSum = get32(in + 8);
  if (loaded.deck.seed == 0) return false;
  if (loaded.deck.count < 1 || loaded.deck.count > MAX_QUESTIONS) return false;
  if (loaded.deck.drawn < 1 || loaded.deck.drawn > loaded.deck.count) return false;
  saved = loaded;
  return true;
}

}  // namespace thinkalike
