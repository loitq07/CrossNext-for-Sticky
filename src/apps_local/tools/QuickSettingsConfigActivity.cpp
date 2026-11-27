#include "QuickSettingsConfigActivity.h"

#include <CrossPointSettings.h>
#include <OpdsServerStore.h>

#include <cstdio>

#include "../Shelf.h"
#include "../ui/ExtraIcons.h"
#include "../ui/PixelMenuTheme.h"

namespace {
bool isViLang() {
  return I18N.getLanguage() == Language::VI ||
         (!I18N.hasLanguagePack() && SETTINGS.language == static_cast<uint8_t>(Language::VI));
}
}  // namespace

void QuickSettingsConfigActivity::onEnter() {
  UiListActivity::onEnter();
  app.on(ACTION_USER, &QuickSettingsConfigActivity::moveAction, this);
  draft = quickmenu::config();
  eligibleMask = quickui::supportedShortcutMask();
  plugins = SETTINGS.pluginsEnabled || !OPDS_STORE.hasServers();

  static constexpr quickui::Shortcut kQuickGroup[] = {
      quickui::Shortcut::Wifi,     quickui::Shortcut::Bluetooth, quickui::Shortcut::KeySound,
      quickui::Shortcut::Dark,     quickui::Shortcut::Refresh,   quickui::Shortcut::Screenshot,
      quickui::Shortcut::Rotate,   quickui::Shortcut::Sleep,     quickui::Shortcut::Off,
      quickui::Shortcut::Restart,  quickui::Shortcut::Light,     quickui::Shortcut::Warm,
      quickui::Shortcut::Tilt,     quickui::Shortcut::Lock,
  };
  static constexpr quickui::Shortcut kHomeGroup[] = {
      quickui::Shortcut::Files,     quickui::Shortcut::Library,  quickui::Shortcut::Stats,
      quickui::Shortcut::Transfer,  quickui::Shortcut::ReadLater, quickui::Shortcut::Services,
      quickui::Shortcut::Games,     quickui::Shortcut::Apps,     quickui::Shortcut::AppStore,
      quickui::Shortcut::Settings,  quickui::Shortcut::UsbDrive,
  };

  baseCount = 0;
  for (const auto s : kQuickGroup) {
    if (eligibleMask & (1u << static_cast<int>(s))) baseChoices[baseCount++] = s;
  }
  homeSectionStart = baseCount;
  for (const auto s : kHomeGroup) {
    if (eligibleMask & (1u << static_cast<int>(s))) baseChoices[baseCount++] = s;
  }
}

int QuickSettingsConfigActivity::visibilityRows() const {
  int count = baseCount;
  for (const auto& entry : draft.shortcuts)
    if (entry.title[0]) ++count;
  return count;
}

quickui::Shortcut QuickSettingsConfigActivity::idAtRow(const int index) const {
  if (view == View::Order) return quickmenu::at(draft, index, eligibleMask);
  if (index < baseCount) return baseChoices[index];
  int row = baseCount;
  for (int n = 0; n < quickmenu::kShortcutCount; ++n) {
    if (draft.shortcuts[n].title[0] && index == row++)
      return static_cast<quickui::Shortcut>(quickmenu::kBaseCount + n);
  }
  return quickui::Shortcut::Count;
}

int QuickSettingsConfigActivity::listCount() const {
  switch (view) {
    case View::Root:
      return 4;
    case View::Visibility:
      return visibilityRows() + 2;  // + Add Game, + Add App
    case View::Order:
      return quickmenu::visibleCount(draft, eligibleMask);
    case View::ChooseGame:
    case View::ChooseApp:
      return shelf::folders()[pickerFolder()].count;
  }
  return 0;
}

const char* QuickSettingsConfigActivity::headerTitle() const {
  switch (view) {
    case View::Root:
      return tr(STR_QUICK_CUSTOMISE);
    case View::Visibility:
      return tr(STR_HOME_MENU_VISIBILITY);
    case View::Order:
      return tr(STR_HOME_MENU_ORDER);
    case View::ChooseGame:
      return tr(STR_HOME_ADD_GAME);
    case View::ChooseApp:
      return tr(STR_HOME_ADD_APP);
  }
  return tr(STR_QUICK_CUSTOMISE);
}

void QuickSettingsConfigActivity::show(const View next, const View returnView) {
  app.clearTapFlash();
  view = next;
  if (next == View::ChooseGame || next == View::ChooseApp) pickerReturnView = returnView;
  noticeText = nullptr;
  nav.reset();
  resetUi();
  requestUpdate();
}

