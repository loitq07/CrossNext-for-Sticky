#pragma once

// Unified Design Tokens (CrossDesign System - CDS)
//
// Single Source of Truth (SSoT) for layout metrics, component dimensions,
// stroke weights, corner radii, typography slots, e-ink dithering patterns,
// and control styles across CrossNext (Xteink X4 Pro and Seeed Sticky).
//
// Freestanding: depends only on FreeInkUI core types and plain constants,
// enabling compilation in both host-tests and target firmware builds.

#include <FreeInkUI.h>

#include <algorithm>
#include <cstdint>

#include "../../../lib/GfxRenderer/ChromeRegion.h"

namespace cds {

namespace fui = freeink::ui;

// ============================================================================
// 1. SPACING & GRID TOKENS (4px / 8px Baseline Grid)
// ============================================================================

constexpr int16_t kSpace2   = 2;   // Micro-spacing between adjacent line elements
constexpr int16_t kSpaceXs  = 4;   // Extra-small spacing (badge padding, icon-text gap)
constexpr int16_t kSpaceSm  = 8;   // Small spacing (stepper gaps, compact button pad)
constexpr int16_t kSpaceMd  = 12;  // Medium spacing: standard gutter between elements
constexpr int16_t kSpaceLg  = 16;  // Large spacing: standard screen margin & card padding
constexpr int16_t kSpaceXl  = 24;  // Extra-large spacing (section separations)
constexpr int16_t kSpaceXxl = 32;  // Double extra-large (major layout blocks, footer gap)

// Semantic aliases
constexpr int16_t kMargin = kSpaceLg;  // 16px: standard horizontal screen margin
constexpr int16_t kGutter = kSpaceMd;  // 12px: standard gap between cards and items

// ============================================================================
// 2. COMPONENT SIZING TOKENS
// ============================================================================

// Header & Navigation chrome
constexpr int16_t kHeaderHeight        = 72;  // Unified single-tier header height
constexpr int16_t kHeaderVisibleHeight = 72;  // Viewable height on touch panels
constexpr int16_t kHeaderRuleGap       = 2;   // Air gap between header and rule
constexpr int16_t kHeaderButtonSize    = 48;  // Dimension of header action boxes

// Touch targets & navigation
constexpr int16_t kTouchTargetMin   = 48;  // Apple/Android standard minimum touch target
constexpr int16_t kBackButtonWidth  = 48;  // Width of back button tap target
constexpr int16_t kBackButtonReserve = 72; // Header width reserved for back navigation
constexpr int16_t kPagerButtonSize  = 48;  // Standard pagination arrow target size

// Row heights
constexpr int16_t kRowCompact  = 48;  // Dense lists (OptionPopup, font picker, Wi-Fi items)
constexpr int16_t kRowStandard = 60;  // Standard list rows (Settings, File Browser, Apps)
constexpr int16_t kRowDetailed = 76;  // Detailed rows (with icon + subtitle + badge)

// Battery & Status glyphs
constexpr int16_t kBatteryWidth      = 26;  // Standard battery outer width
constexpr int16_t kBatteryHeight     = 16;  // Standard battery outer height
constexpr int16_t kBatteryCapWidth   = 3;   // Battery positive terminal width
constexpr int16_t kBatteryCapHeight  = 8;   // Battery positive terminal height
constexpr int16_t kBatteryPercentGap = 6;   // Gap between percentage text and battery box

// Dialogs & Popups
constexpr int16_t kDialogSideMargin  = 24;  // Inset from safe screen edges
constexpr int16_t kDialogPadding     = 16;  // Internal content padding

// ============================================================================
// 3. STROKE WEIGHT TOKENS
// ============================================================================

constexpr uint8_t kStrokeHairline = 1;  // Hairline: inner grids, secondary outlines
constexpr uint8_t kStrokeBorder   = 2;  // Standard e-ink border: buttons, selected cards
constexpr uint8_t kStrokeRule     = 3;  // Rule: primary horizontal section dividers
constexpr uint8_t kStrokeHeavy    = 4;  // Heavy: back arrow strokes, emphasis frames

// ============================================================================
// 4. CORNER RADII TOKENS (Theme-dependent)
// ============================================================================

constexpr uint8_t kRadiusNone    = 0;  // Sharp corners (Classic / Dark chrome)
constexpr uint8_t kRadiusControl = 6;  // Rounded controls (Light chrome buttons/pills)
constexpr uint8_t kRadiusCard    = 8;  // Rounded cards & dialog frames (Light chrome)

// Returns appropriate corner radius based on active theme
inline uint8_t cornerRadius(const uint8_t standardRadius = kRadiusControl) {
  return chrome::lightHeader ? standardRadius : kRadiusNone;
}

// ============================================================================
// 5. E-INK PAINTS & DITHER PATTERNS
// ============================================================================

inline constexpr auto ink         = fui::Paint::solid(fui::Color::Black);
inline constexpr auto paper       = fui::Paint::solid(fui::Color::White);
inline constexpr auto ditherLight = fui::Paint::dither(fui::Color::LightGray);  // 25% gray
inline constexpr auto ditherDark  = fui::Paint::dither(fui::Color::DarkGray);   // 50% gray

// Header polarity helpers
inline fui::Paint headerBackground() {
  return chrome::lightHeader ? paper : ink;
}

inline fui::Color headerForeground() {
  return chrome::lightHeader ? fui::Color::Black : fui::Color::White;
}

inline fui::Paint headerDivider() {
  return chrome::lightHeader ? ditherDark : fui::Paint{};
}

// ============================================================================
// 6. TYPOGRAPHY HIERARCHY & INK-CENTERING
// ============================================================================

// Standard Font Slots
constexpr fui::FontId kFontSmall   = fui::FONT_SLOT_SMALL;  // Slot 0: 10pt (captions, metadata, clock)
constexpr fui::FontId kFontBody    = fui::FONT_SLOT_BODY;   // Slot 1: 12/14pt (rows, buttons, content)
constexpr fui::FontId kFontTitle   = fui::FONT_SLOT_TITLE;  // Slot 2: 16/20pt (headers, dialog titles)
constexpr fui::FontId kFontTile    = 3;                     // Slot 3: Dense tile cut
constexpr fui::FontId kFontProse   = 4;                     // Slot 4: Reading body prose

// Metric parameters for capital-ink vertical centering on e-ink displays.
// GfxRendererTarget centers the font's LINE BOX (advanceY) rather than ink,
// clamping to 0 when box.height < lineHeight. inkCentred offsets the box
// so capital glyphs land optically centered.
struct CutMetrics {
  int16_t lineHeight;  // EpdFontData::advanceY
  int16_t ascender;    // EpdFontData::ascender
  int16_t inkHeight;   // Capital 'H' glyph height
};

constexpr CutMetrics kTileCutMetrics{28, 23, 16};     // baemin_11
constexpr CutMetrics kBodyCutMetrics{45, 38, 27};     // baemin_18
constexpr CutMetrics kTitleCutMetrics{60, 50, 36};    // baemin_24

// Compute optical ink-centered bounding rect for a given box and CutMetrics
constexpr fui::Rect inkCentred(const fui::Rect& box, const CutMetrics& cut) {
  return fui::Rect{
      box.x,
      static_cast<int16_t>(box.y + (box.height + cut.inkHeight) / 2 - cut.ascender),
      box.width,
      cut.lineHeight};
}

// Compute optical ink-centered bounding rect dynamically using the draw target
inline fui::Rect inkCentred(const fui::DrawTarget& target, fui::Rect box, const fui::TextStyle style) {
  const auto inkBounds = target.inkBounds(style.font, "H", style);
  if (inkBounds.height <= 0) return box;
  box.y = static_cast<int16_t>(box.y + (box.height - inkBounds.height) / 2 - inkBounds.y);
  box.height = target.lineHeight(style.font);
  return box;
}

// ============================================================================
// 7. COMPONENT STYLE SETS
// ============================================================================

// Standard interactive button styles
inline fui::StyleSet buttonStyles(const bool light = chrome::lightHeader) {
  fui::StyleSet s;
  s.explicitlySet = true;
  s.normal.background = paper;
  s.normal.foreground = ink;
  s.normal.border = ink;
  s.normal.borderWidth = light ? kStrokeHairline : kStrokeBorder;
  s.normal.radius = light ? kRadiusControl : kRadiusNone;

  // Selected / Active state: inverted fill for crisp tactile feedback
  s.selected = s.normal;
  s.selected.background = ink;
  s.selected.foreground = paper;
  s.focused = s.selected;
  s.active = s.selected;

  // Disabled state: dithered border and text
  s.disabled = s.normal;
  s.disabled.border = ditherDark;
  s.disabled.foreground = ditherDark;
  return s;
}

// Standard list row styles (Optimized for e-ink scrolling to prevent ghosting)
inline fui::StyleSet listRowStyles(const bool light = chrome::lightHeader) {
  fui::StyleSet s;
  s.explicitlySet = true;
  s.normal.background = paper;
  s.normal.foreground = ink;
  s.normal.border = ink;
  s.normal.borderWidth = kStrokeHairline;
  s.normal.radius = light ? cornerRadius(4) : kRadiusNone;

  if (light) {
    // Light theme: gentle dither highlight on selection
    s.selected = s.normal;
    s.selected.background = ditherLight;
    s.selected.foreground = ink;
  } else {
    // Dark theme: e-ink outline selection avoids full-black repainting/ghosting
    s.selected = s.normal;
    s.selected.background = paper;
    s.selected.foreground = ink;
    s.selected.border = ink;
    s.selected.borderWidth = kStrokeBorder;
  }
  s.focused = s.selected;
  s.active = s.selected;

  s.disabled = s.normal;
  s.disabled.foreground = ditherDark;
  return s;
}

// Card and panel container styles
inline fui::StyleSet cardStyles(const bool light = chrome::lightHeader) {
  fui::StyleSet s;
  s.explicitlySet = true;
  s.normal.background = paper;
  s.normal.foreground = ink;
  s.normal.border = ink;
  s.normal.borderWidth = light ? kStrokeHairline : kStrokeBorder;
  s.normal.radius = light ? kRadiusCard : kRadiusNone;

  s.selected = s.normal;
  s.selected.borderWidth = kStrokeBorder;
  s.focused = s.selected;
  s.active = s.selected;
  s.disabled = s.normal;
  return s;
}

// Tab bar styles
inline fui::StyleSet tabStyles(const bool light = chrome::lightHeader) {
  fui::StyleSet s = buttonStyles(light);
  s.normal.borderWidth = 0;
  s.selected.borderWidth = 0;
  s.focused.borderWidth = 0;
  s.active.borderWidth = 0;
  s.disabled.borderWidth = 0;
  return s;
}

// Header action chip styles
inline fui::StyleSet headerChipStyles(const bool active, const bool light = chrome::lightHeader) {
  fui::StyleSet s;
  s.explicitlySet = true;
  const auto fg = light ? ink : paper;
  const auto bg = light ? paper : ink;

  if (active) {
    // Active chip: solid knockout
    s.normal.background = fg;
    s.normal.foreground = bg;
    s.normal.border = fg;
    s.normal.borderWidth = kStrokeHairline;
  } else {
    // Inactive chip: outline on header band
    s.normal.background = bg;
    s.normal.foreground = fg;
    s.normal.border = fg;
    s.normal.borderWidth = kStrokeBorder;
  }
  s.normal.radius = light ? kRadiusControl : kRadiusNone;
  s.selected = s.normal;
  s.focused = s.normal;
  s.active = s.normal;
  s.disabled = s.normal;
  return s;
}

// ============================================================================
// 8. UNIFIED BATTERY GLYPH COMPONENT
// ============================================================================

struct BatteryRenderProps {
  int percent = 0;
  bool charging = false;
  fui::Color color = fui::Color::White;
};

inline void paintBattery(fui::DrawTarget& target, const fui::Rect icon, const BatteryRenderProps& props) {
  const auto fg = fui::Paint::solid(props.color);
  const auto boltPaint = (props.color == fui::Color::Black) ? paper : ink;

  // 1. Battery outer frame (thickness: kStrokeBorder = 2px)
  target.stroke(icon, fg, kStrokeBorder);

  // 2. Battery positive terminal nub on the right: 3px wide, 8px high, centered vertically
  const int nubY = icon.y + (icon.height - kBatteryCapHeight) / 2;
  target.fill(fui::makeRect(icon.right(), nubY, kBatteryCapWidth, kBatteryCapHeight), fg);

  // 3. Inner fill cavity: 3px inset from border
  constexpr int kPad = 3;
  const int maxFill = std::max(0, icon.width - kPad * 2);
  const int fillH = std::max(0, icon.height - kPad * 2);
  const int percent = std::max(0, std::min(100, props.percent));
  const int fillW = props.charging ? maxFill : (maxFill * percent) / 100;

  if (fillW > 0) {
    target.fill(fui::makeRect(icon.x + kPad, icon.y + kPad, fillW, fillH), fg);
  }

  // 4. Lightning bolt when charging: crisp high-contrast zigzag inverted against fill
  if (props.charging) {
    const int bx = icon.x + (icon.width - 10) / 2;
    const int by = icon.y + (icon.height - 14) / 2;
    target.line({int16_t(bx + 6), int16_t(by)}, {int16_t(bx + 2), int16_t(by + 7)}, 2, boltPaint);
    target.line({int16_t(bx + 2), int16_t(by + 7)}, {int16_t(bx + 7), int16_t(by + 5)}, 2, boltPaint);
    target.line({int16_t(bx + 7), int16_t(by + 5)}, {int16_t(bx + 3), int16_t(by + 13)}, 2, boltPaint);
  }
}

}  // namespace cds
