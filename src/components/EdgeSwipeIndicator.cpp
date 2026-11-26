#include "EdgeSwipeIndicator.h"

#include <ChromeRegion.h>
#include <GfxRenderer.h>
#include <HalDisplay.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {
constexpr int kEarDepth = 44;
constexpr int kHaloDepth = 2;
constexpr int kEarHalfSpan = 68;
constexpr int kHaloHalfSpan = 72;
constexpr int kBoxDepth = 48;
constexpr int kBoxSpan = 144;
constexpr int kIconInset = 18;
constexpr float kPi = 3.14159265f;

int bellExtent(const int offset, const int halfSpan, const int depth) {
  if (offset <= -halfSpan || offset >= halfSpan) return 0;
  const float t = static_cast<float>(offset) / static_cast<float>(halfSpan);
  const float factor = 0.5f * (1.0f + std::cos(kPi * t));
  return static_cast<int>(std::lround(static_cast<float>(depth) * factor));
}

void putTone(const GfxRenderer& renderer, const int x, const int y, const bool black, const int headerLimitY) {
  const bool pixelState = (headerLimitY > 0 && y < headerLimitY) ? !black : black;
  renderer.drawPixel(x, y, pixelState);
}

void fillToneBox(const GfxRenderer& renderer, const int x, const int y, const int w, const int h, const bool black,
                 const int headerLimitY) {
  for (int py = y; py < y + h; ++py) {
    for (int px = x; px < x + w; ++px) {
      putTone(renderer, px, py, black, headerLimitY);
    }
  }
}

void drawBackArrowIcon(const GfxRenderer& renderer, const int cx, const int cy, const int headerLimitY) {
  // Bold 3px-stroke left-pointing arrow, crisp on 1-bit e-ink.
  fillToneBox(renderer, cx - 6, cy - 1, 14, 3, false, headerLimitY);
  for (int d = 0; d <= 6; ++d) {
    const int px = cx - 7 + d;
    fillToneBox(renderer, px, cy - d - 1, 1, 3, false, headerLimitY);
    fillToneBox(renderer, px, cy + d - 1, 1, 3, false, headerLimitY);
  }
}

void drawHomeHouseIcon(const GfxRenderer& renderer, const int cx, const int cy, const int headerLimitY) {
  // Solid white house silhouette with a black doorway cutout.
  for (int dy = -8; dy <= -1; ++dy) {
    const int halfW = dy + 8;
    fillToneBox(renderer, cx - halfW, cy + dy, halfW * 2 + 1, 1, false, headerLimitY);
  }
  fillToneBox(renderer, cx - 8, cy, 17, 1, false, headerLimitY);
  fillToneBox(renderer, cx - 6, cy + 1, 13, 7, false, headerLimitY);
  fillToneBox(renderer, cx - 2, cy + 3, 5, 5, true, headerLimitY);
}

void drawReaderSettingsIcon(const GfxRenderer& renderer, const int cx, const int cy, const int headerLimitY) {
  // Two horizontal slider tracks with contrasting knobs.
  fillToneBox(renderer, cx - 8, cy - 5, 17, 2, false, headerLimitY);
  fillToneBox(renderer, cx - 4, cy - 7, 4, 6, false, headerLimitY);
  fillToneBox(renderer, cx - 8, cy + 3, 17, 2, false, headerLimitY);
  fillToneBox(renderer, cx + 1, cy + 1, 4, 6, false, headerLimitY);
}

void drawQuickSettingsIcon(const GfxRenderer& renderer, const int cx, const int cy, const int headerLimitY) {
  // Sun / brightness glyph: core disc + 8 rays.
  for (int dy = -4; dy <= 4; ++dy) {
    for (int dx = -4; dx <= 4; ++dx) {
      if (dx * dx + dy * dy <= 16) {
        putTone(renderer, cx + dx, cy + dy, false, headerLimitY);
      }
    }
  }
  fillToneBox(renderer, cx - 1, cy - 8, 2, 3, false, headerLimitY);
  fillToneBox(renderer, cx - 1, cy + 6, 2, 3, false, headerLimitY);
  fillToneBox(renderer, cx - 8, cy - 1, 3, 2, false, headerLimitY);
  fillToneBox(renderer, cx + 6, cy - 1, 3, 2, false, headerLimitY);
  for (const int sx : {-1, 1}) {
    for (const int sy : {-1, 1}) {
      fillToneBox(renderer, cx + sx * 5 - (sx < 0 ? 1 : 0), cy + sy * 5 - (sy < 0 ? 1 : 0), 2, 2, false,
                  headerLimitY);
    }
  }
}