void QuickSettingsConfigActivity::onBackButton() {
  switch (view) {
    case View::Root:
      finish();
      return;
    case View::ChooseGame:
    case View::ChooseApp:
      show(pickerReturnView);
      return;
    case View::Visibility:
    case View::Order:
      show(View::Root);
      return;
  }
}

bool QuickSettingsConfigActivity::save() {
  if (quickmenu::Store::instance().save(draft)) return true;
  draft = quickmenu::config();
  noticeText = tr(STR_HOME_MENU_SAVE_FAILED);
  requestUpdate();
  return false;
}

void QuickSettingsConfigActivity::moveAction(const freeink::ui::ActionEvent& event, void* context) {
  auto& self = *static_cast<QuickSettingsConfigActivity*>(context);
  if (self.view != View::Order) return;
  const auto id = self.idAtRow(self.nav.selected);
  if (!quickmenu::move(self.draft, id, event.value, self.eligibleMask)) return;
  self.app.clearTapFlash();
  self.noticeText = nullptr;
  self.save();
  self.moveSelectionTo(quickmenu::indexOf(self.draft, id, self.eligibleMask));
}

void QuickSettingsConfigActivity::activateIndex(const int index) {
  app.clearTapFlash();
  if (index < 0 || index >= listCount()) return;
  if (view == View::Root) {
    switch (index) {
      case 0:
        show(View::Visibility);
        break;
      case 1:
        show(View::Order);
        break;
      case 2:
        show(View::ChooseGame, View::Root);
        break;
      case 3:
        show(View::ChooseApp, View::Root);
        break;
    }
    return;
  }
  if (view == View::Order) {
    requestUpdate();
    return;
  }
  if (view == View::ChooseGame || view == View::ChooseApp) {
    const int folder = pickerFolder();
    const auto* title = shelf::folders()[folder].items[index].title;
    const auto existing = quickmenu::findAppShortcut(draft, folder, title);
    if (existing != quickui::Shortcut::Count) {
      if (quickmenu::visibleCount(draft, eligibleMask) <= 1) {
        noticeText = isViLang() ? "Phải giữ ít nhất 1 mục." : "Keep at least 1 item visible.";
      } else {
        quickmenu::removeAppShortcut(draft, existing);
        noticeText = nullptr;
        save();
      }
    } else if (quickmenu::addAppShortcut(draft, folder, title, eligibleMask) == quickui::Shortcut::Count) {
      noticeText =
          quickmenu::visibleCount(draft, eligibleMask) >= quickmenu::kMaxVisible
              ? (isViLang() ? "Tối đa 12 mục (3 hàng). Hãy ẩn bớt 1 mục." : "12 items maximum (3 rows). Hide one first.")
              : (isViLang() ? "Tối đa 4 lối tắt game/app." : "4 app/game shortcuts maximum.");
    } else if (save()) {
      show(View::Visibility);
      const auto added = quickmenu::findAppShortcut(draft, folder, title);
      for (int row = baseCount; row < visibilityRows(); ++row) {
        if (idAtRow(row) == added) {
          moveSelectionTo(row);
          break;
        }
      }
    }
    requestUpdate();
    return;
  }
  if (view == View::Visibility && index >= visibilityRows()) {
    show(index == visibilityRows() ? View::ChooseGame : View::ChooseApp, View::Visibility);
    return;
  }

  const auto id = idAtRow(index);
  if (id == quickui::Shortcut::Count) return;
  if (quickmenu::isAppShortcut(id)) {
    if (quickmenu::visibleCount(draft, eligibleMask) <= 1) {
      noticeText = isViLang() ? "Phải giữ ít nhất 1 mục." : "Keep at least 1 item visible.";
    } else {
      quickmenu::removeAppShortcut(draft, id);
      noticeText = nullptr;
      nav.reset();
      save();
    }
    requestUpdate();
    return;
  }

  const bool shown = (draft.visible & (1u << static_cast<int>(id))) != 0;
  if (!quickmenu::setVisible(draft, id, !shown, eligibleMask)) {
    noticeText = shown ? (isViLang() ? "Phải giữ ít nhất 1 mục." : "Keep at least 1 item visible.")
                       : (isViLang() ? "Tối đa 12 mục (3 hàng). Hãy ẩn bớt 1 mục."
                                     : "12 items maximum (3 rows). Hide one first.");
  } else {
    noticeText = nullptr;
    save();
  }
  requestUpdate();
}

void QuickSettingsConfigActivity::drawChrome() {
  pixelui::drawHeader(renderer, headerTitle(), I18N.getLanguage() != Language::EN);
}

