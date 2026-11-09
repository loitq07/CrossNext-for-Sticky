#pragma once

#include <cstdint>

class GfxRenderer;

namespace highlightsleep {
// Called before entering a new sleep screen; only a successful highlight paint arms rotation.
void disarm();
void render(GfxRenderer& renderer);
uint64_t timerMicros();
}  // namespace highlightsleep
