#include "lastpiece.h"

#include <Arduino.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "core/graphics.h"
#include "core/storage.h"
#include "core/text.h"
#include "lastpiece_flow.h"
#include "lastpiece_icon.h"
#include "lastpiece_pack.h"
#include "lastpiece_pieces.h"
#include "lastpiece_rules.h"
#include "lastpiece_save.h"
#include "ui/ui.h"

namespace lp = lastpiece;
namespace pieces = lastpiecePieces;

namespace {

constexpr const char* SAVE_FILE = "lastpiece";
constexpr const char* CARD_FOLDER = "/lastpiece";
// Each difficulty's files on the card: <name>.txt, and <name>-solutions.txt, which is checked
// against its boards.
constexpr const char* FILE_NAMES[lp::kDifficulties] = {"beginner", "intermediate", "advanced", "expert"};

// The status capsule and the record line under the band, the board, a message line, the
// captured tray, and three buttons along the bottom, as Grid Gambit's. The Up and Down keys step
// through the challenges, so the buttons get the width.
constexpr int MARGIN = 16;
constexpr int GAP = 12;
constexpr int INNER_WIDTH = SCREEN_WIDTH - 2 * MARGIN;
constexpr int STATUS_TOP = ui::CONTENT_TOP;
constexpr int STATUS_HEIGHT = 44;
constexpr int RECORD_TOP = STATUS_TOP + STATUS_HEIGHT + 4;
constexpr int RECORD_HEIGHT = 30;
constexpr int BUTTON_TOP = SCREEN_HEIGHT - MARGIN - ui::PILL_HEIGHT;
constexpr int SLOT = 46;
constexpr int SLOT_GAP = 6;
constexpr int TRAY_TOP = BUTTON_TOP - GAP - SLOT;
constexpr int MESSAGE_HEIGHT = 30;
constexpr int MESSAGE_TOP = TRAY_TOP - 8 - MESSAGE_HEIGHT;
constexpr int CELL = 106;
constexpr int BOARD_SIDE = lp::kSide * CELL + 2 * ui::BOARD_FRAME;
constexpr int BOARD_LEFT = (SCREEN_WIDTH - BOARD_SIDE) / 2;
constexpr int BOARD_TOP = MESSAGE_TOP - 8 - BOARD_SIDE;
constexpr int CELLS_LEFT = BOARD_LEFT + ui::BOARD_FRAME;
constexpr int CELLS_TOP = BOARD_TOP + ui::BOARD_FRAME;
constexpr int BUTTONS = 3;
constexpr int BUTTON_WIDTH = (INNER_WIDTH - (BUTTONS - 1) * GAP) / BUTTONS;
constexpr int BRACKET = 22;

static_assert(RECORD_TOP + RECORD_HEIGHT <= BOARD_TOP, "the board overlaps the record line");
// Nine captures at most: the tray's slots shrink to fit, but never below the small pieces.
static_assert(lp::kMaxMoves * pieces::SMALL + (lp::kMaxMoves - 1) * SLOT_GAP <= INNER_WIDTH, "the tray can't fit");

// The LEVELS list: a row for each difficulty, then a note on what was read from the card.
constexpr int ROW_STEP = ui::ROW_HEIGHT + ui::ROW_GAP;
constexpr int NOTE_TOP = ui::CONTENT_TOP + lp::kDifficulties * ROW_STEP + 16;

// The cache for the solver, which checks each challenge on the card can be won: 4096
// slots, 32 KB, taken once the first time the card is read, and kept.
constexpr uint32_t MEMO_SLOTS = 4096;
constexpr uint32_t MEMO_SLOTS_FALLBACK = 256;

enum Button { UNDO, RESET, LEVELS };
enum class Mode { Play, Levels };

int buttonLeft(int b) { return MARGIN + b * (BUTTON_WIDTH + GAP); }
int cellLeft(int i) { return CELLS_LEFT + lp::colOf(i) * CELL; }
int cellTop(int i) { return CELLS_TOP + lp::rowOf(i) * CELL; }
int rowTop(int d) { return ui::CONTENT_TOP + d * ROW_STEP; }

// The cell under a screen position, or kNoCell. draw() places cells with the same sums.
int cellAt(int x, int y) {
  if (x < CELLS_LEFT || y < CELLS_TOP) return lp::kNoCell;
  const int c = (x - CELLS_LEFT) / CELL;
  const int r = (y - CELLS_TOP) / CELL;
  if (c >= lp::kSide || r >= lp::kSide) return lp::kNoCell;
  return r * lp::kSide + c;
}

// Corner brackets inside a cell, for a piece the selected one can take.
void drawBrackets(uint8_t* frame, int left, int top, int side) {
  const int w = ui::FRAME;
  const int in = 6;
  const int l = left + in, t = top + in, r = left + side - in, b = top + side - in;
  fillRect(frame, l, t, BRACKET, w, true);
  fillRect(frame, l, t, w, BRACKET, true);
  fillRect(frame, r - BRACKET, t, BRACKET, w, true);
  fillRect(frame, r - w, t, w, BRACKET, true);
  fillRect(frame, l, b - w, BRACKET, w, true);
  fillRect(frame, l, b - BRACKET, w, BRACKET, true);
  fillRect(frame, r - BRACKET, b - w, BRACKET, w, true);
  fillRect(frame, r - w, b - BRACKET, w, BRACKET, true);
}

void drawTextRight(uint8_t* frame, Font font, int right, int top, int height, const char* text) {
  ui::drawTextLeft(frame, font, right - textWidth(font, text), top, height, text);
}

// The dither of a dimmed control, one dot in four, inside a row that can't be chosen.
void dimRow(uint8_t* frame, int top) {
  const int left = ui::MARGIN;
  const int width = SCREEN_WIDTH - 2 * ui::MARGIN;
  for (int y = top + 2; y < top + ui::ROW_HEIGHT - 2; y++) {
    for (int x = left + 2; x < left + width - 2; x++) {
      if (x % 2 == 0 && y % 2 == 0) setPixel(frame, x, y, true);
    }
  }
}

bool sameName(const std::string& file, const std::string& wanted) {
  if (file.size() != wanted.size()) return false;
  for (size_t i = 0; i < file.size(); i++) {
    const char c = file[i] >= 'A' && file[i] <= 'Z' ? static_cast<char>(file[i] - 'A' + 'a') : file[i];
    if (c != wanted[i]) return false;
  }
  return true;
}

class LastPiece final : public Game {
 public:
  const char* name() const override { return "LAST PIECE"; }
  const uint8_t* icon() const override { return lastpieceIcon::ICON; }

