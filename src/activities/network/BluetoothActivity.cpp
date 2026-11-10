#if FREEINK_CAP_BLE_HID_HOST
#include "BluetoothActivity.h"

#include <BleKeyboardHost.h>
#include <GfxRenderer.h>
#include <I18n.h>

#include "I18nKeys.h"
#include "MappedInputManager.h"
#include "apps_local/ui/PixelMenuTheme.h"
#include "components/UITheme.h"

namespace fui = freeink::ui;

BluetoothActivity::BluetoothActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : UiListActivity("Bluetooth", renderer, mappedInput, false, true) {}

void BluetoothActivity::onEnter() {
  UiListActivity::onEnter();
  if (SETTINGS.bluetooth) {
    if (!BleHid.isRunning()) {
      BleHid.begin("CrossNext");
      if (SETTINGS.bluetoothDevice[0] != '\0') {
        BleHid.armSelectedPeerReconnect(SETTINGS.bluetoothDevice);
      }
    }
    BleHid.startScan(8000);
  }
  rebuildRows();
}

void BluetoothActivity::onExit() {
  if (BleHid.isScanning()) {
    BleHid.stopScan();
  }
  UiListActivity::onExit();
}

void BluetoothActivity::loop() {
  UiListActivity::loop();
  if (BleHid.isRunning()) {
    BleHid.poll();
  }
  const uint32_t now = millis();
  if (now - lastPoll_ >= 200) {
    lastPoll_ = now;
    const bool scanning = BleHid.isScanning();
    const bool connected = BleHid.isConnected();
    const bool connecting = BleHid.isConnecting();
    const uint8_t devCount = BleHid.deviceCount();

    char failReason[48] = {0};
    const bool hasFailure = BleHid.takeConnectFailure(failReason, sizeof(failReason));
    if (hasFailure) {
      lastFailureReason_ = failReason;
    } else if (connected) {
      lastFailureReason_.clear();
    }

    if (scanning != lastScanning_ || connected != lastConnected_ ||
        connecting != lastConnecting_ || devCount != lastDeviceCount_ || hasFailure) {
      lastScanning_ = scanning;
      lastConnected_ = connected;
      lastConnecting_ = connecting;
      lastDeviceCount_ = devCount;
      rebuildRows();
      requestUpdate();
    }
  }
}

int BluetoothActivity::listCount() const {
  return static_cast<int>(rows_.size());
}

const char* BluetoothActivity::headerTitle() const {
  return I18N.get(StrId::STR_BLUETOOTH);
}

