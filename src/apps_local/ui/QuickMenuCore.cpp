#include "QuickMenuCore.h"

#include <cstring>

namespace quickmenu {
namespace {
bool valid(quickui::Shortcut id) { return static_cast<int>(id) < kCount; }
bool eligible(quickui::Shortcut id, uint32_t eligibleMask) {
  return (eligibleMask & (1u << static_cast<int>(id))) != 0;
}
uint32_t read32(const uint8_t* p) {
  return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}
void write32(uint8_t* p, uint32_t value) {
  for (int n = 0; n < 4; ++n) p[n] = static_cast<uint8_t>(value >> (n * 8));
}
bool utf8(const char* s) {
  while (*s) {
    const auto c = static_cast<uint8_t>(*s++);
    if (c < 32 || c == 127) return false;
    if (c < 128) continue;
    int more = c >= 0xc2 && c <= 0xdf ? 1 : c >= 0xe0 && c <= 0xef ? 2 : c >= 0xf0 && c <= 0xf4 ? 3 : -1;
    if (more < 0) return false;
    const auto first = static_cast<uint8_t>(*s);
    if ((c == 0xe0 && first < 0xa0) || (c == 0xed && first >= 0xa0) || (c == 0xf0 && first < 0x90) ||
        (c == 0xf4 && first >= 0x90))
      return false;
    while (more--)
      if ((static_cast<uint8_t>(*s++) & 0xc0) != 0x80) return false;
  }
  return true;
}
}  // namespace

Config defaultConfig(const bool hasLight, const uint8_t legacySlots[quickui::kSlots], const uint32_t eligibleMask) {
  Config cfg;
  if (!hasLight) {
    static constexpr quickui::Shortcut kStickyDefaults[8] = {
        quickui::Shortcut::Wifi,       quickui::Shortcut::Bluetooth, quickui::Shortcut::KeySound,
        quickui::Shortcut::Dark,       quickui::Shortcut::Refresh,   quickui::Shortcut::Screenshot,
        quickui::Shortcut::Rotate,     quickui::Shortcut::Sleep,
    };
    for (quickui::Shortcut s : kStickyDefaults) {
      if (eligible(s, eligibleMask)) cfg.visible |= (1u << static_cast<int>(s));
    }
    return cfg;
  }

  // Frontlight devices preserve their legacy 4-slot order at the front.
  bool allValid = legacySlots != nullptr;
  uint32_t seen = 0;
  if (allValid) {
    for (int i = 0; i < quickui::kSlots; ++i) {
      if (legacySlots[i] >= kCount) {
        allValid = false;
        break;
      }
      const auto s = static_cast<quickui::Shortcut>(legacySlots[i]);
      if (!eligible(s, eligibleMask) || (seen & (1u << static_cast<int>(s)))) {
        allValid = false;
        break;
      }
      seen |= (1u << static_cast<int>(s));
    }
  }
  if (allValid) {
    cfg.visible = seen;
    int pos = 0;
    for (int i = 0; i < quickui::kSlots; ++i) cfg.order[pos++] = static_cast<quickui::Shortcut>(legacySlots[i]);
    const Config base;
    for (const auto s : base.order) {
      if (!(seen & (1u << static_cast<int>(s)))) cfg.order[pos++] = s;
    }
  } else {
    static constexpr quickui::Shortcut kLightDefaults[4] = {
        quickui::Shortcut::Dark,
        quickui::Shortcut::Refresh,
        quickui::Shortcut::Rotate,
        quickui::Shortcut::Light,
    };
    for (quickui::Shortcut s : kLightDefaults) {
      if (eligible(s, eligibleMask)) cfg.visible |= (1u << static_cast<int>(s));
    }
  }
  return cfg;
}

bool isAppShortcut(quickui::Shortcut id) {
  return static_cast<int>(id) >= kBaseCount && static_cast<int>(id) < kBaseCount + kShortcutCount;
}

const AppShortcutEntry* appShortcut(const Config& config, quickui::Shortcut id) {
  return isAppShortcut(id) ? &config.shortcuts[static_cast<int>(id) - kBaseCount] : nullptr;
}

quickui::Shortcut findAppShortcut(const Config& config, const int folder, const char* title) {
  if (!title || !*title) return quickui::Shortcut::Count;
  for (int n = 0; n < kShortcutCount; ++n)
    if (config.shortcuts[n].folder == folder && !std::strcmp(config.shortcuts[n].title, title))
      return static_cast<quickui::Shortcut>(kBaseCount + n);
  return quickui::Shortcut::Count;
}

void removeAppShortcut(Config& config, quickui::Shortcut id) {
  if (!isAppShortcut(id)) return;
  config.visible &= ~(1u << static_cast<int>(id));
  config.shortcuts[static_cast<int>(id) - kBaseCount] = {};
}

quickui::Shortcut addAppShortcut(Config& config, const int folder, const char* title, const uint32_t eligibleMask) {
  if (folder < 0 || folder > 1 || !title || !*title || std::strlen(title) >= kItemBytes || !utf8(title))
    return quickui::Shortcut::Count;
  auto id = findAppShortcut(config, folder, title);
  if (id != quickui::Shortcut::Count && (config.visible & (1u << static_cast<int>(id)))) return id;
  if (id == quickui::Shortcut::Count) {
    for (int n = 0; n < kShortcutCount; ++n) {
      if (!config.shortcuts[n].title[0]) {
        id = static_cast<quickui::Shortcut>(kBaseCount + n);
        break;
      }
    }
  }
  if (id == quickui::Shortcut::Count || visibleCount(config, eligibleMask) >= kMaxVisible)
    return quickui::Shortcut::Count;
  auto& target = config.shortcuts[static_cast<int>(id) - kBaseCount];
  target.folder = static_cast<uint8_t>(folder);
  std::strcpy(target.title, title);
  config.visible |= (1u << static_cast<int>(id));
  return id;
}

int visibleCount(const Config& config, const uint32_t eligibleMask) {
  int count = 0;
  for (int n = 0; n < kCount; ++n) {
    const auto id = static_cast<quickui::Shortcut>(n);
    if ((config.visible & (1u << n)) && eligible(id, eligibleMask) &&
        (!isAppShortcut(id) || config.shortcuts[n - kBaseCount].title[0] != '\0'))
      ++count;
  }
  return count;
}

int collectVisible(const Config& config, const uint32_t eligibleMask, quickui::Shortcut* out, const int maxOut) {
  int count = 0;
  for (const auto id : config.order) {
    if (count >= maxOut) break;
    if (!valid(id) || !(config.visible & (1u << static_cast<int>(id))) || !eligible(id, eligibleMask)) continue;
    if (isAppShortcut(id) && !config.shortcuts[static_cast<int>(id) - kBaseCount].title[0]) continue;
    if (out) out[count] = id;
    ++count;
  }
  return count;
}

quickui::Shortcut at(const Config& config, int index, const uint32_t eligibleMask) {
  for (const auto id : config.order) {
    if (!valid(id) || !(config.visible & (1u << static_cast<int>(id))) || !eligible(id, eligibleMask)) continue;
    if (isAppShortcut(id) && !config.shortcuts[static_cast<int>(id) - kBaseCount].title[0]) continue;
    if (index-- == 0) return id;
  }
  return quickui::Shortcut::Count;
}

int indexOf(const Config& config, const quickui::Shortcut id, const uint32_t eligibleMask) {
  const int count = visibleCount(config, eligibleMask);
  for (int n = 0; n < count; ++n)
    if (at(config, n, eligibleMask) == id) return n;
  return -1;
}

bool move(Config& config, const quickui::Shortcut id, const int direction, const uint32_t eligibleMask) {
  if (direction != -1 && direction != 1) return false;
  const int count = visibleCount(config, eligibleMask);
  const int index = indexOf(config, id, eligibleMask);
  if (index < 0 || index + direction < 0 || index + direction >= count) return false;
  const auto other = at(config, index + direction, eligibleMask);
  int from = -1, to = -1;
  for (int n = 0; n < kCount; ++n) {
    if (config.order[n] == id) from = n;
    if (config.order[n] == other) to = n;
  }
  if (from < 0 || to < 0) return false;
  config.order[from] = other;
  config.order[to] = id;
  return true;
}

bool setVisible(Config& config, const quickui::Shortcut id, const bool shown, const uint32_t eligibleMask) {
  if (!valid(id) || !eligible(id, eligibleMask)) return false;
  if (shown && isAppShortcut(id) && !config.shortcuts[static_cast<int>(id) - kBaseCount].title[0]) return false;
  const uint32_t bit = 1u << static_cast<int>(id);
  const bool currentlyShown = (config.visible & bit) != 0;
  if (shown == currentlyShown) return true;
  if (shown && visibleCount(config, eligibleMask) >= kMaxVisible) return false;
  if (!shown && visibleCount(config, eligibleMask) <= 1) return false;  // Keep at least 1 tile on the panel.
  if (shown)
    config.visible |= bit;
  else
    config.visible &= ~bit;
  return true;
}

uint32_t checksum(const uint8_t* data, size_t length) {
  uint32_t value = 2166136261u;
  while (length--) value = (value ^ *data++) * 16777619u;
  return value;
}

void encode(const Config& config, const uint32_t generation, uint8_t* bytes) {
  std::memset(bytes, 0, kSaveBytes);
  std::memcpy(bytes, "QMC1", 4);
  write32(bytes + 4, generation);
  write32(bytes + 8, config.visible);
  uint8_t* out = bytes + 12;
  for (const auto& entry : config.shortcuts) {
    *out++ = entry.folder;
    std::memcpy(out, entry.title, kItemBytes);
    out += kItemBytes;
  }
  static_assert(12 + kShortcutCount * (1 + kItemBytes) + kCount <= kSaveBytes - 4);
  for (int n = 0; n < kCount; ++n) *out++ = static_cast<uint8_t>(config.order[n]);
  write32(bytes + kSaveBytes - 4, checksum(bytes, kSaveBytes - 4));
}

bool decode(const uint8_t* bytes, Config& config, uint32_t& generation, const size_t length) {
  if (length != kSaveBytes || std::memcmp(bytes, "QMC1", 4) != 0) return false;
  if (read32(bytes + length - 4) != checksum(bytes, length - 4)) return false;
  const uint32_t mask = read32(bytes + 8);
  const uint32_t validMask = (kCount >= 32) ? 0xFFFFFFFFu : ((1u << kCount) - 1u);
  if (mask & ~validMask) return false;

  config = {};
  config.visible = mask;
  const uint8_t* in = bytes + 12;
  for (int n = 0; n < kShortcutCount; ++n) {
    auto& entry = config.shortcuts[n];
    entry.folder = *in++;
    if (entry.folder > 1 || !std::memchr(in, 0, kItemBytes) || !utf8(reinterpret_cast<const char*>(in))) return false;
    std::memcpy(entry.title, in, kItemBytes);
    in += kItemBytes;
    if ((mask & (1u << (kBaseCount + n))) && !*entry.title) return false;
    if (*entry.title) {
      for (int k = 0; k < n; ++k) {
        if (config.shortcuts[k].folder == entry.folder && !std::strcmp(config.shortcuts[k].title, entry.title))
          return false;
      }
    }
  }
  uint32_t seen = 0;
  for (int n = 0; n < kCount; ++n) {
    const uint8_t value = *in++;
    if (value >= kCount || (seen & (1u << value))) return false;
    seen |= (1u << value);
    config.order[n] = static_cast<quickui::Shortcut>(value);
  }
  generation = read32(bytes + 4);
  return true;
}

}  // namespace quickmenu
