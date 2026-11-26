#include "EdgeSwipeIndicator.h"
#include "GfxRenderer.h"
#include "HalDisplay.h"

#include <cstdio>
#include <vector>

namespace {
int checks = 0;
int failures = 0;
#define CHECK(v)                                  \
  do {                                            \
    ++checks;                                     \
    if (!(v)) {                                   \
      ++failures;                                 \
      std::printf("FAIL %d: %s\n", __LINE__, #v); \
    }                                             \
  } while (0)

int countInkInRect(const GfxRenderer& r, int x, int y, int w, int h, uint8_t value) {
  int count = 0;
  for (int py = y; py < y + h; ++py) {
    for (int px = x; px < x + w; ++px) {
      if (r.pixelAt(px, py) == value) ++count;
    }
  }
  return count;
}
}  // namespace

int main() {
  using Edge = MappedInputManager::EdgeSwipe;
  using Icon = EdgeSwipeIndicator::Icon;

  // 1. Icon selection matches edge + context.
  {
    MappedInputManager::EdgeSwipeContext ctx{};
    ctx.allowBack = true;
    ctx.allowHome = true;
    CHECK(EdgeSwipeIndicator::iconForEdge(Edge::Left, ctx) == Icon::Back);
    CHECK(EdgeSwipeIndicator::iconForEdge(Edge::Right, ctx) == Icon::Back);
    CHECK(EdgeSwipeIndicator::iconForEdge(Edge::Bottom, ctx) == Icon::Home);
    CHECK(EdgeSwipeIndicator::iconForEdge(Edge::Top, ctx) == Icon::QuickSettings);
    ctx.allowReaderSettings = true;
    CHECK(EdgeSwipeIndicator::iconForEdge(Edge::Bottom, ctx) == Icon::ReaderSettings);
  }

  // 2. Bounds clamping keeps left/right ears below header and inside screen.
  {
    const auto bTopClamp = EdgeSwipeIndicator::computeBounds(Edge::Left, 10, 20, 480, 800, 72);
    CHECK(bTopClamp.x == 0);
    CHECK(bTopClamp.y >= 72);
    CHECK(bTopClamp.width == 48 && bTopClamp.height == 144);

    const auto bBotClamp = EdgeSwipeIndicator::computeBounds(Edge::Right, 470, 795, 480, 800, 0);
    CHECK(bBotClamp.x == 480 - 48);
    CHECK(bBotClamp.y + bBotClamp.height <= 800 - 8);
  }

  // 3. Draw on all 4 edges with corresponding icons over a checkerboard background.
  for (auto edge : {Edge::Left, Edge::Right, Edge::Top, Edge::Bottom}) {
    GfxRenderer r(480, 800);
    for (int y = 0; y < 800; ++y) {
      for (int x = 0; x < 480; ++x) {
        r.drawPixel(x, y, ((x + y) & 1) == 0);
      }
    }
    const auto icon = edge == Edge::Left || edge == Edge::Right ? Icon::Back
                      : edge == Edge::Bottom                    ? Icon::Home
                                                                : Icon::QuickSettings;
    EdgeSwipeIndicator::drawOnRenderer(r, edge, icon, 240, 400, 0);
    const auto bounds = EdgeSwipeIndicator::computeBounds(edge, 240, 400, 480, 800, 0);
    const int blackCount = countInkInRect(r, bounds.x, bounds.y, bounds.width, bounds.height, 1);
    const int whiteCount = countInkInRect(r, bounds.x, bounds.y, bounds.width, bounds.height, 0);
    CHECK(blackCount > 2500);
    CHECK(whiteCount > 400);

    const int cx = edge == Edge::Left     ? 18
                   : edge == Edge::Right  ? 480 - 18
                                          : 240;
    const int cy = edge == Edge::Top      ? 18
                   : edge == Edge::Bottom ? 800 - 18
                                          : 400;
    const int iconWhite = countInkInRect(r, cx - 8, cy - 8, 17, 17, 0);
    CHECK(iconWhite >= 30);
  }

  // 4. ReaderSettings icon has white slider bars & knobs.
  {
    GfxRenderer r(480, 800);
    EdgeSwipeIndicator::drawOnRenderer(r, Edge::Bottom, Icon::ReaderSettings, 240, 790, 0);
    const int iconWhite = countInkInRect(r, 240 - 9, 800 - 18 - 8, 19, 17, 0);
    CHECK(iconWhite >= 40);
  }

  // 5. Inverted header compensation flips pixel polarity in header band.
  {
    GfxRenderer r(480, 800);
    EdgeSwipeIndicator::drawOnRenderer(r, Edge::Top, Icon::QuickSettings, 240, 10, 60);
    CHECK(r.pixelAt(240, 2) == 0);
  }

  // 6. Fast-window guard suppresses show/flash when panel cannot fast-window.
  {
    GfxRenderer r(480, 800);
    HalDisplay d;
    d.fastWindow = false;
    EdgeSwipeIndicator ind;
    CHECK(!ind.show(r, d, Edge::Left, Icon::Back, 10, 400));
    CHECK(!ind.isVisible());
    CHECK(r.displayWindowCalls == 0);
  }

  // 7. Full show/hide, restoreBufferOnly, and flash + fallback cleanup round-trip.
  {
    GfxRenderer r(480, 800);
    for (int y = 0; y < 800; ++y) {
      for (int x = 0; x < 480; ++x) {
        r.drawPixel(x, y, ((x * 3 + y) % 5) == 0);
      }
    }
    const std::vector<uint8_t> baseline = r.pixels;
    HalDisplay d;
    EdgeSwipeIndicator ind;

    CHECK(ind.show(r, d, Edge::Left, Icon::Back, 8, 360));
    CHECK(ind.isVisible());
    CHECK(r.displayWindowCalls == 1);
    CHECK(r.pixels != baseline);
    ind.hide(r, d);
    CHECK(!ind.isVisible());
    CHECK(r.displayWindowCalls == 2);
    CHECK(r.pixels == baseline);

    CHECK(ind.flash(r, d, Edge::Right, Icon::Back, 472, 420, 1000));
    CHECK(!ind.isVisible());
    CHECK(r.displayWindowCalls == 3);
    CHECK(r.pixels == baseline);
    ind.servicePendingGlassCleanup(r, d, 1050);
    CHECK(r.displayWindowCalls == 3);
    ind.servicePendingGlassCleanup(r, d, 1125);
    CHECK(r.displayWindowCalls == 4);

    CHECK(ind.flash(r, d, Edge::Bottom, Icon::Home, 240, 790, 2000));
    CHECK(r.displayWindowCalls == 5);
    ind.clearPendingGlassCleanup();
    ind.servicePendingGlassCleanup(r, d, 3000);
    CHECK(r.displayWindowCalls == 5);
  }

  std::printf("edge-swipe-indicator: %d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
