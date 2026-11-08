#pragma once

#include <BoardConfig.h>

#if FREEINK_CAP_BUZZER && !CROSSPOINT_EMULATED
#include <Buzzer.h>
#endif

class HalBuzzer {
 public:
  static void click(uint8_t volume = 1) {
#if FREEINK_CAP_BUZZER && !CROSSPOINT_EMULATED
    if (!BoardConfig::hasBuzzer()) return;
    static freeink::Buzzer buzzer;
    if (buzzer.begin()) {
      uint32_t duty = (volume == 0) ? 20 : (volume == 1 ? 90 : 512);
      uint32_t durationMs = (volume == 0) ? 7 : (volume == 1 ? 10 : 12);
      buzzer.tone(4000, durationMs, duty);
    }
#endif
  }

  static void beep(uint32_t freqHz = 2500, uint32_t durationMs = 30, uint8_t volume = 1) {
#if FREEINK_CAP_BUZZER && !CROSSPOINT_EMULATED
    if (!BoardConfig::hasBuzzer()) return;
    static freeink::Buzzer buzzer;
    if (buzzer.begin()) {
      uint32_t duty = (volume == 0) ? 25 : (volume == 1 ? 110 : 512);
      uint32_t dur = (volume == 0) ? (durationMs * 2 / 3) : durationMs;
      buzzer.tone(freqHz, dur, duty);
    }
#endif
  }

  static void prepareForDeepSleep() {
#if FREEINK_CAP_BUZZER && !CROSSPOINT_EMULATED
    if (!BoardConfig::hasBuzzer()) return;
    static freeink::Buzzer buzzer;
    buzzer.end();
    if (BoardConfig::ACTIVE.audio.buzzer != BoardConfig::PIN_UNASSIGNED) {
      pinMode(static_cast<uint8_t>(BoardConfig::ACTIVE.audio.buzzer), INPUT_PULLDOWN);
    }
#endif
  }
};
