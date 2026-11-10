#define main existingSettingsSuiteMain
#include "../settings-redesign/test_screens.cpp"
#undef main
#include "../../src/apps_local/ui/WifiScreens.h"
#include "../../src/apps_local/ui/MenuFontScreen.h"
#include "../../src/apps_local/library/LibraryControls.h"

int main(int argc, char** argv) {
  checks = failures = 0;
  {
    Canvas c(480, 800);
    fui::ListNav nav;
    fui::ListItem items[4];
    for (auto& item : items) item.label = "Clock";
    fui::ListProps props;
    settingsui::styleRows(props);
    props.items = items;
    props.count = 4;
    nav.requestSelection(2);
    nav.syncToProps(fui::makeRect(20, 80, 440, 600), props.rowHeight, 0, 4, props);
    props.rowStyles.selected = props.rowStyles.normal; // touch mode hides only the paint
    fui::list(c.frame, fui::makeRect(20, 80, 440, 600), props);
    CHECK(props.selectedIndex == 2);
    CHECK(!nav.followPending && !nav.consumeRebuildNeeded());
  }
  for (bool light : {false, true}) {
    chrome::lightHeader = light;
    for (bool wide : {false, true}) {
      for (auto language : {Language::EN, Language::VI}) {
        I18N.setLanguage(language);
        for (int kind = 0; kind < 8; ++kind) {
          Canvas c(wide ? 800 : 480, wide ? 480 : 800);
          fui::ListItem items[3];
          for (int n = 0; n < 3; ++n) {
            items[n].label = tr(STR_CLOCK);
            items[n].actionValue = n;
          }
          items[1].enabled = false;
          fui::ListProps props;
          if (kind == 0) pixelui::styleList(props);
          if (kind == 1) settingsui::styleRows(props);
          if (kind == 2) settingsui::styleAppearance(props);
          if (kind == 3) menufontui::styleSections(props, wide);
          if (kind == 4) { pixelui::styleList(props); libraryui::lightRows(props); }
          if (kind == 5) libraryui::filterCards(props, c.target);
          fui::ListItem preferences[int(libraryui::Preference::Count)];
          if (kind == 6) libraryui::preferenceRows(props, preferences, {});
          if (kind == 7) { settingsui::styleRows(props); quickui::styleConfiguration(props); }
          props.items = items;
          props.count = 3;
          props.action = 70;
          props.inputMask = fui::InputTouch;
          props.selectedIndex = -1;
          const auto band = fui::makeRect(20, 40, c.width - 40, c.height - 40);
          fui::list(c.frame, band, props);
          paintclock::notePainted();
          fui::InputSnapshot next;
          next.focusNext = true;
          CHECK(!c.interactions.route(next));
          CHECK(c.interactions.hasButtonFocus());
          CHECK(fui::hasState(c.frame.stateFor(70, 0, fui::StateNormal), fui::StateFocused));
          CHECK(props.rowStyles.focused.background.color == fui::Color::Black);
          CHECK(props.rowStyles.focused.foreground.color == fui::Color::White);
          c.interactions.clear();
          c.target.blackFills.clear();
          fui::list(c.frame, band, props);
          paintclock::notePainted();
          bool visible = false;
          for (const auto& r : c.target.blackFills) visible |= r.width > 100 && r.height > 40;
          CHECK(visible);
          if (argc > 1 && kind == 1 && language == Language::EN)
            c.write(std::string(argv[1]) + (light ? "/light" : "/dark") + (wide ? "-wide.pbm" : "-tall.pbm"));
          CHECK(!c.interactions.route(next));
          fui::InputSnapshot confirm;
          confirm.confirm = true;
          CHECK(c.interactions.route(confirm).value == 2); // disabled row skipped
          fui::Rect first = c.interactions.data()[0].rect;
          fui::InputSnapshot touch;
          touch.touchReleased = true;
          touch.touchX = first.x + first.width / 2;
          touch.touchY = first.y + first.height / 2;
          CHECK(c.interactions.route(touch).value == 0);
          CHECK(!c.interactions.hasButtonFocus());
          CHECK(!c.interactions.route(next));
          CHECK(c.interactions.hasButtonFocus());
          CHECK(c.interactions.route(confirm).value == 0);
        }
      }
      // A primary Wi-Fi action must yield its highlight to the keyboard cursor.
      Canvas wifi(wide ? 800 : 480, wide ? 480 : 800);
      fui::ButtonProps button;
      wifiscreens::button(wifi.screen, button, fui::makeRect(20, 100, 180, 56), "Edit", 71, 0);
      wifiscreens::button(wifi.screen, button, fui::makeRect(220, 100, 180, 56), "Connect", 71, 1, true);
      fui::InputSnapshot next;
      next.focusNext = true;
      paintclock::notePainted();
      wifi.interactions.route(next);
      wifi.interactions.clear();
      wifiscreens::button(wifi.screen, button, fui::makeRect(20, 100, 180, 56), "Edit", 71, 0);
      CHECK(fui::hasState(button.state, fui::StateSelected));
      wifiscreens::button(wifi.screen, button, fui::makeRect(220, 100, 180, 56), "Connect", 71, 1, true);
      CHECK(!fui::hasState(button.state, fui::StateSelected));
    }
  }
  std::printf("Navigation feedback: %d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
