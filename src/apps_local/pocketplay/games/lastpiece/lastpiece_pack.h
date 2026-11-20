#pragma once

#include <cstddef>
#include <cstdint>

#include "lastpiece_rules.h"

// One difficulty's challenges, read from the text files the player copies onto the card, with
// no card code: the game reads the files and hands the bytes here.
//
// The boards file (such as beginner.txt) holds each challenge as 4 lines of 4 cells, top row
// first. A cell is P N B R Q K for a pawn, knight, bishop, rook, queen or king, in either case,
// or . for an empty square. Spaces between cells are optional. Blank lines separate challenges,
// and a line starting with # is a comment. Challenges are numbered 1, 2, ... in the file's order,
// the bad ones included, so a solution's number still matches after a typo.
//
// The solutions file (such as beginner-solutions.txt) holds one line per challenge, such as
// "01: 3,0->2,1 2,1->1,2 1,2->0,1": the challenge's number, then each capture as the row and
// column it leaves and the row and column it takes, with row 0 at the top.
namespace lastpiece {

constexpr int kMaxChallenges = 99;
constexpr size_t kMaxFileBytes = 16 * 1024;

struct Challenge {
  Board board;
  uint8_t number = 0;                  // its place in the file, from 1
  uint8_t solution[kMaxMoves] = {};    // each capture as from << 4 | to
  uint8_t solutionLength = 0;          // 0 when the solutions file has none for it
};

struct Pack {
  Challenge challenges[kMaxChallenges];
  uint8_t count = 0;         // the challenges that can be played
  uint8_t skipped = 0;       // challenges left out: a bad cell or row, too few or too many pieces, or no win
  uint8_t overLimit = 0;     // good challenges past kMaxChallenges
  uint8_t badSolutions = 0;  // solution lines that don't parse, don't play, or name no challenge
};

inline int moveFrom(uint8_t m) { return m >> 4; }
inline int moveTo(uint8_t m) { return m & 0x0F; }

// Reads a boards file into `pack`, replacing what was there. Each board is checked with
// solvable(), so nothing that can't be won is offered.
void parsePack(const uint8_t* text, size_t size, Pack& pack, Memo& memo);

// Adds the solutions in a solutions file to the challenges already in `pack`. A solution is kept
// only if every move is a capture and it ends on one piece.
void parseSolutions(const uint8_t* text, size_t size, Pack& pack);

}  // namespace lastpiece
