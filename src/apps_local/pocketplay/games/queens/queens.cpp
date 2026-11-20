#include "queens.h"

#include <Arduino.h>

#include <cmath>
#include <cstdio>

#include "core/graphics.h"
#include "core/storage.h"
#include "core/text.h"
#include "queens_flow.h"
#include "queens_icon.h"
#include "queens_rules.h"
#include "queens_save.h"
#include "ui/ui.h"

namespace qn = queens;

namespace {

constexpr const char* SAVE_FILE = "queens";
constexpr const char* DONE_FILE = "queens-done";

// The status capsule under the band, the board, the rule and count, a message line, and four
// buttons along the bottom. Everything under the board sits where the largest board leaves
// it, so nothing moves between sizes.
constexpr int MARGIN = 16;
constexpr int GAP = 12;
constexpr int INNER_WIDTH = SCREEN_WIDTH - 2 * MARGIN;
constexpr int STATUS_TOP = ui::CONTENT_TOP;
constexpr int STATUS_HEIGHT = 44;
constexpr int BOARD_TOP = STATUS_TOP + STATUS_HEIGHT + 8;
constexpr int GRID_ROOM = INNER_WIDTH - 2 * ui::BOARD_FRAME;  // 430
constexpr int REGION_RULE = 5;
constexpr int RULE_TOP = BOARD_TOP + INNER_WIDTH + 6;
constexpr int LINE_HEIGHT = 24;
constexpr int COUNT_TOP = RULE_TOP + LINE_HEIGHT;
constexpr int BUTTON_TOP = SCREEN_HEIGHT - MARGIN - ui::PILL_HEIGHT;
constexpr int MESSAGE_TOP = COUNT_TOP + LINE_HEIGHT + 4;
constexpr int MESSAGE_HEIGHT = BUTTON_TOP - 6 - MESSAGE_TOP;
constexpr int BUTTONS = 4;
constexpr int BUTTON_WIDTH = (INNER_WIDTH - (BUTTONS - 1) * GAP) / BUTTONS;

// The menu: a row a size, then two rows of two pills.
constexpr int ROW_STEP = ui::ROW_HEIGHT + ui::ROW_GAP;
constexpr int MENU_PILLS_TOP = ui::CONTENT_TOP + qn::kBanks * ROW_STEP + 8;
constexpr int MENU_PILL_WIDTH = (INNER_WIDTH - GAP) / 2;

enum Button { UNDO, HINT, NEXT, MORE };
enum MenuPill { PREVIOUS, START_OVER, DOTS, RULES };

int buttonLeft(int b) { return MARGIN + b * (BUTTON_WIDTH + GAP); }
int menuPillLeft(int p) { return MARGIN + (p % 2) * (MENU_PILL_WIDTH + GAP); }
int menuPillTop(int p) { return MENU_PILLS_TOP + (p / 2) * (ui::PILL_HEIGHT + GAP); }

// Where the board goes for a grid of n, which draw() and onTap() both ask for.
struct Layout {
  int n = 6;
  int cell = GRID_ROOM / 6;
  int gridLeft = 0;
  int gridTop = BOARD_TOP + ui::BOARD_FRAME;
};

Layout layoutFor(int n) {
  Layout l;
  l.n = n;
  l.cell = GRID_ROOM / n;
  l.gridLeft = (SCREEN_WIDTH - l.cell * n) / 2;
  l.gridTop = BOARD_TOP + ui::BOARD_FRAME + (GRID_ROOM - l.cell * n) / 2;
  return l;
}

void drawTextRight(uint8_t* frame, Font font, int right, int top, int height, const char* text) {
  ui::drawTextLeft(frame, font, right - textWidth(font, text), top, height, text);
}

// A filled polygon, by scanlines and the even-odd rule: the crown and the star.
void fillPolygon(uint8_t* frame, const int* xs, const int* ys, int count) {
  int top = ys[0];
  int bottom = ys[0];
  for (int k = 1; k < count; k++) {
    if (ys[k] < top) top = ys[k];
    if (ys[k] > bottom) bottom = ys[k];
  }
  for (int y = top; y <= bottom; y++) {
    const float sy = y + 0.5f;
    float cuts[16];
    int cutCount = 0;
    for (int k = 0; k < count && cutCount < 16; k++) {
      const int j = (k + 1) % count;
      const float y0 = static_cast<float>(ys[k]);
      const float y1 = static_cast<float>(ys[j]);
      if ((sy < y0) == (sy < y1)) continue;
      cuts[cutCount++] = xs[k] + (sy - y0) * (xs[j] - xs[k]) / (y1 - y0);
    }
    for (int a = 1; a < cutCount; a++) {
      for (int b = a; b > 0 && cuts[b] < cuts[b - 1]; b--) {
        const float t = cuts[b];
        cuts[b] = cuts[b - 1];
        cuts[b - 1] = t;
      }
    }
    for (int k = 0; k + 1 < cutCount; k += 2) {
      for (int x = static_cast<int>(std::ceil(cuts[k] - 0.5f)); x + 0.5f < cuts[k + 1]; x++) setPixel(frame, x, y, true);
    }
  }
}

// The source's crown: a band and three points.
void drawQueen(uint8_t* frame, int cx, int cy, int s) {
  const int xs[] = {cx - s, cx - s, cx - s / 2, cx, cx + s / 2, cx + s, cx + s};
  const int ys[] = {cy + s * 6 / 10, cy - s * 5 / 10, cy, cy - s * 8 / 10, cy, cy - s * 5 / 10, cy + s * 6 / 10};
  fillPolygon(frame, xs, ys, 7);
}

// The source's five-pointed star.
void drawStar(uint8_t* frame, int cx, int cy, int s) {
  int xs[10];
  int ys[10];
  for (int k = 0; k < 10; k++) {
    const float a = -3.14159265f / 2 + k * 3.14159265f / 5;
    const float r = (k % 2 == 0) ? 1.0f : 0.42f;
    xs[k] = cx + static_cast<int>(std::floor(std::cos(a) * r * s + 0.5f));
    ys[k] = cy + static_cast<int>(std::floor(std::sin(a) * r * s + 0.5f));
  }
  fillPolygon(frame, xs, ys, 10);
}

// A cross, two pixels thick: a cell marked empty.
void drawCross(uint8_t* frame, int cx, int cy, int d) {
  for (int k = -d; k <= d; k++) {
    for (int o = 0; o < 2; o++) {
      setPixel(frame, cx + k + o, cy + k, true);
      setPixel(frame, cx + k + o, cy - k, true);
    }
  }
}

constexpr const char* RULES_TEXT[] = {
    "PUT ONE QUEEN IN EVERY ROW, EVERY COLUMN AND EVERY REGION: THE AREAS INSIDE THE THICK BORDERS. STAR BATTLE "
    "PUTS TWO STARS IN EACH. NO TWO MAY TOUCH, NOT EVEN DIAGONALLY.",
    "TAP A SQUARE ONCE FOR A CROSS (NOTHING HERE), AGAIN FOR A QUEEN OR STAR, AND A THIRD TIME TO CLEAR IT. WITH DOTS "
    "ON, SQUARES ALREADY RULED OUT GET A DOT. A RINGED PIECE BREAKS A RULE.",
    "EVERY PUZZLE HAS ONE SOLUTION, FOUND BY REASONING: NO GUESSING.",
};

class Queens final : public Game {
 public:
  const char* name() const override { return "QUEENS"; }
  const uint8_t* icon() const override { return queensIcon::ICON; }

