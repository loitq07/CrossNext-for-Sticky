#include "wordladder.h"

#include <Arduino.h>

#include <cstdio>
#include <ctime>
#include <string>

#include "core/graphics.h"
#include "core/storage.h"
#include "core/text.h"
#include "ui/ui.h"
#include "wordladder_flow.h"
#include "wordladder_icon.h"
#include "wordladder_rules.h"
#include "wordladder_save.h"

namespace wl = wordladder;

namespace {

constexpr const char* SAVE_BLOB = "wordladder";

// From the top: the status capsule, the ladder of seven rows, the message line, the keys, and
// three pills along the bottom.
constexpr int MARGIN = 16;
constexpr int GAP = 12;
constexpr int INNER_WIDTH = SCREEN_WIDTH - 2 * MARGIN;
constexpr int STATUS_TOP = ui::CONTENT_TOP;
constexpr int STATUS_HEIGHT = 44;
constexpr int LADDER_TOP = STATUS_TOP + STATUS_HEIGHT + 12;
constexpr int ROW_STEP = 50;
constexpr int CELL = 44;        // tall
constexpr int CELL_WIDTH = 48;  // five of them leave room for TARGET beside them
constexpr int CELL_PITCH = 56;
constexpr int BUTTON_TOP = SCREEN_HEIGHT - MARGIN - ui::PILL_HEIGHT;
constexpr int BUTTONS = 3;
constexpr int BUTTON_WIDTH = (INNER_WIDTH - (BUTTONS - 1) * GAP) / BUTTONS;

// QWERTY, as Wordle's keys are.
constexpr const char* KEY_ROWS[3] = {"qwertyuiop", "asdfghjkl", "zxcvbnm"};
constexpr int KEY_ROW_LENGTHS[3] = {10, 9, 7};
constexpr int KEY_GAP = 6;
constexpr int KEY_HEIGHT = 50;
constexpr int KEY_WIDTH = (INNER_WIDTH - 9 * KEY_GAP) / 10;
constexpr int KEYS_TOP = BUTTON_TOP - GAP - 3 * KEY_HEIGHT - 2 * KEY_GAP;
constexpr int MESSAGE_TOP = LADDER_TOP + wl::kRows * ROW_STEP;
constexpr int MESSAGE_HEIGHT = KEYS_TOP - 4 - MESSAGE_TOP;

// The menu: the day's line, a rule and your record, with two columns of pills along the bottom.
constexpr int MENU_PILL_WIDTH = (INNER_WIDTH - GAP) / 2;
constexpr int MENU_LINE_HEIGHT = 96;
constexpr int MENU_PILLS = 6;
constexpr int MENU_PILLS_TOP = BUTTON_TOP - (MENU_PILLS / 2 - 1) * (ui::PILL_HEIGHT + GAP);

enum Button { HINT, UNDO, MENU };
enum MenuPill { TODAY, EARLIER, LATER, START_OVER, ANSWER, RULES };

int buttonLeft(int b) { return MARGIN + b * (BUTTON_WIDTH + GAP); }
int menuPillLeft(int p) { return MARGIN + (p % 2) * (MENU_PILL_WIDTH + GAP); }
int menuPillTop(int p) { return MENU_PILLS_TOP + (p / 2) * (ui::PILL_HEIGHT + GAP); }

int keyLeft(int row, int index) {
  const int count = KEY_ROW_LENGTHS[row];
  const int width = count * KEY_WIDTH + (count - 1) * KEY_GAP;
  return (SCREEN_WIDTH - width) / 2 + index * (KEY_WIDTH + KEY_GAP);
}
int keyTop(int row) { return KEYS_TOP + row * (KEY_HEIGHT + KEY_GAP); }

// The word's cells, centred, with the row's label to their right.
int wordLeft(size_t length) { return (SCREEN_WIDTH - static_cast<int>(length) * CELL_PITCH + (CELL_PITCH - CELL_WIDTH)) / 2; }
int cellLeft(size_t length, int i) { return wordLeft(length) + i * CELL_PITCH; }
int slotTop(int slot) { return LADDER_TOP + slot * ROW_STEP; }

void fillGrey(uint8_t* frame, int left, int top, int width, int height) {
  for (int y = top; y < top + height; y++) {
    for (int x = left; x < left + width; x++) {
      if (x % 2 == 0 && y % 2 == 0) setPixel(frame, x, y, true);
    }
  }
}

std::string upper(const std::string& s) {
  std::string out = s;
  for (char& c : out) {
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
  }
  return out;
}

// One line at Size20 if it fits, else wrapped at Size14, centred in the box either way.
void drawMessage(uint8_t* frame, int top, int height, const char* text) {
  if (textWidth(Font::Size20, text) <= INNER_WIDTH) {
    drawTextInBox(frame, Font::Size20, MARGIN, top, INNER_WIDTH, height, text);
  } else {
    const int h = wrappedHeight(Font::Size14, INNER_WIDTH, text);
    drawWrapped(frame, Font::Size14, MARGIN, top + (height - h) / 2, INNER_WIDTH, text, Align::Center);
  }
}

constexpr const char* RULES_TEXT[] = {
    "CHANGE THE START WORD INTO THE TARGET, ONE LETTER AT A TIME. EVERY STEP MUST BE A WORD, AND NONE TWICE.",
    "TAP A LETTER OF THE NEWEST WORD TO FRAME IT, OR MOVE THE FRAME WITH UP AND DOWN, THEN TAP ITS NEW LETTER ON THE KEYS.",
    "UNDERLINED LETTERS ARE ALREADY WHERE THE TARGET HAS THEM. PAR IS THE SHORTEST LADDER. UNDO TAKES BACK A STEP, AND "
    "HINT ADDS THE NEXT WORD OF A SHORTEST LADDER FROM WHERE YOU ARE.",
    "A NEW LADDER EVERY DAY. MENU HAS EARLIER DAYS, EACH WITH ITS OWN LADDER.",
};

class WordLadder final : public Game {
 public:
  const char* name() const override { return "WORD LADDER"; }
  const uint8_t* icon() const override { return wordladderIcon::ICON; }

