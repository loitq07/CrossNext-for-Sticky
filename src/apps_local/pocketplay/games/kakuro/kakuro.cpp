#include "kakuro.h"

#include <Arduino.h>

#include <cstdio>
#include <string>

#include "core/graphics.h"
#include "core/storage.h"
#include "core/text.h"
#include "kakuro_flow.h"
#include "kakuro_icon.h"
#include "kakuro_rules.h"
#include "kakuro_save.h"
#include "ui/ui.h"

namespace kk = kakuro;

namespace {

constexpr const char* SAVE_FILE = "kakuro";

// From the top: the status capsule, the board, the selected cell's two runs, a message line, the
// digit keys, and four pills.
constexpr int MARGIN = 16;
constexpr int GAP = 12;
constexpr int INNER_WIDTH = SCREEN_WIDTH - 2 * MARGIN;
constexpr int STATUS_TOP = ui::CONTENT_TOP;
constexpr int STATUS_HEIGHT = 40;
constexpr int GRID_TOP = STATUS_TOP + STATUS_HEIGHT + 8;
constexpr int GRID_SIDE = 432;
constexpr int LINE_HEIGHT = 22;
constexpr int INFO_TOP = GRID_TOP + GRID_SIDE + 6;
constexpr int MESSAGE_TOP = INFO_TOP + 2 * LINE_HEIGHT + 2;
constexpr int BUTTON_TOP = SCREEN_HEIGHT - MARGIN - ui::PILL_HEIGHT;
constexpr int PAD_HEIGHT = 52;
constexpr int PAD_TOP = BUTTON_TOP - GAP - PAD_HEIGHT;
constexpr int PAD_GAP = 6;
constexpr int PAD_WIDTH = (INNER_WIDTH - 8 * PAD_GAP) / 9;
constexpr int PAD_LEFT = (SCREEN_WIDTH - 9 * PAD_WIDTH - 8 * PAD_GAP) / 2;
constexpr int BUTTONS = 4;
constexpr int BUTTON_GAP = 8;
constexpr int BUTTON_WIDTH = (INNER_WIDTH - (BUTTONS - 1) * BUTTON_GAP) / BUTTONS;
static_assert(MESSAGE_TOP + LINE_HEIGHT <= PAD_TOP - 4, "the message line fits above the keys");

// The MORE page: a row a level, the puzzle you're on, and the lesser actions at the bottom.
constexpr int ROW_STEP = ui::ROW_HEIGHT + ui::ROW_GAP;
constexpr int MORE_LINE_TOP = ui::CONTENT_TOP + kk::kLevels * ROW_STEP + 8;
constexpr int HALF_WIDTH = (INNER_WIDTH - GAP) / 2;
constexpr int MORE_PILLS_TOP = BUTTON_TOP - ui::PILL_HEIGHT - GAP;

enum Button { ERASE, HINT, NEXT, MORE };

int buttonLeft(int b) { return MARGIN + b * (BUTTON_WIDTH + BUTTON_GAP); }
int padLeft(int d) { return PAD_LEFT + (d - 1) * (PAD_WIDTH + PAD_GAP); }

struct Layout {
  int cell;
  int left;
};

Layout layoutFor(int n) {
  Layout l;
  l.cell = GRID_SIDE / n;
  l.left = (SCREEN_WIDTH - l.cell * n) / 2;
  return l;
}

void drawDiagonal(uint8_t* frame, int left, int top, int size) {
  for (int k = 0; k < size; k++) setPixel(frame, left + k, top + k, true);
}

void drawSmallCross(uint8_t* frame, int left, int top, int size) {
  for (int k = 0; k < size; k++) {
    for (int o = 0; o < 2; o++) {
      setPixel(frame, left + k + o, top + k, true);
      setPixel(frame, left + size - 1 - k + o, top + k, true);
    }
  }
}

void fillGrey(uint8_t* frame, int left, int top, int width, int height) {
  for (int y = top; y < top + height; y++) {
    for (int x = left; x < left + width; x++) {
      if (x % 2 == 0 && y % 2 == 0) setPixel(frame, x, y, true);
    }
  }
}

constexpr const char* RULES_TEXT[] = {
    "FILL THE WHITE CELLS WITH DIGITS 1 TO 9. EACH RUN OF WHITE CELLS, ACROSS OR DOWN, ADDS UP TO THE CLUE AT ITS START: "
    "ACROSS IN THE TOP RIGHT OF THE CLUE CELL, DOWN IN THE BOTTOM LEFT. NO DIGIT APPEARS TWICE IN A RUN.",
    "TAP A CELL, THEN A DIGIT. UNDER THE BOARD ARE THE CELL'S RUNS AND EVERY WAY TO MAKE THEM, SUCH AS ACROSS 16 IN 2: 79.",
    "A DIGIT TWICE IN A RUN, OR A RUN THAT ADDS UP WRONG, IS CROSSED. HINT FILLS A CELL, AND ITS DIGIT IS UNDERLINED.",
    "EVERY PUZZLE HAS ONE SOLUTION, FOUND BY REASONING ABOUT THE RUNS.",
};

class Kakuro final : public Game {
 public:
  const char* name() const override { return "KAKURO"; }
  const uint8_t* icon() const override { return kakuroIcon::ICON; }

