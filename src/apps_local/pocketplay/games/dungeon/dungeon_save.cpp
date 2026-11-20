#include "dungeon_save.h"

namespace dungeon {

namespace {

void putWord(uint8_t* out, uint64_t value) {
  for (int i = 0; i < 8; i++) out[i] = static_cast<uint8_t>(value >> (8 * i));
}

uint64_t getWord(const uint8_t* in) {
  uint64_t value = 0;
  for (int i = 0; i < 8; i++) value |= static_cast<uint64_t>(in[i]) << (8 * i);
  return value;
}

// Bits for dungeons that exist, above the tutorial.
constexpr uint64_t LOW_VALID = ~uint64_t{1};
constexpr uint64_t HIGH_VALID = kPuzzleCount > 64 ? (uint64_t{1} << (kPuzzleCount - 64)) - 1 : 0;

}  // namespace

size_t packGame(const Board& board, const Progress& progress, uint8_t* out) {
  out[0] = static_cast<uint8_t>(board.index());
  putWord(out + 1, board.wallMask());
  putWord(out + 9, board.floorMask());
  putWord(out + 17, progress.low);
  putWord(out + 25, progress.high);
  return SAVE_BYTES;
}

bool unpackGame(uint8_t version, const uint8_t* in, size_t size, Board& board, Progress& progress) {
  if (version != SAVE_VERSION || size != SAVE_BYTES) return false;
  if (!isPlayable(in[0])) return false;

  Progress record;
  record.low = getWord(in + 17);
  record.high = getWord(in + 25);
  if ((record.low & ~LOW_VALID) != 0 || (record.high & ~HIGH_VALID) != 0) return false;

  progress = record;
  board.restore(in[0], getWord(in + 1), getWord(in + 9));
  return true;
}

}  // namespace dungeon