  // The card is read every time the game opens, so files changed on a computer are picked up
  // without a restart. A solved challenge stays on the board until Up or Down, as in Sokoban.
  void open() override {
    if (!loaded_) {
      loaded_ = true;
      load();
    }
    readCard();
    if (pack().count == 0) {
      for (int d = 0; d < lp::kDifficulties; d++) {
        if (packs_[d].count > 0) {
          state_.difficulty = static_cast<uint8_t>(d);
          break;
        }
      }
    }
    if (lp::sync(state_, pack())) persist();
    mode_ = Mode::Play;
  }

  TapResult onTap(int x, int y) override {
    if (mode_ == Mode::Levels) return tapLevels(x, y);
    if (ui::isOnBack(x, y)) return TapResult::Leave;
    for (int b = 0; b < BUTTONS; b++) {
      if (ui::isInBox(x, y, buttonLeft(b), BUTTON_TOP, BUTTON_WIDTH, ui::PILL_HEIGHT)) {
        return canAct(static_cast<Button>(b)) ? tapButton(static_cast<Button>(b)) : TapResult::Nothing;
      }
    }
    if (pack().count == 0) return TapResult::Nothing;
    const int cell = cellAt(x, y);
    if (cell != lp::kNoCell) return apply(lp::tapCell(state_, cell));
    return apply(lp::tapOff(state_));
  }

