#pragma once

#include <FreeInkUI.h>

#include <algorithm>

#include "../../../lib/GfxRenderer/ChromeRegion.h"
#include "../../components/HeaderBackTapTarget.h"
#include "UnifiedDesignTokens.h"

namespace backui {
namespace fui = freeink::ui;
constexpr int kWidth = 72;
constexpr int kSize = 44;
constexpr int kReserve = 72;

inline fui::Rect rect(const fui::DeviceContext& device, fui::Rect band) {
  const auto safe = device.safeRect();
  const int top = band.y > safe.y ? band.y : safe.y;
  const int height = band.bottom() - top;
  return fui::makeRect(safe.x, top, kWidth, height);
}

inline void paint(fui::DrawTarget& target, fui::Rect box, bool light = chrome::lightHeader) {
  const int x = box.x + 20;
  const int top = box.height >= 72 ? box.bottom() - 72 : box.y;
  const int y = top + (box.height >= 72 ? 36 : (box.height > 0 ? box.height / 2 : 36));
  const auto ink = fui::Paint::solid(light ? fui::Color::Black : fui::Color::White);
  target.line({int16_t(x + 16), int16_t(y - 14)}, {int16_t(x), int16_t(y)}, 4, ink);
  target.line({int16_t(x), int16_t(y)}, {int16_t(x + 16), int16_t(y + 14)}, 4, ink);
  target.line({int16_t(x), int16_t(y)}, {int16_t(x + 28), int16_t(y)}, 4, ink);
}

// The input manager folds this published region into logical Button::Back.
inline void publish(bool enabled, fui::Rect box) {
  if (enabled) {
    const int w = std::max(int(box.width), int(cds::kTouchTargetMin));
    const int h = std::max(int(box.height), int(cds::kTouchTargetMin));
    HeaderBackTapTarget::set(box.x, box.y, w, h);
  } else {
    HeaderBackTapTarget::clear();
  }
}
}  // namespace backui
