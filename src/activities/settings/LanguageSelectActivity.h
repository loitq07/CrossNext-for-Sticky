#pragma once

#include <GfxRenderer.h>
#include <I18n.h>
#include <LanguagePack.h>

#include <vector>

#include "activities/UiListActivity.h"

class MappedInputManager;

/**
 * Activity for selecting UI language.
 *
 * FORK CHANGE (upstream 2.0 language packs): rows are the languages compiled
 * into this build (custom_i18n_builtin_langs) followed by the .cplang packs
 * found in /languages on the microSD. Switching applies in place, as before;
 * only replacing an already loaded pack with another one needs a restart.
 */
class LanguageSelectActivity final : public UiListActivity {
 public:
  explicit LanguageSelectActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  void onEnter() override;
  void onExit() override;

 private:
  static constexpr const auto& builtinLanguages = SORTED_LANGUAGE_INDICES;
  static constexpr int builtinCount = static_cast<int>(sizeof(SORTED_LANGUAGE_INDICES));

  int listCount() const override { return static_cast<int>(rowItems.size()); }
  void buildScreen(UiScreen& screen) override;
  void activateIndex(int index) override;
  void applySelection(int index);
  bool isActive(int index) const;
  const char* headerTitle() const override;
  void buildRows();
  void finishSelection();

  // Borrowed by rowItems' labels while this screen is open.
  languagepack::Catalog catalog;
  bool catalogFailed = false;
  std::vector<freeink::ui::ListItem> rowItems;
};