  // Opens on today's ladder, as the source does on load; a reopen keeps the day on screen.
  void open() override {
    state_.today = today();
    if (!loaded_) {
      loaded_ = true;
      load();
      wl::openDay(state_, state_.today);
      return;
    }
    state_.mode = wl::Mode::Board;
  }

  TapResult onTap(int x, int y) override {
    if (ui::isOnBack(x, y)) {
      if (state_.mode != wl::Mode::Board) return apply(wl::closePage(state_));
      return TapResult::Leave;
    }
    switch (state_.mode) {
      case wl::Mode::Menu:
        return tapMenu(x, y);
      case wl::Mode::Answer:
      case wl::Mode::Rules:
        if (ui::isInBox(x, y, MARGIN, BUTTON_TOP, INNER_WIDTH, ui::PILL_HEIGHT)) return apply(wl::closePage(state_));
        return TapResult::Nothing;
      case wl::Mode::Board:
        break;
    }
    for (int b = 0; b < BUTTONS; b++) {
      if (!ui::isInBox(x, y, buttonLeft(b), BUTTON_TOP, BUTTON_WIDTH, ui::PILL_HEIGHT)) continue;
      switch (static_cast<Button>(b)) {
        case HINT:
          return apply(wl::hintTap(state_));
        case UNDO:
          return apply(wl::undoTap(state_));
        case MENU:
          return apply(wl::openMenu(state_));
      }
    }
    // A key answers across half the gap around it, as the source's do.
    for (int row = 0; row < 3; row++) {
      for (int i = 0; i < KEY_ROW_LENGTHS[row]; i++) {
        if (ui::isInBox(x, y, keyLeft(row, i) - KEY_GAP / 2, keyTop(row) - KEY_GAP / 2, KEY_WIDTH + KEY_GAP, KEY_HEIGHT + KEY_GAP)) {
          return apply(wl::pressKey(state_, KEY_ROWS[row][i]));
        }
      }
    }
    // A letter of the newest word.
    const wl::Ladder& l = state_.ladder;
    const int top = slotTop(wl::newestSlot(l));
    if (y >= top - 4 && y < top + ROW_STEP) {
      for (int i = 0; i < static_cast<int>(l.puzzle.length); i++) {
        const int left = cellLeft(l.puzzle.length, i) - (CELL_PITCH - CELL_WIDTH) / 2;
        if (x >= left && x < left + CELL_PITCH) return apply(wl::choose(state_, i));
      }
    }
    return TapResult::Nothing;
  }

  // The source's side buttons move the frame along the newest word.
  TapResult onUpPressed() override { return apply(wl::stepFrame(state_, -1)); }
  TapResult onDownPressed() override { return apply(wl::stepFrame(state_, 1)); }

