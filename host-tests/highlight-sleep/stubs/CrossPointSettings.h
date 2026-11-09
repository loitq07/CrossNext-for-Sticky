#pragma once
#include <cstdint>
struct CrossPointSettings {
  enum { DARK = 0, HIGHLIGHTS = 10 };
  uint8_t sleepScreen = HIGHLIGHTS;
  uint8_t sleepHighlightMinutes = 5;
};
inline CrossPointSettings SETTINGS;
