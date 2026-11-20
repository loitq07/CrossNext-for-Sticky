#include "gridgambit_save.h"

namespace gridgambit {

size_t pack(const State& state, uint8_t* out) {
  size_t at = 0;
  out[at++] = state.level;
  out[at++] = state.counted ? 1 : 0;
  for (int l = 0; l < kLevels; l++) {
    out[at++] = static_cast<uint8_t>(state.solved[l] >> 8);
    out[at++] = static_cast<uint8_t>(state.solved[l] & 0xFF);
  }
  for (int i = 0; i < kCells; i += 2) {
    out[at++] = static_cast<uint8_t>(state.start.cell[i] << 4 | state.start.cell[i + 1]);
  }
  out[at++] = state.moveCount;
  for (int k = 0; k < state.moveCount; k++) out[at++] = state.moves[k];
  return at;
}

bool unpack(uint8_t version, const uint8_t* in, size_t size, State& state) {
  if (version != SAVE_VERSION || size < SAVE_HEADER) return false;
  State s;
  size_t at = 0;
  s.level = in[at++];
  if (s.level >= kLevels) return false;
  const uint8_t counted = in[at++];
  if (counted > 1) return false;
  s.counted = counted == 1;
  for (int l = 0; l < kLevels; l++) {
    s.solved[l] = static_cast<uint16_t>(in[at] << 8 | in[at + 1]);
    at += 2;
  }
  for (int i = 0; i < kCells; i += 2) {
    const uint8_t pair = in[at++];
    s.start.cell[i] = pair >> 4;
    s.start.cell[i + 1] = pair & 0x0F;
    if (s.start.cell[i] >= kPieceTypes || s.start.cell[i + 1] >= kPieceTypes) return false;
  }
  const int pieces = pieceCount(s.start);
  if (pieces < 2 || pieces > kMaxPieces) return false;
  s.moveCount = in[at++];
  if (s.moveCount > pieces - 1 || size != SAVE_HEADER + s.moveCount) return false;
  for (int k = 0; k < s.moveCount; k++) s.moves[k] = in[at++];
  if (!replay(s)) return false;
  s.sel = kNoCell;
  s.message = Message::Help;
  state = s;
  return true;
}

}  // namespace gridgambit
