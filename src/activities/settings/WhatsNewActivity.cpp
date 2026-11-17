#include "WhatsNewActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include "CrossPointState.h"
#include "MappedInputManager.h"
#include "apps_local/ui/PixelMenuTheme.h"
#include "components/UITheme.h"

namespace fui = freeink::ui;

WhatsNewActivity::WhatsNewActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : UiListActivity("WhatsNew", renderer, mappedInput, false, true) {}

const char* WhatsNewActivity::headerTitle() const {
  return I18N.getLanguage() != Language::EN ? "Có gì mới (v" CROSSPOINT_VERSION ")" : "What's New (v" CROSSPOINT_VERSION ")";
}

void WhatsNewActivity::onEnter() {
  UiListActivity::onEnter();
  const bool isVi = I18N.getLanguage() != Language::EN;

  if (isVi) {
    rowLabels_[0] = "1. Kho ứng dụng App Store";
    rowSubtitles_[0] = "Cài đặt & cập nhật game, app trực tiếp";

    rowLabels_[1] = "2. Ghi âm (Voice Notes)";
    rowSubtitles_[1] = "Ghi âm qua mic tích hợp, chuyển văn bản";

    rowLabels_[2] = "3. Instapaper & Máy tính";
    rowSubtitles_[2] = "Đọc báo offline & máy tính tiện lợi";

    rowLabels_[3] = "4. Web truyền tệp mới";
    rowSubtitles_[3] = "Giao diện tab hiện đại, xem ảnh trực tiếp";

    rowLabels_[4] = "5. Tối ưu hệ thống & sửa lỗi";
    rowSubtitles_[4] = "Sửa lỗi đơ SPI tải file, nhận diện màn hình";

    rowLabels_[5] = "✓ Đóng & Tiếp tục sử dụng";
    rowSubtitles_[5] = "Chạm hoặc bấm Confirm để vào trang chủ";
  } else {
    rowLabels_[0] = "1. Native App Store";
    rowSubtitles_[0] = "Browse & install on-device games and apps";

    rowLabels_[1] = "2. Voice Notes";
    rowSubtitles_[1] = "Audio recording via mic & transcription";

    rowLabels_[2] = "3. Instapaper & Calculator";
    rowSubtitles_[2] = "Read saved articles offline & calculator";

    rowLabels_[3] = "4. Modern Web Portal";
    rowSubtitles_[3] = "Tabbed UI, image preview & inline download";

    rowLabels_[4] = "5. System Fixes & Stability";
    rowSubtitles_[4] = "Fix SPI freeze on download, auto display detect";

    rowLabels_[5] = "✓ Done & Continue";
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
  pixelui::FontScope fonts(uiTarget, renderer, I18N.getLanguage() != Language::EN);
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
