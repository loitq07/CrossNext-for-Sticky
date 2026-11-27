#include "AppStoreActivity.h"

#include <HalStorage.h>
#include <I18n.h>
#include <Logging.h>
#include <WiFi.h>
#include <mbedtls/sha256.h>

#include <cstdio>
#include <cstring>

#include "../Shelf.h"
#include "../../HapticFeedback.h"
#include "../../activities/DynamicAppActivity.h"
#include "../../activities/network/WifiSelectionActivity.h"
#include "../../components/UITheme.h"
#include "../../network/HttpDownloader.h"
#include "../ui/PixelMenuTheme.h"
#include "../ui/SettingsMenuStyle.h"
#include "../ui/ToyboxFonts.h"
#include "../modules/NativeModule.h"
#include "../i18n/UiLanguage.h"
#include "../../CrossPointSettings.h"
#include "../../activities/RenderLock.h"

namespace fui = freeink::ui;

namespace {

static const char* TAG = "AppStore";

// FORK CHANGE: also Vietnamese while the saved language is Vietnamese but its
// pack is not loaded yet (first boot after an OTA, before the store fetches it).
bool isVi() {
  return I18N.getLanguage() == Language::VI ||
         (!I18N.hasLanguagePack() && SETTINGS.language == static_cast<uint8_t>(Language::VI));
}

// --- resource packs ------------------------------------------------------------

// A file counts as present when its size matches; the hash is checked once,
// right after download, not on every list rebuild (packs reach 28 MB).
bool resourceFilePresent(const ResourceFile& file) {
  HalFile f;
  if (!Storage.exists(file.path) || !Storage.openFileForRead(TAG, file.path, f)) return false;
  return f.fileSize64() == file.size;
}

bool resourcePackInstalled(const ResourcePack& pack) {
  for (size_t i = 0; i < pack.count; ++i)
    if (!resourceFilePresent(pack.files[i])) return false;
  return true;
}

bool fileMatchesSha256(const char* path, const uint8_t expected[32]) {
  HalFile f;
  if (!Storage.openFileForRead(TAG, path, f)) return false;
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);
  uint8_t buffer[2048];
  int n;
  while ((n = f.read(buffer, sizeof(buffer))) > 0) mbedtls_sha256_update(&ctx, buffer, static_cast<size_t>(n));
  uint8_t digest[32];
  mbedtls_sha256_finish(&ctx, digest);
  mbedtls_sha256_free(&ctx);
  return n == 0 && std::memcmp(digest, expected, sizeof(digest)) == 0;
}

// Parent directory of a card path ("/apps/pet-v2/x.bin" -> "/apps/pet-v2").
std::string parentOf(const char* path) {
  const char* slash = std::strrchr(path, '/');
  return slash && slash != path ? std::string(path, slash) : std::string("/");
}

// Where a catalog item sits on the card, or "" when it is not installed.
std::string installedPathOf(const StoreItem& item) {
  if (item.resources) return resourcePackInstalled(*item.resources) ? std::string(item.folder) : std::string();
  std::string path = std::string(item.folder) + "/" + item.filename;
  if (Storage.exists(path.c_str())) return path;
  path = std::string("/sdcard") + item.folder + "/" + item.filename;
  if (Storage.exists(path.c_str())) return path;
  return "";
}

// Language packs: whether the saved interface language uses this pack file.
bool languagePackNeeded(const StoreItem& item) {
  if (SETTINGS.languagePack[0]) return strcmp(SETTINGS.languagePack, item.filename) == 0;
  if (uilanguage::builtIn(SETTINGS.language)) return false;
  char wanted[sizeof(SETTINGS.languagePack)];
  return uilanguage::defaultPackFilename(SETTINGS.language, wanted, sizeof(wanted)) &&
         strcmp(wanted, item.filename) == 0;
}

// The firmware build a language pack was last fetched for. A pack carries no
// ABI word of its own, and its keys follow the firmware's string table.
std::string languagePackStampPath(const StoreItem& item) {
  return std::string("/.crosspoint/") + item.filename + ".abi";
}

std::string firmwareAbiText() {
  char abi[12];
  snprintf(abi, sizeof(abi), "%08x", static_cast<unsigned>(native_module::firmwareAbi()));
  return abi;
}

