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
    rowLabels_[0] = "1. Đủ 38 ngôn ngữ trên App Store";
    rowSubtitles_[0] = "Tải trực tiếp gói ngôn ngữ (UK, ES, JA, FR, DE...) và font giao diện";

    rowLabels_[1] = "2. Bảng Quick Settings 8 nút mới & Chụp màn hình";
    rowSubtitles_[1] = "H1: Wi-Fi/BT/Âm phím/Tối; H2: Làm mới/Chụp ảnh/Xoay/Ngủ";

    rowLabels_[2] = "3. Đồng bộ icon & bỏ Tilt trên Sticky";
    rowSubtitles_[2] = "Night Mode dùng icon Trắng/Đen đồng bộ Cài đặt";

    rowLabels_[3] = "4. Chỉ báo vuốt cạnh & sửa lỗi màn đen đọc sách";
    rowSubtitles_[3] = "Sửa trang EPUB hóa đen khi kéo cạnh trên lúc bật làm mịn chữ";

    rowLabels_[4] = "5. Sửa lỗi app ngoài & Book Preparer Sticky";
    rowSubtitles_[4] = "Liên kết lại Dice SDK (-nostartfiles), sửa nhãn Sticky";

    rowLabels_[5] = "Đóng & tiếp tục sử dụng";
    rowSubtitles_[5] = "Chạm hoặc bấm Confirm để vào trang chủ";
  } else {
    rowLabels_[0] = "1. All 38 interface languages on App Store";
    rowSubtitles_[0] = "Download any CrossNext language pack (UK, ES, JA, FR, DE...)";

    rowLabels_[1] = "2. Sticky Quick Settings (8 tiles) & Screenshot";
    rowSubtitles_[1] = "R1: Wi-Fi/BT/KeySound/Dark; R2: Refresh/Shot/Rotate/Sleep";

    rowLabels_[2] = "3. Black/White Night Mode icon & no Tilt";
    rowSubtitles_[2] = "Synced Settings Contrast icon and removed Tilt on Sticky";

    rowLabels_[3] = "4. Edge swipe indicator & reader black-page fix";
    rowSubtitles_[3] = "Fixed AA EPUB page turning black on top-edge swipe";

    rowLabels_[4] = "5. App Store SDK fix & Sticky Desktop branding";
    rowSubtitles_[4] = "Linked external apps (-nostartfiles) & Sticky sidebar label";

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
