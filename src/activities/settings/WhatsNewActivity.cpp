#include "WhatsNewActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include "CrossPointSettings.h"
#include "CrossPointState.h"
#include "MappedInputManager.h"
#include "apps_local/ui/PixelMenuTheme.h"
#include "components/UITheme.h"

namespace fui = freeink::ui;

namespace {
// What's New opens on the first boot after an update, before the App Store has
// fetched the Vietnamese pack (English is the only built-in language), so it
// also follows the saved language. Its strings are literals, not pack text.
bool isViText() {
  return I18N.getLanguage() == Language::VI ||
         (!I18N.hasLanguagePack() && SETTINGS.language == static_cast<uint8_t>(Language::VI));
}
}  // namespace

WhatsNewActivity::WhatsNewActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : UiListActivity("WhatsNew", renderer, mappedInput, false, true) {}

const char* WhatsNewActivity::headerTitle() const {
  return isViText() ? "Có gì mới (v" CROSSPOINT_VERSION ")" : "What's New (v" CROSSPOINT_VERSION ")";
}

void WhatsNewActivity::onEnter() {
  UiListActivity::onEnter();
  const bool isVi = isViText();

  if (isVi) {
    rowLabels_[0] = "1. Bảng Quick Settings nhiều toggle cho Sticky";
    rowSubtitles_[0] = "Bật/tắt nhanh Wi-Fi, Bluetooth, Âm phím, Nghiêng lật trang";

    rowLabels_[1] = "2. Chỉ báo vuốt cạnh màn hình (kiểu Boox)";
    rowSubtitles_[1] = "Hiện tai nổi & icon Back/Home/Menu theo vị trí vuốt";

    rowLabels_[2] = "3. Sửa lỗi chạy app ngoài từ App Store";
    rowSubtitles_[2] = "Liên kết lại Dice & External Apps SDK (-nostartfiles)";

    rowLabels_[3] = "4. Book Preparer Desktop & tuỳ biến Sticky";
    rowSubtitles_[3] = "Đổi nhãn thiết bị sang Sticky, tuỳ chỉnh 4 ô hàng đầu";

    rowLabels_[4] = "5. Nền tảng v2.0.5 (23 game Pocket Play)";
    rowSubtitles_[4] = "Giao diện CrossNext CDS, dòng kẻ sách & remote BLE";

    rowLabels_[5] = "Đóng & tiếp tục sử dụng";
    rowSubtitles_[5] = "Chạm hoặc bấm Confirm để vào trang chủ";
  } else {
    rowLabels_[0] = "1. Multi-toggle Quick Settings panel on Sticky";
    rowSubtitles_[0] = "Toggle Wi-Fi, Bluetooth, Key Sound, Tilt Turn & Night Mode";

    rowLabels_[1] = "2. Edge swipe visual indicator";
    rowSubtitles_[1] = "Floating Boox-style pill with Back/Home/Menu/Panel icon";

    rowLabels_[2] = "3. Fixed App Store external app launch";
    rowSubtitles_[2] = "Updated Dice & External Apps SDK link flags (-nostartfiles)";

    rowLabels_[3] = "4. Desktop Preparer & Quick Settings customizer";
    rowSubtitles_[3] = "Sticky sidebar branding and hardware-filtered slots";

    rowLabels_[4] = "5. CrossNext v2.0.5 base (23 Pocket Play games)";
    rowSubtitles_[4] = "CDS game UI sync, ruled reading lines & BLE remotes";

    rowLabels_[5] = "Done & continue";
    rowSubtitles_[5] = "Tap or press Confirm to enter Home screen";
  }

  for (int i = 0; i < ITEM_COUNT; i++) {
    rowItems_[i].label = rowLabels_[i].c_str();
    rowItems_[i].subtitle = rowSubtitles_[i].c_str();
    rowItems_[i].actionValue = static_cast<int16_t>(i);
    rowItems_[i].labelBold = (i == ITEM_COUNT - 1);
  }
}

void WhatsNewActivity::buildScreen(UiScreen& screen) {
  pixelui::FontScope fonts(uiTarget, renderer, isViText() || I18N.getLanguage() != Language::EN);
  useCompactSettingsRows();
  const auto& metrics = UITheme::getInstance().getMetrics();
  pixelui::begin(screen, mappedInput.hasTouch() ? 0 : metrics.buttonHintsHeight);

  fui::ListProps props;
  props.items = rowItems_;
  props.count = ITEM_COUNT;
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch;
  props.valueInset = 8;
  props.labelText = screen.theme().smallText;
  props.labelText.maxLines = 1;
  syncListViewport(screen, props);
  pixelui::list(screen, props);
}

void WhatsNewActivity::activateIndex(int) {
  dismiss();
}

void WhatsNewActivity::onBackButton() {
  dismiss();
}

void WhatsNewActivity::dismiss() {
  APP_STATE.lastSeenVersion = CROSSPOINT_VERSION;
  APP_STATE.saveToFile();
  finish();
}
