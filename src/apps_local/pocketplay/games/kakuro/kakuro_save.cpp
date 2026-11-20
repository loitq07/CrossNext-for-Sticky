#include "kakuro_save.h"

namespace kakuro {

size_t pack(const State& state, uint8_t* out) {
  size_t at = 0;
  out[at++] = state.level;
  for (int l = 0; l < kLevels; l++) out[at++] = state.place[l];
  for (int l = 0; l < kLevels; l++) {
    for (int k = 0; k < kDoneBytes; k++) out[at++] = state.done[l][k];
  }
  const Board& b = state.board;
  for (int i = 0; i < b.puzzle.n * b.puzzle.n; i++) {
    out[at++] = static_cast<uint8_t>(b.entry[i] | (b.hinted[i] ? HINTED : 0));
  }
  return at;
}

bool unpack(uint8_t version, const uint8_t* in, size_t size, State& state) {
  if (version != SAVE_VERSION || size < SAVE_HEADER) return false;
  const uint8_t lvl = in[0];
  if (lvl >= kLevels) return false;
  for (int l = 0; l < kLevels; l++) {
    if (in[1 + l] >= kBankPuzzles) return false;
  }
  const int n = level(lvl).n;
  if (size != SAVE_HEADER + static_cast<size_t>(n * n)) return false;
  // The digits are checked against the puzzle before anything changes.
  Puzzle p;
  if (!parse(level(lvl).lines[in[1 + lvl]], n, p)) return false;
  const uint8_t* cells = in + SAVE_HEADER;
  for (int i = 0; i < n * n; i++) {
    const uint8_t v = cells[i];
    const uint8_t digit = v & 15;
    if ((v & ~(HINTED | 15)) || digit > 9) return false;
    if (!white(p, i) && v != 0) return false;
    if ((v & HINTED) && digit != p.solution[i]) return false;
  }

  state.level = lvl;
  for (int l = 0; l < kLevels; l++) state.place[l] = in[1 + l];
  for (int l = 0; l < kLevels; l++) {
    for (int k = 0; k < kDoneBytes; k++) state.done[l][k] = in[1 + kLevels + l * kDoneBytes + k];
  }
  loadPuzzle(state);
  for (int i = 0; i < n * n; i++) {
    state.board.entry[i] = cells[i] & 15;
    state.board.hinted[i] = (cells[i] & HINTED) != 0;
  }
  settle(state);
  return true;
}

}  // namespace kakuro
