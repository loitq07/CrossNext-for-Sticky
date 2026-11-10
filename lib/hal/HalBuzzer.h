#pragma once

#include <BoardConfig.h>

#if FREEINK_CAP_BUZZER && !CROSSPOINT_EMULATED
#include <Buzzer.h>
#endif

class HalBuzzer {
#if FREEINK_CAP_BUZZER && !CROSSPOINT_EMULATED
  // The PWM pin has one owner. Separate instances cannot attach the same
  // pin, and a separate sleep instance cannot detach the one playing tones.
  static freeink::Buzzer& instance() {
    static freeink::Buzzer buzzer;
    return buzzer;
  }
#endif

 public:
  static void click(uint8_t volume = 1) {
#if FREEINK_CAP_BUZZER && !CROSSPOINT_EMULATED
    if (!BoardConfig::hasBuzzer()) return;
    auto& buzzer = instance();
    if (buzzer.begin()) {
      uint32_t duty = (volume == 0) ? 120 : (volume == 1 ? 280 : 512);
      uint32_t durationMs = (volume == 0) ? 9 : (volume == 1 ? 12 : 15);
      buzzer.tone(4000, durationMs, duty);
    }
#endif
  }

  static void beep(uint32_t freqHz = 2500, uint32_t durationMs = 30, uint8_t volume = 1) {
#if FREEINK_CAP_BUZZER && !CROSSPOINT_EMULATED
    if (!BoardConfig::hasBuzzer()) return;
    auto& buzzer = instance();
    if (buzzer.begin()) {
      uint32_t duty = (volume == 0) ? 140 : (volume == 1 ? 300 : 512);
      uint32_t dur = (volume == 0) ? (durationMs * 3 / 4) : durationMs;
      buzzer.tone(freqHz, dur, duty);
    }
#endif
  }

  static void prepareForDeepSleep() {
#if FREEINK_CAP_BUZZER && !CROSSPOINT_EMULATED
    if (!BoardConfig::hasBuzzer()) return;
    auto& buzzer = instance();
    buzzer.end();
    if (BoardConfig::ACTIVE.audio.buzzer != BoardConfig::PIN_UNASSIGNED) {
      pinMode(static_cast<uint8_t>(BoardConfig::ACTIVE.audio.buzzer), INPUT_PULLDOWN);
    }
#endif
  }
};