  // Memory is the truth once loaded. The rules show once, on the very first open.
  void open() override {
    if (!loaded_) {
      loaded_ = true;
      qn::loadPuzzle(state_);
      const bool hadSave = load();
      if (!hadSave) state_.mode = qn::Mode::Rules;
      state_.message = qn::isSolved(state_) ? qn::Message::Solved : qn::Message::Help;
      return;
    }
    state_.mode = qn::Mode::Board;
    if (!qn::isSolved(state_)) state_.message = qn::Message::Help;
  }

  TapResult onTap(int x, int y) override {
    if (ui::isOnBack(x, y)) {
      if (state_.mode != qn::Mode::Board) return apply(qn::closePage(state_));
      return TapResult::Leave;
    }
    switch (state_.mode) {
      case qn::Mode::Rules:
        if (ui::isInBox(x, y, MARGIN, BUTTON_TOP, INNER_WIDTH, ui::PILL_HEIGHT)) return apply(qn::closePage(state_));
        return TapResult::Nothing;
      case qn::Mode::More:
        return tapMenu(x, y);
      case qn::Mode::Board:
        break;
    }
    for (int b = 0; b < BUTTONS; b++) {
      if (ui::isInBox(x, y, buttonLeft(b), BUTTON_TOP, BUTTON_WIDTH, ui::PILL_HEIGHT)) return tapButton(static_cast<Button>(b));
    }
    const Layout l = layoutFor(state_.game.puzzle.n);
    const int side = l.cell * l.n;
    if (!ui::isInBox(x, y, l.gridLeft, l.gridTop, side, side)) return TapResult::Nothing;
    return apply(qn::tapCell(state_, ((y - l.gridTop) / l.cell) * l.n + (x - l.gridLeft) / l.cell));
  }