bool isOutdated(const StoreItem& item) {
  if (item.resources) return false;  // pinned content: never part of "update all"
  if (item.languagePack) {
    if (!languagePackNeeded(item)) return false;
    if (installedPathOf(item).empty()) return true;
    const std::string stamp = languagePackStampPath(item);
    return !Storage.exists(stamp.c_str()) || Storage.readFile(stamp.c_str()) != firmwareAbiText().c_str();
  }
  if (!item.module) return false;
  const std::string path = installedPathOf(item);
  return !path.empty() && native_module::fileAbi(path.c_str()) != native_module::firmwareAbi();
}

// The firmware build (module ABI) whose update prompt was last shown, so a
// "later" is not asked again on every boot of the same firmware.
constexpr char kPromptedPath[] = "/.crosspoint/modules-prompted.txt";

}  // namespace

AppStoreActivity::AppStoreActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : UiTabListActivity("AppStore", renderer, mappedInput) {}

std::unique_ptr<Activity> AppStoreActivity::create(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<AppStoreActivity>(renderer, mappedInput);
}

std::unique_ptr<Activity> AppStoreActivity::createUpdater(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                                          const bool fromShelf) {
  auto store = std::make_unique<AppStoreActivity>(renderer, mappedInput);
  store->updaterMode = true;
  store->exitViaShelf = fromShelf;
  return store;
}

int AppStoreActivity::outdatedCount() {
  int count = 0;
  for (size_t i = 0; i < kStoreItemCount; ++i)
    if (isOutdated(kStoreItems[i])) ++count;
  return count;
}

bool AppStoreActivity::takeBootPrompt() {
  char abi[12];
  snprintf(abi, sizeof(abi), "%08x", static_cast<unsigned>(native_module::firmwareAbi()));
  if (Storage.exists(kPromptedPath) && Storage.readFile(kPromptedPath) == abi) return false;
  if (outdatedCount() == 0) return false;
  Storage.writeFile(kPromptedPath, String(abi));
  return true;
}

void AppStoreActivity::onEnter() {
  toybox::ensureFonts(renderer);
  downloading = false;
  startDownloadPending = false;
  pendingAction = PendingAction::None;
  updating = false;
  rebuildRows();
  UiTabListActivity::onEnter();
  // Touch device: land on the list, not on the tab band.
  if (listCount() > 0) activeNav().selected = 1;
  if (updaterMode) {
    if (outdatedCount() == 0)
      pendingAction = PendingAction::Finish;
    else
      showUpdatePrompt();
  }
}

void AppStoreActivity::leave() {
  if (exitViaShelf)
    shelf::leave(renderer, mappedInput);
  else
    finish();
}

void AppStoreActivity::showUpdatePrompt() {
  static const char* const kVi[] = {"Cập nhật ngay", "Để sau"};
  static const char* const kEn[] = {"Update now", "Later"};
  char msg[192];
  snprintf(msg, sizeof(msg),
           isVi() ? "Firmware vừa được cập nhật. %d trò chơi/ứng dụng cần tải bản mới để chạy được."
                  : "The firmware was updated. %d games/apps need a new download to keep working.",
           outdatedCount());
  optionPopup.showMessage(isVi() ? "Cập nhật ứng dụng" : "Update apps", msg, isVi() ? kVi : kEn, 2, 0,
                          [this](const int choice) {
                            pendingAction = choice == 0 ? PendingAction::UpdateAll : PendingAction::Finish;
                          });
  requestUpdate();
}

bool AppStoreActivity::preventAutoSleep() { return downloading || startDownloadPending; }

bool AppStoreActivity::skipLoopDelay() { return downloading || startDownloadPending; }

// --- catalog helpers ---------------------------------------------------------

bool AppStoreActivity::isItemInstalled(const StoreItem& item) const { return !installedPathOf(item).empty(); }

std::string AppStoreActivity::getInstalledPath(const StoreItem& item) const {
  std::string fullPath = std::string(item.folder) + "/" + item.filename;
  if (Storage.exists(fullPath.c_str())) return fullPath;
  std::string sdPath = std::string("/sdcard") + item.folder + "/" + item.filename;
  if (Storage.exists(sdPath.c_str())) return sdPath;
  return fullPath;
}

const char* AppStoreActivity::displayName(const StoreItem& item) const {
  return (isVi() && item.viName && *item.viName) ? item.viName : item.name;
}

