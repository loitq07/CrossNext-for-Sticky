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

  void checkNoTileOverlap() const {
    CHECK(!interactions.overflowed());
    std::vector<const fui::Interaction*> tiles;
    for (size_t i = 0; i < interactions.count(); ++i) {
      const auto& item = interactions.data()[i];
      if (item.action == quickui::Tile && item.value >= 0) tiles.push_back(&item);
    }
    CHECK(tiles.size() == 8);
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
  BoardConfig::homeKey = false;
  BoardConfig::buzzer = true;
  halTiltSensor.available = true;

  CHECK(!quickui::isShortcutSupported(quickui::Shortcut::Light));
  CHECK(!quickui::isShortcutSupported(quickui::Shortcut::Warm));
  CHECK(!quickui::isShortcutSupported(quickui::Shortcut::Lock));
  CHECK(quickui::isShortcutSupported(quickui::Shortcut::Wifi));
  CHECK(quickui::isShortcutSupported(quickui::Shortcut::Bluetooth));
  CHECK(quickui::isShortcutSupported(quickui::Shortcut::KeySound));
  CHECK(quickui::isShortcutSupported(quickui::Shortcut::Tilt));

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
  CHECK(row1[0] == uint8_t(quickui::Shortcut::Dark));
  CHECK(row1[1] == uint8_t(quickui::Shortcut::Refresh));
  CHECK(row1[2] == uint8_t(quickui::Shortcut::Rotate));
  CHECK(row1[3] == uint8_t(quickui::Shortcut::Wifi));
  CHECK(row2[0] == uint8_t(quickui::Shortcut::Bluetooth));
  CHECK(row2[1] == uint8_t(quickui::Shortcut::KeySound));
  CHECK(row2[2] == uint8_t(quickui::Shortcut::Tilt));
  CHECK(row2[3] == uint8_t(quickui::Shortcut::Sleep));

  uint32_t seen = 0;
  for (uint8_t s : row1) seen |= (1u << s);
  for (uint8_t s : row2) seen |= (1u << s);
  CHECK(__builtin_popcount(seen) == 8);
}

void testTwoRowPanelAndDirtyRegion() {
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
        m.shortcuts[0] = uint8_t(quickui::Shortcut::Dark);
        m.shortcuts[1] = uint8_t(quickui::Shortcut::Refresh);
        m.shortcuts[2] = uint8_t(quickui::Shortcut::Rotate);
        m.shortcuts[3] = uint8_t(quickui::Shortcut::Wifi);
        m.extraShortcuts[0] = uint8_t(quickui::Shortcut::Bluetooth);
        m.extraShortcuts[1] = uint8_t(quickui::Shortcut::KeySound);
        m.extraShortcuts[2] = uint8_t(quickui::Shortcut::Tilt);
        m.extraShortcuts[3] = uint8_t(quickui::Shortcut::Sleep);
        m.extraCount = 4;

        Canvas c(shape.first, shape.second);
        const auto lay = quickui::layout(c.device, m.light, m.temperature, m.touch,
                                         quickui::hasBluetoothStatus(m.bluetooth), m.extraCount > 0);
        CHECK(lay.bottom <= shape.second);
        CHECK(lay.tiles.bottom() <= lay.bottom);

        quickui::Work work;
        quickui::build(c.screen, m, work);
        c.checkNoTileOverlap();
        CHECK(c.findTile(quickui::Shortcut::Wifi) != nullptr);
        CHECK(c.findTile(quickui::Shortcut::Bluetooth) != nullptr);
        CHECK(c.findTile(quickui::Shortcut::KeySound) != nullptr);
        CHECK(c.findTile(quickui::Shortcut::Tilt) != nullptr);
        CHECK(c.findTile(quickui::Shortcut::Sleep) != nullptr);

        for (int toggle = 0; toggle < 3; ++toggle) {
          quickui::Model next = m;
          if (toggle == 0) next.keySoundOn = true;
          if (toggle == 1) next.tiltOn = true;
          if (toggle == 2) next.wifiActive = true;
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

        quickui::Model oneRow = m;
        oneRow.extraCount = 0;
        const auto fullDirty = quickui::dirtyRegion(m, oneRow, lay, shape.first);
        CHECK(fullDirty.y == 0 && fullDirty.bottom() == lay.bottom);
      }
    }
  }
}
}  // namespace

int main() {
  testResolveStickyDefaults();
  testTwoRowPanelAndDirtyRegion();
  std::printf("quick-settings-panel: %d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
