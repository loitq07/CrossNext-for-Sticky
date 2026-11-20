#include "dungeon.h"

#include <Arduino.h>

#include "core/storage.h"
#include "dungeon_art.h"
#include "dungeon_guide.h"
#include "dungeon_rules.h"
#include "dungeon_save.h"
#include "dungeon_screens.h"
#include "ui/ui.h"

// D&Diagrams: a nonogram whose clues are a dungeon. Ported from CrossPlay's
// DungeonActivity.cpp, with the same views and the same ways between them.
namespace {

constexpr const char* SAVE_FILE = "dungeon";

namespace ui_ = dungeonui;
using Button = dungeonui::Button;

class DungeonGame final : public Game {
 public:
  // Saved in session.sav to reopen the game after waking, so don't rename it.
  const char* name() const override { return "D&DIAGRAMS"; }
  const uint8_t* icon() const override { return dungeonArt::HOME_ICON; }

  // Memory is the truth once loaded, so progress also survives without a card. Like
  // CrossPlay, every entry opens on the menu, with RESUME if the picked dungeon has
  // marks on it.
  void open() override {
    if (!loaded_) {
      loaded_ = true;
      board_.load(dungeon::kCampaignFirst);
      load();
    }
    selected_ = board_.index();
    view_ = View::Menu;
  }

  TapResult onTap(int x, int y) override {
    if (ui::isOnBack(x, y)) return back();

    switch (view_) {
      case View::Menu:
        return tapMenu(x, y);
      case View::Guide:
        return press(ui_::guideButtonAt(x, y));
      case View::Board:
        return tapBoard(x, y);
      case View::Won:
        return press(ui_::winButtonAt(x, y, moreToPlay()));
    }
    return TapResult::Nothing;
  }

  // On the map, Up and Down step the brackets back and forward through the 64
  // dungeons, row by row, wrapping at either end. Elsewhere they do nothing.
  TapResult onUpPressed() override { return stepSelection(-1); }
  TapResult onDownPressed() override { return stepSelection(1); }

  void draw(uint8_t* frame) override {
    switch (view_) {
      case View::Menu: {
        ui_::MenuModel model;
        model.selectedIndex = selected_;
        model.solvedCount = progress_.solvedCount();
        model.total = dungeon::kCampaignCount;
        // RESUME only when the picked dungeon is the one with marks on it.
        model.hasProgress = selected_ == board_.index() && board_.touched();
        model.progress = &progress_;
        ui_::drawMenu(frame, model, picker_);
        break;
      }
      case View::Guide: {
        ui_::GuideModel model;
        model.page = guidePage_;
        ui_::drawGuide(frame, model);
        break;
      }
      case View::Board: {
        ui_::BoardModel model;
        model.board = &board_;
        model.solvedCount = progress_.solvedCount();
        model.total = dungeon::kCampaignCount;
        ui_::drawBoard(frame, model, layout_);
        break;
      }
      case View::Won: {
        ui_::WinModel model;
        model.clearedIndex = lastCleared_;
        model.solvedCount = progress_.solvedCount();
        model.total = dungeon::kCampaignCount;
        model.moreToPlay = moreToPlay();
        ui_::drawWin(frame, model);
        break;
      }
    }
  }

  bool saveFailed() const override { return saveFailed_; }

  void retrySave() override { persist(); }

 private:
  enum class View : uint8_t { Menu, Guide, Board, Won };

  // CrossPlay's test, kept as it is. It compares against all 65 dungeons, the
  // tutorial included, so NEXT never dims; see README.md.
  bool moreToPlay() const { return progress_.solvedCount() < dungeon::kPuzzleCount; }

  TapResult back() {
    switch (view_) {
      case View::Menu:
        return TapResult::Leave;
      case View::Guide:
        return press(Button::GuidePrevious);
      case View::Board:
      case View::Won:
        view_ = View::Menu;
        return TapResult::Redraw;
    }
    return TapResult::Nothing;
  }

  TapResult tapMenu(int x, int y) {
    const Button button = ui_::menuButtonAt(x, y);
    if (button != Button::None) return press(button);

    // Tapping the map picks a dungeon. It doesn't open it, so a stray tap can't
    // drop the player into a dungeon they didn't choose.
    const int spot = picker_.indexAt(x, y);
    if (spot < 0) return TapResult::Nothing;
    const int picked = dungeon::kCampaignFirst + spot;
    if (!dungeon::isPlayable(picked) || picked == selected_) return TapResult::Nothing;
    selected_ = picked;
    return TapResult::Redraw;
  }

