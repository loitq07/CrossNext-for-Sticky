#pragma once

#include <FreeInkUI.h>

#include "../../components/HeaderBackTapTarget.h"

namespace backui {
namespace fui = freeink::ui;
constexpr int kWidth = 76;
constexpr int kSize = 44;
constexpr int kReserve = 76;

inline fui::Rect rect(const fui::DeviceContext& device, fui::Rect band) {
  const auto safe = device.safeRect();
  const int top = band.y > safe.y ? band.y : safe.y;
  const int height = band.bottom() - top;
  return fui::makeRect(safe.x, top, kWidth, height);
}

inline void paint(fui::DrawTarget& target, fui::Rect box) {
  const int x = box.x + 12, y = box.y + box.height / 2;
  const auto ink = fui::Paint::solid(fui::Color::White);
  target.line({int16_t(x + 18), int16_t(y - 16)}, {int16_t(x), int16_t(y)}, 4, ink);
  target.line({int16_t(x), int16_t(y)}, {int16_t(x + 18), int16_t(y + 16)}, 4, ink);
  target.line({int16_t(x), int16_t(y)}, {int16_t(x + 30), int16_t(y)}, 4, ink);
}

// The input manager folds this published region into logical Button::Back.
inline void publish(bool enabled, fui::Rect box) {
  if (enabled)
    HeaderBackTapTarget::set(box.x, box.y, box.width, box.height);
  else
    HeaderBackTapTarget::clear();
}
}  // namespace backui
