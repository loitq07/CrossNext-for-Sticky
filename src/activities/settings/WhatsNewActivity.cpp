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
    rowLabels_[0] = "1. Nâng cấp nền CrossNext 2.0";
    rowSubtitles_[0] = "Kế thừa reader và thư viện từ bản 2.0.3";

    rowLabels_[1] = "2. Cài đặt riêng cho từng sách";
    rowSubtitles_[1] = "Tùy chỉnh chữ, giãn dòng và thanh đọc";

    rowLabels_[2] = "3. Điều khiển Bluetooth mới";
    rowSubtitles_[2] = "Ghép remote, gán phím và lật trang";

    rowLabels_[3] = "4. Ứng dụng FOCUS";
    rowSubtitles_[3] = "Quản lý dự án, công việc và giờ tập trung";

    rowLabels_[4] = "5. Thư viện và plugin";
    rowSubtitles_[4] = "Yêu thích, bộ sưu tập và chi tiết plugin";

    rowLabels_[5] = "Đóng & tiếp tục sử dụng";
    rowSubtitles_[5] = "Chạm hoặc bấm Confirm để vào trang chủ";
  } else {
    rowLabels_[0] = "1. CrossNext 2.0 foundation";
    rowSubtitles_[0] = "Reader and library updates from 2.0.3";

    rowLabels_[1] = "2. Per-book reading settings";
    rowSubtitles_[1] = "Customize text, spacing and reading bars";

    rowLabels_[2] = "3. New Bluetooth controls";
    rowSubtitles_[2] = "Pair remotes, map keys and turn pages";

    rowLabels_[3] = "4. FOCUS app";
    rowSubtitles_[3] = "Projects, tasks and focus sessions";

    rowLabels_[4] = "5. Library and plugins";
    rowSubtitles_[4] = "Favorites, collections and plugin details";

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
