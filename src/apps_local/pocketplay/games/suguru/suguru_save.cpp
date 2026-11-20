#include "suguru_save.h"

namespace suguru {

size_t pack(const State& state, uint8_t* out) {
  size_t at = 0;
  out[at++] = state.bank;
  for (int b = 0; b < kBanks; b++) out[at++] = state.place[b];
  out[at++] = state.check ? 1 : 0;
  for (int i = 0; i < cellCount(state); i++) out[at++] = state.cells[i];
  return at;
}

bool unpack(uint8_t version, const uint8_t* in, size_t size, State& state) {
  if (version != SAVE_VERSION || size < SAVE_HEADER) return false;
  State s;
  size_t at = 0;
  s.bank = in[at++];
  if (s.bank >= kBanks) return false;
  for (int b = 0; b < kBanks; b++) {
    s.place[b] = in[at++];
    if (s.place[b] >= kBankPuzzles) return false;
  }
  const uint8_t check = in[at++];
  if (check > 1) return false;
  s.check = check == 1;

  loadPuzzle(s);
  if (size != SAVE_HEADER + static_cast<size_t>(cellCount(s))) return false;
  for (int i = 0; i < cellCount(s); i++) {
    const uint8_t v = in[at++];
    if (v > kMaxDigit) return false;
    if (isGiven(s, i) && v != s.puzzle.givens[i]) return false;
    s.cells[i] = v;
  }
  s.solved = solved(s.puzzle, s.cells);
  state = s;
  return true;
}

}  // namespace suguru
