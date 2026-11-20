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
    rowLabels_[0] = "1. Đồng bộ giao diện CrossNext cho 23 game";
    rowSubtitles_[0] = "Header chuẩn CDS với pin, đồng hồ, nút bo góc r=6";

    rowLabels_[1] = "2. Thêm 23 game mới từ Pocket Play";
    rowSubtitles_[1] = "Tải từ App Store: Sokoban, Zork, Suguru, Queens...";

    rowLabels_[2] = "3. Dòng kẻ chấm dưới chữ trong sách";
    rowSubtitles_[2] = "Bật dòng kẻ trang vở, dễ theo dõi dòng đọc";

    rowLabels_[3] = "4. App Store: Tab Ngôn ngữ & Tài nguyên";
    rowSubtitles_[3] = "Tách tab riêng tải gói tiếng Việt và đồ họa";

    rowLabels_[4] = "5. Điều khiển Bluetooth & Remote";
    rowSubtitles_[4] = "Ghép remote BLE, gán phím và lật trang";

    rowLabels_[5] = "Đóng & tiếp tục sử dụng";
    rowSubtitles_[5] = "Chạm hoặc bấm Confirm để vào trang chủ";
  } else {
    rowLabels_[0] = "1. CrossNext CDS UI sync for 23 games";
    rowSubtitles_[0] = "Native header with battery, clock, and r=6 rounded buttons";

    rowLabels_[1] = "2. 23 new games from Pocket Play";
    rowSubtitles_[1] = "Download from App Store: Sokoban, Zork, Suguru...";

    rowLabels_[2] = "3. Dotted line guides in reader";
    rowSubtitles_[2] = "Ruled-paper look, easier to follow reading";

    rowLabels_[2] = "3. CrossNext 2.0 foundation";
    rowSubtitles_[2] = "Reader and library updates from 2.0.3";

    rowLabels_[3] = "4. App Store: Languages & Resources";
    rowSubtitles_[3] = "Dedicated tabs for language packs and assets";

    rowLabels_[4] = "5. Bluetooth & remotes";
    rowSubtitles_[4] = "Pair BLE remotes, map keys and turn pages";

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
