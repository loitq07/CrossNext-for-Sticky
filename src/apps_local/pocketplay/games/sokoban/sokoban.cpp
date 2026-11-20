#include "sokoban.h"

#include <Arduino.h>

#include <cstdio>

#include "core/graphics.h"
#include "core/storage.h"
#include "core/text.h"
#include "sokoban_flow.h"
#include "sokoban_icon.h"
#include "sokoban_rules.h"
#include "sokoban_save.h"
#include "ui/ui.h"

namespace sk = sokoban;

namespace {

constexpr const char* GAME_BLOB = "sokoban";
constexpr const char* BEST_BLOB = "sokoban-best";

// The status capsule under the band, a line of counts, the board in whatever room is left,
// a message line, and five buttons along the bottom.
constexpr int MARGIN = 16;
constexpr int GAP = 12;
constexpr int INNER_WIDTH = SCREEN_WIDTH - 2 * MARGIN;
constexpr int STATUS_TOP = ui::CONTENT_TOP;
constexpr int STATUS_HEIGHT = 44;
constexpr int COUNTS_TOP = STATUS_TOP + STATUS_HEIGHT + 6;
constexpr int COUNTS_HEIGHT = 26;
constexpr int BUTTON_TOP = SCREEN_HEIGHT - MARGIN - ui::PILL_HEIGHT;
constexpr int MESSAGE_HEIGHT = 36;
constexpr int MESSAGE_TOP = BUTTON_TOP - 8 - MESSAGE_HEIGHT;
constexpr int ROOM_TOP = COUNTS_TOP + COUNTS_HEIGHT + 8;
constexpr int ROOM_HEIGHT = MESSAGE_TOP - 8 - ROOM_TOP;
constexpr int MAX_CELL = 50;
constexpr int MIN_CELL = 12;
constexpr int BUTTONS = 5;
constexpr int BUTTON_WIDTH = (INNER_WIDTH - (BUTTONS - 1) * GAP) / BUTTONS;

// The reset dialog, over the board.
constexpr int DIALOG_WIDTH = INNER_WIDTH;
constexpr int DIALOG_HEIGHT = 200;
constexpr int DIALOG_TOP = ROOM_TOP + (ROOM_HEIGHT - DIALOG_HEIGHT) / 2;
constexpr int DIALOG_PILL_TOP = DIALOG_TOP + DIALOG_HEIGHT - 24 - ui::PILL_HEIGHT;
constexpr int DIALOG_PILL_WIDTH = (DIALOG_WIDTH - 2 * 24 - GAP) / 2;
constexpr int YES_LEFT = MARGIN + 24;
constexpr int NO_LEFT = YES_LEFT + DIALOG_PILL_WIDTH + GAP;

// The pack list: one row each, then the credits.
constexpr int PACK_ROW_STEP = ui::ROW_HEIGHT + ui::ROW_GAP;
constexpr int CREDITS_TOP = ui::CONTENT_TOP + sk::kPackCount * PACK_ROW_STEP + 16;

constexpr const char* HELP_TEXT = "SWIPE, OR TAP BESIDE THE PLAYER.";
constexpr const char* FULL_TEXT = "8000 MOVES: UNDO OR RESET.";
constexpr const char* CREDITS_TEXT =
    "MICROBAN AND SASQUATCH BY DAVID W. SKINNER. LOMA BY AYMERIC DU PELOUX AND OTHERS. MICROCOSMOS AND "
    "MINICOSMOS BY AYMERIC DU PELOUX.";

enum Button { UNDO, RESET, PREV, NEXT, PACKS };

int buttonLeft(int b) { return MARGIN + b * (BUTTON_WIDTH + GAP); }

// Where the board goes for a level, which draw() and onTap() both ask for, so the two
// can't drift apart.
struct Layout {
  int cell = MAX_CELL;
  int left = 0;
  int top = ROOM_TOP;
};

Layout layoutFor(const sk::Board& b) {
  Layout l;
  int cell = MAX_CELL;
  if (INNER_WIDTH / b.cols < cell) cell = INNER_WIDTH / b.cols;
  if (ROOM_HEIGHT / b.rows < cell) cell = ROOM_HEIGHT / b.rows;
  if (cell < MIN_CELL) cell = MIN_CELL;
  l.cell = cell;
  l.left = (SCREEN_WIDTH - cell * b.cols) / 2;
  l.top = ROOM_TOP + (ROOM_HEIGHT - cell * b.rows) / 2;
  return l;
}

// Text whose right edge is at `right`.
void drawTextRight(uint8_t* frame, Font font, int right, int top, int height, const char* text) {
  ui::drawTextLeft(frame, font, right - textWidth(font, text), top, height, text);
}

class Sokoban final : public Game {
 public:
  const char* name() const override { return "SOKOBAN"; }
  const uint8_t* icon() const override { return sokobanIcon::ICON; }