void drawIcon(const GfxRenderer& renderer, const EdgeSwipeIndicator::Icon icon, const int cx, const int cy,
              const int headerLimitY) {
  switch (icon) {
    case EdgeSwipeIndicator::Icon::Back:
      drawBackArrowIcon(renderer, cx, cy, headerLimitY);
      break;
    case EdgeSwipeIndicator::Icon::Home:
      drawHomeHouseIcon(renderer, cx, cy, headerLimitY);
      break;
    case EdgeSwipeIndicator::Icon::ReaderSettings:
      drawReaderSettingsIcon(renderer, cx, cy, headerLimitY);
      break;
    case EdgeSwipeIndicator::Icon::QuickSettings:
      drawQuickSettingsIcon(renderer, cx, cy, headerLimitY);
      break;
  }
}

int clampAnchor(const int anchor, const int minCenter, const int maxCenter) {
  if (minCenter >= maxCenter) return minCenter;
  return std::clamp(anchor, minCenter, maxCenter);
}
}  // namespace

EdgeSwipeIndicator::Icon EdgeSwipeIndicator::iconForEdge(const MappedInputManager::EdgeSwipe edge,
                                                         const MappedInputManager::EdgeSwipeContext& ctx) {
  switch (edge) {
    case MappedInputManager::EdgeSwipe::Left:
    case MappedInputManager::EdgeSwipe::Right:
      return Icon::Back;
    case MappedInputManager::EdgeSwipe::Bottom:
      return ctx.allowReaderSettings ? Icon::ReaderSettings : Icon::Home;
    case MappedInputManager::EdgeSwipe::Top:
      return Icon::QuickSettings;
    case MappedInputManager::EdgeSwipe::None:
    default:
      return Icon::Back;
  }
}

EdgeSwipeIndicator::Bounds EdgeSwipeIndicator::computeBounds(const MappedInputManager::EdgeSwipe edge,
                                                             const int anchorX, const int anchorY, const int screenW,
                                                             const int screenH, const int headerLimitY) {
  switch (edge) {
    case MappedInputManager::EdgeSwipe::Left:
    case MappedInputManager::EdgeSwipe::Right: {
      const int minCenter = std::max(kHaloHalfSpan + 8, headerLimitY + kHaloHalfSpan + 4);
      const int maxCenter = screenH - kHaloHalfSpan - 8;
      const int cy = clampAnchor(anchorY, minCenter, maxCenter);
      const int x = (edge == MappedInputManager::EdgeSwipe::Left) ? 0 : (screenW - kBoxDepth);
      return {x, cy - kHaloHalfSpan, kBoxDepth, kBoxSpan};
    }
    case MappedInputManager::EdgeSwipe::Bottom: {
      const int cx = clampAnchor(anchorX, kHaloHalfSpan + 8, screenW - kHaloHalfSpan - 8);
      return {cx - kHaloHalfSpan, screenH - kBoxDepth, kBoxSpan, kBoxDepth};
    }
    case MappedInputManager::EdgeSwipe::Top: {
      const int cx = clampAnchor(anchorX, kHaloHalfSpan + 8, screenW - kHaloHalfSpan - 8);
      return {cx - kHaloHalfSpan, 0, kBoxSpan, kBoxDepth};
    }
    case MappedInputManager::EdgeSwipe::None:
    default:
      return {};
  }
}

void EdgeSwipeIndicator::drawOnRenderer(GfxRenderer& renderer, const MappedInputManager::EdgeSwipe edge, const Icon icon,
                                        const int anchorX, const int anchorY, const int headerLimitY) {
  const int screenW = renderer.getScreenWidth();
  const int screenH = renderer.getScreenHeight();
  const Bounds bounds = computeBounds(edge, anchorX, anchorY, screenW, screenH, headerLimitY);
  if (bounds.width <= 0 || bounds.height <= 0) return;

  const auto prevClip = renderer.getClipRect();
  renderer.setClipRect(0, 0, screenW, screenH);

  switch (edge) {
    case MappedInputManager::EdgeSwipe::Left:
    case MappedInputManager::EdgeSwipe::Right: {
      const bool isLeft = edge == MappedInputManager::EdgeSwipe::Left;
      const int cy = bounds.y + kHaloHalfSpan;
      for (int s = -kHaloHalfSpan; s < kHaloHalfSpan; ++s) {
        const int y = cy + s;
        const int innerExt = bellExtent(s, kEarHalfSpan, kEarDepth);
        const int outerExt = bellExtent(s, kHaloHalfSpan, kEarDepth + kHaloDepth);
        if (isLeft) {
          for (int x = 0; x < innerExt; ++x) putTone(renderer, x, y, true, headerLimitY);
          for (int x = innerExt; x < outerExt; ++x) putTone(renderer, x, y, false, headerLimitY);
        } else {
          for (int x = screenW - innerExt; x < screenW; ++x) putTone(renderer, x, y, true, headerLimitY);
          for (int x = screenW - outerExt; x < screenW - innerExt; ++x) putTone(renderer, x, y, false, headerLimitY);
        }
      }
      const int iconX = isLeft ? kIconInset : (screenW - kIconInset);
      drawIcon(renderer, icon, iconX, cy, headerLimitY);
      break;
    }
    case MappedInputManager::EdgeSwipe::Bottom:
    case MappedInputManager::EdgeSwipe::Top: {
      const bool isBottom = edge == MappedInputManager::EdgeSwipe::Bottom;
      const int cx = bounds.x + kHaloHalfSpan;
      for (int s = -kHaloHalfSpan; s < kHaloHalfSpan; ++s) {
        const int x = cx + s;
        const int innerExt = bellExtent(s, kEarHalfSpan, kEarDepth);
        const int outerExt = bellExtent(s, kHaloHalfSpan, kEarDepth + kHaloDepth);
        if (isBottom) {
          for (int y = screenH - innerExt; y < screenH; ++y) putTone(renderer, x, y, true, headerLimitY);
          for (int y = screenH - outerExt; y < screenH - innerExt; ++y) putTone(renderer, x, y, false, headerLimitY);
        } else {
          for (int y = 0; y < innerExt; ++y) putTone(renderer, x, y, true, headerLimitY);
          for (int y = innerExt; y < outerExt; ++y) putTone(renderer, x, y, false, headerLimitY);
        }
      }
      const int iconY = isBottom ? (screenH - kIconInset) : kIconInset;
      drawIcon(renderer, icon, cx, iconY, headerLimitY);
      break;
    }
    case MappedInputManager::EdgeSwipe::None:
      break;
  }

  renderer.setClipRect(prevClip[0], prevClip[1], prevClip[2], prevClip[3]);
}

