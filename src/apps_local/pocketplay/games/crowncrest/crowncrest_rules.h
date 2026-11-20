#pragma once

#include <cstddef>
#include <cstdint>

// Crown & Crest: four six-sided dice, each with its own faces. No screen or card, so it can be
// tested off the device: the screen hands each roll its entropy, and nothing here reaches for
// hardware.
namespace crowncrest {

constexpr int kDice = 4;
constexpr int kFaces = 6;

struct Roll {
  uint8_t face[kDice] = {};  // 1 to 6, or 0 before the first roll
};

inline bool rolled(const Roll& r) { return r.face[0] != 0; }

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

// All four dice together, after stirring in this roll's entropy.
inline void roll(Roll& r, Source& source, uint64_t entropy) {
  source.stir(entropy);
  for (int d = 0; d < kDice; d++) r.face[d] = face([&] { return source.next(); });
}

// ---- The draft ----
// Two players take turns to roll. The roller takes one die, the other player takes two of the
// three left, and the roller gets the last. Then the other player rolls.

constexpr int kPlayers = 2;
constexpr int kPicks = 3;        // the fourth die isn't picked: it's what's left
constexpr uint8_t kNoOne = 0xFF;
constexpr uint32_t kMaxRolls = 99999;  // the count stops here rather than wrapping to 0

struct Turn {
  Roll dice;
  uint8_t roller = 0;              // 0 = player A, 1 = player B
  uint8_t picks = 0;               // 0 to 3 dice taken so far
  uint8_t order[kPicks] = {};      // which die each pick took
  uint32_t rolls = 0;              // rolls by either player since the game began or was reset
};

inline uint8_t other(uint8_t player) { return static_cast<uint8_t>(1 - player); }
inline bool done(const Turn& t) { return t.picks == kPicks; }

// Who rolls next: A first, then whoever didn't roll last.
inline uint8_t nextRoller(const Turn& t) { return rolled(t.dice) ? other(t.roller) : 0; }
inline bool canRoll(const Turn& t) { return !rolled(t.dice) || done(t); }

// Whose pick it is, or kNoOne when no die is waiting to be picked.
inline uint8_t picker(const Turn& t) {
  if (!rolled(t.dice) || done(t)) return kNoOne;
  return t.picks == 0 ? t.roller : other(t.roller);
}

// Who has a die, or kNoOne. The first pick is the roller's, the next two the other player's,
// and once those are made the die left over is the roller's.
inline uint8_t owner(const Turn& t, int die) {
  for (int i = 0; i < t.picks; i++) {
    if (t.order[i] == die) return i == 0 ? t.roller : other(t.roller);
  }
  return done(t) ? t.roller : kNoOne;
}

// Rolls all four dice for the next roller. False, and nothing changes, while dice are unpicked.
inline bool rollTurn(Turn& t, Source& source, uint64_t entropy) {
  if (!canRoll(t)) return false;
  t.roller = nextRoller(t);
  t.picks = 0;
  roll(t.dice, source, entropy);
  if (t.rolls < kMaxRolls) t.rolls++;
  return true;
}

// A new game: blank dice, no picks, a count of 0, and player A rolls first.
inline void reset(Turn& t) { t = Turn{}; }

// Whether a reset would change anything.
inline bool canReset(const Turn& t) { return rolled(t.dice) || t.rolls != 0; }

// Takes a die for whoever's pick it is. False for a die that's taken or when no pick is due.
inline bool pick(Turn& t, int die) {
  if (picker(t) == kNoOne || die < 0 || die >= kDice || owner(t, die) != kNoOne) return false;
  t.order[t.picks++] = static_cast<uint8_t>(die);
  return true;
}

// Takes back this turn's last pick. False when nothing has been picked.
inline bool undoPick(Turn& t) {
  if (t.picks == 0) return false;
  t.picks--;
  return true;
}

}  // namespace crowncrest
