#include "UiLanguage.h"

#include <I18n.h>
#include <I18nKeys.h>
#include <I18nPackKeys.h>
#include <LanguagePack.h>
#include <Logging.h>

#include <cstdio>
#include <cstring>

#include "CrossPointSettings.h"

namespace uilanguage {
namespace {
void setPack(const char* pack) {
  const char* value = pack ? pack : "";
  strncpy(SETTINGS.languagePack, value, sizeof(SETTINGS.languagePack) - 1);
  SETTINGS.languagePack[sizeof(SETTINGS.languagePack) - 1] = '\0';
}
}  // namespace

bool builtIn(const uint8_t language) {
  for (const uint8_t index : SORTED_LANGUAGE_INDICES)
    if (index == language) return true;
  return false;
}

bool defaultPackFilename(const uint8_t language, char* out, const size_t size) {
  if (language >= getLanguageCount() || !out || size == 0) return false;
  const int length = snprintf(out, size, "%s.cplang", LANGUAGE_TAGS[language]);
  return length > 0 && static_cast<size_t>(length) < size && languagepack::validFilename(out);
}

bool applyAtBoot() {
  const uint8_t saved = SETTINGS.language < getLanguageCount() ? SETTINGS.language : 0;
  I18N.setLanguage(static_cast<Language>(saved));
  if (builtIn(saved) && !SETTINGS.languagePack[0]) return false;

  char fallback[sizeof(SETTINGS.languagePack)];
  const char* pack = SETTINGS.languagePack;
  const bool adopting = !pack[0];
  if (adopting) {
    if (!defaultPackFilename(saved, fallback, sizeof(fallback))) {
      I18N.setLanguage(Language::EN);
      return false;
    }
    pack = fallback;
  }
  if (I18N.useLanguagePack(pack)) {
    // The pack's own code decides the language; keep SETTINGS in step with it.
    const auto active = static_cast<uint8_t>(I18N.getLanguage());
    const bool changed = adopting || SETTINGS.language != active;
    if (adopting) setPack(pack);
    SETTINGS.language = active;
    return changed;
  }
  // Missing or damaged pack: English this boot, saved choice kept so that
  // installing the pack brings the language back without a visit to Settings.
  LOG_ERR("I18N", "Language pack %s unavailable; using English", pack);
  I18N.setLanguage(Language::EN);
  return false;
}

Result select(const uint8_t language, const char* pack) {
  if (!pack || !pack[0]) {
    I18N.setLanguage(static_cast<Language>(language));
    SETTINGS.language = language;
    setPack(nullptr);
    return Result::Applied;
  }
  if (I18N.languagePackResident() && strcmp(I18N.residentLanguagePack(), pack) != 0) {
    // Takes effect on the next boot through applyAtBoot().
    SETTINGS.language = language;
    setPack(pack);
    return Result::RestartRequired;
  }
  if (!I18N.useLanguagePack(pack)) {
    // Nothing was resident (a resident pack is either this file, which would
    // have succeeded, or another one, handled above), so the interface was
    // running a built-in language: restore it. loadLanguagePack() left English.
    if (builtIn(SETTINGS.language) && !SETTINGS.languagePack[0])
      I18N.setLanguage(static_cast<Language>(SETTINGS.language));
    return Result::PackMissing;
  }
  SETTINGS.language = static_cast<uint8_t>(I18N.getLanguage());
  setPack(pack);
  return Result::Applied;
}

}  // namespace uilanguage