  // Memory is the truth once loaded. A solved level stays until NEXT, as in the source.
  void open() override {
    if (!loaded_) {
      loaded_ = true;
      sk::loadLevel(state_);
      load();
    }
    state_.mode = sk::Mode::Play;
    if (sk::isSolved(state_)) {
      if (state_.message != sk::Message::NewBest) state_.message = sk::Message::Solved;
    } else {
      state_.message = sk::Message::Title;
    }
  }

  TapResult onTap(int x, int y) override {
    if (ui::isOnBack(x, y)) {
      if (state_.mode == sk::Mode::Packs) return apply(sk::closePacks(state_));
      return TapResult::Leave;
    }
    switch (state_.mode) {
      case sk::Mode::Packs:
        for (int p = 0; p < sk::kPackCount; p++) {
          if (ui::isInBox(x, y, ui::MARGIN, ui::CONTENT_TOP + p * PACK_ROW_STEP, SCREEN_WIDTH - 2 * ui::MARGIN, ui::ROW_HEIGHT)) {
            return apply(sk::choosePack(state_, p));
          }
        }
        return TapResult::Nothing;
      case sk::Mode::ConfirmReset:
        if (ui::isInBox(x, y, YES_LEFT, DIALOG_PILL_TOP, DIALOG_PILL_WIDTH, ui::PILL_HEIGHT)) return apply(sk::answerReset(state_, true));
        if (ui::isInBox(x, y, NO_LEFT, DIALOG_PILL_TOP, DIALOG_PILL_WIDTH, ui::PILL_HEIGHT)) return apply(sk::answerReset(state_, false));
        return TapResult::Nothing;
      case sk::Mode::Play:
        break;
    }

    for (int b = 0; b < BUTTONS; b++) {
      if (ui::isInBox(x, y, buttonLeft(b), BUTTON_TOP, BUTTON_WIDTH, ui::PILL_HEIGHT)) return tapButton(static_cast<Button>(b));
    }
    if (!ui::isInBox(x, y, MARGIN, ROOM_TOP, INNER_WIDTH, ROOM_HEIGHT)) return TapResult::Nothing;

    // A tap on the board steps toward it from the player, along the axis it's further on.
    const sk::Board& b = state_.game.board;
    const Layout l = layoutFor(b);
    const int px = l.left + (b.player % b.cols) * l.cell + l.cell / 2;
    const int py = l.top + (b.player / b.cols) * l.cell + l.cell / 2;
    const int dx = x - px;
    const int dy = y - py;
    if (dx > -l.cell / 2 && dx < l.cell / 2 && dy > -l.cell / 2 && dy < l.cell / 2) return TapResult::Nothing;
    return apply(sk::go(state_, sk::dirOf(dx, dy)));
  }

  TapResult onSwipe(int fromX, int fromY, int toX, int toY) override {
    return apply(sk::go(state_, sk::dirOf(toX - fromX, toY - fromY)));
  }

  TapResult onUpPressed() override { return apply(sk::upButton(state_)); }
  TapResult onDownPressed() override { return apply(sk::downButton(state_)); }

