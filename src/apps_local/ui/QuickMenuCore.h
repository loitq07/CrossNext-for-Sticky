#pragma once

#include <cstddef>
#include <cstdint>

#include "QuickAction.h"

namespace quickmenu {

constexpr int kCount = static_cast<int>(quickui::Shortcut::Count);
constexpr int kBaseCount = static_cast<int>(quickui::Shortcut::AppShortcut0);
constexpr int kShortcutCount = quickui::kAppShortcutCount;
constexpr int kMaxVisible = quickui::kMaxItems;
constexpr size_t kItemBytes = 64;
constexpr char kSaveVersion = '1';
constexpr size_t kSaveBytes = 320;  // 4 magic + 4 gen + 4 mask + 4*(1+64) shortcuts + 29 order + 19 pad + 4 crc = 320

struct AppShortcutEntry {
  uint8_t folder = 0;
  char title[kItemBytes]{};
};

struct Config {
  uint32_t visible = 0;
  AppShortcutEntry shortcuts[kShortcutCount]{};
  quickui::Shortcut order[kCount] = {
      quickui::Shortcut::Wifi,         quickui::Shortcut::Bluetooth,    quickui::Shortcut::KeySound,
      quickui::Shortcut::Dark,         quickui::Shortcut::Refresh,      quickui::Shortcut::Screenshot,
      quickui::Shortcut::Rotate,       quickui::Shortcut::Sleep,        quickui::Shortcut::Files,
      quickui::Shortcut::Library,      quickui::Shortcut::ReadLater,    quickui::Shortcut::Transfer,
      quickui::Shortcut::Stats,        quickui::Shortcut::Games,        quickui::Shortcut::Apps,
      quickui::Shortcut::Services,     quickui::Shortcut::Settings,     quickui::Shortcut::UsbDrive,
      quickui::Shortcut::AppStore,     quickui::Shortcut::Off,          quickui::Shortcut::Restart,
      quickui::Shortcut::Light,        quickui::Shortcut::Warm,         quickui::Shortcut::Tilt,
      quickui::Shortcut::Lock,         quickui::Shortcut::AppShortcut0, quickui::Shortcut::AppShortcut1,
      quickui::Shortcut::AppShortcut2, quickui::Shortcut::AppShortcut3,
  };
};

Config defaultConfig(bool hasLight, const uint8_t legacySlots[quickui::kSlots], uint32_t eligibleMask);
bool isAppShortcut(quickui::Shortcut id);
const AppShortcutEntry* appShortcut(const Config& config, quickui::Shortcut id);
quickui::Shortcut findAppShortcut(const Config& config, int folder, const char* title);
quickui::Shortcut addAppShortcut(Config& config, int folder, const char* title, uint32_t eligibleMask);
void removeAppShortcut(Config& config, quickui::Shortcut id);
int visibleCount(const Config& config, uint32_t eligibleMask);
int collectVisible(const Config& config, uint32_t eligibleMask, quickui::Shortcut* out, int maxOut);
quickui::Shortcut at(const Config& config, int index, uint32_t eligibleMask);
int indexOf(const Config& config, quickui::Shortcut id, uint32_t eligibleMask);
bool move(Config& config, quickui::Shortcut id, int direction, uint32_t eligibleMask);
bool setVisible(Config& config, quickui::Shortcut id, bool shown, uint32_t eligibleMask);
uint32_t checksum(const uint8_t* data, size_t length);
void encode(const Config& config, uint32_t generation, uint8_t* bytes);
bool decode(const uint8_t* bytes, Config& config, uint32_t& generation, size_t length = kSaveBytes);

}  // namespace quickmenu
