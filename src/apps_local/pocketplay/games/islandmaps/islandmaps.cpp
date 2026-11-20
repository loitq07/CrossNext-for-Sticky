#include "islandmaps.h"

#include <Arduino.h>
#include <esp_random.h>

#include <cstdio>

#include "core/graphics.h"
#include "core/storage.h"
#include "core/text.h"
#include "islandmaps_icon.h"
#include "islandmaps_maps.h"
#include "islandmaps_rules.h"
#include "islandmaps_save.h"
#include "ui/ui.h"

namespace im = islandmaps;

namespace {

constexpr const char* SAVE_FILE = "islandmaps";

// From the top: the island's name and its place in the list, below where the shell puts a warning,
// then the island as large as it fits, then FIRST MAP and RANDOM side by side.
constexpr int MARGIN = 16;
constexpr int INNER_WIDTH = SCREEN_WIDTH - 2 * MARGIN;
constexpr int BUTTON_GAP = 12;
constexpr int BUTTON_WIDTH = (INNER_WIDTH - BUTTON_GAP) / 2;
constexpr int FIRST_LEFT = MARGIN;
constexpr int RANDOM_LEFT = SCREEN_WIDTH - MARGIN - BUTTON_WIDTH;
constexpr int NAME_TOP = ui::CONTENT_TOP;
constexpr int NAME_HEIGHT = 48;
constexpr int POSITION_TOP = NAME_TOP + NAME_HEIGHT + 4;
constexpr int POSITION_HEIGHT = 24;
constexpr int MAP_TOP = POSITION_TOP + POSITION_HEIGHT + 12;
constexpr int BUTTON_TOP = SCREEN_HEIGHT - MARGIN - ui::PILL_HEIGHT;
constexpr int MAP_BOTTOM = BUTTON_TOP - 12;
constexpr int MAP_HEIGHT = MAP_BOTTOM - MAP_TOP;
constexpr int MAX_CELL = 64;
constexpr int TILE_GAP = 4;  // white between tiles, as the PDF's grid lines

static_assert(INNER_WIDTH / im::kMaxWidth >= 32 && MAP_HEIGHT / im::kMaxHeight >= 32,
              "the largest island must still have tiles big enough to read");

class IslandMaps final : public Game {
 public:
  const char* name() const override { return "ISLAND MAPS"; }
  const uint8_t* icon() const override { return islandmapsIcon::ICON; }

  void open() override {
    if (loaded_) return;
    loaded_ = true;
    load();
  }

  TapResult onTap(int x, int y) override {
    if (ui::isOnBack(x, y)) return TapResult::Leave;
    // On the first island already, FIRST MAP changes nothing, so show() returns Nothing.
    if (ui::isInBox(x, y, FIRST_LEFT, BUTTON_TOP, BUTTON_WIDTH, ui::PILL_HEIGHT)) return show(0);
    if (ui::isInBox(x, y, RANDOM_LEFT, BUTTON_TOP, BUTTON_WIDTH, ui::PILL_HEIGHT)) {
      return show(im::pickOther(index_, im::kIslandCount, [] { return static_cast<uint32_t>(esp_random()); }));
    }
    return TapResult::Nothing;
  }

  TapResult onUpPressed() override { return show(im::step(index_, -1, im::kIslandCount)); }
  TapResult onDownPressed() override { return show(im::step(index_, 1, im::kIslandCount)); }

  void draw(uint8_t* frame) override {
    const im::Island& island = im::kIslands[index_];
    char position[16];
    std::snprintf(position, sizeof(position), "%d/%d", index_ + 1, im::kIslandCount);
    ui::drawBand(frame, name(), nullptr, true);
    // A name too long for the line, such as ISLAND OF THE EVIL EYE, drops a size rather than a word.
    const Font font = textWidth(Font::Size30, island.name) <= INNER_WIDTH ? Font::Size30 : Font::Size20;
    drawTextInBox(frame, font, MARGIN, NAME_TOP, INNER_WIDTH, NAME_HEIGHT, island.name);
    drawTextInBox(frame, Font::Size14, MARGIN, POSITION_TOP, INNER_WIDTH, POSITION_HEIGHT, position);
    drawIsland(frame, island);
    ui::drawPill(frame, FIRST_LEFT, BUTTON_TOP, BUTTON_WIDTH, "FIRST MAP", false);
    ui::drawPill(frame, RANDOM_LEFT, BUTTON_TOP, BUTTON_WIDTH, "RANDOM", true);
  }

  bool saveFailed() const override { return saveFailed_; }
  void retrySave() override { persist(); }

 private:
  TapResult show(int index) {
    if (index == index_) return TapResult::Nothing;
    index_ = index;
    persist();
    return TapResult::Redraw;
  }

  // Square tiles as large as the island allows, capped so a small island doesn't fill the screen,
  // and centred in the space between the name and the buttons.
  static void drawIsland(uint8_t* frame, const im::Island& island) {
    int cell = INNER_WIDTH / island.width;
    if (MAP_HEIGHT / island.height < cell) cell = MAP_HEIGHT / island.height;
    if (cell > MAX_CELL) cell = MAX_CELL;
    const int width = island.width * cell - TILE_GAP;
    const int height = island.height * cell - TILE_GAP;
    const int left = MARGIN + (INNER_WIDTH - width) / 2;
    const int top = MAP_TOP + (MAP_HEIGHT - height) / 2;
    for (int row = 0; row < island.height; row++) {
      for (int column = 0; column < island.width; column++) {
        if (!im::isLand(island, column, row)) continue;
        fillRect(frame, left + column * cell, top + row * cell, cell - TILE_GAP, cell - TILE_GAP, true);
      }
    }
  }

  void load() {
    if (!storage::ready()) return;
    uint8_t data[im::SAVE_BYTES];
    uint8_t version = 0;
    size_t size = 0;
    if (!storage::load(SAVE_FILE, version, data, sizeof(data), size)) return;
    if (!im::unpack(version, data, size, index_)) Serial.println("Ignoring saved Island Maps place");
  }

  void persist() {
    if (!storage::ready()) {
      saveFailed_ = false;
      return;
    }
    uint8_t data[im::SAVE_BYTES];
    const size_t size = im::pack(index_, data);
    saveFailed_ = !storage::save(SAVE_FILE, im::SAVE_VERSION, data, size);
  }

  int index_ = 0;
  bool loaded_ = false;
  bool saveFailed_ = false;
};

}  // namespace

Game& islandMapsGame() {
  static IslandMaps game;
  return game;
}