  void draw(uint8_t* frame) override {
    switch (state_.mode) {
      case wl::Mode::Menu:
        drawMenu(frame);
        return;
      case wl::Mode::Answer:
        drawAnswer(frame);
        return;
      case wl::Mode::Rules:
        drawRules(frame);
        return;
      case wl::Mode::Board:
        break;
    }
    ui::drawBand(frame, "WORD LADDER", nullptr, true);
    const wl::Ladder& l = state_.ladder;

    // Outlined while playing, and solid once solved, with the full refresh.
    std::string status;
    const int hints = wl::hintCount(l);
    if (l.solved) {
      status = "SOLVED IN " + std::to_string(l.steps.size());
    } else {
      status = wl::dateLabel(state_.day) + (state_.day == state_.today ? "" : " (EARLIER)");
    }
    status += "   PAR " + std::to_string(l.puzzle.par);
    if (hints > 0) status += "   HINTS " + std::to_string(hints);
    if (l.solved) {
      fillRect(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, true);
    } else {
      drawRectOutline(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, 2);
    }
    const Font statusFont = textWidth(Font::Size20, status.c_str()) <= INNER_WIDTH - 16 ? Font::Size20 : Font::Size14;
    drawTextInBox(frame, statusFont, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, status.c_str(), !l.solved);

    drawLadder(frame);
    drawMessage(frame, MESSAGE_TOP, MESSAGE_HEIGHT, messageText().c_str());
    drawKeys(frame);

    static const char* const LABELS[BUTTONS] = {"HINT", "UNDO", "MENU"};
    for (int b = 0; b < BUTTONS; b++) {
      const bool can = b == HINT ? wl::canHint(state_) : b == UNDO ? wl::canUndo(state_) : true;
      if (can) {
        ui::drawPill(frame, buttonLeft(b), BUTTON_TOP, BUTTON_WIDTH, LABELS[b], true);
      } else {
        ui::drawDimmedPill(frame, buttonLeft(b), BUTTON_TOP, BUTTON_WIDTH, LABELS[b]);
      }
    }
  }

  bool saveFailed() const override { return saveFailed_; }
  void retrySave() override { persist(); }

 private:
  // No clock yet: 1 November 2026, the first day, stands in for today, as in the source.
  static long today() {
    const time_t now = time(nullptr);
    tm parts;
    localtime_r(&now, &parts);
    const long day = wl::daysFromCivil(parts.tm_year + 1900, parts.tm_mon + 1, parts.tm_mday);
    return parts.tm_year + 1900 < 2026 ? wl::kStartDay : day;
  }

  TapResult tapMenu(int x, int y) {
    for (int p = 0; p < MENU_PILLS; p++) {
      if (!ui::isInBox(x, y, menuPillLeft(p), menuPillTop(p), MENU_PILL_WIDTH, ui::PILL_HEIGHT)) continue;
      switch (static_cast<MenuPill>(p)) {
        case TODAY:
          return apply(wl::todayTap(state_));
        case EARLIER:
          return apply(wl::earlierTap(state_));
        case LATER:
          return apply(wl::laterTap(state_));
        case START_OVER:
          return apply(wl::startOverTap(state_));
        case ANSWER:
          return apply(wl::openAnswer(state_));
        case RULES:
          return apply(wl::openRules(state_));
      }
    }
    return TapResult::Nothing;
  }

  TapResult apply(wl::Change change) {
    switch (change) {
      case wl::Change::None:
        return TapResult::Nothing;
      case wl::Change::View:
        return TapResult::Redraw;
      case wl::Change::Move:
        persist();
        return TapResult::Redraw;
      case wl::Change::NewLadder:
        persist();
        return TapResult::RedrawFull;
      case wl::Change::Solved:
        persist();
        return TapResult::RedrawFull;
    }
    return TapResult::Nothing;
  }

  std::string messageText() const {
    const wl::Ladder& l = state_.ladder;
    const std::string word = upper(state_.word);
    switch (state_.message) {
      case wl::Message::Help:
        return "TAP A LETTER OF " + upper(wl::current(l)) + ", THEN A KEY.";
      case wl::Message::NotWord:
        return word + " IS NOT IN THE WORD LIST.";
      case wl::Message::Used:
        return word + " IS ALREADY IN THE LADDER.";
      case wl::Message::Full:
        return "THE LADDER IS FULL: UNDO A STEP.";
      case wl::Message::Hint:
        return "HINT: " + word;
      case wl::Message::NoLadder:
        return "NO LADDER FROM HERE: UNDO A STEP.";
      case wl::Message::Solved: {
        std::string s = "SOLVED IN " + std::to_string(l.steps.size()) + " STEPS (PAR " + std::to_string(l.puzzle.par) + ")";
        const int h = wl::hintCount(l);
        if (h > 0) s += ", HINTS " + std::to_string(h);
        return s + ".";
      }
    }
    return std::string();
  }

