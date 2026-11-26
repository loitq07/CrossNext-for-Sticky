#pragma once

#include <cstddef>
#include <cstdint>

#include "MappedInputManager.h"

class GfxRenderer;
class HalDisplay;

// Transient Boox-style edge-swipe visual indicator ("ear" tab with an icon)
// rendered directly into a narrow edge window via GfxRenderer::displayWindow()
// so the user gets immediate confirmation that an edge gesture is recognized.
class EdgeSwipeIndicator {
 public:
  enum class Icon : uint8_t { Back, Home, ReaderSettings, QuickSettings };

  static Icon iconForEdge(MappedInputManager::EdgeSwipe edge, const MappedInputManager::EdgeSwipeContext& ctx);

  // Draw the indicator while the finger is held past threshold.
  bool show(GfxRenderer& renderer, HalDisplay& display, MappedInputManager::EdgeSwipe edge, Icon icon, int anchorX,
            int anchorY);

  // Erase an active live indicator from both framebuffer and glass.
  void hide(GfxRenderer& renderer, HalDisplay& display);

  // Restore saved framebuffer pixels in RAM without touching the panel.
  // If armCleanup is true, arms a fallback displayWindow() in case no screen
  // transition or repaint overwrites the glass.
  void restoreBufferOnly(GfxRenderer& renderer, bool armCleanup = false, uint32_t nowMs = 0);

  // Fast-flick feedback: show the indicator on the glass via displayWindow(),
  // immediately restore the clean framebuffer in RAM, and arm fallback cleanup.
  bool flash(GfxRenderer& renderer, HalDisplay& display, MappedInputManager::EdgeSwipe edge, Icon icon, int anchorX,
             int anchorY, uint32_t nowMs);

  void clearPendingGlassCleanup() { pendingCleanup = false; }
  void servicePendingGlassCleanup(GfxRenderer& renderer, HalDisplay& display, uint32_t nowMs);

  bool isVisible() const { return visible; }

  // Exposed for host unit tests.
  struct Bounds {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
  };
  static Bounds computeBounds(MappedInputManager::EdgeSwipe edge, int anchorX, int anchorY, int screenW, int screenH,
                              int headerLimitY);
  static void drawOnRenderer(GfxRenderer& renderer, MappedInputManager::EdgeSwipe edge, Icon icon, int anchorX,
                             int anchorY, int headerLimitY);

 private:
  static constexpr size_t kSavedBufferCapacity = 1280;
  static constexpr uint32_t kCleanupDelayMs = 120;

  bool visible = false;
  bool pendingCleanup = false;
  uint32_t cleanupAtMs = 0;
  Bounds drawnBounds{};
  Bounds cleanupBounds{};
  uint8_t savedRegion[kSavedBufferCapacity]{};
  size_t savedLen = 0;
};
