#include "chesspuzzles.h"

#include <Arduino.h>

#include <cstdio>

#include "chesspuzzles_banks.h"
#include "chesspuzzles_flow.h"
#include "chesspuzzles_icon.h"
#include "chesspuzzles_pieces.h"
#include "chesspuzzles_rules.h"
#include "chesspuzzles_save.h"
#include "core/graphics.h"
#include "core/storage.h"
#include "core/text.h"
#include "ui/ui.h"

namespace cp = chesspuzzles;

namespace {

constexpr const char* SAVE_FILE = "chesspuzzles";

// The board screen: the status capsule and the record line under the band, the board with its
// rank and file letters, the message, and four buttons along the bottom.
constexpr int MARGIN = 16;
constexpr int GAP = 12;
constexpr int INNER_WIDTH = SCREEN_WIDTH - 2 * MARGIN;
constexpr int STATUS_TOP = ui::CONTENT_TOP;
constexpr int STATUS_HEIGHT = 44;
constexpr int RECORD_TOP = STATUS_TOP + STATUS_HEIGHT + 4;
constexpr int RECORD_HEIGHT = 26;
constexpr int SQ = 52;
constexpr int BOARD_SIDE = 8 * SQ + 2 * ui::BOARD_FRAME;
constexpr int BOARD_LEFT = (SCREEN_WIDTH - BOARD_SIDE) / 2;
constexpr int BOARD_TOP = RECORD_TOP + RECORD_HEIGHT + 6;
constexpr int CELLS_LEFT = BOARD_LEFT + ui::BOARD_FRAME;
constexpr int CELLS_TOP = BOARD_TOP + ui::BOARD_FRAME;
constexpr int FILES_TOP = BOARD_TOP + BOARD_SIDE + 2;
constexpr int FILES_HEIGHT = 16;
constexpr int BUTTON_TOP = SCREEN_HEIGHT - MARGIN - ui::PILL_HEIGHT;
constexpr int MESSAGE_TOP = FILES_TOP + FILES_HEIGHT + 4;
constexpr int MESSAGE_HEIGHT = BUTTON_TOP - 8 - MESSAGE_TOP;
constexpr int PIECE_INSET = (SQ - 2 * cp::kPieceSize) / 2;
constexpr int MARK = 14;  // the last move's corner marks

// The bank list: a row for each bank, and a line under them.
constexpr int ROW_STEP = ui::ROW_HEIGHT + ui::ROW_GAP;
constexpr int ROW_WIDTH = SCREEN_WIDTH - 2 * ui::MARGIN;
constexpr int LIST_NOTE_TOP = ui::CONTENT_TOP + cp::kBanks * ROW_STEP + 8;

enum Button { HINT, SHOW, NEXT, BANK };
constexpr int BUTTONS = 4;
enum class Screen : uint8_t { Board, Banks };

// The bank button is as wide as its longest name needs at Size14, so it keeps one cut on every
// bank, and the other three share the rest. draw() and onTap() both ask.
struct Box {
  int left;
  int width;
};

Box buttonBox(int b) {
  static int bankWidth = 0;
  if (!bankWidth) {
    for (const cp::BankDef& def : cp::kBankList) {
      const int w = textWidth(Font::Size14, def.name) + 2 * ui::PILL_RADIUS + 8;
      if (w > bankWidth) bankWidth = w;
    }
  }
  const int other = (INNER_WIDTH - bankWidth - 3 * GAP) / 3;
  if (b == BANK) return {MARGIN + INNER_WIDTH - bankWidth, bankWidth};
  return {MARGIN + b * (other + GAP), other};
}

void drawTextRight(uint8_t* frame, Font font, int right, int top, int height, const char* text) {
  ui::drawTextLeft(frame, font, right - textWidth(font, text), top, height, text);
}

class ChessPuzzles final : public Game {
 public:
  const char* name() const override { return "CHESS PUZZLES"; }
  const uint8_t* icon() const override { return chesspuzzlesIcon::ICON; }

  void open() override {
    if (!loaded_) {
      loaded_ = true;
      if (!load()) cp::start(state_, state_.bank, state_.index[state_.bank]);
    }
    screen_ = Screen::Board;
    state_.sel = cp::kNoSquare;
  }

  TapResult onTap(int x, int y) override {
    if (screen_ == Screen::Banks) return tapBanks(x, y);
    if (ui::isOnBack(x, y)) return TapResult::Leave;
    for (int b = 0; b < BUTTONS; b++) {
      const Box box = buttonBox(b);
      if (ui::isInBox(x, y, box.left, BUTTON_TOP, box.width, ui::PILL_HEIGHT)) return tapButton(static_cast<Button>(b));
    }
    const int sq = squareAt(x, y);
    if (sq != cp::kNoSquare) return apply(cp::tapSquare(state_, sq));
    return apply(cp::tapOff(state_));
  }

