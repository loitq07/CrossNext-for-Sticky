#include "ui.h"

#include <cstdio>

#include "battery_icon.h"
#include "core/graphics.h"
#include "core/text.h"

namespace ui {

namespace {
constexpr int BORDER = 3;

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
  drawRectOutline(frame, left, top, width, height, BORDER);
  drawTextInBox(frame, Font::Size20, left, top, width, height, label);
}

void drawHeader(uint8_t* frame, const char* title, bool hasRightButton) {
  drawButton(frame, MARGIN, HEADER_TOP, HEADER_BUTTON_WIDTH, HEADER_BUTTON_HEIGHT, "BACK");

  const int side = MARGIN + HEADER_BUTTON_WIDTH;
  const int left = side + GAP;
  const int right = hasRightButton ? SCREEN_WIDTH - side - GAP : SCREEN_WIDTH - MARGIN;
  drawTextInBox(frame, Font::Size20, left, HEADER_TOP, right - left, HEADER_BUTTON_HEIGHT, title);
}

bool isOnBack(int x, int y) {
  if (orientation() == Orientation::Landscape) return isInBox(x, y, MARGIN, 0, HEADER_BUTTON_WIDTH, LANDSCAPE_BAND_HEIGHT);
  return isInBox(x, y, MARGIN, HEADER_TOP, HEADER_BUTTON_WIDTH, HEADER_BUTTON_HEIGHT);
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
constexpr int BACK_HEIGHT = 46;
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
  fillRect(frame, 0, 0, width, height, true);
  fillRect(frame, 0, height + BAND_RULE_GAP, width, RULE, true);

  int left = MARGIN + 4;
  if (hasBack) {
    const int backTop = (height - BACK_HEIGHT) / 2;
    drawRoundRectOutline(frame, MARGIN, backTop, HEADER_BUTTON_WIDTH, BACK_HEIGHT, PILL_RADIUS, RULE, false);
    drawTextInBox(frame, Font::Size20, MARGIN, backTop, HEADER_BUTTON_WIDTH, BACK_HEIGHT, "BACK", false);
    left = MARGIN + HEADER_BUTTON_WIDTH + GAP;
  }

  int right = width - MARGIN - 4;
  if (rightLabel != nullptr && rightLabel[0] != '\0') {
    const int baseline = (height + capHeight(labelFont)) / 2;
    drawText(frame, labelFont, right, baseline, rightLabel, Align::Right, false);
    right -= textWidth(labelFont, rightLabel) + GAP;
  }
  if (landscape && right > LANDSCAPE_WARNING_LEFT - GAP) right = LANDSCAPE_WARNING_LEFT - GAP;

  drawTextLeft(frame, largestFitting(title, right - left), left, 0, height, title, false);
}

// Labels are Size14, the cut CrossPlay's buttons use, stepping down only if one
// doesn't fit.
void drawPill(uint8_t* frame, int left, int top, int width, const char* label, bool filled, int height) {
  if (filled) {
    fillRoundRect(frame, left, top, width, height, PILL_RADIUS, true);
  } else {
    drawRoundRectOutline(frame, left, top, width, height, PILL_RADIUS, RULE);
  }
  const Font font = textWidth(Font::Size14, label) <= width - 2 * PILL_RADIUS ? Font::Size14 : Font::Size10;
  drawTextInBox(frame, font, left, top, width, height, label, !filled);
}

void drawDimmedPill(uint8_t* frame, int left, int top, int width, const char* label, int height) {
  constexpr int R = PILL_RADIUS;
  // One dot in four, inside the rounded corners.
  for (int y = top; y < top + height; y++) {
    for (int x = left; x < left + width; x++) {
      if (x % 2 != 0 || y % 2 != 0) continue;
      int dx = 0;
      int dy = 0;
      if (x < left + R) dx = left + R - x;
      if (x >= left + width - R) dx = x - (left + width - R - 1);
      if (y < top + R) dy = top + R - y;
      if (y >= top + height - R) dy = y - (top + height - R - 1);
      if (dx * dx + dy * dy <= R * R) setPixel(frame, x, y, true);
    }
  }
  drawRoundRectOutline(frame, left, top, width, height, R, HAIRLINE);
  const Font font = textWidth(Font::Size14, label) <= width - 2 * R ? Font::Size14 : Font::Size10;
  drawTextInBox(frame, font, left, top, width, height, label);
}

void drawRow(uint8_t* frame, int top, const char* label, const uint8_t* icon, int height) {
  drawRectOutline(frame, MARGIN, top, ROW_WIDTH, height, HAIRLINE);
  drawTextLeft(frame, Font::Size20, MARGIN + ROW_TEXT_INSET, top, height, label);
  if (icon != nullptr) {
    drawIcon(frame, icon, ICON_SIZE, ICON_SIZE, MARGIN + ROW_WIDTH - ROW_TEXT_INSET - ICON_SIZE, top + (height - ICON_SIZE) / 2);
  }
}

}  // namespace ui