  void draw(uint8_t* frame) override {
    if (state_.mode == sk::Mode::Packs) {
      drawPacks(frame);
      return;
    }
    ui::drawBand(frame, name(), nullptr, true);

    // Outlined while playing, and solid once solved: a fill that appears once, with the
    // full refresh, is the payoff rather than a cost.
    const bool won = sk::isSolved(state_);
    const sk::PackDef& pack = sk::kPacks[state_.pack];
    char text[64];
    std::snprintf(text, sizeof(text), "%s   %d/%d", won ? "SOLVED" : pack.label, state_.place[state_.pack] + 1, pack.count);
    if (won) {
      fillRect(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, true);
    } else {
      drawRectOutline(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, 2);
    }
    drawTextInBox(frame, Font::Size20, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, text, !won);

    std::snprintf(text, sizeof(text), "MOVES %u   PUSHES %u", state_.game.moves, state_.game.pushes);
    ui::drawTextLeft(frame, Font::Size14, MARGIN, COUNTS_TOP, COUNTS_HEIGHT, text);
    const uint16_t best = sk::currentBest(state_);
    if (best) {
      std::snprintf(text, sizeof(text), "BEST %u", best);
    } else {
      std::snprintf(text, sizeof(text), "BEST -");
    }
    drawTextRight(frame, Font::Size14, MARGIN + INNER_WIDTH, COUNTS_TOP, COUNTS_HEIGHT, text);

    if (state_.mode == sk::Mode::ConfirmReset) {
      drawConfirm(frame);
    } else {
      drawBoard(frame);
    }

    drawTextInBox(frame, Font::Size14, MARGIN, MESSAGE_TOP, INNER_WIDTH, MESSAGE_HEIGHT, messageText());

    static const char* const LABELS[BUTTONS] = {"UNDO", "RESET", "PREV", "NEXT", "PACKS"};
    for (int b = 0; b < BUTTONS; b++) {
      if (canAct(static_cast<Button>(b))) {
        ui::drawPill(frame, buttonLeft(b), BUTTON_TOP, BUTTON_WIDTH, LABELS[b], true);
      } else {
        ui::drawDimmedPill(frame, buttonLeft(b), BUTTON_TOP, BUTTON_WIDTH, LABELS[b]);
      }
    }
  }

  bool saveFailed() const override { return saveFailed_; }

  void retrySave() override {
    persistGame();
    if (!saveFailed_) persistBest();
  }

 private:
  bool canAct(Button b) const {
    switch (b) {
      case UNDO:
        return sk::canUndo(state_);
      case RESET:
        return sk::canReset(state_);
      default:
        return state_.mode == sk::Mode::Play;
    }
  }

  TapResult tapButton(Button b) {
    switch (b) {
      case UNDO:
        return apply(sk::undoMove(state_));
      case RESET:
        return apply(sk::askReset(state_));
      case PREV:
        return apply(sk::stepLevel(state_, -1));
      case NEXT:
        return apply(sk::stepLevel(state_, 1));
      case PACKS:
        return apply(sk::openPacks(state_));
    }
    return TapResult::Nothing;
  }

  // The list and the dialog live only in memory; everything else is saved at once.
  TapResult apply(sk::Change change) {
    switch (change) {
      case sk::Change::None:
        return TapResult::Nothing;
      case sk::Change::View:
        return TapResult::Redraw;
      case sk::Change::Move:
        persistGame();
        return TapResult::Redraw;
      case sk::Change::NewLevel:
        persistGame();
        return TapResult::RedrawFull;
      case sk::Change::Solved:
        persistGame();
        if (state_.bestChanged) persistBest();
        return TapResult::RedrawFull;
    }
    return TapResult::Nothing;
  }

  const char* messageText() {
    switch (state_.message) {
      case sk::Message::Title:
        return sk::currentLevel(state_).title;
      case sk::Message::Solved:
      case sk::Message::NewBest:
        // The capsule already says SOLVED, and the counts line has the pushes.
        std::snprintf(messageText_, sizeof(messageText_), "%s %u MOVES. NEXT FOR ANOTHER.",
                      state_.message == sk::Message::NewBest ? "NEW BEST:" : "SOLVED IN", state_.game.moves);
        return messageText_;
      case sk::Message::HistoryFull:
        return FULL_TEXT;
      case sk::Message::None:
        break;
    }
    return HELP_TEXT;
  }

  // Walls never change, so they can afford to be solid. Everything that moves is outlined,
  // and black only where it's the player's own progress: a box home, and the player.
  void drawBoard(uint8_t* frame) {
    const sk::Board& b = state_.game.board;
    const Layout l = layoutFor(b);
    const int c = l.cell;
    const int stroke = c >= 30 ? 3 : 2;
    for (int i = 0; i < b.rows * b.cols; i++) {
      const int x = l.left + (i % b.cols) * c;
      const int y = l.top + (i / b.cols) * c;
      const char ch = b.cells[i];
      if (ch == '#') {
        fillRect(frame, x, y, c, c, true);
      } else if (ch == '.') {
        const int r = c / 6 < 2 ? 2 : c / 6;
        drawRing(frame, x + c / 2, y + c / 2, r, r, true);
      } else if (ch == '$') {
        const int inset = c / 8 < 1 ? 1 : c / 8;
        drawRectOutline(frame, x + inset, y + inset, c - 2 * inset, c - 2 * inset, stroke);
        const int inner = inset + stroke + (c >= 24 ? 3 : 1);
        if (c - 2 * inner > 2) drawRectOutline(frame, x + inner, y + inner, c - 2 * inner, c - 2 * inner, ui::HAIRLINE);
      } else if (ch == '*') {
        const int inset = c / 8 < 1 ? 1 : c / 8;
        fillRect(frame, x + inset, y + inset, c - 2 * inset, c - 2 * inset, true);
      }
      if (i == b.player) {
        // Two fifths of the square, so it still stands out on the smallest boards.
        const int r = c * 2 / 5 < 4 ? 4 : c * 2 / 5;
        drawRing(frame, x + c / 2, y + c / 2, r, r, true);
      }
    }
  }

