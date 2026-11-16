#include "Shelf.h"

#include <HalStorage.h>
#include <I18n.h>
#include <Logging.h>
#include <strings.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <vector>

#include "../activities/ActivityManager.h"
#include "../activities/DynamicAppActivity.h"
#include "ShelfFolderActivity.h"
#include "ShelfHidden.h"
#include "ShelfState.h"
#include "appstore/AppStoreActivity.h"
#include "carnivoro/CarnivoroIcon.h"
#include "components/icons/listIcons.h"
#include "flashcards/FlashcardsActivity.h"
#include "instapaper/InstapaperActivity.h"
#include "modules/NativeModule.h"
#include "notes/NotesActivity.h"
#include "player/PlayerActivity.h"
#include "pomodoro/PomodoroIcon.h"
#include "quotes/QuotesActivity.h"
#include "tools/ToolsActivity.h"
#include "ui/ExtraIcons.h"
#include "ui/ToyboxIcons.h"
#include "voicenotes/VoiceNotesActivity.h"
#include "wallpapers/WallpapersActivity.h"
#include "words/WordsActivity.h"

namespace {

// Placeholder for a built-in app that ships as a native module (its original
// source, built by tools_local/native_modules). The row keeps its title, icon
// and position, and only appears once the module is on the card: the SD scan
// swaps in the loader. Never actually called.
std::unique_ptr<Activity> moduleNotInstalled(GfxRenderer&, MappedInputManager&) { return nullptr; }

// Icons come from tools_local/toybox/icons.txt via Lucide. Picked for silhouette
// rather than literalness: a crown, a hull, a grid and a card suit share no
// shape, so a row is scannable before the label is read.
constexpr shelf::Item kGames[] = {
    {"CHESS", &icon_chess_32, &moduleNotInstalled},
    {"CHECKERS", &icon_checkers_32, &moduleNotInstalled},
    {"GO", &icon_go_32, &moduleNotInstalled},
    {"MINESWEEPER", &icon_minesweeper_32, &moduleNotInstalled},
    {"PICROSS", &icon_picross_32, &moduleNotInstalled},
    {"WORDLE", &icon_wordle_32, &moduleNotInstalled},
    {"2048", &icon_connections_32, &moduleNotInstalled},
    {"THE INTERCEPT", &icon_underhand_32, &moduleNotInstalled},
    {"BATTLESHIP", &icon_battleship_32, &moduleNotInstalled},
    {"SOLITAIRE", &icon_solitaire_32, &moduleNotInstalled},
    {"HEARTS", &icon_hearts_32, &moduleNotInstalled},
    {"MURDLE", &icon_murdle_32, &moduleNotInstalled},
    {"CARNIVORO", &carnivoroPawIcon, &moduleNotInstalled},
    {"CONNECT FOUR", &icon_connectfour_32, &moduleNotInstalled},
    {"SUDOKU", &icon_sudoku_32, &moduleNotInstalled},
    {"OTHELLO", &extra_circle_dot_32, &moduleNotInstalled},
};
constexpr shelf::Item kApps[] = {
    {"READING QUEST", &icon_dungeon_32, &moduleNotInstalled},
    {"PET", &icon_cat_animals_32, &moduleNotInstalled},
    {"HABITS", &icon_tick_24, &moduleNotInstalled},
    {"TO DO LIST", &icon_murdle_face_clues_32, &NotesActivity::createTodo},
    {"NOTES", &extra_notes_32, &NotesActivity::create},
    {"VOICE NOTES", &extra_mic_32, &voicenotes::VoiceNotesActivity::create},
    {"POMODORO", &pomodoro::timerIcon, &moduleNotInstalled},
    {"WALLPAPERS", &icon_wallpapers_32, &WallpapersActivity::create},
    {"INSTAPAPER", &icon_instapaper_32, &InstapaperActivity::create},
    {"CALCULATOR", &icon_calculator_32, &moduleNotInstalled},
    {"QUOTES", &icon_library_32, &QuotesActivity::create},
    {"FLASHCARDS", &extra_cards_32, &FlashcardsActivity::create},
    {"WORDS", &icon_library_32, &WordsActivity::create},
    {"CALENDAR", &extra_calendar_32, &ToolsActivity::calendar},
    {"CLOCK", &extra_clock_32, &ToolsActivity::clock},
    {"COUNTER", &icon_calculator_32, &ToolsActivity::counter},
};

// The two rows Home grows, in reading order. Titles are Title Case because
// these sit in upstream's Home list and have to look like it; the folder screen
// shouts its own header, which is our side of the line. A third folder is one row here and
// nothing else: the Home hook counts this table rather than knowing its length.
constexpr shelf::Folder kFolders[] = {
    {"Games", UIIcon::Games, &icon_go_settings_32, kGames, static_cast<int>(sizeof(kGames) / sizeof(shelf::Item))},
    {"Apps", UIIcon::Apps, &icon_apps_32, kApps, static_cast<int>(sizeof(kApps) / sizeof(shelf::Item))},
};

constexpr int kFolderCount = static_cast<int>(sizeof(kFolders) / sizeof(kFolders[0]));

// An item with no icon draws a blank gutter and nothing says so. Caught at
// compile time rather than in a test, because a test can be forgotten and this
// cannot: a new row without an icon does not build.
constexpr bool everyItemHasAnIcon() {
  for (const auto& folder : kFolders) {
    for (int i = 0; i < folder.count; ++i) {
      if (folder.items[i].icon == nullptr) return false;
    }
  }
  return true;
}
constexpr bool everyItemHasFontKey() {
  for (const auto& folder : kFolders)
    for (int n = 0; n < folder.count; ++n)
      if (menufont::itemFor(folder.items[n].title) == menufont::kNoItem) return false;
  return true;
}
static_assert(everyItemHasFontKey(), "Add a stable interface font key for every shelf item");
static_assert(everyItemHasAnIcon(), "every shelf item needs an icon; see tools_local/toybox/icons.txt");

constexpr bool everyFolderHasAMark() {
  for (const auto& folder : kFolders) {
    if (folder.mark == nullptr) return false;
  }
  return true;
}
static_assert(everyFolderHasAMark(), "every shelf folder needs a mark; see tools_local/toybox/icons.txt");

// Beside the reader's own state and the player's name, so clearing
// `.crosspoint/` clears this too and there is one place to look. Inside the
// guard because the host build has no storage and an unused constant is a
// -Werror failure there.
#if defined(ARDUINO_ARCH_ESP32) || defined(SIMULATOR)
constexpr char kStatePath[] = "/.crosspoint/shelf.cfg";
// Beside it rather than inside it. The position is one short line a navigation
// rewrites constantly; this is a list that changes a handful of times in a
// device's life, and the two have no reason to share a write, a buffer or a
// parser -- shelf.cfg's is a fixed 96 bytes precisely because it is small.
constexpr char kHiddenPath[] = "/.crosspoint/shelf-hidden.cfg";
#endif

// Where leave() sends an app. Set when an item is opened, read when it leaves.
// -1 means "nothing is open below Home", which is what a folder itself sees.
int openFolderIndex = -1;

// Set when a folder or an app of ours is opened, taken by Home on entry. See
// takeHomeReturn().
bool homeReturn = false;

// The remembered position, mirroring /.crosspoint/shelf.cfg:
//
// - `lastFolder`: which shelf row Home should land on when you come back out.
//   CrossPoint restores Home's selection by matching the departing activity's
//   name against its own HomeMenuItem list, which cannot know about ours, so
//   without this you leave GAMES and the cursor is sitting on Browse Files.
// - `resumeRow`: per folder, the row it reopens on. The row of the item last
//   opened from it, or -- when you paged and then walked out without opening
//   anything -- the first row of the page you were looking at. It is the page
//   you were ON, not the page holding the game you last played; those are the
//   same thing until you browse and leave, and browsing and leaving is the case
//   that was wrong.
// - `openTitle`: the item that was open when the device went to sleep, which is
//   what wake reopens instead of dropping you on Home.
//
// All of it survives the device going to sleep, which none of it did before.
// `main.cpp` deep-sleeps on the idle timeout and says of it that wake is
// effectively a chip reset, so every time Mario put the device down and came
// back the shelf had forgotten which game he was playing.
//
// Written next to the reader's own state rather than into CrossPointState,
// which is upstream's file: a fork-local fact belongs in a fork-local file, and
// player.cfg already established the pattern.
shelf::State state;
bool stateLoaded = false;

// The items no folder shows, by title, mirroring shelf-hidden.cfg. Loaded on
// the first question anyone asks of it and not at boot, because the shelf has
// no init hook -- the same lazy load the position uses, for the same reason.
shelf::HiddenSet hiddenItems;
bool hiddenLoaded = false;

// The Activity that the open item launched, by name. `openFolderIndex` alone
// cannot answer "is that item still what is on screen": the Home gesture leaves
// an app without going through leave(), so a sleep from Home would otherwise
// record the game you left ten minutes ago as still open. A pushed sub-screen
// (the frontlight panel) also fails this check, and falls back to Home the way
// every wake used to.
std::string openActivityName;

// A title that does not fit the state file cannot be resumed, and would fail
// silently at the write. Caught at compile time instead, next to the icon and
// mark checks, so a long-titled new game does not build.
constexpr bool everyTitleFitsTheStateFile() {
  for (const auto& folder : kFolders) {
    for (int i = 0; i < folder.count; ++i) {
      if (shelf::constexprLength(folder.items[i].title) > shelf::MAX_ITEM_TITLE) return false;
    }
  }
  return true;
}
static_assert(everyTitleFitsTheStateFile(), "shelf item titles must fit shelf::MAX_ITEM_TITLE; see ShelfState.h");
static_assert(kFolderCount <= shelf::MAX_FOLDERS, "raise shelf::MAX_FOLDERS in ShelfState.h");

const int* itemLimits() {
  static int limits[shelf::MAX_FOLDERS] = {};
  for (int i = 0; i < kFolderCount; ++i) limits[i] = shelf::folders()[i].count - 1;
  return limits;
}

// The folder and row of the item with this title, case-insensitively. The one
// place a title is turned back into a row, shared by wake and by the
// autostart environment variable.
bool findItemByTitle(const char* title, int& folder, int& item) {
  const auto* fld = shelf::folders();
  for (int f = 0; f < kFolderCount; ++f) {
    for (int i = 0; i < fld[f].count; ++i) {
      if (strcasecmp(fld[f].items[i].title, title) == 0) {
        folder = f;
        item = i;
        return true;
      }
    }
  }
  return false;
}

// The parse and the format live in ShelfState.cpp, where a host test can reach
// them without a card: the file has to survive a truncated write, a file
// written before wake could resume, and a game renamed since it was written.
void loadState() {
  stateLoaded = true;
#if defined(ARDUINO_ARCH_ESP32) || defined(SIMULATOR)
  if (!Storage.exists(kStatePath)) return;
  char buffer[96] = {};
  if (Storage.readFileToBuffer(kStatePath, buffer, sizeof(buffer)) == 0) return;
  // Fails quietly: the worst a corrupt file can cost is starting at the top,
  // and there is nothing for anyone to do about it.
  shelf::parseState(buffer, kFolderCount, itemLimits(), state);
#endif
}

void saveState() {
#if defined(ARDUINO_ARCH_ESP32) || defined(SIMULATOR)
  char line[96];
  const size_t used = shelf::formatState(state, kFolderCount, line, sizeof(line));
  if (used == 0) {
    LOG_ERR("SHELF", "State line did not fit %d bytes", static_cast<int>(sizeof(line)));
    return;
  }
  Storage.writeFile(kStatePath, String(line));
#endif
}

// Every path that reads or writes the remembered position goes through this
// first. Lazily rather than at boot because the shelf has no init hook, and
// unconditionally rather than only on the read paths because openFolder passes
// the current resumeRow back in: without the load, the first navigation of a
// session would write the defaults over the saved file and the persistence
// would silently do nothing.
void ensureLoaded() {
  if (!stateLoaded) loadState();
}

// The hidden list, read once. Read whole rather than into a fixed buffer: the
// worst case is every item in the registry, and that grows every time Mario
// adds a game -- a buffer sized for today is a setting silently lost on the
// day the twenty-first one lands.
void ensureHiddenLoaded() {
  if (hiddenLoaded) return;
  hiddenLoaded = true;
#if defined(ARDUINO_ARCH_ESP32) || defined(SIMULATOR)
  if (!Storage.exists(kHiddenPath)) return;
  shelf::parseHidden(Storage.readFile(kHiddenPath).c_str(), hiddenItems);
#endif
}

void saveHiddenItems() {
#if defined(ARDUINO_ARCH_ESP32) || defined(SIMULATOR)
  // An empty set writes an empty file rather than removing it: a file that
  // exists and says nothing is hidden is one state, and a missing file that
  // means the same thing is the same state by another route. One write path,
  // and `exists` above is the only place that has to know both.
  if (!Storage.writeFile(kHiddenPath, String(shelf::formatHidden(hiddenItems).c_str()))) {
    LOG_ERR("SHELF", "Could not write %s; the list is only in RAM until the next boot", kHiddenPath);
  }
#endif
}

// Only when something actually changed. Opening a folder happens on every Back,
// and SPIFFS sectors have a finite erase count, so an unconditional write here
// would be a write per navigation for no gain.
void saveIfChanged(const int folder, const int row) {
  ensureLoaded();
  if (state.lastFolder == folder && (folder < 0 || state.resumeRow[folder] == row)) return;
  state.lastFolder = folder;
  if (folder >= 0) state.resumeRow[folder] = row;
  saveState();
}

// The item wake should reopen, or none. Kept separate from saveIfChanged
// because the two facts change on different events: the position changes as you
// navigate, this changes when an item opens, closes, or is left behind.
void setOpenTitle(const char* title, shelf::LaunchOrigin origin = shelf::LaunchOrigin::Folder) {
  ensureLoaded();
  const char* wanted = title == nullptr ? "" : title;
  if (strcmp(state.openTitle, wanted) == 0 && state.origin == origin) return;
  snprintf(state.openTitle, sizeof(state.openTitle), "%s", wanted);
  state.origin = origin;
  saveState();
}

// Replaces the running activity, or logs and stays put. Every launch in this
// file funnels through here so an OOM cannot leave the shelf thinking it opened
// something it did not.
bool replaceWith(std::unique_ptr<Activity> activity, const char* what) {
  if (!activity) {
    LOG_ERR("SHELF", "OOM opening %s", what);
    return false;
  }
  activityManager.replaceActivity(std::move(activity));
  // Read back from the activity itself rather than written down beside the
  // registry: a table mapping titles to activity names would be a second copy
  // of a fact the activity already carries, and would rot the first time one is
  // renamed. Asked after the request, because Activity::name is not ours to
  // read directly.
  openActivityName = activityManager.currentActivityName();
  return true;
}

}  // namespace

