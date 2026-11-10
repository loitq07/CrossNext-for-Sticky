#define main existingSettingsSuiteMain
#include "../settings-redesign/test_screens.cpp"
#undef main
#include "../../src/apps_local/chess/ChessScreens.h"
#include "../../src/apps_local/chess/puzzles/ChessPuzzleScreens.h"
#include "../../src/apps_local/checkers/DraughtsScreens.h"
#include "../../src/apps_local/go/GoScreens.h"
#include "../../src/apps_local/minesweeper/MinesweeperScreens.h"
#include "../../src/apps_local/solitaire/SolitaireScreens.h"
#include "../../src/apps_local/hearts/HeartsScreens.h"
#include "../../src/apps_local/picross/PicrossScreens.h"
#include "../../src/apps_local/wordle/WordleScreens.h"
#include "../../src/apps_local/pocketgames/PocketGamesScreens.h"
#include "../../src/apps_local/flashcards/FlashSettingsScreens.h"
#include "../../src/apps_local/pomodoro/PomodoroScreens.h"
#include "../../src/apps_local/habits/HabitsScreens.h"

static void buildMenu(int kind, toybox::Screen& screen) {
  switch (kind) {
    case 0: chessui::buildSettings(screen, {}); break;
    case 1: {
      draughtsui::Model model;
      model.view = draughtsui::View::Settings;
      draughtsui::build(screen, model);
      break;
    }
    case 2: goui::buildSettings(screen, {}); break;
    case 3: {
      mineui::MenuModel model;
      model.settings = true;
      mineui::buildMenu(screen, model);
      break;
    }
    case 4: solitaireui::buildSetup(screen, false, false); break;
    case 5: solitaireui::buildSettings(screen, {}); break;
    case 6: heartsui::buildSetup(screen, {}, false); break;
    case 7: heartsui::buildSettings(screen, {}); break;
    case 8: picrossui::buildSizes(screen, {}); break;
    case 9: {
      wordleui::Work work;
      wordleui::buildLengths(screen, 5, work);
      break;
    }
    case 10: {
      pocketgames::State state;
      pocketui::Model model;
      model.view = pocketui::View::Level;
      model.state = &state;
      pocketui::build(screen, model);
      break;
    }
    case 11: case 12: {
      chesspuzzleui::Model model;
      chesspuzzle::Progress progress;
      model.progress = &progress;
      model.view = kind == 11 ? chesspuzzleui::View::Difficulty : chesspuzzleui::View::ConfirmReset;
      chesspuzzleui::Work work;
      chesspuzzleui::build(screen, model, work);
      break;
    }
    case 13: flashui::textSettings(screen, {}); break;
    case 14: case 15: {
      pomodoro::State state;
      pomodoroui::Work work;
      pomodoroui::build(screen, {&state, kind == 14 ? pomodoroui::View::Settings : pomodoroui::View::ConfirmReset}, work);
      break;
    }
    case 16: habitsui::buildMenu(screen, "Reading"); break;
    case 17: habitsui::buildDelete(screen, "Reading"); break;
  }
}

int main() {
  checks = failures = 0;
  for (bool light : {false, true}) {
    chrome::lightHeader = light;
    for (bool wide : {false, true}) {
      for (auto language : {Language::EN, Language::VI}) {
        I18N.setLanguage(language);
        for (int kind = 0; kind < 18; ++kind) {
          Canvas canvas(wide ? 800 : 480, wide ? 480 : 800);
          canvas.target.allowEllipsis = true; // Layout truncation is covered by the wider baseline suite.
          const auto rebuild = [&] {
            std::fill(canvas.pixels.begin(), canvas.pixels.end(), 255);
            fui::InputSnapshot empty;
            toybox::Frame frame(canvas.target, canvas.device, empty, canvas.interactions);
            toybox::Screen screen(frame);
            buildMenu(kind, screen);
            canvas.interactions.paintButtonFocus(canvas.target);
            paintclock::notePainted();
          };
          rebuild();
          CHECK(!canvas.interactions.overflowed());
          CHECK(canvas.interactions.count() > 1);
          const int count = canvas.interactions.count();
          for (int step = 0; step < count; ++step) {
            const auto previousPixels = canvas.pixels;
            fui::InputSnapshot next;
            next.focusNext = true;
            CHECK(!canvas.interactions.route(next));
            fui::Rect rect;
            CHECK(canvas.interactions.buttonFocusRect(rect));
            const auto& entry = canvas.interactions.data()[canvas.interactions.focusedIndex()];
            const auto action = entry.action;
            const auto value = entry.value;
            CHECK(!fui::hasState(entry.state, fui::StateDisabled));
            CHECK(rect.width > 0 && rect.height > 0);
            rebuild();
            if (step > 0) {
              if (canvas.pixels == previousPixels)
                std::printf("No visible focus: kind=%d step=%d action=%d value=%d light=%d wide=%d language=%d\n",
                            kind, step, int(action), int(value), light, wide, int(language));
              CHECK(canvas.pixels != previousPixels);
            }
            fui::InputSnapshot confirm;
            confirm.confirm = true;
            const auto event = canvas.interactions.route(confirm);
            CHECK(event.action == action && event.value == value);
          }
        }
      }
    }
  }
  std::printf("Game/Flashcards settings navigation: %d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