void AppStoreActivity::rebuildRows() {
  rows_.clear();
  const int outdated = outdatedCount();
  if (outdated > 0) {
    Row all;
    all.catalogIndex = -1;
    rows_.push_back(all);
    char label[64];
    snprintf(label, sizeof(label), isVi() ? "Cập nhật tất cả (%d)" : "Update all (%d)", outdated);
    updateAllLabel_ = label;
  }
  for (size_t i = 0; i < kStoreItemCount; ++i) {
    const StoreItem& item = kStoreItems[i];
    // "All" is the games and apps; languages and resources have their own tabs.
    if (currentCategory == StoreCategory::All
            ? item.category != StoreCategory::Games && item.category != StoreCategory::Apps
            : item.category != currentCategory)
      continue;
    Row row;
    row.catalogIndex = static_cast<int>(i);
    row.installed = isItemInstalled(item);
    row.outdated = row.installed && isOutdated(item);
    rows_.push_back(row);
  }

  rowItems_.assign(rows_.size(), fui::ListItem{});
  for (size_t i = 0; i < rows_.size(); ++i) {
    rowItems_[i].actionValue = static_cast<int16_t>(i);
    if (rows_[i].catalogIndex < 0) {
      rowItems_[i].label = updateAllLabel_.c_str();
      rowItems_[i].value = ">";
      rowItems_[i].labelBold = true;
      continue;
    }
    const StoreItem& item = kStoreItems[rows_[i].catalogIndex];
    rowItems_[i].label = displayName(item);
    rowItems_[i].value = rows_[i].outdated    ? (isVi() ? "CẬP NHẬT" : "UPDATE")
                         : rows_[i].installed ? (isVi() ? "ĐÃ CÀI" : "INSTALLED")
                                              : (isVi() ? "TẢI VỀ" : "GET");
    rowItems_[i].actionValue = static_cast<int16_t>(i);
  }
}

// --- tabs --------------------------------------------------------------------

const char* AppStoreActivity::tabLabel(const int index) const {
  switch (index) {
    case 0:
      return isVi() ? "Tất cả" : "All";
    case 1:
      return isVi() ? "Trò chơi" : "Games";
    case 2:
      return isVi() ? "Ứng dụng" : "Apps";
    case 3:
      return isVi() ? "Ngôn ngữ" : "Languages";
    default:
      return isVi() ? "Tài nguyên" : "Resources";
  }
}

void AppStoreActivity::selectCategory(const StoreCategory category) {
  currentCategory = category;
  rebuildRows();
  auto& n = activeNav();
  n.selected = listCount() > 0 ? 1 : 0;
  n.top = 0;
  n.followOnBuild = true;
  requestUpdate();
}

void AppStoreActivity::onTabAction(const int index) {
  if (optionPopup.isActive()) return;
  if (index != activeTab()) selectCategory(static_cast<StoreCategory>(index));
  app.clearTapFlash();
}

void AppStoreActivity::stepTab(const int direction) {
  const int next = (activeTab() + direction + tabCount()) % tabCount();
  selectCategory(static_cast<StoreCategory>(next));
}

// --- screen ------------------------------------------------------------------

void AppStoreActivity::drawChrome() { pixelui::drawHeader(renderer, isVi() ? "Cửa hàng ứng dụng" : "App Store", true); }

void AppStoreActivity::buildScreen(UiScreen& screen) {
  pixelui::FontScope fonts(uiTarget, renderer, isVi() || I18N.getLanguage() != Language::EN);
  uiTarget.setFont(0, toybox::kButtonFontId);
  uiTarget.setFont(4, toybox::kLibraryFontId);
  const int hints = mappedInput.hasTouch() ? 0 : UITheme::getInstance().getMetrics().buttonHintsHeight;
  pixelui::begin(screen, hints);

  fui::TabItem tabs[kTabCount];
  for (int i = 0; i < kTabCount; ++i) tabs[i] = fui::tabItem(i, activeTab() == i, true, tabLabel(i));
  pixelui::tabs(screen, tabs, kTabCount, ACTION_TAB, 0);

  fui::ListProps props;
  props.items = rowItems_.data();
  props.count = static_cast<uint16_t>(rowItems_.size());
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch;  // physical buttons stay in loop()
  settingsui::styleRows(props);
  props.labelText.maxLines = 1;
  props.subtitleText.maxLines = 2;
  syncTabListViewport(screen, props);
  if (chrome::lightHeader)
    fui::list(screen.frame(), screen.body(), props);
  else
    pixelui::list(screen, props, false, true, pixelui::kBorder, true);
}

