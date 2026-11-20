#pragma once

#include <cstdint>

// Moving through the list of islands, with no screen code, so it can be tested on the PC.
namespace islandmaps {

// One step forward or back, wrapping past either end, as the home page's pages do.
constexpr int step(int index, int delta, int count) { return ((index + delta) % count + count) % count; }

// A number in 0..n-1, every one equally likely. next() gives 32 random bits. Values at the top that
// would make some numbers come up more often are thrown back.
template <typename Next>
uint32_t uniform(uint32_t n, Next next) {
  const uint64_t limit = (uint64_t{1} << 32) / n * n;
  for (;;) {
    const uint32_t value = next();
    if (value < limit) return value % n;
  }
}

// Any island but the current one, every other one equally likely.
template <typename Next>
int pickOther(int current, int count, Next next) {
  if (count < 2) return current;
  const int r = static_cast<int>(uniform(static_cast<uint32_t>(count - 1), next));
  return r >= current ? r + 1 : r;
}

}  // namespace islandmaps
