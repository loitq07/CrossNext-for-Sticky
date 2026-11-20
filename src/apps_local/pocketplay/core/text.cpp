#include "text.h"

#include <Arduino.h>

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "fonts/toybox_10.h"
#include "fonts/toybox_14.h"
#include "fonts/toybox_20.h"
#include "fonts/toybox_30.h"
#include "fonts/toybox_44.h"
#include "fonts/toybox_64.h"
#include "graphics.h"

namespace {

const EpdFontData& dataFor(Font font) {
  switch (font) {
    case Font::Size10:
      return toybox_10;
    case Font::Size14:
      return toybox_14;
    case Font::Size20:
      return toybox_20;
    case Font::Size30:
      return toybox_30;
    case Font::Size44:
      return toybox_44;
    case Font::Size64:
      break;
  }
  return toybox_64;
}

// ASCII is the first interval in every size. Returns nullptr for anything else.
const EpdGlyph* findGlyph(const EpdFontData& data, char character) {
  const auto code = static_cast<unsigned char>(character);
  const EpdUnicodeInterval& ascii = data.intervals[0];
  if (code < ascii.first || code > ascii.last) return nullptr;
  return &data.glyph[ascii.offset + code - ascii.first];
}

// Flat-topped with no overshoot, so 'H' gives the height the eye lines text up by.
const EpdGlyph& capitalGlyph(const EpdFontData& data) { return *findGlyph(data, 'H'); }

// Rounded per glyph, so the same two letters are always the same distance apart.
int advanceOf(const EpdFontData& data, char character) {
  const EpdGlyph* glyph = findGlyph(data, character);
  return fp4::toPixel((glyph != nullptr ? *glyph : capitalGlyph(data)).advanceX);
}

int runWidth(const EpdFontData& data, const char* text, int length) {
  int width = 0;
  for (int i = 0; i < length; i++) {
    width += advanceOf(data, text[i]);
  }
  return width;
}

// Glyph bits run continuously with no padding at row ends: most significant bit
// first, 1 = ink. Only ink is drawn, so text can sit on a black or white background.
void drawGlyph(uint8_t* frame, const EpdFontData& data, const EpdGlyph& glyph, int pen, int baseline, bool black) {
  const uint8_t* bits = data.bitmap + glyph.dataOffset;
  const int left = pen + glyph.left;
  const int top = baseline - glyph.top;

  int bit = 0;
  for (int row = 0; row < glyph.height; row++) {
    for (int col = 0; col < glyph.width; col++, bit++) {
      if (bits[bit / 8] & (0x80 >> (bit % 8))) setPixel(frame, left + col, top + row, black);
    }
  }
}

void drawMissing(uint8_t* frame, const EpdFontData& data, char character, int pen, int baseline, bool black) {
  static bool logged = false;
  if (!logged) {
    logged = true;
    Serial.printf("No glyph for byte 0x%02X. Further ones are not logged.\n", static_cast<unsigned char>(character));
  }

  const EpdGlyph& capital = capitalGlyph(data);
  const int left = pen + capital.left;
  const int top = baseline - capital.top;
  const int thickness = std::max(1, capital.height / 10);
  fillRect(frame, left, top, capital.width, thickness, black);
  fillRect(frame, left, top + capital.height - thickness, capital.width, thickness, black);
  fillRect(frame, left, top, thickness, capital.height, black);
  fillRect(frame, left + capital.width - thickness, top, thickness, capital.height, black);
}

void drawRun(uint8_t* frame, const EpdFontData& data, int left, int baseline, const char* text, int length, bool black) {
  int pen = left;
  for (int i = 0; i < length; i++) {
    if (const EpdGlyph* glyph = findGlyph(data, text[i])) {
      drawGlyph(frame, data, *glyph, pen, baseline, black);
    } else {
      drawMissing(frame, data, text[i], pen, baseline, black);
    }
    pen += advanceOf(data, text[i]);
  }
}

int alignedLeft(int x, int width, Align align) {
  switch (align) {
    case Align::Center:
      return x - width / 2;
    case Align::Right:
      return x - width;
    case Align::Left:
      break;
  }
  return x;
}

// Finds how much of text fits on one line. Sets length to the characters to draw
// and returns how many to skip, which also passes over the space or '\n' it broke at.
int measureLine(const EpdFontData& data, const char* text, int width, int& length) {
  int used = 0;
  int lastSpace = -1;
  int i = 0;
  for (; text[i] != '\0' && text[i] != '\n'; i++) {
    // A space that hangs past the edge is invisible, so it never forces a break.
    if (text[i] == ' ') {
      lastSpace = i;
    } else if (i > 0 && used + advanceOf(data, text[i]) > width) {
      if (lastSpace >= 0) {
        length = lastSpace;
        return lastSpace + 1;
      }
      length = i;
      return i;
    }
    used += advanceOf(data, text[i]);
  }
  length = i;
  return text[i] == '\n' ? i + 1 : i;
}

}  // namespace

int textWidth(Font font, const char* text) { return runWidth(dataFor(font), text, static_cast<int>(strlen(text))); }

int lineHeight(Font font) { return dataFor(font).advanceY; }

int capHeight(Font font) {
  const EpdFontData& data = dataFor(font);
  return capitalGlyph(data).height;
}

void drawText(uint8_t* frame, Font font, int x, int baseline, const char* text, Align align, bool black) {
  const EpdFontData& data = dataFor(font);
  const int length = static_cast<int>(strlen(text));
  drawRun(frame, data, alignedLeft(x, runWidth(data, text, length), align), baseline, text, length, black);
}

void drawTextInBox(uint8_t* frame, Font font, int left, int top, int width, int height, const char* text, bool black) {
  const EpdFontData& data = dataFor(font);
  const EpdGlyph& capital = capitalGlyph(data);
  const int baseline = top + (height - capital.height) / 2 + capital.top;
  drawText(frame, font, left + width / 2, baseline, text, Align::Center, black);
}

void fitText(Font font, const char* text, int maxWidth, char* out, size_t outSize) {
  if (outSize == 0) return;
  snprintf(out, outSize, "%s", text);
  if (textWidth(font, out) <= maxWidth) return;

  constexpr const char* ENDING = "..";
  constexpr size_t ENDING_LENGTH = 2;
  if (outSize <= ENDING_LENGTH) {
    out[0] = '\0';
    return;
  }

  size_t keep = strlen(out);
  if (keep > outSize - 1 - ENDING_LENGTH) keep = outSize - 1 - ENDING_LENGTH;
  do {
    memcpy(out + keep, ENDING, ENDING_LENGTH + 1);
  } while (textWidth(font, out) > maxWidth && keep-- > 0);
  if (textWidth(font, out) > maxWidth) out[0] = '\0';
}

int drawWrapped(uint8_t* frame, Font font, int left, int top, int width, const char* text, Align align, bool black) {
  const EpdFontData& data = dataFor(font);
  const int anchor = align == Align::Left ? left : align == Align::Center ? left + width / 2 : left + width;

  int baseline = top + data.ascender;
  int lines = 0;
  while (*text != '\0') {
    int length = 0;
    const int consumed = measureLine(data, text, width, length);
    drawRun(frame, data, alignedLeft(anchor, runWidth(data, text, length), align), baseline, text, length, black);
    text += consumed;
    baseline += data.advanceY;
    lines++;
  }
  return lines * data.advanceY;
}

int wrappedHeight(Font font, int width, const char* text) {
  const EpdFontData& data = dataFor(font);
  int lines = 0;
  while (*text != '\0') {
    int length = 0;
    text += measureLine(data, text, width, length);
    lines++;
  }
  return lines * data.advanceY;
}