  // Down is the next puzzle and Up the one before, as the source's side buttons.
  TapResult onDownPressed() override { return screen_ == Screen::Board ? apply(cp::next(state_)) : TapResult::Nothing; }
  TapResult onUpPressed() override { return screen_ == Screen::Board ? apply(cp::previous(state_)) : TapResult::Nothing; }

  void draw(uint8_t* frame) override {
    if (screen_ == Screen::Banks) {
      drawBanks(frame);
      return;
    }
    ui::drawBand(frame, name(), nullptr, true);
    drawStatus(frame);
    drawRecord(frame);
    if (state_.ok) drawBoard(frame);
    drawMessage(frame);
    drawButtons(frame);
  }

  bool saveFailed() const override { return saveFailed_; }
  void retrySave() override { persist(); }

 private:
  TapResult tapButton(Button b) {
    switch (b) {
      case HINT:
        return apply(cp::showHint(state_));
      case SHOW:
        return apply(cp::showMove(state_));
      case NEXT:
        return apply(cp::next(state_));
      case BANK:
        screen_ = Screen::Banks;
        return TapResult::Redraw;
    }
    return TapResult::Nothing;
  }

  TapResult tapBanks(int x, int y) {
    if (ui::isOnBack(x, y)) {
      screen_ = Screen::Board;
      return TapResult::Redraw;
    }
    for (int k = 0; k < cp::kBanks; k++) {
      if (ui::isInBox(x, y, ui::MARGIN, ui::CONTENT_TOP + k * ROW_STEP, ROW_WIDTH, ui::ROW_HEIGHT)) {
        screen_ = Screen::Board;
        return apply(cp::pickBank(state_, k));
      }
    }
    return TapResult::Nothing;
  }

  // The selection and hints live only in memory; moves and puzzles are saved at once.
  TapResult apply(cp::Change change) {
    switch (change) {
      case cp::Change::None:
        return TapResult::Nothing;
      case cp::Change::View:
        return TapResult::Redraw;
      case cp::Change::Move:
        persist();
        return TapResult::Redraw;
      case cp::Change::Full:
        persist();
        return TapResult::RedrawFull;
    }
    return TapResult::Nothing;
  }

  // The board turns so your side is at the bottom: square (col, row) on screen, row 0 at the top.
  int squareOn(int col, int row) const {
    return state_.me == 'w' ? (7 - row) * 8 + col : row * 8 + (7 - col);
  }
  int screenCol(int sq) const { return state_.me == 'w' ? cp::fileOf(sq) : 7 - cp::fileOf(sq); }
  int screenRow(int sq) const { return state_.me == 'w' ? 7 - cp::rankOf(sq) : cp::rankOf(sq); }
  int squareLeft(int sq) const { return CELLS_LEFT + screenCol(sq) * SQ; }
  int squareTop(int sq) const { return CELLS_TOP + screenRow(sq) * SQ; }

  // The square under a screen position, or kNoSquare. drawBoard() places squares the same way.
  int squareAt(int x, int y) const {
    if (!state_.ok || x < CELLS_LEFT || y < CELLS_TOP) return cp::kNoSquare;
    const int c = (x - CELLS_LEFT) / SQ;
    const int r = (y - CELLS_TOP) / SQ;
    if (c >= 8 || r >= 8) return cp::kNoSquare;
    return squareOn(c, r);
  }

  void drawStatus(uint8_t* frame) const {
    // Outlined while playing, and solid once solved: a fill that appears once, with the full
    // refresh, is the payoff.
    const bool solid = state_.done;
    const char* text = !state_.ok ? "NO PUZZLE" : state_.done ? "SOLVED!" : state_.me == 'w' ? "WHITE TO MOVE" : "BLACK TO MOVE";
    if (solid) {
      fillRect(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, true);
    } else {
      drawRectOutline(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, 2);
    }
    drawTextInBox(frame, Font::Size20, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, text, !solid);
  }