  // The source's side buttons: the previous and next puzzle in the bank.
  TapResult onUpPressed() override { return state_.mode == qn::Mode::Board ? apply(qn::stepPuzzle(state_, -1)) : TapResult::Nothing; }
  TapResult onDownPressed() override { return state_.mode == qn::Mode::Board ? apply(qn::stepPuzzle(state_, 1)) : TapResult::Nothing; }

  void draw(uint8_t* frame) override {
    switch (state_.mode) {
      case qn::Mode::Rules:
        drawRules(frame);
        return;
      case qn::Mode::More:
        drawMenu(frame);
        return;
      case qn::Mode::Board:
        break;
    }
    const qn::Bank& bank = qn::kBankList[state_.bank];
    ui::drawBand(frame, bank.title, nullptr, true);

    // Outlined while playing, and solid once solved: a fill that appears once, with the
    // full refresh, is the payoff rather than a cost.
    const bool won = qn::isSolved(state_);
    char text[64];
    std::snprintf(text, sizeof(text), "%s   %d/%d", won ? "SOLVED" : bank.label, state_.place[state_.bank] + 1, qn::kBankPuzzles);
    if (won) {
      fillRect(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, true);
    } else {
      drawRectOutline(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, 2);
    }
    drawTextInBox(frame, Font::Size20, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, text, !won);

    drawBoard(frame);

    const bool one = bank.pieces == 1;
    // The source's two lines: the rule, then the count. How many you've solved is on MORE.
    ui::drawTextLeft(frame, Font::Size14, MARGIN, RULE_TOP, LINE_HEIGHT,
                     one ? "ONE QUEEN PER ROW, COLUMN, REGION." : "TWO STARS PER ROW, COLUMN, REGION.");
    std::snprintf(text, sizeof(text), "NONE TOUCH.   %d OF %d %s PLACED", qn::placed(state_.game), bank.size * bank.pieces,
                  one ? "QUEENS" : "STARS");
    ui::drawTextLeft(frame, Font::Size14, MARGIN, COUNT_TOP, LINE_HEIGHT, text);

    // One line at Size20 when it fits, else wrapped at Size14, centred either way.
    const char* message = messageText();
    if (textWidth(Font::Size20, message) <= INNER_WIDTH) {
      drawTextInBox(frame, Font::Size20, MARGIN, MESSAGE_TOP, INNER_WIDTH, MESSAGE_HEIGHT, message);
    } else {
      const int height = wrappedHeight(Font::Size14, INNER_WIDTH, message);
      drawWrapped(frame, Font::Size14, MARGIN, MESSAGE_TOP + (MESSAGE_HEIGHT - height) / 2, INNER_WIDTH, message, Align::Center);
    }

    static const char* const LABELS[BUTTONS] = {"UNDO", "HINT", "NEXT", "MORE"};
    for (int b = 0; b < BUTTONS; b++) {
      const bool can = b == UNDO ? qn::canUndo(state_) : b == HINT ? qn::canHint(state_) : true;
      if (can) {
        ui::drawPill(frame, buttonLeft(b), BUTTON_TOP, BUTTON_WIDTH, LABELS[b], true);
      } else {
        ui::drawDimmedPill(frame, buttonLeft(b), BUTTON_TOP, BUTTON_WIDTH, LABELS[b]);
      }
    }
  }