void QuickSettingsConfigActivity::provideRow(void* context, const uint16_t index, freeink::ui::ListItem& row) {
  auto& self = *static_cast<QuickSettingsConfigActivity*>(context);
  row = {};
  row.actionValue = index;
  if (self.view == View::Root) {
    switch (index) {
      case 0:
        row.label = tr(STR_HOME_MENU_VISIBILITY);
        break;
      case 1:
        row.label = tr(STR_HOME_MENU_ORDER);
        break;
      case 2:
        row.label = tr(STR_HOME_ADD_GAME);
        break;
      case 3:
        row.label = tr(STR_HOME_ADD_APP);
        break;
    }
    row.value = ">";
    return;
  }
  if (self.view == View::ChooseGame || self.view == View::ChooseApp) {
    const auto& item = shelf::folders()[self.pickerFolder()].items[index];
    row.label = shelf::label(item.title);
    row.toggle = false;
    row.toggleChecked =
        quickmenu::findAppShortcut(self.draft, self.pickerFolder(), item.title) != quickui::Shortcut::Count;
    row.icon = freeink::ui::bitmapFromIcon(row.toggleChecked ? extra_checked_32 : extra_unchecked_32);
    return;
  }
  if (self.view == View::Visibility) {
    if (index == 0)
      row.sectionHeading = tr(STR_QUICK_CUSTOMISE);
    else if (index == self.homeSectionStart && self.homeSectionStart < self.baseCount)
      row.sectionHeading = tr(STR_HOME_MENU);
    else if (index == self.baseCount)
      row.sectionHeading = isViLang() ? "Lối tắt ứng dụng" : "Shortcuts";
  }
  if (self.view == View::Visibility && index >= self.visibilityRows()) {
    row.label = index == self.visibilityRows() ? tr(STR_HOME_ADD_GAME) : tr(STR_HOME_ADD_APP);
    row.value = ">";
    return;
  }

  const auto id = self.idAtRow(index);
  const auto* appEntry = quickmenu::appShortcut(self.draft, id);
  row.label = appEntry ? shelf::label(appEntry->title) : quickui::label(id, true, true, self.plugins);

  if (self.view == View::Order) {
    const int rowNum = index / 4 + 1;
    const int colNum = index % 4 + 1;
    std::snprintf(self.subtitleBuf, sizeof(self.subtitleBuf), isViLang() ? "Hàng %d · Ô %d" : "Row %d · Slot %d",
                  rowNum, colNum);
    row.subtitle = self.subtitleBuf;
    const freeink::Icon* appIcon = nullptr;
    if (appEntry && appEntry->folder < shelf::folderCount()) {
      const auto& folder = shelf::folders()[appEntry->folder];
      for (int n = 0; n < folder.count; ++n) {
        if (!std::strcmp(folder.items[n].title, appEntry->title)) {
          appIcon = folder.items[n].icon;
          break;
        }
      }
    }
    row.icon = freeink::ui::bitmapFromIcon(appIcon ? *appIcon : quickui::icon(id));
    return;
  }

  row.toggle = false;
  row.toggleChecked = (self.draft.visible & (1u << static_cast<int>(id))) != 0;
  row.icon = freeink::ui::bitmapFromIcon(row.toggleChecked ? extra_checked_32 : extra_unchecked_32);
  if (appEntry) row.subtitle = tr(STR_HOME_REMOVE_SHORTCUT);
}

void QuickSettingsConfigActivity::buildScreen(UiScreen& screen) {
  pixelui::FontScope fonts(uiTarget, renderer, true);
  useCompactSettingsRows();
  pixelui::begin(screen, noticeText ? 52 : 0);

  props = {};
  pixelui::styleList(props);
  props.labelText.maxLines = 1;
  props.subtitleText.maxLines = 2;
  props.rowProvider = &QuickSettingsConfigActivity::provideRow;
  props.rowProviderCtx = this;
  props.count = listCount();
  props.action = ACTION_ROW;
  if (view == View::Order) {
    homemenuui::orderControls(screen, orderWork, nav.selected, listCount(), ACTION_USER);
    syncListViewport(screen, props);
    rowText.bind(props, screen.target(), screen.body());
    pixelui::list(screen, props);
    if (noticeText) {
      pixelui::text(
          screen.target(),
          freeink::ui::makeRect(screen.body().x, screen.device().safeRect().bottom() - 48, screen.body().width, 44),
          noticeText, pixelui::kSmall);
    }
    return;
  }
  syncListViewport(screen, props);
  rowText.bind(props, screen.target(), screen.body());
  pixelui::list(screen, props);
  if (noticeText) {
    const auto safe = screen.device().safeRect();
    pixelui::text(screen.target(), freeink::ui::makeRect(safe.x + 12, safe.bottom() - 48, safe.width - 24, 44),
                  noticeText, pixelui::kSmall);
  }
}