void AppStoreActivity::drawDownloadPopup() {
  char msg[96];
  const int percent = downloadTotal > 0 ? static_cast<int>(downloadProgress * 100 / downloadTotal) : 0;
  const char* name = (selectedCatalogIndex >= 0 && selectedCatalogIndex < static_cast<int>(kStoreItemCount))
                         ? displayName(kStoreItems[selectedCatalogIndex])
                         : "";
  if (updating)
    snprintf(msg, sizeof(msg), isVi() ? "Cập nhật %s (%d/%d)... %d%%" : "Updating %s (%d/%d)... %d%%", name,
             updateOk + updateFailed + 1, updateTotal, percent);
  else
    snprintf(msg, sizeof(msg), isVi() ? "Đang tải %s... %d%%" : "Downloading %s... %d%%", name, percent);
  const Rect box = GUI.drawPopup(renderer, msg);
  GUI.fillPopupProgress(renderer, box, percent);
}

void AppStoreActivity::render(RenderLock&& lock) {
  if (downloading) {
    renderer.clearScreen();
    drawChrome();
    renderUi();
    drawDownloadPopup();  // the theme popup pushes the frame itself
    return;
  }
  if (optionPopup.isActive()) {
    // Repaint the store under the dialog so it never stacks on a stale frame
    // (e.g. the progress popup that preceded a result notice).
    renderer.clearScreen();
    drawChrome();
    renderUi();
    optionPopup.processRender(renderer, mappedInput);
    return;
  }
  UiListActivity::render(std::move(lock));
}

// --- input -------------------------------------------------------------------

bool AppStoreActivity::handleCustomInput() {
  return optionPopup.handleInput(mappedInput, [this] { requestUpdate(); });
}

bool AppStoreActivity::handleButtons() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    haptic_feedback::touchAction();
    leave();
    return true;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (ringPos() == 0)
      stepTab(1);
    else
      activateIndex(ringPos() - 1);
    return true;
  }
  return false;
}

void AppStoreActivity::activateIndex(const int index) {
  if (optionPopup.isActive() || index < 0 || index >= listCount()) return;
  app.clearTapFlash();
  const Row& row = rows_[index];
  if (row.catalogIndex < 0) {
    startUpdateAll();
    return;
  }
  if (row.installed && !row.outdated)
    showItemActions(row.catalogIndex);
  else
    checkWifiAndDownload(row.catalogIndex);
}

void AppStoreActivity::showItemActions(const int catalogIndex) {
  selectedCatalogIndex = catalogIndex;
  if (kStoreItems[catalogIndex].resources || kStoreItems[catalogIndex].languagePack) {
    // Nothing to open: resource packs serve shelf apps and language packs are selected in Settings.
    static const char* const kResVi[] = {"Tải lại", "Xoá", "Huỷ"};
    static const char* const kResEn[] = {"Download again", "Delete", "Cancel"};
    optionPopup.show(displayName(kStoreItems[catalogIndex]), isVi() ? kResVi : kResEn, 3, 2,
                     [this](const int choice) {
                       if (choice == 0) pendingAction = PendingAction::Reinstall;
                       if (choice == 1) pendingAction = PendingAction::ConfirmDelete;
                     });
    requestUpdate();
    return;
  }
  static const char* const kVi[] = {"Mở", "Xoá", "Huỷ"};
  static const char* const kEn[] = {"Open", "Delete", "Cancel"};
  const StoreItem& item = kStoreItems[catalogIndex];
  optionPopup.show(displayName(item), isVi() ? kVi : kEn, 3, 0,
                   [this](const int choice) {
                     if (choice == 0) pendingAction = PendingAction::Open;
                     if (choice == 1) pendingAction = PendingAction::ConfirmDelete;
                   });
  requestUpdate();
}

void AppStoreActivity::showNotice(const char* title, const char* message) {
  static const char* const kVi[] = {"Đóng"};
  static const char* const kEn[] = {"Close"};
  optionPopup.showMessage(title, message, isVi() ? kVi : kEn, 1, 0, nullptr);
  requestUpdate(true);
}