  // Up and Down step through the challenges at this difficulty, wrapping at the ends.
  TapResult onUpPressed() override { return mode_ == Mode::Play ? apply(lp::step(state_, pack(), -1)) : TapResult::Nothing; }
  TapResult onDownPressed() override { return mode_ == Mode::Play ? apply(lp::step(state_, pack(), 1)) : TapResult::Nothing; }

  void draw(uint8_t* frame) override {
    if (mode_ == Mode::Levels) {
      drawLevels(frame);
      return;
    }
    ui::drawBand(frame, name(), nullptr, true);
    drawStatus(frame);
    drawRecord(frame);
    drawBoard(frame);
    drawTextInBox(frame, Font::Size14, MARGIN, MESSAGE_TOP, INNER_WIDTH, MESSAGE_HEIGHT, messageText());
    drawTray(frame);
    drawButtons(frame);
  }

  bool saveFailed() const override { return saveFailed_; }
  void retrySave() override { persist(); }

 private:
  const lp::Pack& pack() const { return packs_[state_.difficulty]; }

  bool canAct(Button b) const {
    switch (b) {
      case UNDO:
        return lp::canUndo(state_);
      case RESET:
        return lp::canUndo(state_);
      case LEVELS:
        return true;
    }
    return false;
  }

  TapResult tapButton(Button b) {
    switch (b) {
      case UNDO:
        return apply(lp::undoMove(state_));
      case RESET:
        return apply(lp::resetChallenge(state_));
      case LEVELS:
        // Read again, so a card changed while the game was open still shows what's on it.
        readCard();
        if (lp::sync(state_, pack())) persist();
        mode_ = Mode::Levels;
        return TapResult::Redraw;
    }
    return TapResult::Nothing;
  }

  TapResult tapLevels(int x, int y) {
    if (ui::isOnBack(x, y)) {
      mode_ = Mode::Play;
      return TapResult::Redraw;
    }
    for (int d = 0; d < lp::kDifficulties; d++) {
      if (!ui::isInBox(x, y, ui::MARGIN, rowTop(d), SCREEN_WIDTH - 2 * ui::MARGIN, ui::ROW_HEIGHT)) continue;
      if (lp::chooseDifficulty(state_, packs_[d], d) == lp::Change::None) return TapResult::Nothing;
      mode_ = Mode::Play;
      persist();
      return TapResult::RedrawFull;
    }
    return TapResult::Nothing;
  }

  // The selection, hint and messages live only in memory; every capture is saved at once.
  TapResult apply(lp::Change change) {
    switch (change) {
      case lp::Change::None:
        return TapResult::Nothing;
      case lp::Change::View:
        return TapResult::Redraw;
      case lp::Change::Move:
        persist();
        return TapResult::Redraw;
      case lp::Change::Full:
        persist();
        return TapResult::RedrawFull;
    }
    return TapResult::Nothing;
  }

  // Each difficulty's boards, and its solutions if there are any. A file that's missing leaves
  // that difficulty empty; one that's there but too big to read is noted on the LEVELS list.
  void readCard() {
    cardReady_ = storage::ready();
    for (int d = 0; d < lp::kDifficulties; d++) {
      packs_[d].count = 0;
      packs_[d].skipped = 0;
      packs_[d].overLimit = 0;
      packs_[d].badSolutions = 0;
      tooBig_[d] = false;
      present_[d] = false;
    }
    if (!cardReady_) return;
    if (!memo_.slots) takeMemo();
    storage::ensureFolder(CARD_FOLDER);
    const std::vector<std::string> files = storage::listFolder(CARD_FOLDER, 200);
    for (int d = 0; d < lp::kDifficulties; d++) {
      const std::string boards = std::string(FILE_NAMES[d]) + ".txt";
      size_t size = 0;
      const storage::Blob data = storage::readFile((std::string(CARD_FOLDER) + "/" + boards).c_str(), lp::kMaxFileBytes, size);
      if (!data) {
        for (const std::string& file : files) tooBig_[d] = tooBig_[d] || sameName(file, boards);
        continue;
      }
      present_[d] = true;
      lp::parsePack(data.get(), size, packs_[d], memo_);
      const std::string solutions = std::string(CARD_FOLDER) + "/" + FILE_NAMES[d] + "-solutions.txt";
      const storage::Blob answers = storage::readFile(solutions.c_str(), lp::kMaxFileBytes, size);
      if (answers) lp::parseSolutions(answers.get(), size, packs_[d]);
    }
  }

