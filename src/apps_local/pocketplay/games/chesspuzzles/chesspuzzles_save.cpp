#include "chesspuzzles_save.h"

namespace chesspuzzles {

namespace {

void put16(uint8_t* out, size_t& at, uint16_t v) {
  out[at++] = static_cast<uint8_t>(v >> 8);
  out[at++] = static_cast<uint8_t>(v & 0xFF);
}

uint16_t get16(const uint8_t* in, size_t& at) {
  const uint16_t v = static_cast<uint16_t>(in[at] << 8 | in[at + 1]);
  at += 2;
  return v;
}

}  // namespace

size_t pack(const State& state, uint8_t* out) {
  size_t at = 0;
  out[at++] = state.bank;
  for (int k = 0; k < kBanks; k++) {
    put16(out, at, state.index[k]);
    put16(out, at, state.solved[k]);
    put16(out, at, state.clean[k]);
  }
  out[at++] = state.ply;
  out[at++] = state.mistakes;
  out[at++] = state.shown ? 1 : 0;
  out[at++] = state.done ? 1 : 0;
  return at;
}

bool unpack(uint8_t version, const uint8_t* in, size_t size, State& state) {
  if (version != SAVE_VERSION || size != SAVE_SIZE) return false;
  State s;
  size_t at = 0;
  const uint8_t bank = in[at++];
  if (bank >= kBanks) return false;
  for (int k = 0; k < kBanks; k++) {
    s.index[k] = get16(in, at);
    s.solved[k] = get16(in, at);
    s.clean[k] = get16(in, at);
    if (s.index[k] >= kBankList[k].count || s.clean[k] > s.solved[k]) return false;
  }
  const uint8_t ply = in[at++];
  const uint8_t mistakes = in[at++];
  const uint8_t shown = in[at++];
  const uint8_t done = in[at++];
  if (shown > 1 || done > 1) return false;
  if (!start(s, bank, s.index[bank])) return false;
  // Your moves come at odd plies, so a puzzle under way is at one; a finished one is past the end.
  const int n = s.puzzle.moveCount;
  if (done ? ply != n : (ply < 1 || ply >= n || ply % 2 == 0)) return false;
  start(s, bank, s.index[bank], ply);
  s.mistakes = mistakes;
  s.shown = shown == 1;
  s.done = done == 1;
  if (s.done) s.message = Message::Solved;
  state = s;
  return true;
}

}  // namespace chesspuzzles
