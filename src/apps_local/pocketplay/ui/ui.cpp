#include "ui.h"

#include <algorithm>
#include <cstdio>

#include "battery_icon.h"
#include "core/graphics.h"
#include "core/text.h"

// Forward declarations for CrossNext system singletons exported in firmware.elf
class HalPowerManager {
 public:
  uint16_t getBatteryPercentage() const;
};
extern HalPowerManager powerManager;

class HalClock {
 public:
  bool formatTime(char* buf, size_t bufSize, bool use12Hour = false) const;
};
extern HalClock halClock;

namespace ui {

namespace {
constexpr int BORDER = 2;

constexpr int BATTERY_WIDTH = 28;
constexpr int BATTERY_HEIGHT = 14;
constexpr int BATTERY_BORDER = 2;
constexpr int BATTERY_INSET = BATTERY_BORDER + 1;  // a white gap inside the outline
constexpr int BATTERY_NUB_WIDTH = 3;
constexpr int BATTERY_NUB_HEIGHT = 6;
constexpr int BATTERY_TEXT_GAP = 6;

constexpr int BOLT_WIDTH = 14;
constexpr int BOLT_HEIGHT = 23;
constexpr uint8_t BOLT[] = {
    0xFF, 0x0F,  // ........####..
    0xFE, 0x1F,  // .......####...
    0xFE, 0x1F,  // .......####...
    0xFC, 0x3F,  // ......####....
    0xFC, 0x3F,  // ......####....
    0xF8, 0x7F,  // .....####.....
    0xF8, 0x7F,  // .....####.....
    0xF0, 0xFF,  // ....####......
    0xF0, 0xFF,  // ....####......
    0xE0, 0x07,  // ...##########.
    0xC0, 0x0F,  // ..##########..
    0xFE, 0x1F,  // .......####...
    0xFE, 0x1F,  // .......####...
    0xFC, 0x3F,  // ......####....
    0xFC, 0x3F,  // ......####....
    0xF8, 0x7F,  // .....####.....
    0xF8, 0x7F,  // .....####.....
    0xF0, 0xFF,  // ....####......
    0xF1, 0xFF,  // ....###.......
    0xE3, 0xFF,  // ...###........
    0xE7, 0xFF,  // ...##.........
    0xCF, 0xFF,  // ..##..........
    0xDF, 0xFF,  // ..#...........
};
}  // namespace

void drawBattery(uint8_t* frame, int left, int top, int height, int percent, bool charging) {
  const int bodyTop = top + (height - BATTERY_HEIGHT) / 2;
  drawRectOutline(frame, left, bodyTop, BATTERY_WIDTH, BATTERY_HEIGHT, BATTERY_BORDER);
  fillRect(frame, left + BATTERY_WIDTH, bodyTop + (BATTERY_HEIGHT - BATTERY_NUB_HEIGHT) / 2, BATTERY_NUB_WIDTH, BATTERY_NUB_HEIGHT, true);

  const int fill = batteryFillWidth(percent, BATTERY_WIDTH - 2 * BATTERY_INSET);
  fillRect(frame, left + BATTERY_INSET, bodyTop + BATTERY_INSET, fill, BATTERY_HEIGHT - 2 * BATTERY_INSET, true);

  char label[12] = "--%";
  if (percent >= 0) snprintf(label, sizeof(label), "%d%%", percent);
  const int textLeft = left + BATTERY_WIDTH + BATTERY_NUB_WIDTH + BATTERY_TEXT_GAP;
  drawTextLeft(frame, Font::Size20, textLeft, top, height, label);

  // After the number, so the battery and number stay put when it comes and goes.
  if (charging) {
    drawIcon(frame, BOLT, BOLT_WIDTH, BOLT_HEIGHT, textLeft + textWidth(Font::Size20, label) + BATTERY_TEXT_GAP, top + (height - BOLT_HEIGHT) / 2);
  }
}

bool isInBox(int x, int y, int left, int top, int width, int height) { return x >= left && x < left + width && y >= top && y < top + height; }

void drawButton(uint8_t* frame, int left, int top, int width, int height, const char* label) {
  fillRoundRect(frame, left, top, width, height, PILL_RADIUS, false);
  drawRoundRectOutline(frame, left, top, width, height, PILL_RADIUS, BORDER, true);
  const Font font = textWidth(Font::Size20, label) <= width - 2 * PILL_RADIUS - 8 ? Font::Size20 :
                    (textWidth(Font::Size14, label) <= width - 2 * PILL_RADIUS - 4 ? Font::Size14 : Font::Size10);
  drawTextInBox(frame, font, left, top, width, height, label, true);
}

void drawBackArrow(uint8_t* frame, int left, int cy, bool white) {
  const int x = left + 20;
  const int y = cy;
  const bool ink = !white;
  drawLine(frame, x + 16, y - 14, x, y, 4, ink);
  drawLine(frame, x, y, x + 16, y + 14, 4, ink);
  drawLine(frame, x, y, x + 28, y, 4, ink);
}

void drawHeader(uint8_t* frame, const char* title, bool hasRightButton) {
  (void)hasRightButton;
  drawBand(frame, title, nullptr, true);
}

bool isOnBack(int x, int y) {
  const int h = orientation() == Orientation::Landscape ? LANDSCAPE_BAND_HEIGHT : BAND_HEIGHT;
  return x >= 0 && x <= 80 && y >= 0 && y <= h + 8;
}

void drawWarning(uint8_t* frame, const char* text) {
  if (orientation() == Orientation::Landscape) {
    // White on the band, which drawBand() left black here.
    drawTextInBox(frame, Font::Size14, LANDSCAPE_WARNING_LEFT, 0, LANDSCAPE_WARNING_WIDTH, LANDSCAPE_BAND_HEIGHT, text, false);
    return;
  }
  drawTextInBox(frame, Font::Size14, 0, WARNING_TOP, SCREEN_WIDTH, lineHeight(Font::Size14), text);
}

namespace {

constexpr int ROW_WIDTH = SCREEN_WIDTH - 2 * MARGIN;
constexpr int ROW_TEXT_INSET = 14;

// The cuts a title tries, largest first.
constexpr Font TITLE_CUTS[] = {Font::Size30, Font::Size20, Font::Size14};

Font largestFitting(const char* text, int width) {
  for (const Font cut : TITLE_CUTS) {
    if (textWidth(cut, text) <= width) return cut;
  }
  return Font::Size10;
}

}  // namespace

void drawTextLeft(uint8_t* frame, Font font, int left, int top, int height, const char* text, bool black) {
  const int cap = capHeight(font);
  drawText(frame, font, left, top + (height - cap) / 2 + cap, text, Align::Left, black);
}

void drawBand(uint8_t* frame, const char* title, const char* rightLabel, bool hasBack, Font labelFont) {
  const bool landscape = orientation() == Orientation::Landscape;
  const int width = screenWidth();
  const int height = landscape ? LANDSCAPE_BAND_HEIGHT : BAND_HEIGHT;
  
  // Solid black header band (CDS kHeaderHeight = 72)
  fillRect(frame, 0, 0, width, height, true);
  fillRect(frame, 0, height + BAND_RULE_GAP, width, RULE, true);

  // 1. Back button: CrossNext chevron arrow (white)
  int left = 16;
  if (hasBack) {
    drawBackArrow(frame, 0, height / 2, true);
    left = 72;
  }

  // 2. Status cluster on right side
  int right = width - 16;

  // Battery icon & percentage
  const int percent = std::max(0, std::min(100, static_cast<int>(powerManager.getBatteryPercentage())));
  const int battW = 24;
  const int battH = 12;
  const int nubW = 3;
  const int nubH = 6;
  const int battX = right - battW - nubW;
  const int battY = (height - battH) / 2;

  // Battery outline (white)
  drawRectOutline(frame, battX, battY, battW, battH, 2, false);
  // Terminal nub (white)
  fillRect(frame, battX + battW, battY + (battH - nubH) / 2, nubW, nubH, false);
  // Battery fill
  const int innerW = battW - 4;
  const int innerH = battH - 4;
  const int fillW = (percent * innerW) / 100;
  if (fillW > 0) {
    fillRect(frame, battX + 2, battY + 2, fillW, innerH, false);
  }
  right = battX - 6;

  // Battery percentage text (white)
  char pctBuf[8];
  std::snprintf(pctBuf, sizeof(pctBuf), "%d%%", percent);
  const int pctW = textWidth(Font::Size14, pctBuf);
  const int baseline = (height + capHeight(Font::Size14)) / 2;
  drawText(frame, Font::Size14, right, baseline, pctBuf, Align::Right, false);
  right -= pctW + 12;

  // System clock (white)
  char clockBuf[16] = {0};
  if (halClock.formatTime(clockBuf, sizeof(clockBuf), true) && clockBuf[0] != '\0') {
    const int clkW = textWidth(Font::Size14, clockBuf);
    drawText(frame, Font::Size14, right, baseline, clockBuf, Align::Right, false);
    right -= clkW + 14;
  }

  // Optional game rightLabel (e.g. "LEVEL 5", "1/64")
  if (rightLabel != nullptr && rightLabel[0] != '\0') {
    Font rFont = labelFont;
    if (textWidth(rFont, rightLabel) > 160) rFont = Font::Size14;
    const int rBase = (height + capHeight(rFont)) / 2;
    const int rW = textWidth(rFont, rightLabel);
    drawText(frame, rFont, right, rBase, rightLabel, Align::Right, false);
    right -= rW + 14;
  }

  if (landscape && right > LANDSCAPE_WARNING_LEFT - GAP) {
    right = LANDSCAPE_WARNING_LEFT - GAP;
  }

  // 3. Title typography
  const int availTitleWidth = right - left;
  if (availTitleWidth > 0 && title != nullptr && title[0] != '\0') {
    const Font tFont = largestFitting(title, availTitleWidth);
    drawTextLeft(frame, tFont, left, 0, height, title, false);
  }
}

void drawPill(uint8_t* frame, int left, int top, int width, const char* label, bool filled, int height) {
  if (filled) {
    fillRoundRect(frame, left, top, width, height, PILL_RADIUS, true);
  } else {
    fillRoundRect(frame, left, top, width, height, PILL_RADIUS, false);
    drawRoundRectOutline(frame, left, top, width, height, PILL_RADIUS, RULE, true);
  }
  const Font font = textWidth(Font::Size20, label) <= width - 2 * PILL_RADIUS - 8 ? Font::Size20 :
                    (textWidth(Font::Size14, label) <= width - 2 * PILL_RADIUS - 4 ? Font::Size14 : Font::Size10);
  drawTextInBox(frame, font, left, top, width, height, label, !filled);
}

void drawDimmedPill(uint8_t* frame, int left, int top, int width, const char* label, int height) {
  constexpr int R = PILL_RADIUS;
  fillRoundRect(frame, left, top, width, height, R, false);
  fillDitherRoundRect(frame, left, top, width, height, R);
  drawRoundRectOutline(frame, left, top, width, height, R, HAIRLINE, true);
  const Font font = textWidth(Font::Size20, label) <= width - 2 * R - 8 ? Font::Size20 :
                    (textWidth(Font::Size14, label) <= width - 2 * R - 4 ? Font::Size14 : Font::Size10);
  drawTextInBox(frame, font, left, top, width, height, label, true);
}

void drawRow(uint8_t* frame, int top, const char* label, const uint8_t* icon, int height) {
  fillRoundRect(frame, MARGIN, top, ROW_WIDTH, height, PILL_RADIUS, false);
  drawRoundRectOutline(frame, MARGIN, top, ROW_WIDTH, height, PILL_RADIUS, HAIRLINE, true);
  drawTextLeft(frame, Font::Size20, MARGIN + ROW_TEXT_INSET, top, height, label, true);
  if (icon != nullptr) {
    drawIcon(frame, icon, ICON_SIZE, ICON_SIZE, MARGIN + ROW_WIDTH - ROW_TEXT_INSET - ICON_SIZE, top + (height - ICON_SIZE) / 2, true);
  }
}

}  // namespace ui
