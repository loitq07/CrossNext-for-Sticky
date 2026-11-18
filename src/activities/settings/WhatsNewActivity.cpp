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
    rowLabels_[0] = "1. Tiếng Việt trên thẻ nhớ";
    rowSubtitles_[0] = "Tự tải qua App Store, firmware nhẹ hơn";

    rowLabels_[1] = "2. Cách mở cài đặt sách";
    rowSubtitles_[1] = "Vuốt lên, chạm giữa trang hoặc chạm mép";

    rowLabels_[2] = "3. Nhận bản thử nghiệm (Beta)";
    rowSubtitles_[2] = "Bật trong Cài đặt > Hệ thống";

    rowLabels_[3] = "4. Menu mở nhanh hơn";
    rowSubtitles_[3] = "Cài đặt, Trò chơi, Ứng dụng mở nhanh hơn";

    rowLabels_[4] = "5. Web truyền tệp offline";
    rowSubtitles_[4] = "Icon không cần Internet, kéo thả để tải lên";

    rowLabels_[5] = "Đóng & tiếp tục sử dụng";
    rowSubtitles_[5] = "Chạm hoặc bấm Confirm để vào trang chủ";
  } else {
    rowLabels_[0] = "1. Vietnamese on the microSD";
    rowSubtitles_[0] = "Fetched by the App Store; lighter firmware";

    rowLabels_[1] = "2. Open book settings your way";
    rowSubtitles_[1] = "Swipe up, tap page center or tap an edge";

    rowLabels_[2] = "3. Beta updates";
    rowSubtitles_[2] = "Opt in under Settings > System";

    rowLabels_[3] = "4. Faster menus";
    rowSubtitles_[3] = "Settings, Games and Apps open faster";

    rowLabels_[4] = "5. Offline web file portal";
    rowSubtitles_[4] = "Icons without Internet, drag & drop uploads";

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