  void drawWord(uint8_t* frame, int top, const std::string& w, bool target, int framed) {
    const wl::Puzzle& p = state_.ladder.puzzle;
    for (size_t i = 0; i < p.length; i++) {
      const int left = cellLeft(p.length, static_cast<int>(i));
      if (target) {
        drawRoundRectOutline(frame, left, top, CELL_WIDTH, CELL, 8, ui::HAIRLINE);
      } else if (framed == static_cast<int>(i)) {
        drawRectOutline(frame, left, top, CELL_WIDTH, CELL, ui::FRAME);
      } else {
        drawRectOutline(frame, left, top, CELL_WIDTH, CELL, ui::HAIRLINE);
      }
      const char letter[2] = {static_cast<char>(w[i] - 'a' + 'A'), 0};
      drawTextInBox(frame, Font::Size30, left, top, CELL_WIDTH, CELL, letter);
      // Already where the target has it: underlined in the gap below the cell.
      if (!target && w[i] == p.target[i]) fillRect(frame, left + 8, top + CELL + 2, CELL_WIDTH - 16, 3, true);
    }
  }

  void drawLadder(uint8_t* frame) {
    const wl::Ladder& l = state_.ladder;
    const size_t n = l.puzzle.length;
    const int labelLeft = cellLeft(n, static_cast<int>(n) - 1) + CELL_WIDTH + 12;
    const int newest = static_cast<int>(l.steps.size()) - 1;
    for (const wl::Row& r : wl::rows(l)) {
      const int top = slotTop(r.slot);
      std::string label;
      switch (r.kind) {
        case wl::RowKind::Gap: {
          const std::string s = "... " + std::to_string(r.step) + " MORE STEP" + (r.step == 1 ? "" : "S") + " ...";
          drawTextInBox(frame, Font::Size14, MARGIN, top, INNER_WIDTH, CELL, s.c_str());
          continue;
        }
        case wl::RowKind::Start:
          drawWord(frame, top, l.puzzle.start, false, (newest < 0 && !l.solved) ? l.pos : -1);
          label = "START";
          break;
        case wl::RowKind::Step:
          drawWord(frame, top, l.steps[static_cast<size_t>(r.step)], false, (r.step == newest && !l.solved) ? l.pos : -1);
          label = std::to_string(r.step + 1) + (((l.hinted >> r.step) & 1) ? " HINT" : "");
          break;
        case wl::RowKind::Target:
          drawWord(frame, top, l.puzzle.target, true, -1);
          label = "TARGET";
          break;
      }
      ui::drawTextLeft(frame, Font::Size14, labelLeft, top, CELL, label.c_str());
    }
  }

  // Keys dim once solved, since they can't act.
  void drawKeys(uint8_t* frame) {
    const bool live = !state_.ladder.solved;
    for (int row = 0; row < 3; row++) {
      for (int i = 0; i < KEY_ROW_LENGTHS[row]; i++) {
        const int left = keyLeft(row, i);
        const int top = keyTop(row);
        if (!live) fillGrey(frame, left, top, KEY_WIDTH, KEY_HEIGHT);
        drawRectOutline(frame, left, top, KEY_WIDTH, KEY_HEIGHT, live ? 2 : 1);
        const char letter[2] = {static_cast<char>(KEY_ROWS[row][i] - 'a' + 'A'), 0};
        drawTextInBox(frame, Font::Size20, left, top, KEY_WIDTH, KEY_HEIGHT, letter);
      }
    }
  }

