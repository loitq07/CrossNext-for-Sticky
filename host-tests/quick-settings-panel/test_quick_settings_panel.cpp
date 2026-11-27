#include <BoardConfig.h>
#include <FreeInkUIDisplayTarget.h>
#include <HalFrontlight.h>
#include <HalTiltSensor.h>
#include <I18n.h>

#include <array>
#include <cstdio>
#include <cstring>
#include <vector>

#include "../../src/apps_local/ui/PixelMenu.h"
#include "../../src/apps_local/ui/QuickActionSupport.h"
#include "../../src/apps_local/ui/QuickMenuCore.h"
#include "../../src/apps_local/ui/QuickSettingsDirtyRegion.h"
#include "../../src/apps_local/ui/QuickSettingsScreen.h"
#include "../../src/apps_local/ui/ToyboxScreen.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "../../src/apps_local/ui/fonts/toybox_10.h"
#include "../../src/apps_local/ui/fonts/toybox_14.h"
#include "../../src/apps_local/ui/fonts/toybox_20.h"
#include "../../src/apps_local/ui/fonts/toybox_30.h"
#pragma GCC diagnostic pop

namespace fui = freeink::ui;

namespace {
int checks = 0;
int failures = 0;

#define CHECK(expr)                                                          \
  do {                                                                       \
    ++checks;                                                                \
    if (!(expr)) {                                                           \
      ++failures;                                                            \
      std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #expr);           \
    }                                                                        \
  } while (false)

struct FontBridge {
  std::array<fui::FontGlyph, 224> glyphs{};
  fui::BitmapFont font{};
  explicit FontBridge(const EpdFontData& source) {
    for (uint32_t cp = 32; cp <= 255; ++cp) {
      for (uint32_t i = 0; i < source.intervalCount; ++i) {
        const auto& interval = source.intervals[i];
        if (cp < interval.first || cp > interval.last) continue;
        const auto& g = source.glyph[interval.offset + cp - interval.first];
        glyphs[cp - 32] = {static_cast<uint16_t>(g.dataOffset),
                           g.width,
                           g.height,
                           static_cast<uint8_t>(fp4::toPixel(g.advanceX)),
                           static_cast<int8_t>(g.left),
                           static_cast<int8_t>(-g.top)};
        break;
      }
    }
    font = {source.bitmap, glyphs.data(), 32, 255, source.advanceY, static_cast<uint8_t>(source.ascender), 64, 64, 1};
  }
};

struct Canvas {
  std::vector<uint8_t> pixels;
  FontBridge tiny{toybox_10}, small{toybox_14}, body{toybox_20}, title{toybox_30};
  fui::DisplayTarget target;
  fui::DeviceContext device;
  fui::InputSnapshot input{};
  toybox::Interactions interactions;
  toybox::Frame frame;
  toybox::Screen screen;

  Canvas(int w, int h)
      : pixels(size_t((w + 7) / 8) * h, 255),
        target(pixels.data(), w, h, (w + 7) / 8, fui::Orientation::LandscapeCounterClockwise),
        device(target.deviceContext()),
        frame(target, device, input, interactions),
        screen(frame) {
    device.width = int16_t(w);
    device.height = int16_t(h);
    device.hasTouch = true;
    target.setFont(0, small.font);
    target.setFont(3, tiny.font);
    target.setFont(fui::HEADER_CLOCK_FONT, tiny.font);
    target.setFont(toybox::kSmallFont, small.font);
    target.setFont(toybox::kUiFont, body.font);
    target.setFont(toybox::kDisplayFont, title.font);
  }

  const fui::Interaction* findTile(quickui::Shortcut s) const {
    for (size_t i = 0; i < interactions.count(); ++i) {
      const auto& item = interactions.data()[i];
      if (item.action == quickui::Tile && item.value == int16_t(s)) return &item;
    }
    return nullptr;
  }