namespace shelf {

const char* label(const char* stableTitle) {
  if (!stableTitle) return "";
  if (std::strcmp(stableTitle, "CHESS") == 0) return tr(STR_UI_CHESS_902609);
  if (std::strcmp(stableTitle, "BATTLESHIP") == 0) return tr(STR_UI_BATTLESHIP_4046EE);
  if (std::strcmp(stableTitle, "SOLITAIRE") == 0) return tr(STR_UI_SOLITAIRE_8C3ACB);
  if (std::strcmp(stableTitle, "HEARTS") == 0) return tr(STR_UI_HEARTS_29FBCB);
  if (std::strcmp(stableTitle, "D&DIAGRAMS") == 0) return tr(STR_UI_D_DIAGRAMS_8F0A7E);
  if (std::strcmp(stableTitle, "CHECKERS") == 0) return tr(STR_UI_CHECKERS_3296AA);
  if (std::strcmp(stableTitle, "CONNECT FOUR") == 0) return tr(STR_UI_CONNECT_FOUR_A81733);
  if (std::strcmp(stableTitle, "MINESWEEPER") == 0) return tr(STR_UI_MINESWEEPER_A2F226);
  if (std::strcmp(stableTitle, "TOY BATTLE") == 0) return tr(STR_UI_TOY_BATTLE_7C7DF7);
  if (std::strcmp(stableTitle, "FOREHEAD") == 0) return tr(STR_UI_FOREHEAD_82AFB5);
  if (std::strcmp(stableTitle, "THE INTERCEPT") == 0) return tr(STR_INTERCEPT_NAME);
  if (std::strcmp(stableTitle, "READING QUEST") == 0) return tr(STR_UI_READING_QUEST_6AD39C);
  if (std::strcmp(stableTitle, "PET") == 0) return tr(STR_PET_APP);
  if (std::strcmp(stableTitle, "FLASHCARDS") == 0) return tr(STR_UI_FLASHCARDS_E0439E);
  if (std::strcmp(stableTitle, "CARNIVORO") == 0) return tr(STR_CAR_TITLE);
  if (std::strcmp(stableTitle, "WORDS") == 0) return tr(STR_WORDS_APP);
  if (std::strcmp(stableTitle, "CALCULATOR") == 0) {
    if (I18N.getLanguage() == Language::VI) return "Máy tính";
    if (I18N.getLanguage() == Language::ES) return "Calculadora";
    return "Calculator";
  }
  if (std::strcmp(stableTitle, "INSTAPAPER") == 0) return "Instapaper";
  if (std::strcmp(stableTitle, "QUOTES") == 0) return tr(STR_UI_QUOTES_171E81);
  if (std::strcmp(stableTitle, "WALLPAPERS") == 0) return tr(STR_UI_WALLPAPERS_2AA278);
  if (std::strcmp(stableTitle, "CALENDAR") == 0) return tr(STR_UI_CALENDAR_09D04E);
  if (std::strcmp(stableTitle, "CLOCK") == 0) return tr(STR_UI_CLOCK_6E3C9D);
  if (std::strcmp(stableTitle, "COUNTER") == 0) return tr(STR_UI_COUNTER_C1AA9D);
  if (std::strcmp(stableTitle, "TO DO LIST") == 0) return tr(STR_UI_TO_DO_LIST_D61BE9);
  if (std::strcmp(stableTitle, "NOTES") == 0) return tr(STR_UI_NOTES_E69FC1);
  if (std::strcmp(stableTitle, "HABITS") == 0) return tr(STR_UI_HABITS_630758);
  if (std::strcmp(stableTitle, "Voice Notes") == 0 || std::strcmp(stableTitle, "VOICE NOTES") == 0) {
    if (I18N.getLanguage() == Language::VI) return "Ghi chú thoại";
    if (I18N.getLanguage() == Language::ES) return "Notas de voz";
    return "Voice Notes";
  }
  if (std::strcmp(stableTitle, "APP STORE") == 0) {
    if (I18N.getLanguage() == Language::VI) return "Cửa hàng App";
    if (I18N.getLanguage() == Language::ES) return "Tienda Apps";
    return "App Store";
  }
  if (std::strcmp(stableTitle, "Games") == 0) return tr(STR_SHORT_GAMES);
  if (std::strcmp(stableTitle, "Apps") == 0) return tr(STR_SHORT_APPS);
  return stableTitle;
}

namespace {
std::vector<shelf::Item> s_games;
std::vector<shelf::Item> s_apps;
std::vector<std::string> s_dynamicTitles;
std::map<std::string, std::string> s_dynamicPaths;
shelf::Folder s_folders[kFolderCount];
bool s_foldersInitialized = false;

std::unique_ptr<Activity> launchDynamicApp(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  auto it = s_dynamicPaths.find(state.openTitle);
  if (it == s_dynamicPaths.end()) return nullptr;
  // A native module (an app's original source) first; anything else is a
  // plugin on the small host API.
  std::unique_ptr<Activity> activity;
  switch (native_module::create(it->second, renderer, mappedInput, activity)) {
    case native_module::Result::Ok:
      return activity;
    case native_module::Result::NotNative:
      return DynamicAppActivity::create(it->second, renderer, mappedInput);
    case native_module::Result::AbiMismatch:
      // Built for another firmware (the device was updated since): the store
      // lists it as needing an update.
      LOG_ERR("SHELF", "%s was built for another firmware; opening the App Store", it->second.c_str());
      return AppStoreActivity::createUpdater(renderer, mappedInput, true);
    default:
      LOG_ERR("SHELF", "Could not load module %s", it->second.c_str());
      return nullptr;
  }
}

void scanDirectoryForApps(const char* dirPath, std::vector<shelf::Item>& list, const freeink::Icon* icon) {
  if (!Storage.exists(dirPath)) return;
  std::vector<String> files = Storage.listFiles(dirPath, 50);
  for (const auto& file : files) {
    if (file.endsWith(".elf") || file.endsWith(".so")) {
      if (file.endsWith(".stripped.elf")) continue;

      String name = file;
      int dot = name.lastIndexOf('.');
      if (dot > 0) name = name.substring(0, dot);
      name.toUpperCase();

      std::string title = name.c_str();
      std::string fullPath = std::string(dirPath) + "/" + file.c_str();

      bool exists = false;
      for (auto& item : list) {
        // "CONNECT FOUR" ships as connectfour.elf: compare without spaces.
        std::string bare = item.title;
        bare.erase(std::remove(bare.begin(), bare.end(), ' '), bare.end());
        if (title == item.title || title == bare) {
          exists = true;
          s_dynamicPaths[item.title] = fullPath;
          item.create = &launchDynamicApp;
          break;
        }
      }
      if (exists) continue;

      s_dynamicTitles.push_back(title);
      const char* stableTitle = s_dynamicTitles.back().c_str();
      s_dynamicPaths[stableTitle] = fullPath;

      list.push_back(shelf::Item{stableTitle, icon, &launchDynamicApp});
    }
  }
}

void ensureFoldersPopulated() {
  if (s_foldersInitialized) return;
  s_games.assign(kGames, kGames + sizeof(kGames) / sizeof(kGames[0]));
  s_apps.assign(kApps, kApps + sizeof(kApps) / sizeof(kApps[0]));

#if defined(ARDUINO_ARCH_ESP32) || defined(SIMULATOR)
  if (Storage.ready()) {
    scanDirectoryForApps("/games", s_games, &icon_chess_32);
    scanDirectoryForApps("/sdcard/games", s_games, &icon_chess_32);
    scanDirectoryForApps("/apps", s_apps, &icon_calculator_32);
    scanDirectoryForApps("/sdcard/apps", s_apps, &icon_calculator_32);
  }
#endif

  // Module rows whose file is not on the card are not installed: hide them.
  for (auto* list : {&s_games, &s_apps}) {
    list->erase(std::remove_if(list->begin(), list->end(),
                               [](const shelf::Item& item) { return item.create == &moduleNotInstalled; }),
                list->end());
  }

  s_folders[0] = {kFolders[0].title, kFolders[0].icon, kFolders[0].mark, s_games.data(), static_cast<int>(s_games.size())};
  s_folders[1] = {kFolders[1].title, kFolders[1].icon, kFolders[1].mark, s_apps.data(), static_cast<int>(s_apps.size())};
  s_foldersInitialized = true;
}
}  // namespace

const Folder* folders() {
  ensureFoldersPopulated();
  return s_folders;
}

int folderCount() { return kFolderCount; }

void refreshApps() {
  s_foldersInitialized = false;
  s_games.clear();
  s_apps.clear();
  s_dynamicTitles.clear();
  s_dynamicPaths.clear();
  ensureFoldersPopulated();
}

void openFolder(const int index, GfxRenderer& renderer, MappedInputManager& mappedInput) {
  if (index < 0 || index >= kFolderCount) {
    LOG_ERR("SHELF", "Bad folder index: %d", index);
    return;
  }
  openFolderIndex = -1;
  homeReturn = true;
  ensureLoaded();
  saveIfChanged(index, state.resumeRow[index]);
  setOpenTitle(nullptr);
  replaceWith(ShelfFolderActivity::create(renderer, mappedInput, index), folders()[index].title);
}

bool openItem(const int folder, const int item, GfxRenderer& renderer, MappedInputManager& mappedInput,
              const LaunchOrigin origin) {
  if (folder < 0 || folder >= kFolderCount) {
    LOG_ERR("SHELF", "Bad folder index: %d", folder);
    return false;
  }
  const Folder& parent = folders()[folder];
  if (item < 0 || item >= parent.count) {
    LOG_ERR("SHELF", "Bad item index %d in %s", item, parent.title);
    return false;
  }

  openFolderIndex = origin == LaunchOrigin::Folder ? folder : -1;
  homeReturn = origin == LaunchOrigin::Folder;
  saveIfChanged(folder, item);
  setOpenTitle(parent.items[item].title, origin);
  auto activity = parent.items[item].create(renderer, mappedInput);
  if (activity)
    activity->setInterfaceFontSection(folder == 0 ? menufont::Section::Games : menufont::Section::Apps,
                                      menufont::itemFor(parent.items[item].title));
  if (!replaceWith(std::move(activity), parent.items[item].title)) {
    openFolderIndex = -1;
    setOpenTitle(nullptr);
    return false;
  }
  return true;
}

auto titlesOf(const int folder) {
  return [folder](const int i) { return folders()[folder].items[i].title; };
}

bool isHidden(const int folder, const int item) {
  if (folder < 0 || folder >= kFolderCount) return false;
  if (item < 0 || item >= folders()[folder].count) return false;
  ensureHiddenLoaded();
  return hiddenItems.contains(folders()[folder].items[item].title);
}

void setHidden(const int folder, const int item, const bool hide) {
  if (folder < 0 || folder >= kFolderCount) {
    LOG_ERR("SHELF", "Bad folder index: %d", folder);
    return;
  }
  if (item < 0 || item >= folders()[folder].count) {
    LOG_ERR("SHELF", "Bad item index %d in %s", item, folders()[folder].title);
    return;
  }
  ensureHiddenLoaded();
  if (!hiddenItems.set(folders()[folder].items[item].title, hide)) return;
  LOG_INF("SHELF", "%s is now %s", folders()[folder].items[item].title, hide ? "hidden" : "shown");
  saveHiddenItems();
}

int shownCount(const int folder) {
  if (folder < 0 || folder >= kFolderCount) return 0;
  ensureHiddenLoaded();
  return shownCountIn(hiddenItems, folders()[folder].count, titlesOf(folder));
}

int shownItem(const int folder, const int row) {
  if (folder < 0 || folder >= kFolderCount) return -1;
  ensureHiddenLoaded();
  return shownItemIn(hiddenItems, folders()[folder].count, row, titlesOf(folder));
}

int shownRowFor(const int folder, const int item) {
  if (folder < 0 || folder >= kFolderCount) return 0;
  ensureHiddenLoaded();
  return shownRowForIn(hiddenItems, folders()[folder].count, item, titlesOf(folder));
}

void autostartFromEnv(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  // Once per process: leaving the app afterwards must land on the shelf like
  // any other exit, not bounce straight back in.
  static bool consumed = false;
  if (consumed) {
    return;
  }
  const char* wanted = std::getenv("CROSSPLAY_AUTOSTART");
  if (wanted == nullptr || *wanted == '\0') {
    return;
  }
  consumed = true;
  int folder = -1;
  int item = -1;
  if (!findItemByTitle(wanted, folder, item)) {
    LOG_ERR("SHELF", "Autostart: no item titled '%s'", wanted);
    return;
  }
  LOG_INF("SHELF", "Autostart into %s", folders()[folder].items[item].title);
  openItem(folder, item, renderer, mappedInput);
}

void rememberForWake(const char* currentActivityName) {
  ensureLoaded();
  if (state.openTitle[0] == '\0') return;
  // An item is only still open if the activity it launched is the one on
  // screen. Leaving a game by the Home gesture never passes through leave(),
  // so without this a sleep taken on Home would resume into the game you left.
  const char* onScreen = currentActivityName == nullptr ? "" : currentActivityName;
  if (openActivityName.empty() || openActivityName != onScreen) {
    LOG_DBG("SHELF", "Sleeping on %s, not %s: nothing to resume", onScreen, state.openTitle);
    setOpenTitle(nullptr);
  }
}

bool resumeFromWake(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  ensureLoaded();
  if (state.openTitle[0] == '\0') return false;

  int folder = -1;
  int item = -1;
  if (!findItemByTitle(state.openTitle, folder, item)) {
    // The card outlives firmware updates, so the item may have been renamed or
    // removed since it was written. Home, and forget it.
    LOG_INF("SHELF", "Wake: nothing titled '%s' any more", state.openTitle);
    setOpenTitle(nullptr);
    return false;
  }

  LOG_INF("SHELF", "Wake: resuming %s", folders()[folder].items[item].title);
  return openItem(folder, item, renderer, mappedInput, state.origin);
}

void openPlayer(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  // Only the folder footer offers it, and only a folder that shows the name, so
  // lastFolder is the folder we are standing in. Recorded before the launch for
  // the same reason openItem does it: replaceActivity destroys the caller.
  ensureLoaded();
  openFolderIndex = state.lastFolder;
  homeReturn = true;
  setOpenTitle(nullptr);
  if (!replaceWith(PlayerActivity::create(renderer, mappedInput), "Player")) {
    openFolderIndex = -1;
  }
}

void openAppStore(const int fromFolder, GfxRenderer& renderer, MappedInputManager& mappedInput) {
  openFolderIndex = fromFolder >= 0 && fromFolder < kFolderCount ? fromFolder : -1;
  homeReturn = true;
  setOpenTitle(nullptr);
  if (!replaceWith(AppStoreActivity::create(renderer, mappedInput), "APP STORE")) openFolderIndex = -1;
}

void leave(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  if (openFolderIndex >= 0) {
    openFolder(openFolderIndex, renderer, mappedInput);
    return;
  }
  activityManager.goHome();
}

int lastFolderOnHome() {
  ensureLoaded();
  return state.lastFolder;
}

bool takeHomeReturn() {
  const bool was = homeReturn;
  homeReturn = false;
  return was;
}

int resumeRowIn(const int index) {
  ensureLoaded();
  if (index < 0 || index >= kFolderCount) return 0;
  // Stored as the ITEM, answered as the ROW. The conversion is here, beside the
  // file, and not in the folder that asks: those are the only two units in this
  // feature and the whole hazard is a caller holding one while believing the
  // other -- they are both small ints in the same range, so nothing would say
  // so. The folder does hold items, at the two points where it must (opening
  // one, hiding one) and through ONE named function that produces them
  // (ShelfFolderActivity::itemAtRow); what it never does is store one in a
  // variable that means a row.
  return shownRowFor(index, state.resumeRow[index]);
}

void rememberRowIn(const int index, const int row) {
  if (index < 0 || index >= kFolderCount) {
    LOG_ERR("SHELF", "Bad folder index: %d", index);
    return;
  }
  // `row` is a SHOWN row, so this is also the range check: a row past the end
  // of what the folder is showing has no item and is refused, which is the same
  // guard the registry count used to give when the two were the same number.
  const int item = shownItem(index, row);
  if (item < 0) {
    LOG_ERR("SHELF", "Bad row %d in %s", row, folders()[index].title);
    return;
  }
  saveIfChanged(index, item);
}

}  // namespace shelf
