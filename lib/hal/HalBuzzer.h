#pragma once

#include <BoardConfig.h>

#if FREEINK_CAP_BUZZER && !CROSSPOINT_EMULATED
#include <Buzzer.h>
#endif

class HalBuzzer {
 public:
  static void click() {
#if FREEINK_CAP_BUZZER && !CROSSPOINT_EMULATED
    if (!BoardConfig::hasBuzzer()) return;
    static freeink::Buzzer buzzer;
    if (buzzer.begin()) {
      buzzer.tone(4000, 12);  // 4 kHz for 12 ms - crisp click matching Sticky vendor profile
    }
#endif
  }

  static void beep(uint32_t freqHz = 2500, uint32_t durationMs = 30) {
#if FREEINK_CAP_BUZZER && !CROSSPOINT_EMULATED
    if (!BoardConfig::hasBuzzer()) return;
    static freeink::Buzzer buzzer;
    if (buzzer.begin()) {
      buzzer.tone(freqHz, durationMs);
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