  void drawConfirm(uint8_t* frame) {
    drawRectOutline(frame, MARGIN, DIALOG_TOP, DIALOG_WIDTH, DIALOG_HEIGHT, ui::RULE);
    drawTextInBox(frame, Font::Size20, MARGIN, DIALOG_TOP + 20, DIALOG_WIDTH, 36, "RESTART THIS LEVEL?");
    drawTextInBox(frame, Font::Size14, MARGIN, DIALOG_TOP + 60, DIALOG_WIDTH, 30, "YOUR MOVES WILL BE LOST.");
    ui::drawPill(frame, YES_LEFT, DIALOG_PILL_TOP, DIALOG_PILL_WIDTH, "YES", true);
    ui::drawPill(frame, NO_LEFT, DIALOG_PILL_TOP, DIALOG_PILL_WIDTH, "NO", false);
  }

  // Each pack with how many of its levels you've solved, the one you're in framed.
  void drawPacks(uint8_t* frame) {
    ui::drawBand(frame, "PACKS", nullptr, true);
    const int width = SCREEN_WIDTH - 2 * ui::MARGIN;
    for (int p = 0; p < sk::kPackCount; p++) {
      const int top = ui::CONTENT_TOP + p * PACK_ROW_STEP;
      ui::drawRow(frame, top, sk::kPacks[p].label, nullptr);
      if (p == state_.pack) drawRectOutline(frame, ui::MARGIN, top, width, ui::ROW_HEIGHT, ui::FRAME);
      int solved = 0;
      const int first = sk::firstLevel(p);
      for (int k = 0; k < sk::kPacks[p].count; k++) solved += state_.best[first + k] ? 1 : 0;
      char text[24];
      std::snprintf(text, sizeof(text), "SOLVED %d/%d", solved, sk::kPacks[p].count);
      drawTextRight(frame, Font::Size14, ui::MARGIN + width - 14, top, ui::ROW_HEIGHT, text);
    }
    drawWrapped(frame, Font::Size14, MARGIN, CREDITS_TOP, INNER_WIDTH, CREDITS_TEXT);
  }

  void load() {
    if (!storage::ready()) return;
    size_t size = 0;
    const storage::Blob best = storage::loadBlob(BEST_BLOB, size);
    if (best && !sk::unpackBest(best.get(), size, state_)) Serial.println("Ignoring saved Sokoban record");
    const storage::Blob game = storage::loadBlob(GAME_BLOB, size);
    if (game && !sk::unpackGame(game.get(), size, state_)) Serial.println("Ignoring saved Sokoban game");
  }

  // The save is your place in every pack and the moves on the board, so it outlasts a
  // solved level.
  void persistGame() {
    if (!storage::ready()) {
      saveFailed_ = false;
      return;
    }
    const size_t size = sk::packGame(state_, buffer_);
    saveFailed_ = !storage::saveBlob(GAME_BLOB, buffer_, size);
  }

  void persistBest() {
    if (!storage::ready()) return;
    const size_t size = sk::packBest(state_, buffer_);
    if (storage::saveBlob(BEST_BLOB, buffer_, size)) {
      state_.bestChanged = false;
    } else {
      saveFailed_ = true;
    }
  }

  static_assert(sk::BEST_BYTES <= sk::GAME_MAX, "one buffer holds either save");

  sk::State state_{};
  uint8_t buffer_[sk::GAME_MAX] = {};
  char messageText_[80] = {};
  bool loaded_ = false;
  bool saveFailed_ = false;
};

}  // namespace

Game& sokobanGame() {
  static Sokoban game;
  return game;
}
