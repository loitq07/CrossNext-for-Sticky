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
    rowLabels_[0] = "1. Sửa game tải về không mở được";
    rowSubtitles_[0] = "Hiện đủ tên, mở được Xúc Xắc, Đố Vui…";

    rowLabels_[1] = "2. Hearts kiểu CrossPlay";
    rowSubtitles_[1] = "Điểm ván, lượt n/13, bàn la bàn";

    rowLabels_[2] = "3. Thêm 14 game/app vào kệ";
    rowSubtitles_[2] = "Toy Battle, Yahtzee, XKCD, Wikipedia…";

    rowLabels_[3] = "4. Nâng cấp nền CrossNext 2.0";
    rowSubtitles_[3] = "Kế thừa reader và thư viện từ bản 2.0.3";

    rowLabels_[4] = "5. Điều khiển Bluetooth & Remote";
    rowSubtitles_[4] = "Ghép remote BLE, gán phím và lật trang";

    rowLabels_[5] = "Đóng & tiếp tục sử dụng";
    rowSubtitles_[5] = "Chạm hoặc bấm Confirm để vào trang chủ";
  } else {
    rowLabels_[0] = "1. Fixed store games not opening";
    rowSubtitles_[0] = "Names show and Dice, Trivia… launch";

    rowLabels_[1] = "2. CrossPlay-style Hearts";
    rowSubtitles_[1] = "Round score, trick n/13, compass table";

    rowLabels_[2] = "3. 14 more games/apps on the shelf";
    rowSubtitles_[2] = "Toy Battle, Yahtzee, XKCD, Wikipedia…";

    rowLabels_[3] = "4. CrossNext 2.0 foundation";
    rowSubtitles_[3] = "Reader and library updates from 2.0.3";

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
