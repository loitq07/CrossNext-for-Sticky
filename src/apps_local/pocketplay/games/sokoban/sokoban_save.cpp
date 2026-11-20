#include "sokoban_save.h"

namespace sokoban {

size_t packGame(const State& state, uint8_t* out) {
  size_t at = 0;
  out[at++] = GAME_VERSION;
  out[at++] = state.pack;
  for (int p = 0; p < kPackCount; p++) out[at++] = state.place[p];
  const uint16_t count = state.game.count;
  out[at++] = static_cast<uint8_t>(count & 0xFF);
  out[at++] = static_cast<uint8_t>(count >> 8);
  for (int i = 0; i < count; i += 4) {
    uint8_t byte = 0;
    for (int k = 0; k < 4 && i + k < count; k++) byte |= static_cast<uint8_t>((state.game.history[i + k] & 3) << (2 * k));
    out[at++] = byte;
  }
  return at;
}

bool unpackGame(const uint8_t* in, size_t size, State& state) {
  if (size < GAME_HEADER || in[0] != GAME_VERSION) return false;
  size_t at = 1;
  const uint8_t pack = in[at++];
  if (pack >= kPackCount) return false;
  uint8_t place[kPackCount];
  for (int p = 0; p < kPackCount; p++) {
    place[p] = in[at++];
    if (place[p] >= kPacks[p].count) return false;
  }
  const int count = in[at] | (in[at + 1] << 8);
  at += 2;
  if (count > kMaxHistory || size != GAME_HEADER + static_cast<size_t>((count + 3) / 4)) return false;

  // Check every move on a board of its own first: the state's game, with its history, is
  // too big to copy onto the stack, and must be left alone if the save is refused.
  auto dirAt = [&](int i) { return static_cast<Dir>((in[at + i / 4] >> (2 * (i % 4))) & 3); };
  Board trial;
  build(kPacks[pack].levels[place[pack]], trial);
  for (int i = 0; i < count; i++) {
    if (solved(trial) || step(trial, dirAt(i)) == Step::Blocked) return false;
  }

  state.pack = pack;
  for (int p = 0; p < kPackCount; p++) state.place[p] = place[p];
  loadLevel(state);
  for (int i = 0; i < count; i++) move(state.game, dirAt(i));
  return true;
}

size_t packBest(const State& state, uint8_t* out) {
  size_t at = 0;
  out[at++] = BEST_VERSION;
  for (int i = 0; i < kTotalLevels; i++) {
    out[at++] = static_cast<uint8_t>(state.best[i] & 0xFF);
    out[at++] = static_cast<uint8_t>(state.best[i] >> 8);
  }
  return at;
}

bool unpackBest(const uint8_t* in, size_t size, State& state) {
  if (size != BEST_BYTES || in[0] != BEST_VERSION) return false;
  for (int i = 0; i < kTotalLevels; i++) state.best[i] = static_cast<uint16_t>(in[1 + 2 * i] | (in[2 + 2 * i] << 8));
  return true;
}

}  // namespace sokoban
