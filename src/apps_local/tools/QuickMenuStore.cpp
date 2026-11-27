#include "QuickMenuStore.h"

#include <CrossPointSettings.h>
#include <HalFrontlight.h>
#include <HalStorage.h>
#include <Logging.h>

#include <cstring>

#include "../ui/QuickActionSupport.h"

namespace quickmenu {
namespace {
constexpr const char* kPaths[] = {"/.crosspoint/quick-menu-a.bin", "/.crosspoint/quick-menu-b.bin"};
}

Store& Store::instance() {
  static Store store;
  return store;
}

const Config& config() {
  auto& store = Store::instance();
  store.load();
  return store.get();
}

bool Store::read(const int index) {
  HalFile file;
  if (!Storage.openFileForRead("QUICK-MENU", kPaths[index], file)) return false;
  if (file.read(buffer, 4) == 4 && !std::memcmp(buffer, "QMC", 3) && buffer[3] > kSaveVersion) readOnly = true;
  if (file.size() != kSaveBytes) return false;
  return file.seek(0) && file.read(buffer, kSaveBytes) == static_cast<int>(kSaveBytes);
}

void Store::load() {
  if (loaded) return;
  loaded = true;
  current = defaultConfig(Frontlight.present(), SETTINGS.quickActions, quickui::supportedShortcutMask());
  bool existed = false;
  for (int n = 0; n < 2; ++n) {
    existed |= Storage.exists(kPaths[n]);
    if (!read(n)) continue;
    if (!std::memcmp(buffer, "QMC", 3) && buffer[3] > kSaveVersion) {
      readOnly = true;
      continue;
    }
    uint32_t found = 0;
    if (decode(buffer, scratch, found, kSaveBytes) &&
        (slot < 0 || (found != generation && found - generation < 0x80000000u))) {
      current = scratch;
      generation = found;
      slot = n;
    }
  }
  if (existed && slot < 0) readOnly = true;
}

bool Store::save(const Config& next) {
  load();
  if (current.visible == next.visible && !std::memcmp(current.shortcuts, next.shortcuts, sizeof(current.shortcuts)) &&
      !std::memcmp(current.order, next.order, sizeof(current.order)))
    return true;
  if (readOnly || !Storage.ensureDirectoryExists("/.crosspoint")) return false;
  encode(next, generation + 1, buffer);
  uint32_t found = 0;
  if (!decode(buffer, scratch, found, kSaveBytes)) return false;
  const int target = slot == 0 ? 1 : 0;
  {
    HalFile file;
    if (!Storage.openFileForWrite("QUICK-MENU", kPaths[target], file) ||
        file.write(buffer, sizeof(buffer)) != sizeof(buffer)) {
      LOG_ERR("QUICK-MENU", "Cannot save quick settings preferences");
      return false;
    }
  }
  if (!read(target) || !decode(buffer, scratch, found, kSaveBytes) || found != generation + 1) {
    LOG_ERR("QUICK-MENU", "Quick settings verification failed");
    return false;
  }
  current = scratch;
  generation = found;
  slot = target;
  return true;
}

}  // namespace quickmenu