  void takeMemo() {
    for (uint32_t slots : {MEMO_SLOTS, MEMO_SLOTS_FALLBACK}) {
      auto* p = static_cast<uint64_t*>(std::calloc(slots, sizeof(uint64_t)));
      if (p) {
        memo_.slots = p;
        memo_.mask = slots - 1;
        return;
      }
    }
    // Without a cache the solver still answers, only slower.
    Serial.println("Last Piece: no memory for the solver's cache");
  }

  void drawStatus(uint8_t* frame) const {
    // Outlined while playing, and solid at the end: a fill that appears once, with the full
    // refresh, is the payoff.
    char text[40];
    const bool any = pack().count > 0;
    const bool won = any && lp::isWon(state_);
    const bool lost = any && lp::isLost(state_);
    if (!any) {
      std::snprintf(text, sizeof(text), "NO CHALLENGES");
    } else if (won) {
      std::snprintf(text, sizeof(text), "SOLVED!");
    } else if (lost) {
      std::snprintf(text, sizeof(text), "NO CAPTURES LEFT");
    } else {
      std::snprintf(text, sizeof(text), "%s  %d/%d", lp::kDifficultyNames[state_.difficulty],
                    state_.challenge[state_.difficulty] + 1, pack().count);
    }
    const bool solid = won || lost;
    if (solid) {
      fillRect(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, true);
    } else {
      drawRectOutline(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, 2);
    }
    drawTextInBox(frame, Font::Size20, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, text, !solid);
  }

  void drawRecord(uint8_t* frame) const {
    const int count = pack().count;
    if (count == 0) return;
    char text[40];
    const int d = state_.difficulty;
    std::snprintf(text, sizeof(text), "SOLVED %d OF %d", lp::solvedCount(state_, d, count), count);
    ui::drawTextLeft(frame, Font::Size14, MARGIN, RECORD_TOP, RECORD_HEIGHT, text);
    if (lp::isSolved(state_, d, state_.challenge[d]) && !lp::isOver(state_)) {
      std::snprintf(text, sizeof(text), "SOLVED BEFORE");
    } else {
      std::snprintf(text, sizeof(text), "%d PIECES", lp::startPieces(state_));
    }
    drawTextRight(frame, Font::Size14, MARGIN + INNER_WIDTH, RECORD_TOP, RECORD_HEIGHT, text);
  }

  // Pieces are outlines and cells hairlines, so a move repaints little black. The selected
  // piece is framed, and the pieces it can take are bracketed.
  void drawBoard(uint8_t* frame) const {
    drawRectOutline(frame, BOARD_LEFT, BOARD_TOP, BOARD_SIDE, BOARD_SIDE, ui::BOARD_FRAME);
    if (pack().count == 0) {
      drawEmpty(frame);
      return;
    }
    for (int k = 1; k < lp::kSide; k++) {
      fillRect(frame, CELLS_LEFT + k * CELL, CELLS_TOP, ui::HAIRLINE, lp::kSide * CELL, true);
      fillRect(frame, CELLS_LEFT, CELLS_TOP + k * CELL, lp::kSide * CELL, ui::HAIRLINE, true);
    }
    bool target[lp::kCells] = {};
    if (state_.hintTo != lp::kNoCell) {
      target[state_.hintTo] = true;
    } else if (state_.sel != lp::kNoCell) {
      int t[8];
      const int n = lp::capturesFrom(state_.board, state_.sel, t);
      for (int k = 0; k < n; k++) target[t[k]] = true;
    }
    const int inset = (CELL - pieces::BIG) / 2;
    for (int i = 0; i < lp::kCells; i++) {
      const uint8_t p = state_.board.cell[i];
      if (p == lp::None) continue;
      drawIcon(frame, pieces::BIG_PIECES[p], pieces::BIG, pieces::BIG, cellLeft(i) + inset, cellTop(i) + inset);
      if (i == state_.sel) drawRectOutline(frame, cellLeft(i) + 5, cellTop(i) + 5, CELL - 9, CELL - 9, ui::FRAME);
      if (target[i]) drawBrackets(frame, cellLeft(i), cellTop(i), CELL);
    }
  }