  void checkNoTileOverlap(size_t expectedTiles) const {
    CHECK(!interactions.overflowed());
    std::vector<const fui::Interaction*> tiles;
    for (size_t i = 0; i < interactions.count(); ++i) {
      const auto& item = interactions.data()[i];
      if (item.action == quickui::Tile && item.value >= 0) tiles.push_back(&item);
    }
    CHECK(tiles.size() == expectedTiles);
    for (size_t i = 0; i < tiles.size(); ++i) {
      CHECK(tiles[i]->rect.width > 0 && tiles[i]->rect.height > 0);
      CHECK((tiles[i]->inputMask & fui::InputLongPress) != 0);
      for (size_t j = i + 1; j < tiles.size(); ++j) {
        const auto ix = quickui::intersect(tiles[i]->rect, tiles[j]->rect);
        CHECK(ix.empty());
      }
    }
  }
};

void testResolveStickyDefaults() {
  Frontlight.presentFlag = false;
  Frontlight.tempFlag = false;
  BoardConfig::sticky = true;
  BoardConfig::homeKey = false;
  BoardConfig::buzzer = true;
  halTiltSensor.available = true;  // Even if IMU probe passes, Sticky excludes Tilt.

  CHECK(!quickui::isShortcutSupported(quickui::Shortcut::Light));
  CHECK(!quickui::isShortcutSupported(quickui::Shortcut::Warm));
  CHECK(!quickui::isShortcutSupported(quickui::Shortcut::Lock));
  CHECK(!quickui::isShortcutSupported(quickui::Shortcut::Tilt));
  CHECK(quickui::isShortcutSupported(quickui::Shortcut::Wifi));
  CHECK(quickui::isShortcutSupported(quickui::Shortcut::Bluetooth));
  CHECK(quickui::isShortcutSupported(quickui::Shortcut::KeySound));
  CHECK(quickui::isShortcutSupported(quickui::Shortcut::Dark));
  CHECK(quickui::isShortcutSupported(quickui::Shortcut::Refresh));
  CHECK(quickui::isShortcutSupported(quickui::Shortcut::Rotate));
  CHECK(quickui::isShortcutSupported(quickui::Shortcut::Sleep));
  CHECK(quickui::isShortcutSupported(quickui::Shortcut::Files));
  CHECK(quickui::isShortcutSupported(quickui::Shortcut::Library));
  CHECK(quickui::isShortcutSupported(quickui::Shortcut::ReadLater));
  CHECK(quickui::isShortcutSupported(quickui::Shortcut::Transfer));
  CHECK(quickui::isShortcutSupported(quickui::Shortcut::Stats));
  CHECK(quickui::isShortcutSupported(quickui::Shortcut::Games));
  CHECK(quickui::isShortcutSupported(quickui::Shortcut::Apps));
  CHECK(quickui::isShortcutSupported(quickui::Shortcut::Services));
  CHECK(quickui::isShortcutSupported(quickui::Shortcut::Settings));
  CHECK(quickui::isShortcutSupported(quickui::Shortcut::AppStore));

  // Verify Bluetooth and Screenshot labels do not regress to "BluetoothOff" / "TAKE SCREENSHOT".
  I18N.setLanguage(Language::EN);
  CHECK(std::strcmp(quickui::label(quickui::Shortcut::Bluetooth), tr(STR_BT_SHORT_NAME)) == 0);
  CHECK(std::strstr(quickui::label(quickui::Shortcut::Bluetooth), "Off") == nullptr);
  CHECK(std::strcmp(quickui::label(quickui::Shortcut::Screenshot), "Screenshot") == 0);
  CHECK(std::strcmp(quickui::label(quickui::Shortcut::Stats), tr(STR_READING_STATS_TITLE)) == 0);
  CHECK(std::strcmp(quickui::label(quickui::Shortcut::Transfer), tr(STR_FILE_TRANSFER)) == 0);
  CHECK(std::strcmp(quickui::label(quickui::Shortcut::ReadLater), tr(STR_READ_LATER)) == 0);
  CHECK(std::strcmp(quickui::label(quickui::Shortcut::Services, true, true, true), tr(STR_PLUGINS)) == 0);
  CHECK(std::strcmp(quickui::label(quickui::Shortcut::Services, true, true, false), tr(STR_OPDS_BROWSER)) == 0);

  // Verify Night Mode uses Black/White Contrast icon (synced with Settings), while Sleep keeps Moon.
  CHECK(fui::bitmapFromIcon(quickui::icon(quickui::Shortcut::Dark)).data == settingsicons::ContrastBits);
  CHECK(fui::bitmapFromIcon(quickui::icon(quickui::Shortcut::Sleep)).data == settingsicons::QuickMoonBits);
  CHECK(fui::bitmapFromIcon(quickui::icon(quickui::Shortcut::Files)).data == settingsrowart::FolderBits);
  CHECK(fui::bitmapFromIcon(quickui::icon(quickui::Shortcut::Settings)).data == settingsrowart::GearBits);

  const uint8_t defaults[4] = {
      uint8_t(quickui::Shortcut::Dark),
      uint8_t(quickui::Shortcut::Refresh),
      uint8_t(quickui::Shortcut::Rotate),
      uint8_t(quickui::Shortcut::Light),
  };
  uint8_t row1[4]{};
  uint8_t row2[4]{};
  const int count = quickui::resolveQuickShortcuts(defaults, row1, row2, 4);
  CHECK(count == 4);
  CHECK(row1[0] == uint8_t(quickui::Shortcut::Wifi));
  CHECK(row1[1] == uint8_t(quickui::Shortcut::Bluetooth));
  CHECK(row1[2] == uint8_t(quickui::Shortcut::KeySound));
  CHECK(row1[3] == uint8_t(quickui::Shortcut::Dark));
  CHECK(row2[0] == uint8_t(quickui::Shortcut::Refresh));
  CHECK(row2[1] == uint8_t(quickui::Shortcut::Screenshot));
  CHECK(row2[2] == uint8_t(quickui::Shortcut::Rotate));
  CHECK(row2[3] == uint8_t(quickui::Shortcut::Sleep));

  uint32_t seen = 0;
  for (uint8_t s : row1) seen |= (1u << s);
  for (uint8_t s : row2) seen |= (1u << s);
  CHECK(__builtin_popcount(seen) == 8);
}

void testQuickMenuCoreAndEncode() {
  Frontlight.presentFlag = false;
  Frontlight.tempFlag = false;
  BoardConfig::sticky = true;
  BoardConfig::homeKey = false;
  BoardConfig::buzzer = true;
  halTiltSensor.available = true;

  const uint32_t mask = quickui::supportedShortcutMask();
  const uint8_t legacy[4] = {0, 1, 2, 3};
  quickmenu::Config cfg = quickmenu::defaultConfig(false, legacy, mask);
  quickui::Shortcut vis[quickmenu::kMaxVisible]{};
  CHECK(quickmenu::collectVisible(cfg, mask, vis, quickmenu::kMaxVisible) == 8);
  CHECK(vis[0] == quickui::Shortcut::Wifi);
  CHECK(vis[1] == quickui::Shortcut::Bluetooth);
  CHECK(vis[2] == quickui::Shortcut::KeySound);
  CHECK(vis[3] == quickui::Shortcut::Dark);
  CHECK(vis[4] == quickui::Shortcut::Refresh);
  CHECK(vis[5] == quickui::Shortcut::Screenshot);
  CHECK(vis[6] == quickui::Shortcut::Rotate);
  CHECK(vis[7] == quickui::Shortcut::Sleep);

  // Add 4 Home/launcher items up to max 12 (3 rows x 4 columns).
  CHECK(quickmenu::setVisible(cfg, quickui::Shortcut::Files, true, mask));
  CHECK(quickmenu::setVisible(cfg, quickui::Shortcut::Library, true, mask));
  CHECK(quickmenu::setVisible(cfg, quickui::Shortcut::Transfer, true, mask));
  CHECK(quickmenu::setVisible(cfg, quickui::Shortcut::Settings, true, mask));
  CHECK(quickmenu::visibleCount(cfg, mask) == 12);

  // Reject a 13th visible item.
  CHECK(!quickmenu::setVisible(cfg, quickui::Shortcut::Stats, true, mask));
  CHECK(quickmenu::visibleCount(cfg, mask) == 12);

  // Hide one, add a Shelf app shortcut, and move it upward across rows.
  CHECK(quickmenu::setVisible(cfg, quickui::Shortcut::Sleep, false, mask));
  CHECK(quickmenu::visibleCount(cfg, mask) == 11);
  const quickui::Shortcut app0 = quickmenu::addAppShortcut(cfg, 1, "CHESS", mask);
  CHECK(app0 == quickui::Shortcut::AppShortcut0);
  CHECK(quickmenu::visibleCount(cfg, mask) == 12);
  CHECK(quickmenu::findAppShortcut(cfg, 1, "CHESS") == app0);
  const int beforeIdx = quickmenu::indexOf(cfg, app0, mask);
  CHECK(beforeIdx == 11);
  CHECK(quickmenu::move(cfg, app0, -1, mask));
  CHECK(quickmenu::indexOf(cfg, app0, mask) == 10);

  // Binary encode/decode roundtrip + tamper detection.
  uint8_t bytes[quickmenu::kSaveBytes]{};
  quickmenu::encode(cfg, 7, bytes);
  quickmenu::Config decoded{};
  uint32_t gen = 0;
  CHECK(quickmenu::decode(bytes, decoded, gen));
  CHECK(gen == 7);
  CHECK(quickmenu::visibleCount(decoded, mask) == 12);
  CHECK(quickmenu::at(decoded, 10, mask) == app0);
  CHECK(std::strcmp(decoded.shortcuts[0].title, "CHESS") == 0);
  bytes[20] ^= 0x5Au;
  CHECK(!quickmenu::decode(bytes, decoded, gen));

  // Removing app shortcut drops count to 11; never allow 0 visible items.
  quickmenu::removeAppShortcut(decoded, app0);
  CHECK(quickmenu::visibleCount(decoded, mask) == 11);
  for (int i = 0; i < quickmenu::kCount; ++i) {
    quickmenu::setVisible(decoded, static_cast<quickui::Shortcut>(i), false, mask);
  }
  CHECK(quickmenu::visibleCount(decoded, mask) == 1);
}

void testThreeRowPanelAndDirtyRegion() {
  I18N.setLanguage(Language::EN);
  for (bool lightHeader : {false, true}) {
    chrome::lightHeader = lightHeader;
    for (auto shape : {std::pair<int, int>{480, 800}, {800, 480}}) {
      for (auto bt : {bluetoothui::State::Off, bluetoothui::State::Connected}) {
        quickui::Model m;
        m.light = false;
        m.temperature = false;
        m.touch = true;
        m.shadeBackdrop = false;
        m.bluetooth = bt;
        m.shortcuts[0] = uint8_t(quickui::Shortcut::Wifi);
        m.shortcuts[1] = uint8_t(quickui::Shortcut::Bluetooth);
        m.shortcuts[2] = uint8_t(quickui::Shortcut::KeySound);
        m.shortcuts[3] = uint8_t(quickui::Shortcut::Dark);
        m.row1Count = 4;
        m.extraShortcuts[0] = uint8_t(quickui::Shortcut::Refresh);
        m.extraShortcuts[1] = uint8_t(quickui::Shortcut::Screenshot);
        m.extraShortcuts[2] = uint8_t(quickui::Shortcut::Rotate);
        m.extraShortcuts[3] = uint8_t(quickui::Shortcut::Sleep);
        m.extraShortcuts[4] = uint8_t(quickui::Shortcut::Files);
        m.extraShortcuts[5] = uint8_t(quickui::Shortcut::Library);
        m.extraShortcuts[6] = uint8_t(quickui::Shortcut::Transfer);
        m.extraShortcuts[7] = uint8_t(quickui::Shortcut::AppShortcut0);
        m.extraCount = 8;
        m.appShortcuts[0].label = "CHESS";

        Canvas c(shape.first, shape.second);
        const auto lay = quickui::layoutForRows(c.device, m.light, m.temperature, m.touch,
                                                quickui::hasBluetoothStatus(m.bluetooth), m.tileRows());
        CHECK(m.tileRows() == 3);
        CHECK(lay.bottom <= shape.second);
        CHECK(lay.tiles.bottom() <= lay.bottom);

        quickui::Work work;
        quickui::build(c.screen, m, work);
        c.checkNoTileOverlap(12);
        CHECK(c.findTile(quickui::Shortcut::Wifi) != nullptr);
        CHECK(c.findTile(quickui::Shortcut::Bluetooth) != nullptr);
        CHECK(c.findTile(quickui::Shortcut::KeySound) != nullptr);
        CHECK(c.findTile(quickui::Shortcut::Dark) != nullptr);
        CHECK(c.findTile(quickui::Shortcut::Refresh) != nullptr);
        CHECK(c.findTile(quickui::Shortcut::Screenshot) != nullptr);
        CHECK(c.findTile(quickui::Shortcut::Rotate) != nullptr);
        CHECK(c.findTile(quickui::Shortcut::Sleep) != nullptr);
        CHECK(c.findTile(quickui::Shortcut::Files) != nullptr);
        CHECK(c.findTile(quickui::Shortcut::Library) != nullptr);
        CHECK(c.findTile(quickui::Shortcut::Transfer) != nullptr);
        CHECK(c.findTile(quickui::Shortcut::AppShortcut0) != nullptr);
        CHECK(c.findTile(quickui::Shortcut::Tilt) == nullptr);

        for (int toggle = 0; toggle < 2; ++toggle) {
          quickui::Model next = m;
          if (toggle == 0) next.keySoundOn = true;
          if (toggle == 1) next.wifiActive = true;
          const auto dirty = quickui::dirtyRegion(m, next, lay, shape.first);
          CHECK(dirty.x == lay.tiles.x && dirty.y == lay.tiles.y &&
                dirty.width == lay.tiles.width && dirty.height == lay.tiles.height);

          Canvas full(shape.first, shape.second);
          quickui::build(full.screen, next, work);

          Canvas partial(shape.first, shape.second);
          partial.pixels = c.pixels;
          partial.target.setClipRect(dirty);
          quickui::build(partial.screen, next, work);
          partial.target.setClipRect(fui::makeRect(0, 0, shape.first, shape.second));

          CHECK(partial.pixels == full.pixels);
          CHECK(partial.pixels != c.pixels);
        }

        quickui::Model twoRows = m;
        twoRows.extraCount = 4;
        const auto fullDirty = quickui::dirtyRegion(m, twoRows, lay, shape.first);
        CHECK(fullDirty.y == 0 && fullDirty.bottom() == lay.bottom);
      }
    }
  }
}
}  // namespace

int main() {
  testResolveStickyDefaults();
  testQuickMenuCoreAndEncode();
  testThreeRowPanelAndDirtyRegion();
  std::printf("quick-settings-panel: %d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
