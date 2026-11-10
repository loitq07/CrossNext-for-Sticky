#pragma once

#include <string>
#include <vector>

#include "CrossPointSettings.h"
#include "activities/UiListActivity.h"

class BluetoothActivity final : public UiListActivity {
 public:
  explicit BluetoothActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;
  void onExit() override;
  void loop() override;

 protected:
  int listCount() const override;
  void buildScreen(UiScreen& screen) override;
  void activateIndex(int index) override;
  const char* headerTitle() const override;

 private:
  enum class RowType {
    ToggleState,
    ScanAction,
    PairedDevice,
    ForgetPaired,
    DiscoveredDevice
  };

  struct RowEntry {
    RowType type;
    std::string label;
    std::string subtitle;
    std::string sectionHeading;
    std::string addr;
    std::string value;
    bool isToggle = false;
    bool toggleChecked = false;
  };

  std::vector<RowEntry> rows_;
  std::vector<freeink::ui::ListItem> rowItems_;

  uint8_t lastDeviceCount_ = 0;
  bool lastConnected_ = false;
  bool lastConnecting_ = false;
  bool lastScanning_ = false;
  std::string lastFailureReason_;
  uint32_t lastPoll_ = 0;

  void rebuildRows();
};