  void open() override {
    if (!loaded_) {
      loaded_ = true;
      kk::loadPuzzle(state_);
      load();
      return;
    }
    state_.view = kk::View::Board;
  }

  TapResult onTap(int x, int y) override {
    if (ui::isOnBack(x, y)) {
      if (state_.view != kk::View::Board) return apply(kk::closePage(state_));
      return TapResult::Leave;
    }
    switch (state_.view) {
      case kk::View::Rules:
        if (ui::isInBox(x, y, MARGIN, BUTTON_TOP, INNER_WIDTH, ui::PILL_HEIGHT)) return apply(kk::closePage(state_));
        return TapResult::Nothing;
      case kk::View::More:
        return tapMore(x, y);
      case kk::View::Board:
        break;
    }
    for (int b = 0; b < BUTTONS; b++) {
      if (!ui::isInBox(x, y, buttonLeft(b), BUTTON_TOP, BUTTON_WIDTH, ui::PILL_HEIGHT)) continue;
      switch (static_cast<Button>(b)) {
        case ERASE:
          return apply(kk::erase(state_));
        case HINT:
          return apply(kk::hint(state_));
        case NEXT:
          return apply(kk::stepPuzzle(state_, 1));
        case MORE:
          return apply(kk::openMore(state_));
      }
    }
    for (int d = 1; d <= 9; d++) {
      if (ui::isInBox(x, y, padLeft(d) - PAD_GAP / 2, PAD_TOP, PAD_WIDTH + PAD_GAP, PAD_HEIGHT)) return apply(kk::putDigit(state_, d));
    }
    const int n = state_.board.puzzle.n;
    const Layout l = layoutFor(n);
    if (!ui::isInBox(x, y, l.left, GRID_TOP, l.cell * n, l.cell * n)) return TapResult::Nothing;
    return apply(kk::selectCell(state_, ((y - GRID_TOP) / l.cell) * n + (x - l.left) / l.cell));
  }

  // The source's up and down: the previous and next puzzle.
  TapResult onUpPressed() override { return state_.view == kk::View::Board ? apply(kk::stepPuzzle(state_, -1)) : TapResult::Nothing; }
  TapResult onDownPressed() override { return state_.view == kk::View::Board ? apply(kk::stepPuzzle(state_, 1)) : TapResult::Nothing; }

  void draw(uint8_t* frame) override {
    switch (state_.view) {
      case kk::View::Rules:
        drawRules(frame);
        return;
      case kk::View::More:
        drawMore(frame);
        return;
      case kk::View::Board:
        break;
    }
    ui::drawBand(frame, name(), nullptr, true);
    const kk::Board& b = state_.board;
    const bool won = kk::solved(b);
    // Outlined while playing, and solid once solved, with the full refresh.
    char text[64];
    if (won) {
      std::snprintf(text, sizeof(text), "SOLVED!  %s  %d/%d", kk::level(state_.level).label, state_.place[state_.level] + 1, kk::kBankPuzzles);
      fillRect(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, true);
    } else {
      // How many you've solved is on MORE, as in Queens.
      std::snprintf(text, sizeof(text), "%s   %d/%d", kk::level(state_.level).label, state_.place[state_.level] + 1, kk::kBankPuzzles);
      drawRectOutline(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, 2);
    }
    const Font statusFont = textWidth(Font::Size20, text) <= INNER_WIDTH - 24 ? Font::Size20 : Font::Size14;
    drawTextInBox(frame, statusFont, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, text, !won);

    drawGrid(frame);

    if (b.selected >= 0) {
      const kk::Puzzle& p = b.puzzle;
      drawRunInfo(frame, INFO_TOP, p.across[b.selected]);
      drawRunInfo(frame, INFO_TOP + LINE_HEIGHT, p.down[b.selected]);
    }
    drawTextInBox(frame, Font::Size14, MARGIN, MESSAGE_TOP, INNER_WIDTH, LINE_HEIGHT, messageText());

    // The keys dim when there's no cell to put a digit in.
    const bool live = kk::canEnter(state_);
    for (int d = 1; d <= 9; d++) {
      const int left = padLeft(d);
      if (!live) fillGrey(frame, left, PAD_TOP, PAD_WIDTH, PAD_HEIGHT);
      drawRectOutline(frame, left, PAD_TOP, PAD_WIDTH, PAD_HEIGHT, live ? 2 : 1);
      const char digit[2] = {static_cast<char>('0' + d), 0};
      drawTextInBox(frame, Font::Size30, left, PAD_TOP, PAD_WIDTH, PAD_HEIGHT, digit);
    }

    static const char* const LABELS[BUTTONS] = {"ERASE", "HINT", "NEXT", "MORE"};
    for (int k = 0; k < BUTTONS; k++) {
      const bool can = k == ERASE ? kk::canErase(state_) : k == HINT ? kk::canHint(state_) : true;
      if (can) {
        ui::drawPill(frame, buttonLeft(k), BUTTON_TOP, BUTTON_WIDTH, LABELS[k], true);
      } else {
        ui::drawDimmedPill(frame, buttonLeft(k), BUTTON_TOP, BUTTON_WIDTH, LABELS[k]);
      }
    }
  }

