#include "lastpiece_save.h"

namespace lastpiece {

size_t pack(const State& state, uint8_t* out) {
  size_t at = 0;
  out[at++] = state.difficulty;
  for (int d = 0; d < kDifficulties; d++) out[at++] = state.challenge[d];
  for (int d = 0; d < kDifficulties; d++) {
    for (int k = 0; k < kSolvedBytes; k++) out[at++] = state.solved[d][k];
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
  s.difficulty = in[at++];
  if (s.difficulty >= kDifficulties) return false;
  for (int d = 0; d < kDifficulties; d++) {
    s.challenge[d] = in[at++];
    if (s.challenge[d] >= kMaxChallenges) return false;
  }
  // The last byte of each difficulty has room for 104 bits; the 5 past challenge 99 stay clear.
  constexpr uint8_t kSpareBits = static_cast<uint8_t>(0xFF << (kMaxChallenges % 8));
  for (int d = 0; d < kDifficulties; d++) {
    for (int k = 0; k < kSolvedBytes; k++) s.solved[d][k] = in[at++];
    if (s.solved[d][kSolvedBytes - 1] & kSpareBits) return false;
  }
  for (int i = 0; i < kCells; i += 2) {
    const uint8_t pair = in[at++];
    s.start.cell[i] = pair >> 4;
    s.start.cell[i + 1] = pair & 0x0F;
    if (s.start.cell[i] >= kPieceTypes || s.start.cell[i + 1] >= kPieceTypes) return false;
  }
  // An empty board is a card with no challenges on it yet.
  const int pieces = pieceCount(s.start);
  if (pieces == 1 || pieces > kMaxPieces) return false;
  s.moveCount = in[at++];
  if ((pieces > 0 && s.moveCount > pieces - 1) || (pieces == 0 && s.moveCount > 0)) return false;
  if (size != SAVE_HEADER + s.moveCount) return false;
  for (int k = 0; k < s.moveCount; k++) s.moves[k] = in[at++];
  if (!replay(s)) return false;
  clearView(s);
  state = s;
  return true;
}

}  // namespace lastpiece
