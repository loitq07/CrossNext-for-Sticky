#include "pegsolitaire.h"

#include <Arduino.h>

#include <cstdio>

#include "core/graphics.h"
#include "core/storage.h"
#include "core/text.h"
#include "pegsolitaire_flow.h"
#include "pegsolitaire_icon.h"
#include "pegsolitaire_rules.h"
#include "pegsolitaire_save.h"
#include "ui/ui.h"

namespace ps = pegsolitaire;

namespace {

constexpr const char* SAVE_FILE = "pegsolitaire";

// The status capsule and a record line under the band, the board, a message line, and three
// buttons along the bottom.
constexpr int MARGIN = 16;
constexpr int GAP = 12;
constexpr int INNER_WIDTH = SCREEN_WIDTH - 2 * MARGIN;
constexpr int STATUS_TOP = ui::CONTENT_TOP;
constexpr int STATUS_HEIGHT = 44;
constexpr int BEST_TOP = STATUS_TOP + STATUS_HEIGHT + 6;
constexpr int BEST_HEIGHT = 26;
constexpr int BUTTON_TOP = SCREEN_HEIGHT - MARGIN - ui::PILL_HEIGHT;
constexpr int MESSAGE_HEIGHT = 40;
constexpr int MESSAGE_TOP = BUTTON_TOP - 8 - MESSAGE_HEIGHT;
constexpr int ROOM_TOP = BEST_TOP + BEST_HEIGHT + 8;
constexpr int ROOM_HEIGHT = MESSAGE_TOP - 8 - ROOM_TOP;
constexpr int BUTTONS = 3;
constexpr int BUTTON_WIDTH = (INNER_WIDTH - (BUTTONS - 1) * GAP) / BUTTONS;

// The source's sizes: English holes 62 apart, the triangle's 86 across and 80 down.
constexpr int ENGLISH_STEP = 62;
constexpr int TRIANGLE_STEP_X = 86;
constexpr int TRIANGLE_STEP_Y = 80;
constexpr int HOLE_RADIUS = 6;
constexpr int PEG_WEIGHT = 4;
constexpr int TARGET_RADIUS = 14;

enum Button { UNDO, NEW, BOARD };

int buttonLeft(int b) { return MARGIN + b * (BUTTON_WIDTH + GAP); }

// Where hole i goes on the screen, the peg's size and how far a tap may land from it. draw()
// and onTap() both ask, so they can't drift apart.
struct Spot {
  int x;
  int y;
};

int pegRadius(int board) { return board == 0 ? 20 : 24; }
int hitReach(int board) { return board == 0 ? ENGLISH_STEP / 2 : 40; }

Spot spotOf(int board, int i) {
  const ps::BoardDef& b = ps::kBoardList[board];
  const int r = b.row[i];
  const int c = b.col[i];
  if (board == 0) {
    const int side = 7 * ENGLISH_STEP;
    const int left = (SCREEN_WIDTH - side) / 2;
    const int top = ROOM_TOP + (ROOM_HEIGHT - side) / 2;
    return {left + (c - 1) * ENGLISH_STEP + ENGLISH_STEP / 2, top + (r - 1) * ENGLISH_STEP + ENGLISH_STEP / 2};
  }
  const int height = 4 * TRIANGLE_STEP_Y;
  const int top = ROOM_TOP + (ROOM_HEIGHT - height) / 2;
  return {SCREEN_WIDTH / 2 + (c - 1) * TRIANGLE_STEP_X - (r - 1) * TRIANGLE_STEP_X / 2, top + (r - 1) * TRIANGLE_STEP_Y};
}

void drawTextRight(uint8_t* frame, Font font, int right, int top, int height, const char* text) {
  ui::drawTextLeft(frame, font, right - textWidth(font, text), top, height, text);
}

class PegSolitaire final : public Game {
 public:
  const char* name() const override { return "PEG SOLITAIRE"; }
  const uint8_t* icon() const override { return pegsolitaireIcon::ICON; }