void BluetoothActivity::rebuildRows() {
  rows_.clear();
  rowItems_.clear();

  // Row 0: Toggle on/off
  RowEntry toggleRow;
  toggleRow.type = RowType::ToggleState;
  toggleRow.label = I18N.get(StrId::STR_BLUETOOTH);
  if (SETTINGS.bluetooth) {
    if (BleHid.isConnected()) {
      const char* name =
          SETTINGS.bluetoothDeviceName[0] != '\0' ? SETTINGS.bluetoothDeviceName : SETTINGS.bluetoothDevice;
      toggleRow.subtitle = std::string(I18N.get(StrId::STR_BLUETOOTH_CONNECTED)) + ": " + name;
    } else {
      toggleRow.subtitle = I18N.get(StrId::STR_STATE_ON);
    }
  } else {
    toggleRow.subtitle = I18N.get(StrId::STR_STATE_OFF);
  }
  toggleRow.isToggle = true;
  toggleRow.toggleChecked = (SETTINGS.bluetooth != 0);
  rows_.push_back(std::move(toggleRow));

  if (SETTINGS.bluetooth) {
    const bool isScanning = BleHid.isScanning();

    // Row 1: Scan action
    RowEntry scanRow;
    scanRow.type = RowType::ScanAction;
    scanRow.label = isScanning ? I18N.get(StrId::STR_BLUETOOTH_SCANNING) : I18N.get(StrId::STR_BLUETOOTH_SCAN);
    scanRow.subtitle = isScanning
                           ? (I18N.getLanguage() == Language::VI ? "Đang tìm kiếm thiết bị lân cận..."
                                                                 : "Searching for nearby devices...")
                           : (I18N.getLanguage() == Language::VI ? "Chạm để quét tìm thiết bị"
                                                                 : "Tap to scan for devices");
    scanRow.value = isScanning ? "..." : ">";
    rows_.push_back(std::move(scanRow));

    // Paired device
    if (SETTINGS.bluetoothDevice[0] != '\0') {
      RowEntry pairedRow;
      pairedRow.type = RowType::PairedDevice;
      pairedRow.sectionHeading = I18N.get(StrId::STR_BLUETOOTH_PAIRED);
      pairedRow.label =
          SETTINGS.bluetoothDeviceName[0] != '\0' ? SETTINGS.bluetoothDeviceName : SETTINGS.bluetoothDevice;
      if (BleHid.isConnected()) {
        pairedRow.subtitle = I18N.get(StrId::STR_BLUETOOTH_CONNECTED);
        pairedRow.value = I18N.getLanguage() == Language::VI ? "Ngắt" : "Disconnect";
      } else if (BleHid.isConnecting()) {
        pairedRow.subtitle = I18N.get(StrId::STR_BLUETOOTH_CONNECTING);
        pairedRow.value = "...";
      } else if (!lastFailureReason_.empty()) {
        pairedRow.subtitle = (I18N.getLanguage() == Language::VI ? "Lỗi: " : "Error: ") + lastFailureReason_;
        pairedRow.value = I18N.getLanguage() == Language::VI ? "Thử lại" : "Retry";
      } else {
        pairedRow.subtitle = I18N.get(StrId::STR_BLUETOOTH_DISCONNECTED);
        pairedRow.value = I18N.getLanguage() == Language::VI ? "Kết nối" : "Connect";
      }
      pairedRow.addr = SETTINGS.bluetoothDevice;
      rows_.push_back(std::move(pairedRow));

      RowEntry forgetRow;
      forgetRow.type = RowType::ForgetPaired;
      forgetRow.label = I18N.get(StrId::STR_BLUETOOTH_FORGET);
      forgetRow.subtitle =
          I18N.getLanguage() == Language::VI ? "Xóa ghép đôi thiết bị này" : "Remove this paired device";
      forgetRow.value = ">";
      forgetRow.addr = SETTINGS.bluetoothDevice;
      rows_.push_back(std::move(forgetRow));
    }

    // Discovered devices
    const uint8_t count = BleHid.deviceCount();
    uint8_t addedCount = 0;
    bool firstDiscovered = true;
    for (uint8_t i = 0; i < count; ++i) {
      const auto& dev = BleHid.device(i);
      // Only display devices that have an advertised name or expose the HID service
      if (!dev.hasName && !dev.hid) {
        continue;
      }
      if (SETTINGS.bluetoothDevice[0] != '\0' && strcmp(dev.addr, SETTINGS.bluetoothDevice) == 0) {
        continue;
      }
      RowEntry devRow;
      devRow.type = RowType::DiscoveredDevice;
      if (firstDiscovered) {
        devRow.sectionHeading = I18N.get(StrId::STR_BLUETOOTH_AVAILABLE);
        firstDiscovered = false;
      }
      devRow.label = dev.name[0] ? dev.name : dev.addr;
      devRow.subtitle = std::string(dev.addr) + " · " + std::to_string(dev.rssi) + " dBm";
      devRow.value = I18N.getLanguage() == Language::VI ? "Ghép đôi" : "Pair";
      devRow.addr = dev.addr;
      rows_.push_back(std::move(devRow));
      ++addedCount;
    }

    // If no devices discovered yet, display helpful status entry
    if (addedCount == 0) {
      RowEntry emptyRow;
      emptyRow.type = RowType::ScanAction;
      emptyRow.sectionHeading = I18N.get(StrId::STR_BLUETOOTH_AVAILABLE);
      if (isScanning) {
        emptyRow.label = I18N.get(StrId::STR_BLUETOOTH_SCANNING);
        emptyRow.subtitle = I18N.getLanguage() == Language::VI ? "Vui lòng bật remote ở chế độ ghép đôi..."
                                                               : "Turn remote on in pairing mode...";
        emptyRow.value = "...";
      } else {
        emptyRow.label = I18N.get(StrId::STR_BLUETOOTH_NO_DEVICES);
        emptyRow.subtitle =
            I18N.getLanguage() == Language::VI ? "Chạm để quét tìm thiết bị" : "Tap to scan for devices";
        emptyRow.value = ">";
      }
      rows_.push_back(std::move(emptyRow));
    }
  }

  rowItems_.reserve(rows_.size());
  for (size_t i = 0; i < rows_.size(); ++i) {
    fui::ListItem item;
    item.label = rows_[i].label.c_str();
    if (!rows_[i].subtitle.empty()) {
      item.subtitle = rows_[i].subtitle.c_str();
    }
    if (!rows_[i].sectionHeading.empty()) {
      item.sectionHeading = rows_[i].sectionHeading.c_str();
    }
    if (!rows_[i].value.empty()) {
      item.value = rows_[i].value.c_str();
    }
    if (rows_[i].isToggle) {
      GUI.setCheckboxRow(item, rows_[i].toggleChecked);
    }
    item.actionValue = static_cast<int16_t>(i);
    rowItems_.push_back(item);
  }
}

