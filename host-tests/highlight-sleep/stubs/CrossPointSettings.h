#pragma once
#include <cstdint>
#include "fontIds.h"
struct CrossPointSettings {
  enum { DARK = 0, HIGHLIGHTS = 10 };
  enum { HIGHLIGHT_ALIGN_LEFT = 0, HIGHLIGHT_ALIGN_CENTER = 1, HIGHLIGHT_ALIGN_RIGHT = 2 };
  uint8_t sleepScreen = HIGHLIGHTS;
  uint8_t sleepHighlightMinutes = 5;
  uint8_t sleepHighlightAlign = HIGHLIGHT_ALIGN_LEFT;
  char sleepHighlightFontFamily[32] = "";
  int getHighlightFontId() const { return NOTOSERIF_14_FONT_ID; }
};
inline CrossPointSettings SETTINGS;
