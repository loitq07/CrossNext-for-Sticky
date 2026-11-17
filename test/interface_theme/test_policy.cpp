#include "../../src/util/InterfaceThemeStyle.h"
using namespace interfacetheme;
using menufont::Section;
// Existing saved values must retain their meaning.
static_assert(int(Override::Licorice) == 4 && int(Override::Mindful) == 5);
constexpr bool legacyCompatibility() {
  for (uint8_t home = 0; home < 4; ++home)
    for (uint8_t general = 0; general < 5; ++general)
      for (int section = 0; section < menufont::kSectionCount; ++section)
        for (uint8_t overrideValue = 0; overrideValue < 5; ++overrideValue) {
          auto base = Style(home);
          auto common = general ? Style(general - 1) : base;
          auto expected = Section(section) == Section::Home ? base
                        : Section(section) == Section::General ? common
                        : overrideValue ? Style(overrideValue - 1) : common;
          if (resolve(home, general, Section(section), overrideValue) != expected) return false;
        }
  return true;
}
static_assert(legacyCompatibility());
constexpr bool mindfulScope() {
  for (uint8_t home = 0; home < 4; ++home) {
    for (int section = 0; section < menufont::kSectionCount; ++section) {
      const auto s = Section(section);
      const auto expected = s == Section::Games || s == Section::Apps ? Style(home) : Style::Mindful;
      if (resolve(home, 5, s, 0) != expected) return false;
    }
    if (resolve(home, 5, Section::Library, 1) != Style::Framed) return false;
    if (resolve(home, 5, Section::Games, 5) != Style::Mindful) return false;
    if (resolve(home, 5, Section::Apps, 4) != Style::Licorice) return false;
  }
  return true;
}
static_assert(mindfulScope());
static_assert(normalized(255) == Override::General);
static_assert(homeStyle(4) == Style::Framed);
static_assert(light(Style::Mindful));
#include "../../src/apps_local/ui/UnifiedDesignTokens.h"
#include <cassert>
int main() {
  namespace fui = freeink::ui;
  chrome::lightHeader = true;
  chrome::mindful = false;
  const auto old = cds::listRowStyles();
  assert(old.selected.background.color == fui::Color::Black);
  chrome::mindful = true;
  for (const auto styles : {cds::buttonStyles(), cds::listRowStyles()}) {
    assert(styles.normal.background.color == fui::Color::White);
    assert(styles.normal.borderWidth == 1);
    assert(styles.selected.background.color == fui::Color::White);
    assert(styles.selected.foreground.color == fui::Color::Black);
    assert(styles.selected.borderWidth == 2);
    assert(styles.focused.borderWidth == 2);
    assert(styles.active.borderWidth == 2);
  }
  chrome::mindful = false;
  assert(cds::listRowStyles().selected.background.color == old.selected.background.color);
}
