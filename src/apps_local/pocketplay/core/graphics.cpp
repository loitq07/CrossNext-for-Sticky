#include "graphics.h"

namespace {

// True if (x, y) is inside a rounded rectangle. Only the four corner squares need
// the circle test: everything else inside the bounds is in.
bool insideRounded(int x, int y, int left, int top, int width, int height, int radius) {
  if (x < left || x >= left + width || y < top || y >= top + height) return false;
  if (radius <= 0) return true;

  // Distance into the corner square, measured from the centre of its circle.
  int dx = 0;
  int dy = 0;
  if (x < left + radius) dx = left + radius - x;
  if (x >= left + width - radius) dx = x - (left + width - radius - 1);
  if (y < top + radius) dy = top + radius - y;
  if (y >= top + height - radius) dy = y - (top + height - radius - 1);
  if (dx == 0 || dy == 0) return true;
  return dx * dx + dy * dy <= radius * radius;
}

Orientation current = Orientation::Portrait;

}  // namespace

void setOrientation(Orientation value) { current = value; }
Orientation orientation() { return current; }
int screenWidth() { return current == Orientation::Landscape ? LANDSCAPE_WIDTH : SCREEN_WIDTH; }
int screenHeight() { return current == Orientation::Landscape ? LANDSCAPE_HEIGHT : SCREEN_HEIGHT; }

// Framebuffer format: 1 bit per pixel, most significant bit first, 1 = white.
void setPixel(uint8_t* frame, int x, int y, bool black) {
  int panelX = 0;
  int panelY = 0;
  if (current == Orientation::Landscape) {
    if (x < 0 || x >= LANDSCAPE_WIDTH || y < 0 || y >= LANDSCAPE_HEIGHT) return;
    panelX = x;
    panelY = y;
  } else {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    panelX = y;
    panelY = SCREEN_WIDTH - 1 - x;
  }
  uint8_t& byte = frame[panelY * EInkDisplay::DISPLAY_WIDTH_BYTES + panelX / 8];
  const auto mask = static_cast<uint8_t>(0x80 >> (panelX % 8));
  if (black) {
    byte &= static_cast<uint8_t>(~mask);
  } else {
    byte |= mask;
  }
}

void fillRect(uint8_t* frame, int left, int top, int width, int height, bool black) {
  for (int y = top; y < top + height; y++) {
    for (int x = left; x < left + width; x++) {
      setPixel(frame, x, y, black);
    }
  }
}

void drawRectOutline(uint8_t* frame, int left, int top, int width, int height, int thickness) {
  fillRect(frame, left, top, width, thickness, true);
  fillRect(frame, left, top + height - thickness, width, thickness, true);
  fillRect(frame, left, top, thickness, height, true);
  fillRect(frame, left + width - thickness, top, thickness, height, true);
}

void drawRing(uint8_t* frame, int centerX, int centerY, int radius, int thickness, bool black) {
  const int outer = radius * radius;
  const int innerRadius = radius - thickness;
  const int inner = innerRadius > 0 ? innerRadius * innerRadius : 0;
  for (int dy = -radius; dy <= radius; dy++) {
    for (int dx = -radius; dx <= radius; dx++) {
      const int distance = dx * dx + dy * dy;
      if (distance <= outer && distance >= inner) setPixel(frame, centerX + dx, centerY + dy, black);
    }
  }
}

void fillRoundRect(uint8_t* frame, int left, int top, int width, int height, int radius, bool black) {
  for (int y = top; y < top + height; y++) {
    for (int x = left; x < left + width; x++) {
      if (insideRounded(x, y, left, top, width, height, radius)) setPixel(frame, x, y, black);
    }
  }
}

// The outer shape minus an inner one, shrunk by the thickness on every side.
void drawRoundRectOutline(uint8_t* frame, int left, int top, int width, int height, int radius, int thickness, bool black) {
  const int innerRadius = radius > thickness ? radius - thickness : 0;
  for (int y = top; y < top + height; y++) {
    for (int x = left; x < left + width; x++) {
      if (!insideRounded(x, y, left, top, width, height, radius)) continue;
      if (insideRounded(x, y, left + thickness, top + thickness, width - 2 * thickness, height - 2 * thickness, innerRadius)) continue;
      setPixel(frame, x, y, black);
    }
  }
}

void drawIcon(uint8_t* frame, const uint8_t* bits, int width, int height, int left, int top, bool black) {
  const int rowBytes = (width + 7) / 8;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      const uint8_t byte = bits[y * rowBytes + x / 8];
      if ((byte & (0x80 >> (x % 8))) == 0) setPixel(frame, left + x, top + y, black);
    }
  }
}
