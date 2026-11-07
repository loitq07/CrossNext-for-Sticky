#pragma once

#include <FreeInkUI.h>

#include "../../components/HeaderBackTapTarget.h"

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

inline void paint(fui::DrawTarget& target, fui::Rect box) {
  const int x = box.x + 20;
  const int y = box.y + (box.height > 0 ? (box.height - 1) / 2 : 35);
  const auto ink = fui::Paint::solid(fui::Color::White);
  target.line({int16_t(x + 16), int16_t(y - 14)}, {int16_t(x), int16_t(y)}, 4, ink);
  target.line({int16_t(x), int16_t(y)}, {int16_t(x + 16), int16_t(y + 14)}, 4, ink);
  target.line({int16_t(x), int16_t(y)}, {int16_t(x + 28), int16_t(y)}, 4, ink);
}

// The input manager folds this published region into logical Button::Back.
inline void publish(bool enabled, fui::Rect box) {
  if (enabled)
    HeaderBackTapTarget::set(box.x, box.y, box.width, box.height);
  else
    HeaderBackTapTarget::clear();
}
}  // namespace backui
