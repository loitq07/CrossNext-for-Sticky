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
    rowLabels_[0] = "1. Tùy biến Quick Settings (tối đa 3 hàng / 12 mục)";
    rowSubtitles_[0] = "Cài đặt > Hiển thị > Quick Settings: ẩn/hiện, xếp vị trí";

    rowLabels_[1] = "2. Chuyển mục Home Menu & +Game/+App lên Quick Settings";
    rowSubtitles_[1] = "Files, Thư viện, Reading Stats, Transfer, Read Later, Plugins";

    rowLabels_[2] = "3. Sửa nhãn BLUETOOTH, Screenshot & icon Night Mode";
    rowSubtitles_[2] = "Hiển thị BLUETOOTH/Screenshot chuẩn, đồng bộ icon Trắng/Đen";

    rowLabels_[3] = "4. Đủ 38 ngôn ngữ trên App Store & chỉ báo vuốt cạnh";
    rowSubtitles_[3] = "Tải gói ngôn ngữ trên App Store, sửa màn đen đọc sách AA";

    rowLabels_[4] = "5. Hỗ trợ mở thẳng Plugins/Game/App từ bảng kéo xuống";
    rowSubtitles_[4] = "Thêm lối tắt +Add Game / +Add App ngay trong Quick Settings";

    rowLabels_[5] = "Đóng & tiếp tục sử dụng";
    rowSubtitles_[5] = "Chạm hoặc bấm Confirm để vào trang chủ";
  } else {
    rowLabels_[0] = "1. Customizable Quick Settings (up to 3 rows / 12 tiles)";
    rowSubtitles_[0] = "Settings > Display > Quick Settings: show/hide and reorder";

    rowLabels_[1] = "2. Move Home items & Game/App shortcuts to Quick Settings";
    rowSubtitles_[1] = "Reading Stats, File Transfer, Read Later, Plugins, +Add Game/App";

    rowLabels_[2] = "3. Clean BLUETOOTH & Screenshot captions";
    rowSubtitles_[2] = "Fixed BluetoothOff/TAKE SCREENSHOT labels & Contrast icon";

    rowLabels_[3] = "4. All 38 App Store languages & edge-swipe indicator";
    rowSubtitles_[3] = "Full language packs, Boox-style swipe tab, AA EPUB black fix";

    rowLabels_[4] = "5. Direct launch for pinned Games, Apps & Plugins";
    rowSubtitles_[4] = "Add up to 4 pinned Game/App tiles to Quick Settings";

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