bool EdgeSwipeIndicator::show(GfxRenderer& renderer, HalDisplay& display, const MappedInputManager::EdgeSwipe edge,
                              const Icon icon, const int anchorX, const int anchorY) {
  if (edge == MappedInputManager::EdgeSwipe::None) return false;
  if (!renderer.hasFrameBuffer() || renderer.getRenderMode() != GfxRenderer::BW || !display.supportsFastWindow()) {
    return false;
  }

  if (visible) {
    restoreBufferOnly(renderer, false, 0);
  }

  const int headerLimitY =
      (chrome::headerHeight > 0 && chrome::invertHeaderForOutput(display.isInverted())) ? chrome::headerHeight : 0;
  const Bounds bounds =
      computeBounds(edge, anchorX, anchorY, renderer.getScreenWidth(), renderer.getScreenHeight(), headerLimitY);
  if (bounds.width <= 0 || bounds.height <= 0) return false;

  savedLen = renderer.readFramebufferRegion(bounds.x, bounds.y, bounds.width, bounds.height, savedRegion,
                                            sizeof(savedRegion));
  if (savedLen == 0) return false;

  drawOnRenderer(renderer, edge, icon, anchorX, anchorY, headerLimitY);
  renderer.displayWindow(bounds.x, bounds.y, bounds.width, bounds.height);

  drawnBounds = bounds;
  visible = true;
  pendingCleanup = false;
  return true;
}

void EdgeSwipeIndicator::hide(GfxRenderer& renderer, HalDisplay& display) {
  if (!visible) return;
  if (renderer.hasFrameBuffer() && savedLen > 0) {
    renderer.writeFramebufferRegion(drawnBounds.x, drawnBounds.y, drawnBounds.width, drawnBounds.height, savedRegion);
    if (display.supportsFastWindow()) {
      renderer.displayWindow(drawnBounds.x, drawnBounds.y, drawnBounds.width, drawnBounds.height);
    }
  }
  visible = false;
  pendingCleanup = false;
  savedLen = 0;
}

void EdgeSwipeIndicator::restoreBufferOnly(GfxRenderer& renderer, const bool armCleanup, const uint32_t nowMs) {
  if (!visible) return;
  if (renderer.hasFrameBuffer() && savedLen > 0) {
    renderer.writeFramebufferRegion(drawnBounds.x, drawnBounds.y, drawnBounds.width, drawnBounds.height, savedRegion);
  }
  if (armCleanup) {
    pendingCleanup = true;
    cleanupAtMs = nowMs + kCleanupDelayMs;
    cleanupBounds = drawnBounds;
  } else {
    pendingCleanup = false;
  }
  visible = false;
  savedLen = 0;
}

bool EdgeSwipeIndicator::flash(GfxRenderer& renderer, HalDisplay& display, const MappedInputManager::EdgeSwipe edge,
                               const Icon icon, const int anchorX, const int anchorY, const uint32_t nowMs) {
  if (!show(renderer, display, edge, icon, anchorX, anchorY)) return false;
  restoreBufferOnly(renderer, true, nowMs);
  return true;
}

void EdgeSwipeIndicator::servicePendingGlassCleanup(GfxRenderer& renderer, HalDisplay& display, const uint32_t nowMs) {
  if (!pendingCleanup) return;
  if (static_cast<int32_t>(nowMs - cleanupAtMs) < 0) return;
  pendingCleanup = false;
  if (renderer.hasFrameBuffer() && renderer.getRenderMode() == GfxRenderer::BW && display.supportsFastWindow()) {
    renderer.displayWindow(cleanupBounds.x, cleanupBounds.y, cleanupBounds.width, cleanupBounds.height);
  }
}