  void drawRecord(uint8_t* frame) const {
    char left[48];
    char right[24];
    const cp::BankDef& def = cp::bankDef(state_);
    std::snprintf(left, sizeof(left), "%s  %u/%u  RATING %d", def.name, state_.index[state_.bank] + 1u,
                  static_cast<unsigned>(def.count), state_.ok ? state_.puzzle.rating : 0);
    std::snprintf(right, sizeof(right), "SOLVED %u", static_cast<unsigned>(state_.solved[state_.bank]));
    // A step down a cut when both don't fit, as with a long bank name and a big count.
    const Font font = textWidth(Font::Size14, left) + textWidth(Font::Size14, right) + 16 <= INNER_WIDTH ? Font::Size14 : Font::Size10;
    ui::drawTextLeft(frame, font, MARGIN, RECORD_TOP, RECORD_HEIGHT, left);
    drawTextRight(frame, font, MARGIN + INNER_WIDTH, RECORD_TOP, RECORD_HEIGHT, right);
  }

  void drawPiece(uint8_t* frame, char p, int left, int top) const {
    const char* const* art = cp::pieceArt(p);
    if (!art) return;
    const bool white = cp::isWhite(p);
    for (int r = 0; r < cp::kPieceSize; r++) {
      for (int c = 0; c < cp::kPieceSize; c++) {
        if (cp::pieceInk(art, r, c, white)) fillRect(frame, left + 2 * c, top + 2 * r, 2, 2, true);
      }
    }
  }

  void drawCorners(uint8_t* frame, int x, int y) const {
    const int w = ui::FRAME;
    const int e = SQ - w;
    fillRect(frame, x, y, MARK, w, true);
    fillRect(frame, x, y, w, MARK, true);
    fillRect(frame, x + SQ - MARK, y, MARK, w, true);
    fillRect(frame, x + e, y, w, MARK, true);
    fillRect(frame, x, y + e, MARK, w, true);
    fillRect(frame, x, y + SQ - MARK, w, MARK, true);
    fillRect(frame, x + SQ - MARK, y + e, MARK, w, true);
    fillRect(frame, x + e, y + SQ - MARK, w, MARK, true);
  }

  // As the source draws it: dark squares with an inner frame, the last move's squares with
  // corner marks, the piece picked up with a heavy frame, and a hint with a double one.
  void drawBoard(uint8_t* frame) const {
    drawRectOutline(frame, BOARD_LEFT, BOARD_TOP, BOARD_SIDE, BOARD_SIDE, ui::BOARD_FRAME);
    for (int sq = 0; sq < cp::kSquares; sq++) {
      const int x = squareLeft(sq), y = squareTop(sq);
      if ((cp::fileOf(sq) + cp::rankOf(sq)) % 2 == 0) drawRectOutline(frame, x + 3, y + 3, SQ - 6, SQ - 6, ui::HAIRLINE);
      drawPiece(frame, state_.board.sq[sq], x + PIECE_INSET, y + PIECE_INSET);
    }
    for (int sq : {state_.board.lastFrom, state_.board.lastTo}) {
      if (sq != cp::kNoSquare) drawCorners(frame, squareLeft(sq), squareTop(sq));
    }
    if (state_.sel != cp::kNoSquare) {
      drawRectOutline(frame, squareLeft(state_.sel) + 1, squareTop(state_.sel) + 1, SQ - 2, SQ - 2, ui::FRAME);
    }
    if (state_.hint > 0) {
      const cp::Move& want = state_.puzzle.moves[state_.ply];
      for (int k = 0; k < state_.hint; k++) {
        const int sq = k == 0 ? want.from : want.to;
        const int x = squareLeft(sq), y = squareTop(sq);
        drawRectOutline(frame, x + 1, y + 1, SQ - 2, SQ - 2, 2);
        drawRectOutline(frame, x + 6, y + 6, SQ - 12, SQ - 12, 2);
      }
    }
    // Files under the board and ranks to its left, turned with it.
    for (int k = 0; k < 8; k++) {
      const char file[2] = {static_cast<char>('A' + (state_.me == 'w' ? k : 7 - k)), '\0'};
      const char rank[2] = {static_cast<char>('1' + (state_.me == 'w' ? 7 - k : k)), '\0'};
      drawTextInBox(frame, Font::Size10, CELLS_LEFT + k * SQ, FILES_TOP, SQ, FILES_HEIGHT, file);
      drawTextInBox(frame, Font::Size10, 0, CELLS_TOP + k * SQ, BOARD_LEFT, SQ, rank);
    }
  }

  void drawMessage(uint8_t* frame) const {
    char text[160];
    messageText(text, sizeof(text));
    // The biggest cut the message fits in, so a long list of themes still shows in full.
    Font font = Font::Size14;
    if (wrappedHeight(font, INNER_WIDTH, text) > MESSAGE_HEIGHT) font = Font::Size10;
    const int height = wrappedHeight(font, INNER_WIDTH, text);
    drawWrapped(frame, font, MARGIN, MESSAGE_TOP + (MESSAGE_HEIGHT - height) / 2, INNER_WIDTH, text, Align::Center);
  }

