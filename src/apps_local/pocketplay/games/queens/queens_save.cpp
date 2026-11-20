#include "queens_save.h"

namespace queens {

size_t packGame(const State& state, uint8_t* out) {
  size_t at = 0;
  out[at++] = state.bank;
  for (int b = 0; b < kBanks; b++) out[at++] = state.place[b];
  out[at++] = state.dots ? 1 : 0;
  const int cells = state.game.puzzle.n * state.game.puzzle.n;
  for (int i = 0; i < cells; i += 4) {
    uint8_t byte = 0;
    for (int k = 0; k < 4 && i + k < cells; k++) {
      const int c = i + k;
      const uint8_t v = state.game.hinted[c] ? 3 : state.game.marks[c];
      byte |= static_cast<uint8_t>(v << (2 * k));
    }
    out[at++] = byte;
  }
  return at;
}

bool unpackGame(uint8_t version, const uint8_t* in, size_t size, State& state) {
  if (version != SAVE_VERSION || size < SAVE_HEADER) return false;
  size_t at = 0;
  const uint8_t bank = in[at++];
  if (bank >= kBanks) return false;
  uint8_t place[kBanks];
  for (int b = 0; b < kBanks; b++) {
    place[b] = in[at++];
    if (place[b] >= kBankPuzzles) return false;
  }
  const uint8_t dots = in[at++];
  if (dots > 1) return false;
  const int n = kBankList[bank].size;
  const int cells = n * n;
  if (size != SAVE_HEADER + static_cast<size_t>((cells + 3) / 4)) return false;
  // The spare bits after the last cell must be clear, so a save is read only one way.
  for (int c = cells; c < (cells + 3) / 4 * 4; c++) {
    if ((in[at + c / 4] >> (2 * (c % 4))) & 3) return false;
  }

  // Check against the puzzle alone before touching the state, so a refused save leaves it as
  // it was without copying the whole game.
  auto markAt = [&](int c) { return static_cast<uint8_t>((in[at + c / 4] >> (2 * (c % 4))) & 3); };
  Puzzle puzzle;
  if (!parseLine(kBankList[bank].lines[place[bank]], puzzle) || puzzle.n != n) return false;
  for (int c = 0; c < cells; c++) {
    // A hint only ever places a right piece.
    if (markAt(c) == 3 && !puzzle.solution[c]) return false;
  }

  state.bank = bank;
  for (int b = 0; b < kBanks; b++) state.place[b] = place[b];
  state.dots = dots == 1;
  loadPuzzle(state);
  for (int c = 0; c < cells; c++) {
    const uint8_t v = markAt(c);
    state.game.marks[c] = v == 3 ? static_cast<uint8_t>(Piece) : v;
    state.game.hinted[c] = v == 3;
  }
  return true;
}

size_t packDone(const State& state, uint8_t* out) {
  size_t at = 0;
  for (int b = 0; b < kBanks; b++) {
    for (int k = 0; k < kDoneBytes; k++) out[at++] = state.done[b][k];
  }
  return at;
}

bool unpackDone(uint8_t version, const uint8_t* in, size_t size, State& state) {
  if (version != DONE_VERSION || size != DONE_BYTES) return false;
  // 200 puzzles leave nothing spare in the last byte of each bank.
  for (int b = 0; b < kBanks; b++) {
    for (int k = 0; k < kDoneBytes; k++) state.done[b][k] = in[b * kDoneBytes + k];
  }
  return true;
}

}  // namespace queens