void AppStoreActivity::runPendingAction() {
  const PendingAction action = pendingAction;
  pendingAction = PendingAction::None;
  if (action == PendingAction::UpdateAll) {
    startUpdateAll();
    return;
  }
  if (action == PendingAction::Finish) {
    leave();
    return;
  }
  if (selectedCatalogIndex < 0 || selectedCatalogIndex >= static_cast<int>(kStoreItemCount)) return;
  const StoreItem& item = kStoreItems[selectedCatalogIndex];

  switch (action) {
    case PendingAction::Open:
      activityManager.pushActivity(DynamicAppActivity::create(getInstalledPath(item), renderer, mappedInput));
      break;
    case PendingAction::ConfirmDelete: {
      static const char* const kVi[] = {"Xoá", "Huỷ"};
      static const char* const kEn[] = {"Delete", "Cancel"};
      char msg[128];
      snprintf(msg, sizeof(msg), isVi() ? "Xoá %s khỏi thẻ nhớ?" : "Remove %s from the SD card?", displayName(item));
      const char* title = item.languagePack ? (isVi() ? "Xoá ngôn ngữ" : "Delete language")
                                            : (isVi() ? "Xoá ứng dụng" : "Delete app");
      optionPopup.showMessage(title, msg, isVi() ? kVi : kEn, 2, 1,
                              [this](const int choice) {
                                if (choice == 0) pendingAction = PendingAction::Delete;
                              });
      requestUpdate();
      break;
    }
    case PendingAction::Reinstall:
      checkWifiAndDownload(selectedCatalogIndex);
      break;
    case PendingAction::Delete:
      if (item.resources) {
        for (size_t i = 0; i < item.resources->count; ++i) Storage.remove(item.resources->files[i].path);
        rebuildRows();
        requestUpdate();
        break;
      }
      Storage.remove(getInstalledPath(item).c_str());
      if (item.languagePack)
        Storage.remove(languagePackStampPath(item).c_str());
      else
        shelf::refreshApps();
      rebuildRows();
      requestUpdate();
      break;
    case PendingAction::WifiSetup:
      launchWifiSetup();
      break;
    default:
      break;
  }
}

void AppStoreActivity::loop() {
  if (startDownloadPending) {
    startDownloadPending = false;
    executeDownload();
    return;
  }
  if (downloading) return;
  if (pendingAction != PendingAction::None && !optionPopup.isActive()) {
    runPendingAction();
    return;
  }
  UiTabListActivity::loop();
}

// --- download ----------------------------------------------------------------

void AppStoreActivity::beginDownload(const int catalogIndex) {
  selectedCatalogIndex = catalogIndex;
  downloading = true;
  downloadProgress = 0;
  downloadTotal = kStoreItems[catalogIndex].sizeBytes;
  cancelDownload = false;
  startDownloadPending = true;
  requestUpdate(true);
}

void AppStoreActivity::checkWifiAndDownload(const int catalogIndex) {
  selectedCatalogIndex = catalogIndex;
  if (WiFi.status() == WL_CONNECTED) {
    beginDownload(catalogIndex);
    return;
  }
  static const char* const kVi[] = {"Kết nối", "Huỷ"};
  static const char* const kEn[] = {"Connect", "Cancel"};
  optionPopup.showMessage(isVi() ? "Chưa kết nối Wi-Fi" : "Wi-Fi not connected",
                          isVi() ? "Cần Wi-Fi để tải ứng dụng. Chọn mạng ngay?"
                                 : "Wi-Fi is required to download apps. Connect now?",
                          isVi() ? kVi : kEn, 2, 0, [this](const int choice) {
                            if (choice == 0) pendingAction = PendingAction::WifiSetup;
                          });
  requestUpdate();
}

void AppStoreActivity::launchWifiSetup() {
  auto wifi = makeUniqueNoThrow<WifiSelectionActivity>(renderer, mappedInput);
  if (!wifi) {
    showNotice(isVi() ? "Lỗi" : "Error", isVi() ? "Không đủ bộ nhớ để mở Wi-Fi." : "Not enough memory for Wi-Fi.");
    return;
  }
  startActivityForResult(std::move(wifi), [this](const ActivityResult& res) {
    const bool online = !res.isCancelled && WiFi.status() == WL_CONNECTED;
    if (updating) {
      if (online) {
        startNextUpdate();
      } else {
        updateFailed += static_cast<int>(updateQueue.size());
        updateQueue.clear();
        finishUpdates();
      }
    } else if (online && selectedCatalogIndex >= 0) {
      beginDownload(selectedCatalogIndex);
    } else {
      requestUpdate();
    }
  });
}