  bool saveFailed() const override { return saveFailed_; }
  void retrySave() override { persist(); }

 private:
  TapResult tapMore(int x, int y) {
    for (int l = 0; l < kk::kLevels; l++) {
      if (ui::isInBox(x, y, ui::MARGIN, ui::CONTENT_TOP + l * ROW_STEP, SCREEN_WIDTH - 2 * ui::MARGIN, ui::ROW_HEIGHT)) {
        return apply(kk::chooseLevel(state_, l));
      }
    }
    if (ui::isInBox(x, y, MARGIN, MORE_PILLS_TOP, HALF_WIDTH, ui::PILL_HEIGHT)) return apply(kk::stepPuzzle(state_, -1));
    if (ui::isInBox(x, y, MARGIN + HALF_WIDTH + GAP, MORE_PILLS_TOP, HALF_WIDTH, ui::PILL_HEIGHT)) {
      return canStartOver() ? apply(kk::startOver(state_)) : TapResult::Nothing;
    }
    if (ui::isInBox(x, y, MARGIN, BUTTON_TOP, INNER_WIDTH, ui::PILL_HEIGHT)) return apply(kk::openRules(state_));
    return TapResult::Nothing;
  }

  bool canStartOver() const {
    const kk::Board& b = state_.board;
    for (int i = 0; i < b.puzzle.n * b.puzzle.n; i++) {
      if (b.entry[i]) return true;
    }
    return false;
  }

  TapResult apply(kk::Change change) {
    switch (change) {
      case kk::Change::None:
        return TapResult::Nothing;
      case kk::Change::View:
        return TapResult::Redraw;
      case kk::Change::Move:
        persist();
        return TapResult::Redraw;
      case kk::Change::NewPuzzle:
      case kk::Change::Solved:
        persist();
        return TapResult::RedrawFull;
    }
    return TapResult::Nothing;
  }

  const char* messageText() {
    switch (state_.message) {
      case kk::Message::Help:
        return "TAP A CELL, THEN A DIGIT.";
      case kk::Message::FromHint:
        return "THAT DIGIT CAME FROM A HINT.";
      case kk::Message::Hint:
        std::snprintf(message_, sizeof(message_), "HINT: %d", state_.hintDigit);
        return message_;
      case kk::Message::Solved: {
        const int h = kk::hintCount(state_.board);
        if (h > 0) {
          std::snprintf(message_, sizeof(message_), "SOLVED! HINTS %d. NEXT FOR ANOTHER.", h);
        } else {
          std::snprintf(message_, sizeof(message_), "SOLVED! NEXT FOR ANOTHER.");
        }
        return message_;
      }
    }
    return "";
  }

  // "ACROSS 16 IN 2: 79", or the number of ways when they won't all fit on the line.
  void drawRunInfo(uint8_t* frame, int top, int runIndex) {
    if (runIndex < 0) return;
    const kk::Run& run = state_.board.puzzle.runs[static_cast<size_t>(runIndex)];
    const int length = static_cast<int>(run.cells.size());
    const std::vector<std::string> ways = kk::combos(length, run.sum);
    std::string s = std::string(run.across ? "ACROSS " : "DOWN ") + std::to_string(run.sum) + " IN " + std::to_string(length) + ":";
    std::string all = s;
    for (const std::string& w : ways) all += " " + w;
    if (textWidth(Font::Size14, all.c_str()) > INNER_WIDTH) all = s + " " + std::to_string(ways.size()) + " WAYS";
    ui::drawTextLeft(frame, Font::Size14, MARGIN, top, LINE_HEIGHT, all.c_str());
  }

