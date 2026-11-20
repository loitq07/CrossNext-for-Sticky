#include "tictactoe_save.h"

namespace tictactoe {

size_t packGame(const Game& game, uint8_t* out) {
  for (int i = 0; i < kCells; i++) {
    out[i] = game.cell[i];
  }
  out[kCells] = game.turn;
  return SAVE_BYTES;
}

bool unpackGame(uint8_t version, const uint8_t* in, size_t size, Game& game) {
  if (version != SAVE_VERSION || size != SAVE_BYTES) return false;

  Game loaded;
  int xCount = 0;
  int oCount = 0;
  for (int i = 0; i < kCells; i++) {
    const uint8_t cell = in[i];
    if (cell == kX) {
      xCount++;
    } else if (cell == kO) {
      oCount++;
    } else if (cell != kEmpty) {
      return false;
    }
    loaded.cell[i] = cell;
  }

  // X opens and players alternate, so the counts decide whose turn it must be.
  const uint8_t turn = in[kCells];
  const bool xToMove = xCount == oCount && turn == kX;
  const bool oToMove = xCount == oCount + 1 && turn == kO;
  if (!xToMove && !oToMove) return false;

  loaded.turn = turn;
  loaded.outcome = Outcome::Running;
  if (winningLine(loaded, kX, nullptr) || winningLine(loaded, kO, nullptr) || boardFull(loaded)) return false;

  game = loaded;
  return true;
}

}  // namespace tictactoe