// --- update all ----------------------------------------------------------------

void AppStoreActivity::startUpdateAll() {
  updateQueue.clear();
  for (size_t i = 0; i < kStoreItemCount; ++i)
    if (isOutdated(kStoreItems[i])) updateQueue.push_back(static_cast<int>(i));
  if (updateQueue.empty()) return;
  updating = true;
  updateTotal = static_cast<int>(updateQueue.size());
  updateOk = updateFailed = 0;
  if (WiFi.status() == WL_CONNECTED)
    startNextUpdate();
  else
    launchWifiSetup();  // joins a saved network on its own when one is in range
}

void AppStoreActivity::startNextUpdate() {
  if (updateQueue.empty()) {
    finishUpdates();
    return;
  }
  const int next = updateQueue.front();
  updateQueue.erase(updateQueue.begin());
  beginDownload(next);
}

void AppStoreActivity::finishUpdates() {
  updating = false;
  shelf::refreshApps();
  rebuildRows();
  char msg[160];
  if (updateFailed == 0)
    snprintf(msg, sizeof(msg), isVi() ? "Đã cập nhật %d trò chơi/ứng dụng." : "Updated %d games/apps.", updateOk);
  else
    snprintf(msg, sizeof(msg),
             isVi() ? "Đã cập nhật %d/%d. %d mục chưa cập nhật được, thử lại trong Cửa hàng."
                    : "Updated %d of %d. %d could not be updated; retry from the store.",
             updateOk, updateTotal, updateFailed);
  static const char* const kVi[] = {"Đóng"};
  static const char* const kEn[] = {"Close"};
  optionPopup.showMessage(isVi() ? "Cập nhật ứng dụng" : "Update apps", msg, isVi() ? kVi : kEn, 1, 0,
                          [this](int) {
                            if (updaterMode) pendingAction = PendingAction::Finish;
                          });
  requestUpdate(true);
}

// Records the firmware build the pack was fetched for and, when the saved
// language was waiting for it (running in English since boot), switches to it.
bool AppStoreActivity::languagePackInstalled(const StoreItem& item) {
  Storage.writeFile(languagePackStampPath(item).c_str(), String(firmwareAbiText().c_str()));
  if (I18N.hasLanguagePack() || !languagePackNeeded(item)) return false;
  bool changed = false;
  {
    RenderLock lock(*this);
    changed = uilanguage::applyAtBoot();
  }
  if (changed) SETTINGS.saveToFile();
  return I18N.hasLanguagePack();
}

