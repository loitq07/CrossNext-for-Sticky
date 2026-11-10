#pragma once
#include <cstdint>
#define INPUT_PULLDOWN 9
namespace pwmtest {
inline bool attached = false;
inline int attaches = 0, detaches = 0, tones = 0;
}
inline bool ledcAttach(uint8_t, uint32_t, uint8_t) {
  // Arduino refuses a second attachment to an occupied pin.
  if (pwmtest::attached) return false;
  pwmtest::attached = true;
  ++pwmtest::attaches;
  return true;
}
inline void ledcDetach(uint8_t) { pwmtest::attached = false; ++pwmtest::detaches; }
inline void ledcWrite(uint8_t, uint32_t) {}
inline void ledcWriteTone(uint8_t, uint32_t frequency) {
  if (pwmtest::attached && frequency) ++pwmtest::tones;
}
inline void delay(uint32_t) {}
inline void pinMode(uint8_t, int) {}
