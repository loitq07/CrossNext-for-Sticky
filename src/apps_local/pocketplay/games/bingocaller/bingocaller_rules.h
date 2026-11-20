#pragma once

#include <cstddef>
#include <cstdint>

// Bingo Caller: calls 30, 75, 80 or 90-ball bingo for players with paper cards. Each call draws
// one number at random from those not called yet. No screen or card, so it can be tested off the
// device: the screen hands each call its entropy, and nothing here reaches for hardware.
namespace bingocaller {

constexpr int kModes = 4;
constexpr uint8_t kBalls[kModes] = {30, 75, 80, 90};
constexpr int kMaxBalls = 90;

struct State {
  uint8_t mode = 0;               // index into kBalls
  uint8_t count = 0;              // how many have been called
  uint8_t order[kMaxBalls] = {};  // the calls, oldest first: 1 to balls(), no repeats
};

inline int balls(const State& s) { return kBalls[s.mode]; }
inline bool finished(const State& s) { return s.count >= balls(s); }
inline int last(const State& s) { return s.count == 0 ? 0 : s.order[s.count - 1]; }

inline bool isCalled(const State& s, int number) {
  for (int i = 0; i < s.count; i++) {
    if (s.order[i] == number) return true;
  }
  return false;
}

// A fresh game of the given mode, nothing called.
inline void start(State& s, int mode) {
  s = State{};
  s.mode = static_cast<uint8_t>(mode);
}

// 75-ball's columns: B 1-15, I 16-30, N 31-45, G 46-60, O 61-75. Other games have no letters.
inline char letter(const State& s, int number) {
  if (balls(s) != 75 || number < 1 || number > 75) return '\0';
  return "BINGO"[(number - 1) / 15];
}

// SplitMix64's finaliser: every input bit flips about half the output bits, so entropy that
// arrives in a few low bits, such as a tap's timing, spreads through the whole state.
inline uint64_t mix(uint64_t z) {
  z += 0x9E3779B97F4A7C15ull;
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
  return z ^ (z >> 31);
}

// The random state, kept between calls. Each call stirs in fresh entropy first, so it never
// depends on one source alone: the chip's random number and the microsecond of the tap.
struct Source {
  uint64_t state = 0;
  uint64_t counter = 0;

  void stir(uint64_t entropy) { state = mix(state ^ mix(entropy)); }

  uint32_t next() {
    counter++;
    return static_cast<uint32_t>(mix(state + counter * 0xD1B54A32D192ED03ull) >> 32);
  }
};

// 0 to n-1, all exactly equally likely: values above the largest multiple of n that fits in
// 32 bits are thrown back, so no result gets the leftover.
template <typename Next>
uint32_t uniform(uint32_t n, Next&& next) {
  const uint32_t limit = 0xFFFFFFFFu - (0xFFFFFFFFu % n + 1) % n;
  uint32_t v;
  do {
    v = next();
  } while (v > limit);
  return v % n;
}

// Calls the next number: one of those left, each equally likely. Returns false, changing
// nothing, once every number has been called.
inline bool call(State& s, Source& source, uint64_t entropy) {
  if (finished(s)) return false;
  uint8_t left[kMaxBalls];
  int n = 0;
  for (int number = 1; number <= balls(s); number++) {
    if (!isCalled(s, number)) left[n++] = static_cast<uint8_t>(number);
  }
  source.stir(entropy);
  s.order[s.count++] = left[uniform(static_cast<uint32_t>(n), [&] { return source.next(); })];
  return true;
}

}  // namespace bingocaller
