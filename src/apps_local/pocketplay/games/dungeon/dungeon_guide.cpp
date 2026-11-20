#include "dungeon_guide.h"

namespace dungeonui {

namespace {

// ---------------------------------------------------------------------------
// The adventurer's guide.
//
// One board, revealed a step at a time. Every page shows the SAME dungeon --
// the tutorial, kPuzzles[0] -- a little further along, with the rule that
// justifies the next step underneath and a ring round the part of the board it
// is about. Read it through and you have watched a dungeon get solved.
//
// The first version was five pages of abstract little diagrams with captions,
// and it was worse in a way worth recording: a diagram is a second thing to
// learn before you can learn the game. Teaching on the real surface means the
// page and the board are furnished identically, down to which creature lives
// there, so nothing has to be translated when the tutorial opens.
//
// Each page's board is written out as text and turned into the same masks the
// player's own board uses, so a page cannot drift from the puzzle: a host test
// asserts every page's walls are a subset of the real solution and that the
// last page IS the solution.
//
//   #  wall        o  floor, marked        .  untouched
// ---------------------------------------------------------------------------

constexpr GuidePage kGuide[] = {
    {"THE MAP",
     "SOMEWHERE IN THIS GRID IS A DUNGEON. YOU FIND IT BY PLACING WALLS, USING NOTHING BUT LOGIC.",
     {"......", "......", "......", "......", "......", "......"},
     {0, 0, 0, 0, false}},

    {"THE NUMBERS",
     "EACH NUMBER COUNTS THE WALLS IN ITS ROW OR COLUMN. TAP A SQUARE TO PLACE ONE.",
     {"###...", "#.....", "......", "#.....", "#.....", "......"},
     {0, 0, 0, 0, true}},

    {"YOUR NOTES",
     "TAP AGAIN TO MARK FLOOR, ONCE MORE TO CLEAR. THOSE MARKS ARE YOUR NOTES, NOT THE ANSWER.",
     {"###ooo", "#ooooo", ".o....", "#.....", "#.....", "......"},
     {0, 0, 0, 0, false}},

    {"TREASURE",
     "A CHEST SITS IN A 3x3 ROOM OF FLOOR. ONE WAY IN, ONE CHEST, AND NOTHING LIVING IN IT.",
     {"###ooo", "#ooooo", ".oooo.", "#.....", "#.....", "......"},
     {0, 3, 3, 3, false}},

    {"MONSTERS",
     "EVERY MONSTER SITS IN A DEAD END, AND EVERY DEAD END HAS A MONSTER IN IT.",
     {"###ooo", "#ooooo", ".oooo.", "#.####", "#....#", "..#..."},
     {2, 0, 1, 1, false}},

    {"ONE DUNGEON",
     "ALL THE FLOOR JOINS UP. SQUARES THAT ONLY TOUCH AT A CORNER ARE NOT JOINED.",
     {"###ooo", "#ooooo", ".oooo.", "#.####", "#o..##", "..#..."},
     {0, 0, 0, 0, false}},

    {"CORRIDORS",
     "CORRIDORS ARE ONE SQUARE WIDE. NO 2x2 OF OPEN FLOOR OUTSIDE A TREASURE ROOM.",
     {"###ooo", "#ooooo", ".oooo.", "#.####", "#ooo#.", "..#..."},
     {3, 1, 2, 2, false}},

    {"YOUR TURN",
     "THAT IS EVERY RULE. SIXTY-FOUR DUNGEONS ARE WAITING FOR YOU.",
     {"###...", "#.....", "..#...", "#.####", "#...#.", "..#..."},
     {0, 0, 0, 0, false}},
};

constexpr int kGuidePages = static_cast<int>(sizeof(kGuide) / sizeof(kGuide[0]));

}  // namespace

int guidePageCount() { return kGuidePages; }

const GuidePage& guidePage(const int page) { return kGuide[(page < 0 || page >= kGuidePages) ? 0 : page]; }

// The tutorial board at the state page `page` shows it.
dungeon::Board guideBoard(const int page) {
  const GuidePage& content = guidePage(page);
  uint64_t walls = 0;
  uint64_t floors = 0;
  const int size = dungeon::kPuzzles[0].size;
  for (int r = 0; r < size; ++r) {
    for (int c = 0; c < size; ++c) {
      const char ch = content.rows[r][c];
      const uint64_t bit = uint64_t{1} << (r * 8 + c);
      if (ch == '#') {
        walls |= bit;
      } else if (ch == 'o') {
        floors |= bit;
      }
    }
  }
  dungeon::Board board;
  // restore() strips anything standing on a monster or a chest, so a page can
  // never draw a wall through the furniture even if the text above says so.
  board.restore(0, walls, floors);
  return board;
}

bool guidePageWalls(const int page, uint64_t& walls) {
  if (page < 0 || page >= kGuidePages) return false;
  walls = guideBoard(page).wallMask();
  return true;
}

}  // namespace dungeonui
