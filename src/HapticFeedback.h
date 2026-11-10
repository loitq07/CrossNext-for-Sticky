#pragma once

#include <HalBuzzer.h>
#include <HalHaptics.h>

#include "CrossPointSettings.h"

// Touch actions and accepted hardware repeats share the same feedback.
// Initial hardware presses are acknowledged once in MappedInputManager::update.
// Sampling a touch contact or classifying a gesture must remain silent.
namespace haptic_feedback {
inline void touchAction(bool longPress = false) {
  const bool enabled = SETTINGS.vibration != CrossPointSettings::VIBRATION_OFF;
  if (longPress) {
    HalHaptics::longPress(enabled, SETTINGS.hapticIntensity);
    if (SETTINGS.keySound) {
      HalBuzzer::beep(2500, 30, SETTINGS.buzzerVolume);
    }
  } else {
    HalHaptics::feedback(enabled, true, SETTINGS.hapticIntensity);
    if (SETTINGS.keySound) {
      HalBuzzer::click(SETTINGS.buzzerVolume);
    }
  }
}
}  // namespace haptic_feedback