  bool saveFailed() const override { return saveFailed_; }

  void retrySave() override {
    persist();
    if (!saveFailed_) persistDone();
  }

 private:
  TapResult tapButton(Button b) {
    switch (b) {
      case UNDO:
        return apply(qn::undoTap(state_));
      case HINT:
        return apply(qn::hintTap(state_));
      case NEXT:
        return apply(qn::stepPuzzle(state_, 1));
      case MORE:
        return apply(qn::openMore(state_));
    }
    return TapResult::Nothing;
  }

  TapResult tapMenu(int x, int y) {
    for (int b = 0; b < qn::kBanks; b++) {
      if (ui::isInBox(x, y, ui::MARGIN, ui::CONTENT_TOP + b * ROW_STEP, SCREEN_WIDTH - 2 * ui::MARGIN, ui::ROW_HEIGHT)) {
        return apply(qn::chooseBank(state_, b));
      }
    }
    for (int p = 0; p < 4; p++) {
      if (!ui::isInBox(x, y, menuPillLeft(p), menuPillTop(p), MENU_PILL_WIDTH, ui::PILL_HEIGHT)) continue;
      switch (static_cast<MenuPill>(p)) {
        case PREVIOUS:
          return apply(qn::stepPuzzle(state_, -1));
        case START_OVER:
          return apply(qn::restart(state_));
        case DOTS:
          return apply(qn::toggleDots(state_));
        case RULES:
          return apply(qn::openRules(state_));
      }
    }
    return TapResult::Nothing;
  }

  // The pages and messages live only in memory; everything else is saved at once.
  TapResult apply(qn::Change change) {
    switch (change) {
      case qn::Change::None:
        return TapResult::Nothing;
      case qn::Change::View:
        return TapResult::Redraw;
      case qn::Change::Move:
        persist();
        return TapResult::Redraw;
      case qn::Change::NewPuzzle:
        persist();
        return TapResult::RedrawFull;
      case qn::Change::Solved:
        persist();
        if (state_.doneChanged) persistDone();
        return TapResult::RedrawFull;
    }
    return TapResult::Nothing;
  }

