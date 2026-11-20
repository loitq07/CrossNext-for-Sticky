#include "lightsout_save.h"

namespace lightsout {

namespace {

void put32(uint8_t* out, uint32_t v) {
  for (int i = 0; i < 4; i++) out[i] = static_cast<uint8_t>(v >> (8 * i));
}

uint32_t get32(const uint8_t* in) {
  uint32_t v = 0;
  for (int i = 0; i < 4; i++) v |= static_cast<uint32_t>(in[i]) << (8 * i);
  return v;
}

}  // namespace

size_t packGame(const Game& game, uint8_t* out) {
  put32(out, game.board);
  out[4] = static_cast<uint8_t>(game.moves);
  out[5] = static_cast<uint8_t>(game.moves >> 8);
  out[6] = game.par;
  out[7] = game.historyCount;
  for (int i = 0; i < game.historyCount; i++) out[SAVE_HEADER + i] = game.history[i];
  return SAVE_HEADER + game.historyCount;
}

bool unpackGame(uint8_t version, const uint8_t* in, size_t size, Game& game) {
  if (version != SAVE_VERSION || size < SAVE_HEADER) return false;

  Game loaded;
  loaded.board = get32(in);
  loaded.moves = static_cast<uint16_t>(in[4] | (in[5] << 8));
  loaded.par = in[6];
  loaded.historyCount = in[7];
  if (size != SAVE_HEADER + loaded.historyCount) return false;
  if (loaded.board == 0 || (loaded.board & ~kAllLights) != 0) return false;
  if (fewestPresses(loaded.board) < 0) return false;
  if (loaded.par < 1 || loaded.par > kCells) return false;
  if (loaded.moves < loaded.historyCount) return false;
  for (int i = 0; i < loaded.historyCount; i++) {
    if (in[SAVE_HEADER + i] >= kCells) return false;
    loaded.history[i] = in[SAVE_HEADER + i];
  }

  game = loaded;
  return true;
}

size_t packStats(const Stats& stats, uint8_t* out) {
  put32(out, stats.solved);
  out[4] = stats.level;
  return STATS_BYTES;
}

bool unpackStats(uint8_t version, const uint8_t* in, size_t size, Stats& stats) {
  if (version != STATS_VERSION || size != STATS_BYTES || in[4] >= kLevelCount) return false;
  stats.solved = get32(in);
  stats.level = in[4];
  return true;
}

}  // namespace lightsout
