#include "bingocaller_save.h"

namespace bingocaller {

size_t pack(const State& state, uint8_t* out) {
  out[0] = state.mode;
  out[1] = state.count;
  for (int i = 0; i < state.count; i++) out[SAVE_HEADER + i] = state.order[i];
  return SAVE_HEADER + state.count;
}

bool unpack(uint8_t version, const uint8_t* in, size_t size, State& state) {
  if (version != SAVE_VERSION || size < SAVE_HEADER) return false;
  const uint8_t mode = in[0];
  const uint8_t count = in[1];
  if (mode >= kModes || count > kBalls[mode] || size != SAVE_HEADER + count) return false;

  bool seen[kMaxBalls + 1] = {};
  for (int i = 0; i < count; i++) {
    const uint8_t number = in[SAVE_HEADER + i];
    if (number < 1 || number > kBalls[mode] || seen[number]) return false;
    seen[number] = true;
  }

  State loaded{};
  loaded.mode = mode;
  loaded.count = count;
  for (int i = 0; i < count; i++) loaded.order[i] = in[SAVE_HEADER + i];
  state = loaded;
  return true;
}

}  // namespace bingocaller
