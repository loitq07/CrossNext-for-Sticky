#include "nonogram_save.h"

namespace nonogram {

size_t packState(const State& state, uint8_t* out) {
  size_t at = 0;
  out[at++] = state.size;
  for (int s = 0; s < kSizes; s++) out[at++] = state.place[s];
  out[at++] = state.cross ? 1 : 0;
  for (int s = 0; s < kSizes; s++) {
    for (int k = 0; k < kDoneBytes; k++) out[at++] = state.done[s][k];
  }
  return at;
}

bool unpackState(uint8_t version, const uint8_t* in, size_t size, State& state) {
  if (version != SAVE_VERSION || size != SAVE_BYTES) return false;
  size_t at = 0;
  if (in[at++] >= kSizes) return false;
  for (int s = 0; s < kSizes; s++) {
    if (in[at++] >= kBankPuzzles) return false;
  }
  if (in[at++] > 1) return false;
  // The bits past the last puzzle must be clear, so a save is read only one way.
  for (int s = 0; s < kSizes; s++) {
    if (in[at + s * kDoneBytes + kDoneBytes - 1] >> (kBankPuzzles % 8)) return false;
  }
  at = 0;
  state.size = in[at++];
  for (int s = 0; s < kSizes; s++) state.place[s] = in[at++];
  state.cross = in[at++] == 1;
  for (int s = 0; s < kSizes; s++) {
    for (int k = 0; k < kDoneBytes; k++) state.done[s][k] = in[at++];
  }
  return true;
}

std::vector<uint8_t> packGrids(const State& state) {
  std::vector<uint8_t> out;
  out.push_back(GRIDS_VERSION);
  for (int s = 0; s < kSizes; s++) {
    const std::vector<Kept>& list = state.kept[s];
    out.push_back(static_cast<uint8_t>(list.size()));
    for (const Kept& k : list) {
      out.push_back(k.puzzle);
      for (size_t i = 0; i < k.cells.size(); i += 4) {
        uint8_t byte = 0;
        for (size_t j = 0; j < 4 && i + j < k.cells.size(); j++) byte |= static_cast<uint8_t>(k.cells[i + j] << (2 * j));
        out.push_back(byte);
      }
    }
  }
  return out;
}

bool unpackGrids(const uint8_t* in, size_t size, State& state) {
  if (size < 1 || in[0] != GRIDS_VERSION) return false;
  size_t at = 1;
  std::vector<Kept> kept[kSizes];
  for (int s = 0; s < kSizes; s++) {
    if (at >= size) return false;
    const size_t count = in[at++];
    if (count > kKeepGrids) return false;
    const int n = kSizeList[s];
    const size_t cells = static_cast<size_t>(n * n);
    const size_t bytes = (cells + 3) / 4;
    for (size_t c = 0; c < count; c++) {
      if (size - at < 1 + bytes) return false;
      Kept k;
      k.puzzle = in[at++];
      if (k.puzzle >= kBankPuzzles) return false;
      for (const Kept& other : kept[s]) {
        if (other.puzzle == k.puzzle) return false;
      }
      for (size_t i = 0; i < bytes * 4; i++) {
        const uint8_t v = (in[at + i / 4] >> (2 * (i % 4))) & 3;
        if (i >= cells ? v != 0 : v > Crossed) return false;
        if (i < cells) k.cells.push_back(v);
      }
      at += bytes;
      Board b;
      decode(bankFor(s)[k.puzzle], n, b.puzzle);
      for (size_t i = 0; i < cells; i++) b.cells[i] = k.cells[i];
      if (blank(b) || solved(b)) return false;
      kept[s].push_back(std::move(k));
    }
  }
  if (at != size) return false;
  for (int s = 0; s < kSizes; s++) state.kept[s] = std::move(kept[s]);
  return true;
}

}  // namespace nonogram
