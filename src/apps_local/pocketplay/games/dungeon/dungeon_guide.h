#pragma once

// The adventurer's guide: the rules, one page at a time, each shown on the
// tutorial's own board a little further along. Moved here from CrossPlay's
// DungeonScreens.cpp unchanged, so the test can check the pages without any
// drawing code: every page's walls must be part of the tutorial's solution, and
// the last page must be all of it.

#include <cstdint>

#include "dungeon_rules.h"

namespace dungeonui {

// Where a guide page wants the eye. Cell coordinates; width 0 means nowhere.
struct Spotlight {
  int8_t row = 0;
  int8_t col = 0;
  int8_t width = 0;
  int8_t height = 0;
  // Ring the clue lanes instead of a patch of board.
  bool lanes = false;
};

struct GuidePage {
  const char* title;
  const char* body;
  const char* rows[8];
  Spotlight spotlight;
};

int guidePageCount();

// Page `page`, clamped to the guide.
const GuidePage& guidePage(int page);

// The tutorial board at the state page `page` shows it.
dungeon::Board guideBoard(int page);

// The walls page `page` shows, for the test. False if the page is out of range.
bool guidePageWalls(int page, uint64_t& walls);

}  // namespace dungeonui