  // With nothing to play, the board says where the challenges go, in the room it has.
  void drawEmpty(uint8_t* frame) const {
    const char* text = cardReady_ ? "NO CHALLENGES ON THE CARD YET. COPY BEGINNER.TXT INTO THE /LASTPIECE FOLDER, "
                                    "AS THE GAME'S README SAYS."
                                  : "NO CARD. CHALLENGES ARE READ FROM THE /LASTPIECE FOLDER ON THE CARD.";
    const int inset = 32;
    const int width = lp::kSide * CELL - 2 * inset;
    const int top = CELLS_TOP + (lp::kSide * CELL - wrappedHeight(Font::Size20, width, text)) / 2;
    drawWrapped(frame, Font::Size20, CELLS_LEFT + inset, top, width, text, Align::Center);
  }

  // A slot for every capture the challenge needs, filled in the order they were made: the
  // player's own progress. Nine captures need narrower slots than Grid Gambit's eight.
  void drawTray(uint8_t* frame) const {
    const int slots = lp::startPieces(state_) - 1;
    if (slots <= 0) return;
    int slot = (INNER_WIDTH - (slots - 1) * SLOT_GAP) / slots;
    if (slot > SLOT) slot = SLOT;
    const int width = slots * slot + (slots - 1) * SLOT_GAP;
    int left = (SCREEN_WIDTH - width) / 2;
    const int inset = (slot - pieces::SMALL) / 2;
    const int top = TRAY_TOP + (SLOT - slot) / 2;
    for (int k = 0; k < slots; k++, left += slot + SLOT_GAP) {
      if (k < state_.moveCount) {
        drawIcon(frame, pieces::SMALL_PIECES[state_.taken[k]], pieces::SMALL, pieces::SMALL, left + inset, top + inset);
      } else {
        drawRectOutline(frame, left, top, slot, slot, ui::HAIRLINE);
      }
    }
  }

  void drawButtons(uint8_t* frame) const {
    static const char* const LABELS[BUTTONS] = {"UNDO", "RESET", "LEVELS"};
    for (int b = 0; b < BUTTONS; b++) {
      if (canAct(static_cast<Button>(b))) {
        ui::drawPill(frame, buttonLeft(b), BUTTON_TOP, BUTTON_WIDTH, LABELS[b], true);
      } else {
        ui::drawDimmedPill(frame, buttonLeft(b), BUTTON_TOP, BUTTON_WIDTH, LABELS[b]);
      }
    }
  }

  const char* messageText() const {
    if (pack().count == 0) return "LEVELS SHOWS WHAT WAS READ.";
    if (lp::isWon(state_)) return pack().count > 1 ? "ONE PIECE LEFT. DOWN FOR THE NEXT." : "ONE PIECE LEFT.";
    if (lp::isLost(state_)) return "NOTHING CAN TAKE: UNDO.";
    switch (state_.message) {
      case lp::Message::Selected:
        return "TAP A BRACKETED PIECE TO TAKE IT.";
      case lp::Message::CannotCapture:
        return "THAT PIECE CAN'T TAKE ANYTHING.";
      case lp::Message::Hint:
        return "HINT: TAKE THE BRACKETED PIECE.";
      case lp::Message::NoWin:
        return "NO WIN FROM HERE: UNDO.";
      case lp::Message::Help:
        break;
    }
    return "EVERY MOVE TAKES. LEAVE ONE PIECE.";
  }

