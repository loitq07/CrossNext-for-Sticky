#pragma once
#include <cstdint>
#define FREEINK_CAP_BUZZER 1
#define CROSSPOINT_EMULATED 0
namespace BoardConfig {
constexpr int PIN_UNASSIGNED = -1;
struct Profile {
  struct { int buzzer = 48; } audio;
};
inline Profile ACTIVE;
inline bool hasBuzzer() { return ACTIVE.audio.buzzer != PIN_UNASSIGNED; }
}