  void drawGrid(uint8_t* frame) {
    const kk::Board& b = state_.board;
    const kk::Puzzle& p = b.puzzle;
    const int n = p.n;
    const Layout l = layoutFor(n);
    const int c = l.cell;
    const Font digitFont = c >= 60 ? Font::Size30 : Font::Size20;
    bool bad[kk::kMaxCells];
    kk::conflicts(b, bad);
    for (int i = 0; i < n * n; i++) {
      const int x = l.left + (i % n) * c;
      const int y = GRID_TOP + (i / n) * c;
      if (kk::white(p, i)) {
        drawRectOutline(frame, x, y, c + 1, c + 1, ui::HAIRLINE);
        if (i == b.selected) drawRectOutline(frame, x + 3, y + 3, c - 5, c - 5, ui::FRAME);
        const int v = b.entry[i];
        if (v == 0) continue;
        const char digit[2] = {static_cast<char>('0' + v), 0};
        drawTextInBox(frame, digitFont, x, y, c, c, digit);
        if (b.hinted[i]) fillRect(frame, x + c / 2 - 9, y + c - 11, 18, 2, true);
        if (bad[i]) drawSmallCross(frame, x + c - 17, y + 6, 11);
        continue;
      }
      if (p.outside[i]) continue;
      // Clue cells and enclosed blocks: a double border, not black, which would ghost.
      drawRectOutline(frame, x, y, c + 1, c + 1, 2);
      drawRectOutline(frame, x + 4, y + 4, c - 7, c - 7, ui::HAIRLINE);
      if (p.clueAcross[i] == 0 && p.clueDown[i] == 0) {
        // An enclosed block with no clue: grey, so it isn't taken for a cell to fill.
        fillGrey(frame, x + 5, y + 5, c - 9, c - 9);
        continue;
      }
      drawDiagonal(frame, x + 4, y + 4, c - 7);
      if (p.clueAcross[i]) {
        const std::string s = std::to_string(p.clueAcross[i]);
        const int w = textWidth(Font::Size14, s.c_str());
        ui::drawTextLeft(frame, Font::Size14, x + c - 8 - w, y + 6, c / 2 - 6, s.c_str());
      }
      if (p.clueDown[i]) {
        ui::drawTextLeft(frame, Font::Size14, x + 8, y + c / 2, c / 2 - 6, std::to_string(p.clueDown[i]).c_str());
      }
    }
  }

  // A row a level with how many you've solved, the one you're on framed; then the lesser actions.
  void drawMore(uint8_t* frame) {
    ui::drawBand(frame, "MORE", nullptr, true);
    const int width = SCREEN_WIDTH - 2 * ui::MARGIN;
    for (int l = 0; l < kk::kLevels; l++) {
      const int top = ui::CONTENT_TOP + l * ROW_STEP;
      ui::drawRow(frame, top, kk::level(l).menu, nullptr);
      if (l == state_.level) drawRectOutline(frame, ui::MARGIN, top, width, ui::ROW_HEIGHT, ui::FRAME);
      char text[24];
      std::snprintf(text, sizeof(text), "SOLVED %d/%d", kk::doneCount(state_, l), kk::kBankPuzzles);
      const int w = textWidth(Font::Size14, text);
      ui::drawTextLeft(frame, Font::Size14, ui::MARGIN + width - 14 - w, top, ui::ROW_HEIGHT, text);
    }
    char line[64];
    std::snprintf(line, sizeof(line), "%s: PUZZLE %d OF %d.", kk::level(state_.level).label, state_.place[state_.level] + 1, kk::kBankPuzzles);
    drawTextInBox(frame, Font::Size20, MARGIN, MORE_LINE_TOP, INNER_WIDTH, 40, line);
    ui::drawPill(frame, MARGIN, MORE_PILLS_TOP, HALF_WIDTH, "PREVIOUS", true);
    if (canStartOver()) {
      ui::drawPill(frame, MARGIN + HALF_WIDTH + GAP, MORE_PILLS_TOP, HALF_WIDTH, "START OVER", true);
    } else {
      ui::drawDimmedPill(frame, MARGIN + HALF_WIDTH + GAP, MORE_PILLS_TOP, HALF_WIDTH, "START OVER");
    }
    ui::drawPill(frame, MARGIN, BUTTON_TOP, INNER_WIDTH, "HOW TO PLAY", true);
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
    uint8_t data[kk::SAVE_MAX];
    uint8_t version = 0;
    size_t size = 0;
    if (storage::load(SAVE_FILE, version, data, sizeof(data), size) && !kk::unpack(version, data, size, state_)) {
      Serial.println("Ignoring saved Kakuro game");
    }
  }

  // Your place in every level and the digits on the board, on every change.
  void persist() {
    if (!storage::ready()) {
      saveFailed_ = false;
      return;
    }
    uint8_t data[kk::SAVE_MAX];
    const size_t size = kk::pack(state_, data);
    saveFailed_ = !storage::save(SAVE_FILE, kk::SAVE_VERSION, data, size);
  }

  kk::State state_{};
  char message_[48] = {};
  bool loaded_ = false;
  bool saveFailed_ = false;
};

}  // namespace

Game& kakuroGame() {
  static Kakuro game;
  return game;
}
