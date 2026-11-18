#include "LanguageSelectActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <Logging.h>
#include <Memory.h>

#include <cstring>

#include "CrossPointSettings.h"
#include "I18nKeys.h"
#include "MappedInputManager.h"
#include "SilentRestart.h"
#include "activities/util/ConfirmationActivity.h"
#include "apps_local/i18n/UiLanguage.h"
#include "apps_local/ui/PixelMenuTheme.h"
#include "apps_local/words/DictionaryDefaults.h"
#include "components/UITheme.h"

namespace fui = freeink::ui;

LanguageSelectActivity::LanguageSelectActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : UiListActivity("LanguageSelect", renderer, mappedInput, false, true) {}

void LanguageSelectActivity::onEnter() {
  UiListActivity::onEnter();
  // One directory listing plus a 112-byte header per pack, only while this
  // screen is open; the catalog is released on exit.
  catalogFailed = !catalog.scan();
  buildRows();
  // Open on the current language, which may sit past the first page; the first
  // screen build pulls the viewport to it (ListNav follow-on-build).
  nav.selected = 0;
  for (int i = 0; i < listCount(); ++i) {
    if (isActive(i)) {
      nav.selected = i;
      break;
    }
  }
}

void LanguageSelectActivity::onExit() {
  UiListActivity::onExit();
  rowItems.clear();
  catalog.clear();
}

const char* LanguageSelectActivity::headerTitle() const { return tr(STR_LANGUAGE); }

bool LanguageSelectActivity::isActive(const int index) const {
  if (index < builtinCount)
    return !I18N.hasLanguagePack() && static_cast<uint8_t>(I18N.getLanguage()) == builtinLanguages[index];
  const auto& entry = catalog[static_cast<size_t>(index - builtinCount)];
  return I18N.hasLanguagePack() && strcmp(I18N.residentLanguagePack(), entry.filename) == 0;
}

void LanguageSelectActivity::buildRows() {
  rowItems.clear();
  rowItems.reserve(static_cast<size_t>(builtinCount) + catalog.size());
  for (int i = 0; i < builtinCount; ++i) {
    fui::ListItem item;
    item.label = I18N.getLanguageName(static_cast<Language>(builtinLanguages[i]));
    item.value = tr(STR_LANGUAGE_BUILT_IN);
    if (i == 0 && catalogFailed) item.subtitle = tr(STR_LANGUAGE_PACK_SCAN_FAILED);
    rowItems.push_back(item);
  }
  for (size_t i = 0; i < catalog.size(); ++i) {
    const auto& entry = catalog[i];
    fui::ListItem item;
    item.label = entry.info.name;
    // Saved but not running: the pack failed to load at boot (or another pack
    // is resident and a restart is pending).
    item.value = strcmp(SETTINGS.languagePack, entry.filename) == 0 && !I18N.hasLanguagePack()
                     ? tr(STR_LANGUAGE_UNAVAILABLE)
                     : tr(STR_LANGUAGE_ON_SD);
    rowItems.push_back(item);
  }
  for (int i = 0; i < listCount(); ++i) {
    if (isActive(i)) rowItems[static_cast<size_t>(i)].value = tr(STR_LANGUAGE_IN_USE);
    rowItems[static_cast<size_t>(i)].actionValue = static_cast<int16_t>(i);
  }
}

void LanguageSelectActivity::finishSelection() {
  dictionarydefaults::apply(SETTINGS);
  SETTINGS.saveToFile();
  finish();
}

void LanguageSelectActivity::activateIndex(const int index) {
  if (index < 0 || index >= listCount()) return;
  // The activated row leaves this screen; a lingering flash would gray an
  // unrelated element on the next render.
  app.clearTapFlash();
  nav.selected = index;
  if (isActive(index)) {
    finish();
    return;
  }

  uilanguage::Result result;
  {
    RenderLock lock(*this);
    if (index < builtinCount) {
      result = uilanguage::select(builtinLanguages[index], nullptr);
    } else {
      const auto& entry = catalog[static_cast<size_t>(index - builtinCount)];
      result = uilanguage::select(static_cast<uint8_t>(I18n::languageFromCode(entry.info.code)), entry.filename);
    }
  }

  switch (result) {
    case uilanguage::Result::Applied:
      finishSelection();
      return;
    case uilanguage::Result::PackMissing: {
      LOG_ERR("I18N", "Language pack could not be loaded");
      {
        RenderLock lock(*this);
        buildRows();
      }
      requestUpdate();
      if (auto error = makeUniqueNoThrow<ConfirmationActivity>(renderer, mappedInput, tr(STR_LANGUAGE),
                                                               tr(STR_LANGUAGE_PACK_LOAD_FAILED), true)) {
        startActivityForResult(std::move(error), nullptr);
      }
      return;
    }
    case uilanguage::Result::RestartRequired: {
      auto confirmation = makeUniqueNoThrow<ConfirmationActivity>(renderer, mappedInput, tr(STR_LANGUAGE),
                                                                  tr(STR_LANGUAGE_RESTART_PROMPT));
      if (!confirmation) {
        LOG_ERR("I18N", "OOM: language confirmation");
        finishSelection();
        return;
      }
      // SETTINGS already names the new pack; it is saved either way and
      // applied by the next boot.
      startActivityForResult(std::move(confirmation), [this](const ActivityResult& confirm) {
        dictionarydefaults::apply(SETTINGS);
        SETTINGS.saveToFile();
        if (!confirm.isCancelled) silentRestartToSettings();
        finish();
      });
      return;
    }
  }
}

void LanguageSelectActivity::buildScreen(UiScreen& screen) {
  pixelui::FontScope fonts(uiTarget, renderer, true);
  useCompactSettingsRows();
  const auto& metrics = UITheme::getInstance().getMetrics();
  // Content: the safe area minus the header band GUI.drawHeader paints.
  pixelui::begin(screen, mappedInput.hasTouch() ? 0 : metrics.buttonHintsHeight);

  fui::ListProps props;
  props.items = rowItems.data();
  props.count = static_cast<uint16_t>(rowItems.size());
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch;  // physical buttons stay in loop()
  syncListViewport(screen, props);
  pixelui::list(screen, props);
}