  void drawMenu(uint8_t* frame) {
    ui::drawBand(frame, "MENU", nullptr, true);
    const wl::Puzzle& p = state_.ladder.puzzle;
    const std::string line = wl::dateLabel(state_.day) + ", LADDER " + std::to_string(p.number) + " OF " +
                             std::to_string(wl::kPuzzleCount) + ": " + upper(p.start) + " TO " + upper(p.target) + ", PAR " +
                             std::to_string(p.par) + ".";
    const int h = wrappedHeight(Font::Size20, INNER_WIDTH, line.c_str());
    drawWrapped(frame, Font::Size20, MARGIN, ui::CONTENT_TOP + (MENU_LINE_HEIGHT - h) / 2, INNER_WIDTH, line.c_str(), Align::Center);
    const int ruleTop = ui::CONTENT_TOP + MENU_LINE_HEIGHT + GAP;
    fillRect(frame, MARGIN, ruleTop, INNER_WIDTH, ui::RULE, true);
    int solved = 0;
    for (const auto& day : state_.days) solved += day.second.solved;
    const size_t played = state_.days.size();
    const std::string record = played == 0 ? std::string("NO DAYS PLAYED YET")
                                           : "SOLVED " + std::to_string(solved) + " OF " + std::to_string(played) +
                                                 (played == 1 ? " DAY PLAYED" : " DAYS PLAYED");
    drawTextInBox(frame, Font::Size20, MARGIN, ruleTop + ui::RULE + GAP, INNER_WIDTH, 40, record.c_str());
    static const char* const LABELS[MENU_PILLS] = {"TODAY", "EARLIER", "LATER", "START OVER", "ANSWER", "HOW TO PLAY"};
    for (int p2 = 0; p2 < MENU_PILLS; p2++) {
      const bool can = p2 == TODAY ? wl::canToday(state_) : p2 == EARLIER ? wl::canEarlier(state_)
                       : p2 == LATER ? wl::canLater(state_) : p2 == START_OVER ? wl::canStartOver(state_) : true;
      if (can) {
        ui::drawPill(frame, menuPillLeft(p2), menuPillTop(p2), MENU_PILL_WIDTH, LABELS[p2], true);
      } else {
        ui::drawDimmedPill(frame, menuPillLeft(p2), menuPillTop(p2), MENU_PILL_WIDTH, LABELS[p2]);
      }
    }
  }

  // A shortest ladder, a word a row, in the board's cells.
  void drawAnswer(uint8_t* frame) {
    ui::drawBand(frame, "ANSWER", nullptr, true);
    const wl::Puzzle& p = state_.ladder.puzzle;
    drawTextInBox(frame, Font::Size20, MARGIN, ui::CONTENT_TOP, INNER_WIDTH, STATUS_HEIGHT, "A SHORTEST LADDER");
    for (size_t k = 0; k < p.path.size(); k++) {
      const int top = LADDER_TOP + static_cast<int>(k) * (ROW_STEP + 12);
      drawWord(frame, top, p.path[k], false, -1);
    }
    ui::drawPill(frame, MARGIN, BUTTON_TOP, INNER_WIDTH, "PLAY", true);
  }

  void drawRules(uint8_t* frame) {
    ui::drawBand(frame, "HOW TO PLAY", nullptr, true);
    constexpr int PARAGRAPH_GAP = 16;
    const int room = BUTTON_TOP - GAP - ui::CONTENT_TOP;
    int needed = 0;
    for (const char* paragraph : RULES_TEXT) needed += wrappedHeight(Font::Size20, INNER_WIDTH, paragraph) + PARAGRAPH_GAP;
    const Font font = needed - PARAGRAPH_GAP <= room ? Font::Size20 : Font::Size14;
    int top = ui::CONTENT_TOP;
    for (const char* paragraph : RULES_TEXT) top += drawWrapped(frame, font, MARGIN, top, INNER_WIDTH, paragraph) + PARAGRAPH_GAP;
    ui::drawPill(frame, MARGIN, BUTTON_TOP, INNER_WIDTH, "PLAY", true);
  }

  void load() {
    if (!storage::ready()) return;
    size_t size = 0;
    const storage::Blob blob = storage::loadBlob(SAVE_BLOB, size);
    if (blob && !wl::unpackDays(blob.get(), size, state_)) Serial.println("Ignoring saved Word Ladder days");
  }

  // Every day's ladder, on every change.
  void persist() {
    if (!storage::ready()) {
      saveFailed_ = false;
      return;
    }
    const std::vector<uint8_t> bytes = wl::packDays(state_);
    saveFailed_ = !storage::saveBlob(SAVE_BLOB, bytes.data(), bytes.size());
  }

  wl::State state_{};
  bool loaded_ = false;
  bool saveFailed_ = false;
};

}  // namespace

Game& wordLadderGame() {
  static WordLadder game;
  return game;
}