  void open() override {
    if (!loaded_) {
      loaded_ = true;
      ps::newGame(state_, 0);
      load();
      // A board saved already finished still counts towards the best.
      if (ps::finish(state_) == ps::Change::Over && state_.bestChanged) persist();
      state_.bestChanged = false;
    }
    state_.sel = ps::kNoCell;
    state_.message = ps::Message::Help;
  }

  TapResult onTap(int x, int y) override {
    if (ui::isOnBack(x, y)) return TapResult::Leave;
    for (int b = 0; b < BUTTONS; b++) {
      if (ui::isInBox(x, y, buttonLeft(b), BUTTON_TOP, BUTTON_WIDTH, ui::PILL_HEIGHT)) return tapButton(static_cast<Button>(b));
    }
    const int board = state_.game.board;
    const int reach = hitReach(board);
    for (int i = 0; i < ps::kBoardList[board].cells; i++) {
      const Spot s = spotOf(board, i);
      if (x > s.x - reach && x < s.x + reach && y > s.y - reach && y < s.y + reach) return apply(ps::tapHole(state_, i));
    }
    return apply(ps::tapOff(state_));
  }

  void draw(uint8_t* frame) override {
    ui::drawBand(frame, name(), nullptr, true);
    const ps::Game& g = state_.game;
    const ps::BoardDef& b = ps::def(g);
    const bool over = ps::isOver(state_);

    // Outlined while playing, and solid with the rating once no jump is left: a fill that
    // appears once, with the full refresh, is the payoff.
    char text[48];
    if (over) {
      ratingText(text, sizeof(text));
      fillRect(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, true);
    } else {
      std::snprintf(text, sizeof(text), "%s   PEGS %d   JUMPS %d", b.name, ps::pegCount(g), g.jumps);
      drawRectOutline(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, 2);
    }
    const Font statusFont = textWidth(Font::Size20, text) <= INNER_WIDTH - 16 ? Font::Size20 : Font::Size14;
    drawTextInBox(frame, statusFont, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, text, !over);

    const uint8_t best = state_.best[g.board];
    if (best) {
      std::snprintf(text, sizeof(text), "BEST ON THIS BOARD: %u %s", best, best == 1 ? "PEG" : "PEGS");
    } else {
      std::snprintf(text, sizeof(text), "NO BEST ON THIS BOARD YET");
    }
    ui::drawTextLeft(frame, Font::Size14, MARGIN, BEST_TOP, BEST_HEIGHT, text);
    // Once over, the capsule holds the rating, so the jump count moves here.
    if (over) {
      std::snprintf(text, sizeof(text), "JUMPS %d", g.jumps);
      drawTextRight(frame, Font::Size14, MARGIN + INNER_WIDTH, BEST_TOP, BEST_HEIGHT, text);
    }

    drawBoard(frame);

    drawTextInBox(frame, Font::Size14, MARGIN, MESSAGE_TOP, INNER_WIDTH, MESSAGE_HEIGHT, messageText());

    for (int bt = 0; bt < BUTTONS; bt++) {
      const char* label = bt == UNDO ? "UNDO" : bt == NEW ? "NEW" : b.name;
      const bool can = bt == UNDO ? ps::canUndo(state_) : bt == NEW ? ps::canNew(state_) : true;
      if (!can) {
        ui::drawDimmedPill(frame, buttonLeft(bt), BUTTON_TOP, BUTTON_WIDTH, label);
      } else {
        // The board button names the board you're on, as in the source, so it's outlined
        // like a setting.
        ui::drawPill(frame, buttonLeft(bt), BUTTON_TOP, BUTTON_WIDTH, label, bt != BOARD);
      }
    }
  }

  bool saveFailed() const override { return saveFailed_; }
  void retrySave() override { persist(); }

 private:
  TapResult tapButton(Button b) {
    switch (b) {
      case UNDO:
        return apply(ps::undoJump(state_));
      case NEW:
        return apply(ps::startAgain(state_));
      case BOARD:
        return apply(ps::switchBoard(state_));
    }
    return TapResult::Nothing;
  }

