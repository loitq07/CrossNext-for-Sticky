#pragma once

#include <cstddef>
#include <cstdint>

// Dice Roller: two six-sided dice, their total, and how many rolls since the last reset. No
// screen or card, so it can be tested off the device: the screen hands each roll its entropy,
// and nothing here reaches for hardware.
namespace diceroller {

constexpr int kDice = 2;
constexpr int kFaces = 6;
constexpr uint32_t kMaxRolls = 99999;  // the count stops here rather than wrapping to 0

struct Roll {
  uint8_t dice[kDice] = {};  // 1 to 6, or 0 before the first roll
  uint32_t rolls = 0;        // rolls since the last reset
};

inline bool rolled(const Roll& r) { return r.dice[0] != 0; }
inline int total(const Roll& r) { return r.dice[0] + r.dice[1]; }

// SplitMix64's finaliser: every input bit flips about half the output bits, so entropy that
// arrives in a few low bits, such as a tap's timing, spreads through the whole state.
inline uint64_t mix(uint64_t z) {
  z += 0x9E3779B97F4A7C15ull;
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
  return z ^ (z >> 31);
}

// The random state, kept between rolls. Each roll stirs in fresh entropy first, so it never
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

// One face, 1 to 6, all exactly equally likely: values above the largest multiple of 6 that
// fits in 32 bits are thrown back, so no face gets the 4 left over.
// `next` gives 32 random bits a call.
constexpr uint32_t kLimit = 0xFFFFFFFFu - (0xFFFFFFFFu % kFaces + 1) % kFaces;  // 4294967291 = 6 * 715827882 - 1

template <typename Next>
uint8_t face(Next&& next) {
  uint32_t v;
  do {
    v = next();
  } while (v > kLimit);
  return static_cast<uint8_t>(v % kFaces + 1);
}

// Both dice together, after stirring in this roll's entropy, and one more on the count.
inline void roll(Roll& r, Source& source, uint64_t entropy) {
  source.stir(entropy);
  for (int d = 0; d < kDice; d++) r.dice[d] = face([&] { return source.next(); });
  if (r.rolls < kMaxRolls) r.rolls++;
}

// Back to the start: blank dice and a count of 0.
inline void reset(Roll& r) { r = Roll{}; }

}  // namespace diceroller
