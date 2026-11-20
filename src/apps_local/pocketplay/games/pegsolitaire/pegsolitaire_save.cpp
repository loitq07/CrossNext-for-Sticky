#include "pegsolitaire_save.h"

namespace pegsolitaire {

size_t pack(const State& state, uint8_t* out) {
  size_t at = 0;
  out[at++] = state.game.board;
  for (int b = 0; b < kBoards; b++) out[at++] = state.best[b];
  out[at++] = static_cast<uint8_t>(state.game.jumps);
  for (int k = 0; k < state.game.jumps; k++) {
    out[at++] = state.game.history[k].from;
    out[at++] = state.game.history[k].to;
  }
  return at;
}

bool unpack(uint8_t version, const uint8_t* in, size_t size, State& state) {
  if (version != SAVE_VERSION || size < SAVE_HEADER) return false;
  size_t at = 0;
  const uint8_t board = in[at++];
  if (board >= kBoards) return false;
  uint8_t best[kBoards];
  for (int b = 0; b < kBoards; b++) {
    best[b] = in[at++];
    if (best[b] >= kBoardList[b].cells) return false;
  }
  const int jumps = in[at++];
  if (jumps > kBoardList[board].cells - 2 || size != SAVE_HEADER + 2 * static_cast<size_t>(jumps)) return false;

  Game g;
  reset(g, board);
  for (int k = 0; k < jumps; k++) {
    const int from = in[at++];
    const int to = in[at++];
    if (from >= kBoardList[board].cells || to >= kBoardList[board].cells || !jump(g, from, to)) return false;
  }
  state.game = g;
  for (int b = 0; b < kBoards; b++) state.best[b] = best[b];
  state.sel = kNoCell;
  state.message = Message::Help;
  return true;
}

}  // namespace pegsolitaire