  TapResult stepSelection(int delta) {
    if (view_ != View::Menu) return TapResult::Nothing;
    const int offset = selected_ - dungeon::kCampaignFirst + delta;
    selected_ = dungeon::kCampaignFirst + (offset + dungeon::kCampaignCount) % dungeon::kCampaignCount;
    return TapResult::Redraw;
  }

  TapResult tapBoard(int x, int y) {
    const Button button = ui_::boardButtonAt(x, y);
    if (button != Button::None) return press(button);

    int row = 0;
    int col = 0;
    if (!layout_.cellAt(x, y, row, col)) return TapResult::Nothing;

    // A monster or a chest refuses a mark, and that tap changes nothing.
    const uint64_t walls = board_.wallMask();
    const uint64_t floors = board_.floorMask();
    board_.tap(row, col);
    if (board_.wallMask() == walls && board_.floorMask() == floors) return TapResult::Nothing;

    if (board_.solved()) return settleWin();
    persist();
    return TapResult::Redraw;
  }

  TapResult press(Button button) {
    switch (button) {
      case Button::None:
        return TapResult::Nothing;
      case Button::Play:
        return openPuzzle(selected_);
      case Button::Guide:
        guidePage_ = 0;
        view_ = View::Guide;
        return TapResult::Redraw;
      case Button::GuidePrevious:
        if (guidePage_ > 0) {
          guidePage_--;
        } else {
          view_ = View::Menu;
        }
        return TapResult::Redraw;
      case Button::GuideNext:
        // Past the last page is the map, so the reader picks where to start.
        if (guidePage_ + 1 < ui_::guidePageCount()) {
          guidePage_++;
        } else {
          view_ = View::Menu;
        }
        return TapResult::Redraw;
      case Button::Reset:
        if (!board_.touched()) return TapResult::Nothing;
        board_.reset();
        persist();
        return TapResult::Redraw;
      case Button::Menu:
        view_ = View::Menu;
        return TapResult::Redraw;
      case Button::Next:
        return openPuzzle(progress_.nextUnsolved());
    }
    return TapResult::Nothing;
  }

  // Every way into a board comes through here, so the tutorial can never open as a
  // level. Reopening the dungeon in hand keeps its marks; another one starts clean.
  TapResult openPuzzle(int requested) {
    const int index = dungeon::isPlayable(requested) ? requested : dungeon::kCampaignFirst;
    if (index != board_.index()) {
      board_.load(index);
      persist();
    }
    view_ = View::Board;
    return TapResult::RedrawFull;
  }

  // Finishing a dungeon records it once, and moves the save on to the next one,
  // because a finished board isn't worth resuming. The full refresh is the payoff.
  TapResult settleWin() {
    lastCleared_ = board_.index();
    progress_.markSolved(board_.index());
    board_.load(progress_.nextUnsolved());
    selected_ = board_.index();
    persist();
    view_ = View::Won;
    return TapResult::RedrawFull;
  }

  void load() {
    if (!storage::ready()) return;

    uint8_t data[dungeon::SAVE_BYTES];
    uint8_t version = 0;
    size_t size = 0;
    if (storage::load(SAVE_FILE, version, data, sizeof(data), size) &&
        !dungeon::unpackGame(version, data, size, board_, progress_)) {
      Serial.println("Ignoring saved D&Diagrams game");
    }
  }

  // Saved on every change. The record is the campaign, so the save is never erased.
  void persist() {
    if (!storage::ready()) {
      saveFailed_ = false;
      return;
    }
    uint8_t data[dungeon::SAVE_BYTES];
    const size_t size = dungeon::packGame(board_, progress_, data);
    saveFailed_ = !storage::save(SAVE_FILE, dungeon::SAVE_VERSION, data, size);
  }

  dungeon::Board board_;
  dungeon::Progress progress_;
  ui_::Layout layout_;
  ui_::PickerLayout picker_;
  View view_ = View::Menu;
  // The dungeon the map has picked, which PLAY opens. Not always the one in board_:
  // looking around the map doesn't disturb the game in progress.
  int selected_ = dungeon::kCampaignFirst;
  int guidePage_ = 0;
  int lastCleared_ = dungeon::kCampaignFirst;
  bool loaded_ = false;
  bool saveFailed_ = false;
};

}  // namespace

Game& dungeonGame() {
  static DungeonGame game;
  return game;
}