  const char* messageText() {
    const bool one = state_.game.puzzle.k == 1;
    const char* piece = one ? "QUEEN" : "STAR";
    const char* pieces = one ? "QUEENS" : "STARS";
    switch (state_.message) {
      case qn::Message::Help:
        return "TAP A SQUARE FOR A CROSS, AGAIN FOR A PIECE, AGAIN TO CLEAR.";
      case qn::Message::Touch:
        std::snprintf(messageText_, sizeof(messageText_), "TWO %s TOUCH.", pieces);
        break;
      case qn::Message::Row:
      case qn::Message::Column:
      case qn::Message::Region:
        std::snprintf(messageText_, sizeof(messageText_), "TOO MANY %s IN A %s.", pieces,
                      state_.message == qn::Message::Row ? "ROW" : state_.message == qn::Message::Column ? "COLUMN" : "REGION");
        break;
      case qn::Message::HintLocked:
        std::snprintf(messageText_, sizeof(messageText_), "THAT %s CAME FROM A HINT.", piece);
        break;
      case qn::Message::HintWrongPiece:
        std::snprintf(messageText_, sizeof(messageText_), "HINT: THAT %s WAS WRONG.", piece);
        break;
      case qn::Message::HintCross:
        std::snprintf(messageText_, sizeof(messageText_), "HINT: A %s BELONGS WHERE THAT CROSS WAS.", piece);
        break;
      case qn::Message::HintPlaced:
        std::snprintf(messageText_, sizeof(messageText_), "HINT: A %s.", piece);
        break;
      case qn::Message::Solved: {
        const int h = qn::hints(state_.game);
        if (h > 0) {
          std::snprintf(messageText_, sizeof(messageText_), "SOLVED! HINTS %d. NEXT FOR ANOTHER.", h);
        } else {
          std::snprintf(messageText_, sizeof(messageText_), "SOLVED! NEXT FOR ANOTHER.");
        }
        break;
      }
    }
    return messageText_;
  }

  void drawBoard(uint8_t* frame) {
    const qn::Game& g = state_.game;
    const Layout l = layoutFor(g.puzzle.n);
    const int n = l.n;
    const int c = l.cell;
    const int side = c * n;
    const uint8_t* regions = g.puzzle.region;

    drawRectOutline(frame, l.gridLeft - ui::BOARD_FRAME, l.gridTop - ui::BOARD_FRAME, side + 2 * ui::BOARD_FRAME,
                    side + 2 * ui::BOARD_FRAME, ui::BOARD_FRAME);
    for (int k = 1; k < n; k++) {
      fillRect(frame, l.gridLeft + k * c, l.gridTop, ui::HAIRLINE, side, true);
      fillRect(frame, l.gridLeft, l.gridTop + k * c, side, ui::HAIRLINE, true);
    }
    // Regions are told apart by thick borders, as printed puzzles are: the panel has no
    // colours. Each runs on past the corners so that they join.
    const int half = REGION_RULE / 2;
    for (int r = 0; r < n; r++) {
      for (int col = 0; col < n; col++) {
        const int i = r * n + col;
        const int x = l.gridLeft + col * c;
        const int y = l.gridTop + r * c;
        if (col < n - 1 && regions[i] != regions[i + 1]) fillRect(frame, x + c - half, y - half, REGION_RULE, c + REGION_RULE, true);
        if (r < n - 1 && regions[i] != regions[i + n]) fillRect(frame, x - half, y + c - half, c + REGION_RULE, REGION_RULE, true);
      }
    }

    bool bad[qn::kMaxCells];
    qn::conflicts(g, bad);
    bool out[qn::kMaxCells] = {};
    if (state_.dots) qn::ruledOut(g, out);
    const int s = c * 32 / 100;
    for (int i = 0; i < n * n; i++) {
      const int x = l.gridLeft + (i % n) * c;
      const int y = l.gridTop + (i / n) * c;
      const int cx = x + c / 2;
      const int cy = y + c / 2;
      if (g.marks[i] == qn::Piece) {
        if (g.puzzle.k == 1) {
          drawQueen(frame, cx, cy + s / 10, s);
        } else {
          drawStar(frame, cx, cy, s + 2);
        }
        // A piece from a hint is underlined, as in the source.
        if (g.hinted[i]) fillRect(frame, cx - s, y + c - 10, 2 * s, 2, true);
        if (bad[i]) drawRing(frame, cx, cy, c * 44 / 100, 3, true);
      } else if (g.marks[i] == qn::Cross) {
        drawCross(frame, cx, cy, c / 6);
      } else if (out[i]) {
        const int r = c / 16 < 2 ? 2 : c / 16;
        drawRing(frame, cx, cy, r, r, true);
      }
    }
  }

