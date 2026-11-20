#pragma once

// How much of the battery icon to fill, kept free of drawing so tests/home/ can check
// it on the PC.
namespace ui {

// Rounds up, so a battery with any charge left never draws as empty. A percent below
// 0 means unknown, which draws empty.
constexpr int batteryFillWidth(int percent, int maxWidth) {
  if (percent <= 0) return 0;
  if (percent >= 100) return maxWidth;
  return (percent * maxWidth + 99) / 100;
}

}  // namespace ui
