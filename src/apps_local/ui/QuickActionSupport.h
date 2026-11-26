#pragma once

#include <BoardConfig.h>
#include <HalFrontlight.h>
#include <HalTiltSensor.h>

#include "QuickAction.h"

namespace quickui {

inline bool isShortcutSupported(Shortcut shortcut) {
  switch (shortcut) {
    case Shortcut::Light:
      return Frontlight.present();
    case Shortcut::Warm:
      return Frontlight.present() && Frontlight.hasColorTemperature();
    case Shortcut::Lock:
      return BoardConfig::hasHomeKey();
    case Shortcut::KeySound:
      return FREEINK_CAP_BUZZER && BoardConfig::hasBuzzer();
    case Shortcut::Tilt:
      return halTiltSensor.isAvailable();
    case Shortcut::Count:
      return false;
    default:
      return true;
  }
}

inline int supportedShortcuts(Shortcut* out, int maxOut) {
  int count = 0;
  for (int i = 0; i < int(Shortcut::Count) && count < maxOut; ++i) {
    const auto shortcut = static_cast<Shortcut>(i);
    if (isShortcutSupported(shortcut)) out[count++] = shortcut;
  }
  return count;
}

// Resolve a 4-slot user configuration against current board hardware and fill an
// optional second row with distinct supported toggles/actions.
inline int resolveQuickShortcuts(const uint8_t configured[kSlots], uint8_t row1[kSlots], uint8_t* row2, int maxRow2) {
  static constexpr Shortcut kFallbackOrder[] = {
      Shortcut::Wifi,   Shortcut::Bluetooth, Shortcut::KeySound, Shortcut::Tilt,  Shortcut::Dark,
      Shortcut::Rotate, Shortcut::Refresh,   Shortcut::Sleep,    Shortcut::Off,   Shortcut::Restart,
      Shortcut::Light,  Shortcut::Warm,      Shortcut::Lock,
  };
  uint32_t used = 0;
  auto markUsed = [&](Shortcut s) { used |= (1u << uint8_t(s)); };
  auto isUsed = [&](Shortcut s) { return (used & (1u << uint8_t(s))) != 0; };
  auto nextAvailable = [&]() -> Shortcut {
    for (Shortcut candidate : kFallbackOrder) {
      if (isShortcutSupported(candidate) && !isUsed(candidate)) {
        markUsed(candidate);
        return candidate;
      }
    }
    return Shortcut::Dark;
  };

  bool valid[kSlots]{};
  for (int i = 0; i < kSlots; ++i) {
    if (configured[i] < uint8_t(Shortcut::Count)) {
      const auto s = static_cast<Shortcut>(configured[i]);
      if (isShortcutSupported(s) && !isUsed(s)) {
        row1[i] = configured[i];
        markUsed(s);
        valid[i] = true;
      }
    }
  }
  for (int i = 0; i < kSlots; ++i) {
    if (!valid[i]) row1[i] = uint8_t(nextAvailable());
  }

  int row2Count = 0;
  if (row2 && maxRow2 > 0) {
    for (Shortcut candidate : kFallbackOrder) {
      if (row2Count >= maxRow2) break;
      if (isShortcutSupported(candidate) && !isUsed(candidate)) {
        row2[row2Count++] = uint8_t(candidate);
        markUsed(candidate);
      }
    }
  }
  return row2Count;
}

}  // namespace quickui