void AppStoreActivity::executeDownload() {
  if (selectedCatalogIndex < 0 || selectedCatalogIndex >= static_cast<int>(kStoreItemCount)) {
    downloading = false;
    requestUpdate();
    return;
  }

  const StoreItem& item = kStoreItems[selectedCatalogIndex];
  if (item.resources) {
    executeResourceDownload(item);
    return;
  }

  // Check heap before TLS
  if (ESP.getFreeHeap() < HttpDownloader::MIN_TLS_FREE_HEAP ||
      ESP.getMaxAllocHeap() < HttpDownloader::MIN_TLS_MAX_ALLOC) {
    LOG_ERR(TAG, "Low heap before download: %u free, %u max block", ESP.getFreeHeap(), ESP.getMaxAllocHeap());
    downloading = false;
    if (updating) {
      ++updateFailed;
      startNextUpdate();
      return;
    }
    showNotice(isVi() ? "Bộ nhớ quá thấp" : "Low memory",
               isVi() ? "Thiết bị thiếu RAM để mở kết nối bảo mật. Vui lòng thử lại sau."
                      : "Device RAM is low for a secure transfer. Please retry.");
    return;
  }

  Storage.ensureDirectoryExists(item.folder);
  std::string destPath = std::string(item.folder) + "/" + item.filename;
  // Native modules are built per firmware (they link against its exact
  // headers), so they live under this firmware's ABI id.
  std::string url = "https://raw.githubusercontent.com/loitq07/CrossNext-for-Sticky/appstore";
  if (item.module) {
    char abiDir[24];
    snprintf(abiDir, sizeof(abiDir), "/modules/%08x", static_cast<unsigned>(native_module::firmwareAbi()));
    url += abiDir;
  }
  url += std::string(item.folder) + "/" + item.filename;

  LOG_INF(TAG, "Starting download: %s -> %s", url.c_str(), destPath.c_str());

  downloadProgress = 0;
  downloadTotal = item.sizeBytes;
  cancelDownload = false;
  lastRenderedPercent = -1;
  lastProgressUpdateMs = millis();

  auto progressCb = [this](size_t downloaded, size_t total) {
    downloadProgress = downloaded;
    if (total > 0) downloadTotal = total;

    // Pump inputs so Back or a tap anywhere cancels.
    mappedInput.update(true);
    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) cancelDownload = true;
    int tx = 0, ty = 0;
    if (mappedInput.wasScreenTapped(tx, ty)) cancelDownload = true;

    const int percent = (downloadTotal > 0) ? static_cast<int>(downloadProgress * 100 / downloadTotal) : 0;
    const unsigned long now = millis();
    if (percent >= 100 || lastRenderedPercent < 0 || percent >= lastRenderedPercent + 10 ||
        now - lastProgressUpdateMs >= 1500) {
      lastRenderedPercent = percent;
      lastProgressUpdateMs = now;
      requestUpdate(true);
    }
  };

  const HttpDownloader::DownloadError err =
      HttpDownloader::downloadToFile(url, destPath, progressCb, &cancelDownload);

  downloading = false;
  const bool languageActivated = err == HttpDownloader::OK && item.languagePack && languagePackInstalled(item);
  if (updating) {
    if (err == HttpDownloader::OK) {
      ++updateOk;
    } else if (err == HttpDownloader::ABORTED) {
      updateFailed += 1 + static_cast<int>(updateQueue.size());  // cancel stops the whole run
      updateQueue.clear();
    } else {
      ++updateFailed;
    }
    startNextUpdate();
    return;
  }
  if (err == HttpDownloader::OK && item.languagePack) {
    haptic_feedback::touchAction();
    rebuildRows();
    showNotice(isVi() ? "Đã cài đặt" : "Installed",
               languageActivated ? (isVi() ? "Giao diện đã chuyển sang ngôn ngữ vừa tải."
                                           : "The interface now uses the downloaded language.")
                                 : (isVi() ? "Chọn ngôn ngữ này trong Cài đặt > Hệ thống > Ngôn ngữ."
                                           : "Choose it in Settings > System > Language."));
  } else if (err == HttpDownloader::OK) {
    haptic_feedback::touchAction();
    shelf::refreshApps();
    rebuildRows();
    char msg[128];
    snprintf(msg, sizeof(msg), isVi() ? "%s đã sẵn sàng trên kệ Shelf." : "%s is ready on the Shelf.",
             displayName(item));
    showNotice(isVi() ? "Đã cài đặt" : "Installed", msg);
  } else if (err == HttpDownloader::ABORTED) {
    requestUpdate(true);
  } else {
    showNotice(isVi() ? "Tải về thất bại" : "Download failed",
               isVi() ? "Không thể tải ứng dụng. Kiểm tra Wi-Fi rồi thử lại."
                      : "Could not download the app. Check Wi-Fi and retry.");
  }
}