  // Each size with how many you've solved, the one you're on framed; then the lesser actions.
  void drawMenu(uint8_t* frame) {
    ui::drawBand(frame, "MORE", nullptr, true);
    const int width = SCREEN_WIDTH - 2 * ui::MARGIN;
    for (int b = 0; b < qn::kBanks; b++) {
      const int top = ui::CONTENT_TOP + b * ROW_STEP;
      ui::drawRow(frame, top, qn::kBankList[b].menu, nullptr);
      if (b == state_.bank) drawRectOutline(frame, ui::MARGIN, top, width, ui::ROW_HEIGHT, ui::FRAME);
      char text[24];
      std::snprintf(text, sizeof(text), "SOLVED %d/%d", qn::doneCount(state_, b), qn::kBankPuzzles);
      drawTextRight(frame, Font::Size14, ui::MARGIN + width - 14, top, ui::ROW_HEIGHT, text);
    }
    const char* labels[4] = {"PREVIOUS", "START OVER", state_.dots ? "DOTS ON" : "DOTS OFF", "RULES"};
    for (int p = 0; p < 4; p++) ui::drawPill(frame, menuPillLeft(p), menuPillTop(p), MENU_PILL_WIDTH, labels[p], true);
  }

  void drawRules(uint8_t* frame) {
    ui::drawBand(frame, "HOW TO PLAY", nullptr, true);
    // Size20 if all three paragraphs fit above PLAY, else Size14.
    constexpr int PARAGRAPH_GAP = 16;
    const int room = BUTTON_TOP - GAP - ui::CONTENT_TOP;
    int needed = 0;
    for (const char* paragraph : RULES_TEXT) needed += wrappedHeight(Font::Size20, INNER_WIDTH, paragraph) + PARAGRAPH_GAP;
    const Font font = needed - PARAGRAPH_GAP <= room ? Font::Size20 : Font::Size14;
    int top = ui::CONTENT_TOP;
    for (const char* paragraph : RULES_TEXT) top += drawWrapped(frame, font, MARGIN, top, INNER_WIDTH, paragraph) + PARAGRAPH_GAP;
    ui::drawPill(frame, MARGIN, BUTTON_TOP, INNER_WIDTH, "PLAY", true);
  }

  // True if there was a save of either kind, so the rules aren't shown again.
  bool load() {
    if (!storage::ready()) return true;
    bool found = false;
    uint8_t data[LOAD_BYTES];
    uint8_t version = 0;
    size_t size = 0;
    if (storage::load(DONE_FILE, version, data, sizeof(data), size)) {
      found = true;
      if (!qn::unpackDone(version, data, size, state_)) Serial.println("Ignoring saved Queens record");
    }
    if (storage::load(SAVE_FILE, version, data, sizeof(data), size)) {
      found = true;
      if (!qn::unpackGame(version, data, size, state_)) Serial.println("Ignoring saved Queens board");
    }
    return found;
  }

  // The save is your place in every bank, so it outlasts a solved puzzle.
  void persist() {
    if (!storage::ready()) {
      saveFailed_ = false;
      return;
    }
    uint8_t data[qn::SAVE_MAX];
    const size_t size = qn::packGame(state_, data);
    saveFailed_ = !storage::save(SAVE_FILE, qn::SAVE_VERSION, data, size);
  }

  void persistDone() {
    if (!storage::ready()) return;
    uint8_t data[qn::DONE_BYTES];
    const size_t size = qn::packDone(state_, data);
    if (storage::save(DONE_FILE, qn::DONE_VERSION, data, size)) {
      state_.doneChanged = false;
    } else {
      saveFailed_ = true;
    }
  }

  // load() reads either save into one buffer.
  static constexpr size_t LOAD_BYTES = qn::DONE_BYTES > qn::SAVE_MAX ? qn::DONE_BYTES : qn::SAVE_MAX;

  qn::State state_{};
  char messageText_[64] = {};
  bool loaded_ = false;
  bool saveFailed_ = false;
};

}  // namespace

Game& queensGame() {
  static Queens game;
  return game;
}
