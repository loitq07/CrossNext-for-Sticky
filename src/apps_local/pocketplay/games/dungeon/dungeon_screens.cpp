#include "dungeon_screens.h"

#include <cstdio>

#include "core/graphics.h"
#include "core/text.h"
#include "dungeon_art.h"
#include "dungeon_guide.h"
#include "ui/ui.h"

// Ported from CrossPlay's DungeonScreens.cpp, which explains each choice at length.
// The measurements are CrossPlay's; only the drawing calls are PocketPlay's.
//
// The board is a plan drawn on paper: a hairline lattice inside a heavy frame,
// walls filling their cells so runs of rock merge into one shape, and floor notes
// as dots.
namespace dungeonui {

namespace {

constexpr const char* TITLE = "D&DIAGRAMS";

// The body every screen lays out in: under the warning line, inside the margins.
constexpr int MARGIN = 16;
constexpr int GUTTER = 12;
constexpr int BODY_LEFT = MARGIN;
constexpr int BODY_WIDTH = SCREEN_WIDTH - 2 * MARGIN;
constexpr int BODY_TOP = ui::CONTENT_TOP;
constexpr int BODY_BOTTOM = SCREEN_HEIGHT - MARGIN;

// Every screen's buttons sit in one row along the bottom.
constexpr int ACTIONS_TOP = BODY_BOTTOM - ui::PILL_HEIGHT;
constexpr int ACTION_WIDTH = (BODY_WIDTH - GUTTER) / 2;
constexpr int ABOVE_ACTIONS = ACTIONS_TOP - GUTTER;

// A clue lane, a gap, then eight cells: 38 + 13 + 8 * 48 + 9 fills the 448 px
// between the margins. The lane is as tall as a line of the Size30 digits, and the
// row lane only as wide as a digit and its chip.
constexpr int CELL = 48;
constexpr int LANE = 56;
constexpr int CLUE_LANE = 38;
constexpr int CLUE_GAP = ui::BOARD_FRAME + 4;  // so a filled clue never touches the frame
constexpr int ART_SIZE = 36;
constexpr int MAP_ART = 32;
constexpr int NAME_STRIP = 34;

// The board screen: the dungeon's name above the board.
constexpr int BOARD_BODY_TOP = BODY_TOP + NAME_STRIP + GUTTER;

// The menu: the picked dungeon, the campaign map, then TUTORIAL.
constexpr int NEXT_HEIGHT = 44;
constexpr int NEXT_TOP = ABOVE_ACTIONS - NEXT_HEIGHT;
constexpr int MAP_TOP = BODY_TOP + NAME_STRIP + GUTTER;
constexpr int MAP_BOTTOM = NEXT_TOP - MARGIN;
constexpr int MAP_LANE = 30;
constexpr int MAP_GAP = 5;

// The guide teaches on a smaller board, because it shares the screen with a caption.
constexpr int GUIDE_CELL = 44;
constexpr int GUIDE_CAPTION_LINES = 4;

constexpr int WIN_MAP_CELL = 34;

constexpr Font NAME_CUTS[] = {Font::Size20, Font::Size14, Font::Size10};

int laneOverhang(int cell) { return (cell - LANE) / 2; }

// One kind of monster per dungeon, picked by a hash of its name rather than its
// position, so the map's columns don't repeat (CrossPlay's reasoning). FNV-1a.
int monsterKind(int puzzleIndex) {
  const int index = (puzzleIndex < 0 || puzzleIndex >= dungeon::kPuzzleCount) ? 0 : puzzleIndex;
  uint32_t hash = 2166136261u;
  for (const char* text = dungeon::kPuzzles[index].name; *text != '\0'; text++) {
    hash ^= static_cast<uint8_t>(*text);
    hash *= 16777619u;
  }
  return static_cast<int>(hash % dungeonArt::MONSTER_KINDS);
}

// The largest cut a name fits in on one line. Never shortened: a cut name looks
// like a different dungeon.
Font fitName(const char* text, int width) {
  for (const Font cut : NAME_CUTS) {
    if (textWidth(cut, text) <= width) return cut;
  }
  return Font::Size10;
}

// 25% grey, every other pixel on both axes: CrossPlay's LightGray. It's the only
// grey light enough to read black digits on.
void fillLightGray(uint8_t* frame, const Rect& box) {
  for (int y = box.y; y < box.y + box.height; y++) {
    for (int x = box.x; x < box.x + box.width; x++) {
      if (x % 2 == 0 && y % 2 == 0) setPixel(frame, x, y, true);
    }
  }
}

void drawArt(uint8_t* frame, const Rect& cell, const uint8_t* bits, int size, bool black) {
  drawIcon(frame, bits, size, size, cell.x + (cell.width - size) / 2, cell.y + (cell.height - size) / 2, black);
}

// Four corner marks and nothing between them: how CrossPlay points at a region
// without covering it.
void cornerBrackets(uint8_t* frame, const Rect& box, int arm = 18, int weight = ui::FRAME) {
  const int right = box.x + box.width;
  const int bottom = box.y + box.height;
  fillRect(frame, box.x, box.y, arm, weight, true);
  fillRect(frame, box.x, box.y, weight, arm, true);
  fillRect(frame, right - arm, box.y, arm, weight, true);
  fillRect(frame, right - weight, box.y, weight, arm, true);
  fillRect(frame, box.x, bottom - weight, arm, weight, true);
  fillRect(frame, box.x, bottom - arm, weight, arm, true);
  fillRect(frame, right - arm, bottom - weight, arm, weight, true);
  fillRect(frame, right - weight, bottom - arm, weight, arm, true);
}

void drawHeader(uint8_t* frame, const char* title, int count, int total) {
  char counter[24];
  snprintf(counter, sizeof(counter), "%d/%d", count, total);
  ui::drawBand(frame, title, counter, true, Font::Size20);
}

// The whole frame and lanes, which is what has to be centred, not just the cells.
int boardDrawnWidth(int size, int cell) { return CLUE_LANE + CLUE_GAP + size * cell + ui::BOARD_FRAME; }
int boardDrawnHeight(int size, int cell) { return LANE + CLUE_GAP + size * cell + ui::BOARD_FRAME; }

Layout layoutBoard(const Rect& body, int size, int cell) {
  Layout layout;
  layout.cell = cell;
  layout.size = size;
  const int x = body.x + (body.width - boardDrawnWidth(size, cell)) / 2;
  const int y = body.y + (body.height - boardDrawnHeight(size, cell)) / 2;
  layout.board = Rect{x + CLUE_LANE + CLUE_GAP, y + LANE + CLUE_GAP, size * cell, size * cell};
  return layout;
}

Rect cellRect(const Layout& layout, int row, int col) {
  return Rect{layout.board.x + col * layout.cell, layout.board.y + row * layout.cell, layout.cell, layout.cell};
}

// Short of the clue: a plain number. Exactly the clue: a solid chip with the number
// knocked out. Over it: a grey chip. The number stays readable in all three, so the
// player can recount a row that has the right count in the wrong places.
void drawClue(uint8_t* frame, const Rect& box, const Rect& chip, int value, int placed) {
  char text[12];
  snprintf(text, sizeof(text), "%d", value);
  bool black = true;
  if (placed == value) {
    fillRect(frame, chip.x, chip.y, chip.width, chip.height, true);
    black = false;
  } else if (placed > value) {
    fillLightGray(frame, chip);
  }
  drawTextInBox(frame, Font::Size30, box.x, box.y, box.width, box.height, text, black);
}

// Shared by the board and the guide, so the guide teaches on the surface the
// player is about to use.
void drawBoardSurface(uint8_t* frame, const dungeon::Board& board, const Layout& layout, const Spotlight& spotlight) {
  const dungeon::Puzzle& puzzle = board.puzzle();
  const int size = board.size();
  const int overhang = laneOverhang(layout.cell);

  for (int i = 0; i < size; i++) {
    const Rect colBox{layout.board.x + i * layout.cell + overhang, layout.board.y - CLUE_GAP - LANE, LANE, LANE};
    const Rect rowBox{layout.board.x - CLUE_GAP - LANE, layout.board.y + i * layout.cell + overhang, LANE, LANE};
    // A chip spans its cell along the lane, so neighbouring chips meet exactly.
    const Rect colChip{layout.board.x + i * layout.cell, colBox.y + 5, layout.cell, LANE - 10};
    const Rect rowChip{rowBox.x + 5, layout.board.y + i * layout.cell, LANE - 10, layout.cell};
    drawClue(frame, colBox, colChip, puzzle.colClues[i], board.colWalls(i));
    drawClue(frame, rowBox, rowChip, puzzle.rowClues[i], board.rowWalls(i));
  }

  // The frame is outside the play area and far heavier than anything inside it.
  drawRectOutline(frame, layout.board.x - ui::BOARD_FRAME, layout.board.y - ui::BOARD_FRAME,
                  layout.board.width + 2 * ui::BOARD_FRAME, layout.board.height + 2 * ui::BOARD_FRAME, ui::BOARD_FRAME);
  for (int i = 1; i < size; i++) {
    fillRect(frame, layout.board.x + i * layout.cell, layout.board.y, ui::HAIRLINE, layout.board.height, true);
    fillRect(frame, layout.board.x, layout.board.y + i * layout.cell, layout.board.width, ui::HAIRLINE, true);
  }

  const uint8_t* monster = dungeonArt::MONSTERS_36[monsterKind(board.index())];
  for (int row = 0; row < size; row++) {
    for (int col = 0; col < size; col++) {
      const Rect cell = cellRect(layout, row, col);
      const dungeon::Mark mark = board.mark(row, col);
      const bool isMonster = board.isMonster(row, col);
      const bool isChest = board.isChest(row, col);

      if (mark == dungeon::Mark::Wall) {
        fillRect(frame, cell.x, cell.y, cell.width, cell.height, true);
      } else if (mark == dungeon::Mark::Floor && !isMonster && !isChest) {
        constexpr int DOT = 10;
        fillRoundRect(frame, cell.x + (cell.width - DOT) / 2, cell.y + (cell.height - DOT) / 2, DOT, DOT, DOT / 2, true);
      }

      // A monster or chest always stands on floor, so it's always black on white.
      if (isMonster) drawArt(frame, cell, monster, ART_SIZE, true);
      if (isChest) drawArt(frame, cell, dungeonArt::CHEST_36, ART_SIZE, true);
    }
  }

  // After every cell, so no neighbour draws over a bracket.
  if (spotlight.lanes) {
    cornerBrackets(frame, Rect{layout.board.x + overhang - 4, layout.board.y - CLUE_GAP - LANE - 4,
                               layout.board.width - 2 * overhang + 8, LANE + 8});
    cornerBrackets(frame, Rect{layout.board.x - CLUE_GAP - CLUE_LANE - 4, layout.board.y + overhang - 4, CLUE_LANE + 8,
                               layout.board.height - 2 * overhang + 8});
  } else if (spotlight.width > 0) {
    // Inside the region, because outside it a bracket can land on the black frame.
    cornerBrackets(frame, Rect{layout.board.x + spotlight.col * layout.cell + 3, layout.board.y + spotlight.row * layout.cell + 3,
                               spotlight.width * layout.cell - 6, spotlight.height * layout.cell - 6});
  }
}

Rect actionRect(int i) { return Rect{BODY_LEFT + i * (ACTION_WIDTH + GUTTER), ACTIONS_TOP, ACTION_WIDTH, ui::PILL_HEIGHT}; }

void drawActions(uint8_t* frame, const char* left, const char* right) {
  ui::drawPill(frame, actionRect(0).x, ACTIONS_TOP, ACTION_WIDTH, left, true);
  ui::drawPill(frame, actionRect(1).x, ACTIONS_TOP, ACTION_WIDTH, right, true);
}

Button actionAt(int x, int y, Button left, Button right) {
  if (actionRect(0).contains(x, y)) return left;
  if (actionRect(1).contains(x, y)) return right;
  return Button::None;
}

// A dungeon on the campaign map: its own monster, in a room filled once cleared.
void drawMapCell(uint8_t* frame, const Rect& box, int index, bool done, bool current) {
  if (done) {
    fillRect(frame, box.x, box.y, box.width, box.height, true);
  } else {
    drawRectOutline(frame, box.x, box.y, box.width, box.height, ui::HAIRLINE);
  }
  // On a cleared room the monster is white, or it would vanish into the black.
  drawArt(frame, box, dungeonArt::MONSTERS_32[monsterKind(index)], MAP_ART, !done);
  if (current) cornerBrackets(frame, Rect{box.x - 5, box.y - 5, box.width + 10, box.height + 10}, 12);
}

Rect guideButton() { return Rect{BODY_LEFT, NEXT_TOP, BODY_WIDTH, NEXT_HEIGHT}; }

}  // namespace

bool Layout::cellAt(int x, int y, int& row, int& col) const {
  if (cell <= 0 || !board.contains(x, y)) return false;
  col = (x - board.x) / cell;
  row = (y - board.y) / cell;
  return row >= 0 && row < size && col >= 0 && col < size;
}

int PickerLayout::indexAt(int x, int y) const {
  if (cell <= 0 || x < grid.x || y < grid.y) return -1;
  const int pitch = cell + gap;
  const int col = (x - grid.x) / pitch;
  const int row = (y - grid.y) / pitch;
  if (col < 0 || col >= cols || row < 0 || row >= rows) return -1;
  return row * cols + col;
}

void drawBoard(uint8_t* frame, const BoardModel& model, Layout& layout) {
  const dungeon::Board& board = *model.board;
  // The game's name in the band and the dungeon's in a strip of its own, because
  // the band can't hold a 33-letter name.
  drawHeader(frame, TITLE, model.solvedCount, model.total);

  const char* name = board.puzzle().name;
  drawTextInBox(frame, fitName(name, BODY_WIDTH), BODY_LEFT, BODY_TOP, BODY_WIDTH, NAME_STRIP, name);

  layout = layoutBoard(Rect{BODY_LEFT, BOARD_BODY_TOP, BODY_WIDTH, ABOVE_ACTIONS - BOARD_BODY_TOP}, board.size(), CELL);
  drawBoardSurface(frame, board, layout, Spotlight{});
  drawActions(frame, "CLEAR", "DUNGEONS");
}

Button boardButtonAt(int x, int y) { return actionAt(x, y, Button::Reset, Button::Menu); }

// The front door. The campaign map is the menu: it's the biggest thing on the
// screen, it's the player's own record, and tapping it picks a dungeon.
void drawMenu(uint8_t* frame, const MenuModel& model, PickerLayout& layout) {
  layout = PickerLayout{};
  drawHeader(frame, TITLE, model.solvedCount, model.total);

  // The picked dungeon on top, its monster then its name, centred together, so the
  // name changes right above the map as the brackets move.
  const char* name = dungeon::kPuzzles[model.selectedIndex].name;
  const Font nameFont = fitName(name, BODY_WIDTH - MAP_ART - GUTTER);
  const int groupWidth = MAP_ART + GUTTER + textWidth(nameFont, name);
  const int groupLeft = BODY_LEFT + (BODY_WIDTH - groupWidth) / 2;
  drawIcon(frame, dungeonArt::MONSTERS_32[monsterKind(model.selectedIndex)], MAP_ART, MAP_ART, groupLeft,
           BODY_TOP + (NAME_STRIP - MAP_ART) / 2);
  ui::drawTextLeft(frame, nameFont, groupLeft + MAP_ART + GUTTER, BODY_TOP, NAME_STRIP, name);

  if (model.progress != nullptr) {
    const int height = MAP_BOTTOM - MAP_TOP;
    int cell = (BODY_WIDTH - MAP_LANE - 7 * MAP_GAP) / 8;
    if (cell > (height - 7 * MAP_GAP) / 8) cell = (height - 7 * MAP_GAP) / 8;
    const int extent = 8 * cell + 7 * MAP_GAP;
    const int left = BODY_LEFT + (BODY_WIDTH - MAP_LANE - extent) / 2;
    layout.grid = Rect{left + MAP_LANE, MAP_TOP + (height - extent) / 2, extent, extent};
    layout.cell = cell;
    layout.gap = MAP_GAP;
    layout.cols = 8;
    layout.rows = 8;

    for (int tier = 0; tier < 8; tier++) {
      const int rowY = layout.grid.y + tier * (cell + MAP_GAP);
      int done = 0;
      for (int slot = 0; slot < 8; slot++) {
        const int index = dungeon::kCampaignFirst + tier * 8 + slot;
        const bool cleared = model.progress->isSolved(index);
        if (cleared) done++;
        drawMapCell(frame, Rect{layout.grid.x + slot * (cell + MAP_GAP), rowY, cell, cell}, index, cleared,
                    model.selectedIndex == index);
      }
      // The tier's number, filled once all eight of its dungeons are cleared.
      char label[12];
      snprintf(label, sizeof(label), "%d", tier + 1);
      const bool tierDone = done == 8;
      if (tierDone) fillRect(frame, left, rowY, MAP_LANE - 6, cell, true);
      drawTextInBox(frame, Font::Size20, left, rowY, MAP_LANE - 6, cell, label, !tierDone);
    }
  }

  const Rect guide = guideButton();
  ui::drawPill(frame, guide.x, guide.y, guide.width, "TUTORIAL", false, guide.height);

  // PLAY takes the whole bar: it's the one thing this screen is for.
  ui::drawPill(frame, BODY_LEFT, ACTIONS_TOP, BODY_WIDTH, model.hasProgress ? "RESUME" : "PLAY", true);
}

Button menuButtonAt(int x, int y) {
  if (guideButton().contains(x, y)) return Button::Guide;
  if (Rect{BODY_LEFT, ACTIONS_TOP, BODY_WIDTH, ui::PILL_HEIGHT}.contains(x, y)) return Button::Play;
  return Button::None;
}

// One page of the guide: the tutorial board a step further on, with the rule under
// it, centred together so the page reads as a picture with its caption.
void drawGuide(uint8_t* frame, const GuideModel& model) {
  const int pages = guidePageCount();
  const int page = (model.page < 0 || model.page >= pages) ? 0 : model.page;
  const GuidePage& content = guidePage(page);
  drawHeader(frame, content.title, page + 1, pages);

  const dungeon::Board board = guideBoard(page);
  const int extent = boardDrawnHeight(board.size(), GUIDE_CELL);

  // Size20 if the caption fits four lines, as every caption is written to.
  Font captionFont = Font::Size20;
  if (wrappedHeight(captionFont, BODY_WIDTH, content.body) > GUIDE_CAPTION_LINES * lineHeight(captionFont)) {
    captionFont = Font::Size14;
  }
  const int captionHeight = wrappedHeight(captionFont, BODY_WIDTH, content.body);
  const int bodyHeight = ABOVE_ACTIONS - BODY_TOP;
  const int top = BODY_TOP + (bodyHeight - (extent + MARGIN + captionHeight)) / 2;

  const Layout layout = layoutBoard(Rect{BODY_LEFT, top, BODY_WIDTH, extent}, board.size(), GUIDE_CELL);
  drawBoardSurface(frame, board, layout, content.spotlight);
  drawWrapped(frame, captionFont, BODY_LEFT, top + extent + MARGIN, BODY_WIDTH, content.body, Align::Center);

  drawActions(frame, page == 0 ? "BACK" : "PREV", page == pages - 1 ? "PLAY" : "NEXT");
}

Button guideButtonAt(int x, int y) { return actionAt(x, y, Button::GuidePrevious, Button::GuideNext); }

// The payoff: the dungeon's name, and the map the player just finished carving.
void drawWin(uint8_t* frame, const WinModel& model) {
  drawHeader(frame, "CLEARED", model.solvedCount, model.total);

  const dungeon::Puzzle& puzzle = dungeon::kPuzzles[model.clearedIndex];
  const int bodyHeight = ABOVE_ACTIONS - BODY_TOP;

  // The largest cut the whole name fits in, on at most two lines.
  constexpr Font CUTS[] = {Font::Size30, Font::Size20, Font::Size14, Font::Size10};
  Font font = Font::Size10;
  for (const Font cut : CUTS) {
    if (wrappedHeight(cut, BODY_WIDTH, puzzle.name) <= 2 * lineHeight(cut)) {
      font = cut;
      break;
    }
  }
  drawWrapped(frame, font, BODY_LEFT, BODY_TOP + 40, BODY_WIDTH, puzzle.name, Align::Center);

  const int extent = puzzle.size * WIN_MAP_CELL;
  const int mapX = BODY_LEFT + (BODY_WIDTH - extent) / 2;
  const int mapY = BODY_TOP + bodyHeight / 2 - extent / 2 + 40;
  drawRectOutline(frame, mapX - ui::FRAME, mapY - ui::FRAME, extent + 2 * ui::FRAME, extent + 2 * ui::FRAME, ui::FRAME);
  for (int row = 0; row < puzzle.size; row++) {
    for (int col = 0; col < puzzle.size; col++) {
      const uint64_t bit = uint64_t{1} << (row * 8 + col);
      const int x = mapX + col * WIN_MAP_CELL;
      const int y = mapY + row * WIN_MAP_CELL;
      if ((puzzle.walls & bit) != 0) {
        fillRect(frame, x, y, WIN_MAP_CELL, WIN_MAP_CELL, true);
      } else if ((puzzle.monsters & bit) != 0 || (puzzle.chests & bit) != 0) {
        constexpr int DOT = 12;
        fillRoundRect(frame, x + (WIN_MAP_CELL - DOT) / 2, y + (WIN_MAP_CELL - DOT) / 2, DOT, DOT, DOT / 2, true);
      }
    }
  }

  if (model.moreToPlay) {
    ui::drawPill(frame, actionRect(0).x, ACTIONS_TOP, ACTION_WIDTH, "NEXT", true);
  } else {
    ui::drawDimmedPill(frame, actionRect(0).x, ACTIONS_TOP, ACTION_WIDTH, "NEXT");
  }
  ui::drawPill(frame, actionRect(1).x, ACTIONS_TOP, ACTION_WIDTH, "DUNGEONS", true);
}

Button winButtonAt(int x, int y, bool moreToPlay) {
  const Button button = actionAt(x, y, Button::Next, Button::Menu);
  return button == Button::Next && !moreToPlay ? Button::None : button;
}

}  // namespace dungeonui