  // The selection and messages live only in memory; every jump is saved at once.
  TapResult apply(ps::Change change) {
    switch (change) {
      case ps::Change::None:
        return TapResult::Nothing;
      case ps::Change::View:
        return TapResult::Redraw;
      case ps::Change::Move:
        persist();
        return TapResult::Redraw;
      case ps::Change::NewGame:
      case ps::Change::Over:
        persist();
        state_.bestChanged = false;
        return TapResult::RedrawFull;
    }
    return TapResult::Nothing;
  }

  void ratingText(char* out, size_t size) const {
    const int n = ps::pegCount(state_.game);
    switch (ps::rate(state_.game)) {
      case ps::Rating::Perfect:
        std::snprintf(out, size, "PERFECT: ONE PEG, IN THE CENTRE!");
        return;
      case ps::Rating::Excellent:
        std::snprintf(out, size, "EXCELLENT: ONE PEG LEFT!");
        return;
      case ps::Rating::VeryGood:
        std::snprintf(out, size, "VERY GOOD: TWO PEGS LEFT.");
        return;
      case ps::Rating::Good:
        std::snprintf(out, size, "GOOD: %d PEGS LEFT.", n);
        return;
      case ps::Rating::TryAgain:
        std::snprintf(out, size, "%d PEGS LEFT. TRY AGAIN!", n);
        return;
    }
  }

  const char* messageText() const {
    if (ps::isOver(state_)) return "NO MORE JUMPS: UNDO, OR NEW.";
    switch (state_.message) {
      case ps::Message::Selected:
        return "TAP A MARKED HOLE TO JUMP.";
      case ps::Message::CannotJump:
        return "THAT PEG CANNOT JUMP.";
      case ps::Message::PickMarked:
        return "PICK ONE OF THE MARKED HOLES.";
      case ps::Message::Help:
        break;
    }
    return "TAP A PEG, THEN THE HOLE TO JUMP TO.";
  }

  // Pegs are thick rings and holes small circles, as in the source: the board changes every
  // jump, so nothing big is filled.
  void drawBoard(uint8_t* frame) {
    const ps::Game& g = state_.game;
    const int board = g.board;
    const ps::BoardDef& b = ps::def(g);
    bool target[ps::kMaxCells] = {};
    if (state_.sel != ps::kNoCell) {
      ps::Jump jumps[6];
      const int n = ps::jumpsFrom(g, state_.sel, jumps);
      for (int k = 0; k < n; k++) target[jumps[k].to] = true;
    }
    const int r = pegRadius(board);
    for (int i = 0; i < b.cells; i++) {
      const Spot s = spotOf(board, i);
      if (ps::hasPeg(g, i)) {
        drawRing(frame, s.x, s.y, r, PEG_WEIGHT, true);
        drawRing(frame, s.x, s.y, 5, 5, true);
        if (i == state_.sel) {
          const int side = r + 7;
          drawRectOutline(frame, s.x - side, s.y - side, 2 * side, 2 * side, ui::RULE);
        }
      } else {
        drawRing(frame, s.x, s.y, HOLE_RADIUS, 2, true);
        if (target[i]) drawRing(frame, s.x, s.y, TARGET_RADIUS, 3, true);
      }
    }
  }

  void load() {
    if (!storage::ready()) return;
    uint8_t data[ps::SAVE_MAX];
    uint8_t version = 0;
    size_t size = 0;
    if (storage::load(SAVE_FILE, version, data, sizeof(data), size) && !ps::unpack(version, data, size, state_)) {
      Serial.println("Ignoring saved Peg Solitaire game");
    }
  }

  // The save holds the best on each board as well as the game, so it's kept after a finish.
  void persist() {
    if (!storage::ready()) {
      saveFailed_ = false;
      return;
    }
    uint8_t data[ps::SAVE_MAX];
    const size_t size = ps::pack(state_, data);
    saveFailed_ = !storage::save(SAVE_FILE, ps::SAVE_VERSION, data, size);
  }

  ps::State state_{};
  bool loaded_ = false;
  bool saveFailed_ = false;
};

}  // namespace

Game& pegSolitaireGame() {
  static PegSolitaire game;
  return game;
}
