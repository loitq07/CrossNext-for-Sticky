#pragma once
#include "fontIds.h"
#include "../apps_local/ui/ToyboxFonts.h"

// FreeInkUI font slots. Row heights, header height, and touch sizes are not
// chosen here: FreeInkApp derives all metric tokens from the body font's line
// height (themeTokensForLineHeight). One fixed tier for every board — the
// user-facing UI-scale setting was removed.
struct UIScaleSpec {
  int smallFontId;
  int bodyFontId;
  int titleFontId;
};

inline UIScaleSpec uiScaleSpec() {
  UIScaleSpec spec{};
  spec.smallFontId = toybox::kButtonFontId;
  spec.bodyFontId = toybox::kUiFontId;
  // All three aliases follow the active interface font selection.
  spec.titleFontId = toybox::kDisplayFontId;
  return spec;
}