  void messageText(char* out, size_t size) const {
    switch (state_.message) {
      case cp::Message::Good:
        std::snprintf(out, size, "GOOD. YOUR MOVE AGAIN.");
        return;
      case cp::Message::NotTheMove:
        std::snprintf(out, size, "NOT THE MOVE. TRY AGAIN.");
        return;
      case cp::Message::HintPiece:
        std::snprintf(out, size, "HINT: MOVE THE FRAMED PIECE.");
        return;
      case cp::Message::HintSquare:
        std::snprintf(out, size, "HINT: FROM THE FRAMED SQUARE TO THE OTHER.");
        return;
      case cp::Message::Unreadable:
        std::snprintf(out, size, "COULD NOT READ PUZZLE %u.", state_.index[state_.bank] + 1u);
        return;
      case cp::Message::Solved: {
        char how[24];
        if (state_.shown) {
          std::snprintf(how, sizeof(how), "WITH HELP");
        } else if (state_.mistakes == 0) {
          std::snprintf(how, sizeof(how), "FIRST TRY");
        } else {
          std::snprintf(how, sizeof(how), "%u WRONG MOVE%s", state_.mistakes, state_.mistakes > 1 ? "S" : "");
        }
        char themes[120];
        cp::themeWords(state_.puzzle.themes, state_.puzzle.themesLen, themes, sizeof(themes));
        std::snprintf(out, size, "SOLVED (%s). %s", how, themes);
        return;
      }
      case cp::Message::Prompt:
        break;
    }
    std::snprintf(out, size, "FIND THE BEST MOVE: TAP A PIECE, THEN ITS SQUARE.");
  }

  void drawButtons(uint8_t* frame) const {
    for (int b = 0; b < BUTTONS; b++) {
      const char* label = b == HINT ? "HINT" : b == SHOW ? "SHOW" : b == NEXT ? "NEXT" : cp::bankDef(state_).name;
      const Box box = buttonBox(b);
      const bool can = (b != HINT && b != SHOW) || (cp::canHelp(state_) && (b != HINT || state_.hint < 2));
      if (!can) {
        ui::drawDimmedPill(frame, box.left, BUTTON_TOP, box.width, label);
      } else {
        // The bank button names the bank you're on, so it's outlined like a setting.
        ui::drawPill(frame, box.left, BUTTON_TOP, box.width, label, b != BANK);
      }
    }
  }

  void drawBanks(uint8_t* frame) const {
    ui::drawBand(frame, "PUZZLES", nullptr, true);
    char text[32];
    for (int k = 0; k < cp::kBanks; k++) {
      const int top = ui::CONTENT_TOP + k * ROW_STEP;
      ui::drawRow(frame, top, cp::kBankList[k].name, nullptr);
      if (k == state_.bank) drawRectOutline(frame, ui::MARGIN, top, ROW_WIDTH, ui::ROW_HEIGHT, ui::FRAME);
      std::snprintf(text, sizeof(text), "%u/%u  SOLVED %u", state_.index[k] + 1u, static_cast<unsigned>(cp::kBankList[k].count),
                    static_cast<unsigned>(state_.solved[k]));
      drawTextRight(frame, Font::Size14, ui::MARGIN + ROW_WIDTH - 14, top, ui::ROW_HEIGHT, text);
    }
    drawWrapped(frame, Font::Size14, MARGIN, LIST_NOTE_TOP, INNER_WIDTH,
                "FROM THE LICHESS PUZZLE DATABASE, EASIEST FIRST. A BANK OPENS AT THE PUZZLE IT WAS ON.", Align::Left);
  }

  bool load() {
    if (!storage::ready()) return false;
    uint8_t data[cp::SAVE_SIZE];
    uint8_t version = 0;
    size_t size = 0;
    if (!storage::load(SAVE_FILE, version, data, sizeof(data), size)) return false;
    if (!cp::unpack(version, data, size, state_)) {
      Serial.println("Ignoring saved Chess Puzzles game");
      return false;
    }
    return true;
  }

  void persist() {
    if (!storage::ready()) {
      saveFailed_ = false;
      return;
    }
    uint8_t data[cp::SAVE_SIZE];
    const size_t size = cp::pack(state_, data);
    saveFailed_ = !storage::save(SAVE_FILE, cp::SAVE_VERSION, data, size);
  }

  cp::State state_{};
  Screen screen_ = Screen::Board;
  bool loaded_ = false;
  bool saveFailed_ = false;
};

}  // namespace

Game& chessPuzzlesGame() {
  static ChessPuzzles game;
  return game;
}