// A resource pack: every file of an app's content folder, fetched from the
// pinned resources release. Files already on the card at the right size are
// kept, so a pack interrupted by a dropped connection resumes where it stopped.
// Each download lands in "<path>.part", is checked against the SHA-256 the
// firmware carries, and only then replaces the real file: a half-written or
// tampered file never takes the place of a working one.
void AppStoreActivity::executeResourceDownload(const StoreItem& item) {
  const ResourcePack& pack = *item.resources;
  auto fail = [this](const char* viMsg, const char* enMsg) {
    downloading = false;
    showNotice(isVi() ? "Tải về thất bại" : "Download failed", isVi() ? viMsg : enMsg);
  };

  if (ESP.getFreeHeap() < HttpDownloader::MIN_TLS_FREE_HEAP ||
      ESP.getMaxAllocHeap() < HttpDownloader::MIN_TLS_MAX_ALLOC) {
    LOG_ERR(TAG, "Low heap before resource download: %u free, %u max block", ESP.getFreeHeap(),
            ESP.getMaxAllocHeap());
    fail("Thiết bị thiếu RAM để mở kết nối bảo mật. Vui lòng thử lại sau.",
         "Device RAM is low for a secure transfer. Please retry.");
    return;
  }

  uint64_t needed = 0;
  for (size_t i = 0; i < pack.count; ++i)
    if (!resourceFilePresent(pack.files[i])) needed += pack.files[i].size;
  uint64_t freeBytes = 0;
  if (needed && Storage.freeBytes(freeBytes) && freeBytes < needed + needed / 20 + 512 * 1024) {
    char msg[160];
    snprintf(msg, sizeof(msg),
             isVi() ? "Thẻ nhớ cần thêm khoảng %u MB trống." : "The card needs about %u MB more free space.",
             static_cast<unsigned>((needed - freeBytes + 1048575) / 1048576 + 1));
    downloading = false;
    showNotice(isVi() ? "Thẻ nhớ đầy" : "Card full", msg);
    return;
  }

  downloadTotal = pack.totalBytes;
  downloadProgress = 0;
  cancelDownload = false;
  lastRenderedPercent = -1;
  lastProgressUpdateMs = millis();
  size_t done = 0;

  for (size_t i = 0; i < pack.count; ++i) {
    const ResourceFile& file = pack.files[i];
    if (resourceFilePresent(file)) {
      done += file.size;
      downloadProgress = done;
      continue;
    }
    Storage.ensureDirectoryExists(parentOf(file.path).c_str());
    const std::string part = std::string(file.path) + ".part";
    const std::string url = std::string(kResourceReleaseUrl) + file.asset;
    LOG_INF(TAG, "Resource %s -> %s", url.c_str(), file.path);

    auto progressCb = [this, done](size_t downloaded, size_t) {
      downloadProgress = done + downloaded;
      mappedInput.update(true);
      if (mappedInput.wasReleased(MappedInputManager::Button::Back)) cancelDownload = true;
      int tx = 0, ty = 0;
      if (mappedInput.wasScreenTapped(tx, ty)) cancelDownload = true;
      const int percent = downloadTotal ? static_cast<int>(downloadProgress * 100 / downloadTotal) : 0;
      const unsigned long now = millis();
      if (percent >= 100 || lastRenderedPercent < 0 || percent >= lastRenderedPercent + 5 ||
          now - lastProgressUpdateMs >= 1500) {
        lastRenderedPercent = percent;
        lastProgressUpdateMs = now;
        requestUpdate(true);
      }
    };
    const HttpDownloader::DownloadError err = HttpDownloader::downloadToFile(url, part, progressCb, &cancelDownload);
    if (err == HttpDownloader::ABORTED) {
      Storage.remove(part.c_str());
      downloading = false;
      rebuildRows();
      requestUpdate(true);
      return;
    }
    if (err != HttpDownloader::OK) {
      Storage.remove(part.c_str());
      rebuildRows();
      fail("Không tải được tài nguyên. Kiểm tra Wi-Fi rồi thử lại; phần đã tải được giữ lại.",
           "Could not download the resources. Check Wi-Fi and retry; finished files are kept.");
      return;
    }
    if (!fileMatchesSha256(part.c_str(), file.sha256)) {
      LOG_ERR(TAG, "Checksum mismatch: %s", file.path);
      Storage.remove(part.c_str());
      rebuildRows();
      fail("Tệp tải về bị lỗi (sai mã kiểm tra). Vui lòng thử lại.",
           "A downloaded file was damaged (checksum mismatch). Please retry.");
      return;
    }
    Storage.remove(file.path);
    if (!Storage.rename(part.c_str(), file.path)) {
      Storage.remove(part.c_str());
      rebuildRows();
      fail("Không ghi được tệp lên thẻ nhớ. Kiểm tra thẻ rồi thử lại.",
           "Could not write to the card. Check it and retry.");
      return;
    }
    done += file.size;
    downloadProgress = done;
  }

  downloading = false;
  haptic_feedback::touchAction();
  rebuildRows();
  char msg[160];
  snprintf(msg, sizeof(msg),
           isVi() ? "Đã cài %s vào %s." : "%s installed in %s.", displayName(item), pack.folder);
  showNotice(isVi() ? "Đã cài đặt" : "Installed", msg);
}
