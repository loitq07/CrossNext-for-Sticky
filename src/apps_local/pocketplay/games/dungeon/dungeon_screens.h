#pragma once

#include <cstdint>

#include "dungeon_rules.h"

// D&Diagrams' four screens, ported from CrossPlay's DungeonScreens.cpp. Each draw
// function fills in where it put the things a tap can hit, so a tap is resolved
// through the same geometry that placed the pixels.
namespace dungeonui {

struct Rect {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;

  bool contains(int px, int py) const { return px >= x && px < x + width && py >= y && py < y + height; }
};

// Where the board was drawn.
struct Layout {
  Rect board;  // the play area, without the clue lanes
  int cell = 0;
  int size = 0;

  // The cell under (x, y), or false if the tap missed the board.
  bool cellAt(int x, int y, int& row, int& col) const;
};

// Where the menu drew the campaign map.
struct PickerLayout {
  Rect grid;
  int cell = 0;
  int gap = 0;
  int cols = 0;
  int rows = 0;

  // The map position under (x, y), 0..63, or -1. A tap in the gap after a cell
  // belongs to that cell, so the small targets are easy to hit with a thumb.
  int indexAt(int x, int y) const;
};

// The buttons, and where each screen puts them. A screen's taps go through
// buttonAt(), so the hit areas are the ones drawn.
enum class Button { None, Play, Guide, Reset, Menu, Next, GuidePrevious, GuideNext };

struct BoardModel {
  const dungeon::Board* board = nullptr;
  int solvedCount = 0;
  int total = 0;
};

struct MenuModel {
  // The dungeon PLAY would open: the one last tapped on the map, or the next
  // unsolved. Tapping the map picks a dungeon, it doesn't open it.
  int selectedIndex = 0;
  int solvedCount = 0;
  int total = 0;
  bool hasProgress = false;  // PLAY reads RESUME
  const dungeon::Progress* progress = nullptr;
};

struct GuideModel {
  int page = 0;
};

struct WinModel {
  int clearedIndex = 0;  // the dungeon just finished, not the one now loaded
  int solvedCount = 0;
  int total = 0;
  bool moreToPlay = true;
};

void drawBoard(uint8_t* frame, const BoardModel& model, Layout& layout);
void drawMenu(uint8_t* frame, const MenuModel& model, PickerLayout& layout);
void drawGuide(uint8_t* frame, const GuideModel& model);
void drawWin(uint8_t* frame, const WinModel& model);

Button boardButtonAt(int x, int y);
Button menuButtonAt(int x, int y);
Button guideButtonAt(int x, int y);
Button winButtonAt(int x, int y, bool moreToPlay);

}  // namespace dungeonui
