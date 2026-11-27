#pragma once

#include <cstdint>

namespace quickui {

enum class Shortcut : uint8_t {
  Dark = 0,
  Refresh,
  Rotate,
  Light,
  Wifi,
  Lock,
  Sleep,
  Off,
  Restart,
  Warm,
  Bluetooth,
  KeySound,
  Tilt,
  Screenshot,
  // Home Menu / launcher actions that can be moved to Quick Settings:
  Files,
  Library,
  ReadLater,
  Transfer,
  Stats,
  Games,
  Apps,
  Services,
  Settings,
  UsbDrive,
  AppStore,
  // Pinned Shelf shortcuts (Games / Apps):
  AppShortcut0,
  AppShortcut1,
  AppShortcut2,
  AppShortcut3,
  Count
};

inline constexpr int kSlots = 4;             // Legacy 4-slot CrossPointSettings quickActions
inline constexpr int kMaxItems = 12;         // Up to 3 rows x 4 tiles
inline constexpr int kAppShortcutCount = 4;  // Up to 4 pinned Game/App shortcuts

}  // namespace quickui
