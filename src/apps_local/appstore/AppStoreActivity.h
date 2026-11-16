#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../../activities/UiTabListActivity.h"
#include "../../components/OptionPopup.h"
#include "AppStoreCatalog.h"

// App Store built on the shared tab/list host (same chrome as Settings), so the
// header, tab bar, rows and dialogs follow the system theme and interface font
// instead of hand-placed rectangles and reading-face text.
class AppStoreActivity final : public UiTabListActivity {
 public:
  explicit AppStoreActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  ~AppStoreActivity() override = default;

  static std::unique_ptr<Activity> create(GfxRenderer& renderer, MappedInputManager& mappedInput);

  // The store opened straight into "update everything": after a firmware
  // update (from Home, once per firmware build) or when a module built for
  // another firmware is opened from the shelf. fromShelf picks how it leaves.
  static std::unique_ptr<Activity> createUpdater(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                                 bool fromShelf);

  // Installed native modules built for another firmware (reads file trailers).
  static int outdatedCount();
  // True once per firmware build when modules need updating; marks it asked.
  static bool takeBootPrompt();

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

  bool preventAutoSleep() override;
  bool skipLoopDelay() override;

 private:
  // Follow-up work chosen inside a popup callback. Deferred to loop() because a
  // popup cannot safely re-open itself from inside its own callback.
  enum class PendingAction : uint8_t { None, Open, ConfirmDelete, Delete, WifiSetup, UpdateAll, Finish };

  // --- UiTabListActivity contract ---
  int listCount() const override { return static_cast<int>(rows_.size()); }
  int tabCount() const override { return 3; }
  int activeTab() const override { return static_cast<int>(currentCategory); }
  const char* tabLabel(int index) const override;
  void buildScreen(UiScreen& screen) override;
  void activateIndex(int index) override;
  void onTabAction(int index) override;
  void stepTab(int direction) override;
  void drawChrome() override;
  bool handleButtons() override;
  bool handleCustomInput() override;

  void rebuildRows();
  void selectCategory(StoreCategory category);
  void showNotice(const char* title, const char* message);
  void showItemActions(int catalogIndex);
  void runPendingAction();
  void drawDownloadPopup();

  void startUpdateAll();
  void startNextUpdate();
  void finishUpdates();
  void leave();
  void showUpdatePrompt();

  void executeDownload();
  void checkWifiAndDownload(int catalogIndex);
  void beginDownload(int catalogIndex);
  void launchWifiSetup();

  bool isItemInstalled(const StoreItem& item) const;
  std::string getInstalledPath(const StoreItem& item) const;
  const char* displayName(const StoreItem& item) const;

  // Updater mode and the update-all queue.
  bool updaterMode = false;
  bool exitViaShelf = true;
  bool updating = false;
  std::vector<int> updateQueue;
  int updateTotal = 0;
  int updateOk = 0;
  int updateFailed = 0;
  std::string updateAllLabel_;

  StoreCategory currentCategory = StoreCategory::All;
  int selectedCatalogIndex = -1;
  PendingAction pendingAction = PendingAction::None;
  OptionPopup optionPopup;

  // Rows of the active tab: catalog index, cached install state, and the
  // strings the ListItems point into (rebuilt only when the tab or an install
  // state changes, never per frame).
  struct Row {
    int catalogIndex = 0;  // -1: the "update all" row
    bool installed = false;
    bool outdated = false;  // a native module built for another firmware
  };
  std::vector<Row> rows_;
  std::vector<freeink::ui::ListItem> rowItems_;

  // Download state
  bool downloading = false;
  size_t downloadProgress = 0;
  size_t downloadTotal = 0;
  int lastRenderedPercent = -1;
  unsigned long lastProgressUpdateMs = 0;
  bool cancelDownload = false;
  bool startDownloadPending = false;
};
