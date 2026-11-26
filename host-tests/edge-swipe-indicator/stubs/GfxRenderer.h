#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

class HalDisplay;

class GfxRenderer {
 public:
  enum RenderMode { BW, GRAYSCALE_LSB, GRAYSCALE_MSB };

  int screenW = 480;
  int screenH = 800;
  bool hasFb = true;
  RenderMode mode = BW;
  std::array<int, 4> clip{0, 0, 480, 800};
  std::vector<uint8_t> pixels;
  int displayWindowCalls = 0;
  int lastWinX = 0, lastWinY = 0, lastWinW = 0, lastWinH = 0;

  explicit GfxRenderer(int w = 480, int h = 800)
      : screenW(w), screenH(h), clip{0, 0, w, h}, pixels(static_cast<size_t>(w) * h, 0) {}

  int getScreenWidth() const { return screenW; }
  int getScreenHeight() const { return screenH; }
  bool hasFrameBuffer() const { return hasFb; }
  RenderMode getRenderMode() const { return mode; }
  std::array<int, 4> getClipRect() const { return clip; }
  void setClipRect(int x, int y, int w, int h) const {
    const_cast<GfxRenderer*>(this)->clip = {x, y, w, h};
  }
  void drawPixel(int x, int y, bool state = true) const {
    auto* self = const_cast<GfxRenderer*>(this);
    if (x < clip[0] || y < clip[1] || x >= clip[0] + clip[2] || y >= clip[1] + clip[3]) return;
    if (x < 0 || y < 0 || x >= screenW || y >= screenH) return;
    self->pixels[static_cast<size_t>(y) * screenW + x] = state ? 1 : 0;
  }
  uint8_t pixelAt(int x, int y) const { return pixels[static_cast<size_t>(y) * screenW + x]; }

  size_t readFramebufferRegion(int x, int y, int w, int h, uint8_t* dst, size_t cap) const {
    if (!dst || w <= 0 || h <= 0) return 0;
    const size_t bits = static_cast<size_t>(w) * h;
    const size_t needed = (bits + 7) / 8;
    if (needed > cap) return 0;
    std::memset(dst, 0, needed);
    for (int row = 0; row < h; ++row) {
      for (int col = 0; col < w; ++col) {
        if (pixelAt(x + col, y + row)) {
          const size_t idx = static_cast<size_t>(row) * w + col;
          dst[idx / 8] |= static_cast<uint8_t>(1u << (idx % 8));
        }
      }
    }
    return needed;
  }
  void writeFramebufferRegion(int x, int y, int w, int h, const uint8_t* src) {
    if (!src || w <= 0 || h <= 0) return;
    for (int row = 0; row < h; ++row) {
      for (int col = 0; col < w; ++col) {
        const size_t idx = static_cast<size_t>(row) * w + col;
        pixels[static_cast<size_t>(y + row) * screenW + (x + col)] = (src[idx / 8] & (1u << (idx % 8))) ? 1 : 0;
      }
    }
  }
  void displayWindow(int x, int y, int w, int h) const {
    auto* self = const_cast<GfxRenderer*>(this);
    ++self->displayWindowCalls;
    self->lastWinX = x;
    self->lastWinY = y;
    self->lastWinW = w;
    self->lastWinH = h;
  }
};