void BluetoothActivity::activateIndex(const int index) {
  app.clearTapFlash();
  if (index < 0 || index >= static_cast<int>(rows_.size())) return;

  const auto& row = rows_[index];
  switch (row.type) {
    case RowType::ToggleState: {
      SETTINGS.bluetooth = !SETTINGS.bluetooth;
      SETTINGS.saveToFile();
      if (SETTINGS.bluetooth) {
        if (!BleHid.isRunning()) {
          BleHid.begin("CrossNext");
        }
        if (SETTINGS.bluetoothDevice[0] != '\0') {
          BleHid.armSelectedPeerReconnect(SETTINGS.bluetoothDevice);
        }
        BleHid.startScan(8000);
      } else {
        BleHid.end(1000);
      }
      lastScanning_ = BleHid.isScanning();
      rebuildRows();
      requestUpdate();
      break;
    }
    case RowType::ScanAction: {
      if (BleHid.isScanning()) {
        BleHid.stopScan();
      } else {
        BleHid.startScan(8000);
      }
      lastScanning_ = BleHid.isScanning();
      rebuildRows();
      requestUpdate();
      break;
    }
    case RowType::PairedDevice: {
      if (BleHid.isConnected()) {
        BleHid.disconnect();
      } else {
        lastFailureReason_.clear();
        BleHid.connect(row.addr.c_str());
      }
      rebuildRows();
      requestUpdate();
      break;
    }
    case RowType::ForgetPaired: {
      BleHid.forget(row.addr.c_str());
      lastFailureReason_.clear();
      SETTINGS.bluetoothDevice[0] = '\0';
      SETTINGS.bluetoothDeviceName[0] = '\0';
      SETTINGS.saveToFile();
      rebuildRows();
      requestUpdate();
      break;
    }
    case RowType::DiscoveredDevice: {
      BleHid.stopScan();
      lastFailureReason_.clear();
      strncpy(SETTINGS.bluetoothDevice, row.addr.c_str(), sizeof(SETTINGS.bluetoothDevice) - 1);
      SETTINGS.bluetoothDevice[sizeof(SETTINGS.bluetoothDevice) - 1] = '\0';
      strncpy(SETTINGS.bluetoothDeviceName, row.label.c_str(), sizeof(SETTINGS.bluetoothDeviceName) - 1);
      SETTINGS.bluetoothDeviceName[sizeof(SETTINGS.bluetoothDeviceName) - 1] = '\0';
      SETTINGS.saveToFile();
      BleHid.connect(row.addr.c_str());
      rebuildRows();
      requestUpdate();
      break;
    }
  }
}

void BluetoothActivity::buildScreen(UiScreen& screen) {
  pixelui::FontScope fonts(uiTarget, renderer, I18N.getLanguage() != Language::EN);
  useCompactSettingsRows();
  const int hints = mappedInput.hasTouch() ? 0 : UITheme::getInstance().getMetrics().buttonHintsHeight;
  pixelui::begin(screen, hints);

  fui::ListProps props;
  props.items = rowItems_.data();
  props.count = static_cast<uint16_t>(rowItems_.size());
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch;
  pixelui::styleList(props);
  props.valueInset = 8;
  props.labelText = screen.theme().smallText;
  props.labelText.bold = true;
  props.labelText.maxLines = 2;
  props.subtitleText.font = 4;
  props.subtitleText.maxLines = 2;
  syncListViewport(screen, props);
  pixelui::list(screen, props);
}

#endif