  // Each difficulty with how many of its challenges you've solved, the one you're on framed, and
  // one that has nothing on the card dimmed. Under them, anything the card's files got wrong.
  void drawLevels(uint8_t* frame) const {
    ui::drawBand(frame, "LEVELS", nullptr, true);
    const int width = SCREEN_WIDTH - 2 * ui::MARGIN;
    std::string note;
    for (int d = 0; d < lp::kDifficulties; d++) {
      const lp::Pack& p = packs_[d];
      const int top = rowTop(d);
      if (p.count == 0) dimRow(frame, top);
      ui::drawRow(frame, top, lp::kDifficultyNames[d], nullptr);
      if (d == state_.difficulty && p.count > 0) drawRectOutline(frame, ui::MARGIN, top, width, ui::ROW_HEIGHT, ui::FRAME);
      char text[48];
      if (p.count > 0) {
        std::snprintf(text, sizeof(text), "SOLVED %d/%d", lp::solvedCount(state_, d, p.count), p.count);
      } else if (tooBig_[d]) {
        std::snprintf(text, sizeof(text), "FILE TOO BIG");
      } else if (present_[d]) {
        std::snprintf(text, sizeof(text), "NONE TO PLAY");
      } else {
        std::snprintf(text, sizeof(text), "NO FILE");
      }
      drawTextRight(frame, Font::Size14, ui::MARGIN + width - 14, top, ui::ROW_HEIGHT, text);
      note += problems(d);
    }
    if (note.empty()) {
      note = cardReady_ ? "CHALLENGES ARE READ FROM /LASTPIECE ON THE CARD. SEE THE GAME'S README FOR THE FORMAT."
                        : "NO CARD. CHALLENGES ARE READ FROM /LASTPIECE ON THE CARD.";
    }
    drawWrapped(frame, Font::Size14, MARGIN, NOTE_TOP, INNER_WIDTH, note.c_str());
  }

  // What was left out of one difficulty's files, such as "BEGINNER: 1 CHALLENGE SKIPPED. ".
  std::string problems(int d) const {
    const lp::Pack& p = packs_[d];
    std::string out;
    char text[64];
    auto add = [&](int n, const char* one, const char* many) {
      if (n == 0) return;
      std::snprintf(text, sizeof(text), "%d %s. ", n, n == 1 ? one : many);
      out += text;
    };
    add(p.skipped, "CHALLENGE SKIPPED", "CHALLENGES SKIPPED");
    add(p.overLimit, "CHALLENGE PAST 99 LEFT OUT", "CHALLENGES PAST 99 LEFT OUT");
    add(p.badSolutions, "SOLUTION DROPPED", "SOLUTIONS DROPPED");
    if (tooBig_[d]) out += "OVER 16 KB, SO NOT READ. ";
    if (!out.empty()) out = std::string(lp::kDifficultyNames[d]) + ": " + out;
    return out;
  }

  bool load() {
    if (!storage::ready()) return false;
    uint8_t data[lp::SAVE_MAX];
    uint8_t version = 0;
    size_t size = 0;
    if (!storage::load(SAVE_FILE, version, data, sizeof(data), size)) return false;
    if (!lp::unpack(version, data, size, state_)) {
      Serial.println("Ignoring saved Last Piece game");
      return false;
    }
    return true;
  }

  // The save holds the solved marks as well as the position, so it's never erased.
  void persist() {
    if (!storage::ready()) {
      saveFailed_ = false;
      return;
    }
    uint8_t data[lp::SAVE_MAX];
    const size_t size = lp::pack(state_, data);
    saveFailed_ = !storage::save(SAVE_FILE, lp::SAVE_VERSION, data, size);
  }

  lp::State state_{};
  lp::Pack packs_[lp::kDifficulties];
  bool tooBig_[lp::kDifficulties] = {};
  bool present_[lp::kDifficulties] = {};  // its boards file was read
  lp::Memo memo_;
  Mode mode_ = Mode::Play;
  bool cardReady_ = false;
  bool loaded_ = false;
  bool saveFailed_ = false;
};

}  // namespace

Game& lastPieceGame() {
  static LastPiece game;
  return game;
}
