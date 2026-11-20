#include "lightsout.h"

#include <Arduino.h>
#include <esp_random.h>

#include <cstdio>

#include "core/graphics.h"
#include "core/storage.h"
#include "core/text.h"
#include "lightsout_icon.h"
#include "lightsout_rules.h"
#include "lightsout_save.h"
#include "ui/ui.h"

namespace lo = lightsout;

namespace {

constexpr const char* SAVE_FILE = "lightsout";
constexpr const char* STATS_FILE = "lightsout-stats";

// The status under the band, the record with the level beside it, the board as the source lays it out (84 px
// cells at x 30), a line of help, and three buttons along the bottom.
constexpr int MARGIN = 16;
constexpr int GAP = 12;
constexpr int INNER_WIDTH = SCREEN_WIDTH - 2 * MARGIN;
constexpr int STATUS_TOP = ui::CONTENT_TOP;
constexpr int STATUS_HEIGHT = ui::PILL_HEIGHT;
constexpr int RECORD_TOP = STATUS_TOP + STATUS_HEIGHT + 8;
constexpr int RECORD_HEIGHT = ui::PILL_HEIGHT;  // the level pill sits at its right
constexpr int LEVEL_WIDTH = 140;               // NORMAL, the widest level, at Size14
constexpr int CELL = 84;
constexpr int BOARD_SIZE = CELL * lo::kSize;
constexpr int BOARD_LEFT = (SCREEN_WIDTH - BOARD_SIZE) / 2;
constexpr int BOARD_TOP = RECORD_TOP + RECORD_HEIGHT + 8;
constexpr int CELL_INSET = 3;
constexpr int RING_WEIGHT = 4;
constexpr int OUTER_RING = 28;
constexpr int INNER_RING = 12;
constexpr int BRACKET_INSET = 8;
constexpr int BRACKET_LENGTH = 18;
constexpr int MESSAGE_TOP = BOARD_TOP + BOARD_SIZE + 12;
constexpr int BUTTON_TOP = SCREEN_HEIGHT - MARGIN - ui::PILL_HEIGHT;
constexpr int BUTTONS = 3;
constexpr int BUTTON_WIDTH = (INNER_WIDTH - (BUTTONS - 1) * GAP) / BUTTONS;

constexpr const char* HELP = "TAP A SQUARE: IT AND ITS NEIGHBOURS SWITCH. SWITCH THEM ALL OFF.";
constexpr const char* HINT_TEXT = "HINT: PRESS THE FRAMED SQUARE.";
constexpr const char* SOLVED_TEXT = "ALL LIGHTS ARE OFF. NEW MAKES THE NEXT PUZZLE.";

enum Button { NEW, UNDO, HINT, LEVEL };

// Where draw() put the buttons, so a tap reads the same geometry.
struct Layout {
  int buttonLeft[BUTTONS] = {MARGIN, MARGIN + (BUTTON_WIDTH + GAP), MARGIN + 2 * (BUTTON_WIDTH + GAP)};
  int levelLeft = SCREEN_WIDTH - MARGIN - LEVEL_WIDTH;
};

class LightsOut final : public Game {
 public:
  const char* name() const override { return "LIGHTS OUT"; }
  const uint8_t* icon() const override { return lightsoutIcon::ICON; }

  // Memory is the truth once loaded. A solved puzzle isn't kept: coming back starts
  // the next one, as the source does with a dark board.
  void open() override {
    if (!loaded_) {
      loaded_ = true;
      load();
      if (!game_.board) startPuzzle();
    } else if (lo::solved(game_)) {
      startPuzzle();
    }
    message_ = Message::Help;
  }

  TapResult onTap(int x, int y) override {
    if (ui::isOnBack(x, y)) return TapResult::Leave;

    for (int b = 0; b < BUTTONS; b++) {
      if (ui::isInBox(x, y, layout_.buttonLeft[b], BUTTON_TOP, BUTTON_WIDTH, ui::PILL_HEIGHT)) return tapButton(static_cast<Button>(b));
    }
    if (ui::isInBox(x, y, layout_.levelLeft, RECORD_TOP, LEVEL_WIDTH, RECORD_HEIGHT)) return tapButton(LEVEL);

    if (!ui::isInBox(x, y, BOARD_LEFT, BOARD_TOP, BOARD_SIZE, BOARD_SIZE)) return TapResult::Nothing;
    const int cell = ((y - BOARD_TOP) / CELL) * lo::kSize + (x - BOARD_LEFT) / CELL;
    if (!lo::press(game_, cell)) return TapResult::Nothing;
    hint_ = -1;
    message_ = Message::Help;
    if (lo::solved(game_)) {
      stats_.solved++;
      persist();
      persistStats();
      return TapResult::RedrawFull;
    }
    persist();
    return TapResult::Redraw;
  }

  void draw(uint8_t* frame) override {
    ui::drawBand(frame, name(), nullptr, true);

    // Outlined while playing, and solid once solved: a fill that appears once, with
    // the full refresh, is the payoff rather than a cost.
    const bool won = lo::solved(game_);
    char status[40];
    if (!won) {
      std::snprintf(status, sizeof(status), "MOVES %u   TARGET %u", game_.moves, game_.par);
      drawRectOutline(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, 2);
    } else {
      if (game_.moves <= game_.par) {
        std::snprintf(status, sizeof(status), "SOLVED IN %u - FEWEST!", game_.moves);
      } else {
        std::snprintf(status, sizeof(status), "SOLVED IN %u - BEST IS %u", game_.moves, game_.par);
      }
      fillRect(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, true);
    }
    drawTextInBox(frame, Font::Size20, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, status, !won);

    char record[24];
    std::snprintf(record, sizeof(record), "SOLVED %lu", static_cast<unsigned long>(stats_.solved));
    ui::drawTextLeft(frame, Font::Size20, MARGIN, RECORD_TOP, RECORD_HEIGHT, record);
    layout_.levelLeft = SCREEN_WIDTH - MARGIN - LEVEL_WIDTH;
    ui::drawPill(frame, layout_.levelLeft, RECORD_TOP, LEVEL_WIDTH, lo::kLevels[stats_.level].name, false);

    for (int i = 0; i < lo::kCells; i++) drawCell(frame, i);

    drawWrapped(frame, Font::Size14, MARGIN, MESSAGE_TOP, INNER_WIDTH, messageText());

    static const char* const LABELS[BUTTONS] = {"NEW", "UNDO", "HINT"};
    for (int b = 0; b < BUTTONS; b++) {
      layout_.buttonLeft[b] = MARGIN + b * (BUTTON_WIDTH + GAP);
      const int left = layout_.buttonLeft[b];
      if (canAct(static_cast<Button>(b))) {
        ui::drawPill(frame, left, BUTTON_TOP, BUTTON_WIDTH, LABELS[b], true);
      } else {
        ui::drawDimmedPill(frame, left, BUTTON_TOP, BUTTON_WIDTH, LABELS[b]);
      }
    }
  }

  bool saveFailed() const override { return saveFailed_; }

  void retrySave() override {
    persist();
    if (!saveFailed_) persistStats();
  }

 private:
  enum class Message { Help, Hint, Level };

  bool canAct(Button b) const {
    switch (b) {
      case UNDO:
        return !lo::solved(game_) && game_.historyCount > 0;
      case HINT:
        return !lo::solved(game_);
      default:
        return true;
    }
  }

  TapResult tapButton(Button b) {
    if (!canAct(b)) return TapResult::Nothing;
    switch (b) {
      case NEW:
        startPuzzle();
        return TapResult::RedrawFull;
      case UNDO:
        lo::undo(game_);
        hint_ = -1;
        message_ = Message::Help;
        persist();
        return TapResult::Redraw;
      case HINT: {
        const int square = lo::hint(game_);
        if (square < 0 || (square == hint_ && message_ == Message::Hint)) return TapResult::Nothing;
        hint_ = square;
        message_ = Message::Hint;
        return TapResult::Redraw;
      }
      case LEVEL:
        stats_.level = static_cast<uint8_t>((stats_.level + 1) % lo::kLevelCount);
        message_ = Message::Level;
        persistStats();
        return TapResult::Redraw;
    }
    return TapResult::Nothing;
  }

  const char* messageText() {
    if (lo::solved(game_)) return SOLVED_TEXT;
    switch (message_) {
      case Message::Hint:
        return HINT_TEXT;
      case Message::Level:
        std::snprintf(levelText_, sizeof(levelText_), "%s: FROM THE NEXT NEW PUZZLE.", lo::kLevels[stats_.level].name);
        return levelText_;
      case Message::Help:
        break;
    }
    return HELP;
  }

  void drawCell(uint8_t* frame, int i) {
    const int left = BOARD_LEFT + (i % lo::kSize) * CELL;
    const int top = BOARD_TOP + (i / lo::kSize) * CELL;
    drawRectOutline(frame, left + CELL_INSET, top + CELL_INSET, CELL - 2 * CELL_INSET, CELL - 2 * CELL_INSET, 2);
    if ((game_.board >> i) & 1) {
      drawRing(frame, left + CELL / 2, top + CELL / 2, OUTER_RING, RING_WEIGHT, true);
      drawRing(frame, left + CELL / 2, top + CELL / 2, INNER_RING, RING_WEIGHT, true);
    }
    if (i == hint_) {
      // Corner brackets, never a fill: the selection changes every move.
      const int near = BRACKET_INSET;
      const int far = CELL - BRACKET_INSET;
      const int w = ui::FRAME;
      const int l = BRACKET_LENGTH;
      fillRect(frame, left + near, top + near, l, w, true);
      fillRect(frame, left + near, top + near, w, l, true);
      fillRect(frame, left + far - l, top + near, l, w, true);
      fillRect(frame, left + far - w, top + near, w, l, true);
      fillRect(frame, left + near, top + far - w, l, w, true);
      fillRect(frame, left + near, top + far - l, w, l, true);
      fillRect(frame, left + far - l, top + far - w, l, w, true);
      fillRect(frame, left + far - w, top + far - l, w, l, true);
    }
  }

  void startPuzzle() {
    // The chip's hardware random numbers. The rules take a seed and never reach for
    // them themselves.
    uint32_t seed = esp_random();
    lo::newPuzzle(game_, stats_.level, seed);
    hint_ = -1;
    message_ = Message::Help;
    persist();
  }

  void load() {
    if (!storage::ready()) return;
    uint8_t data[lo::SAVE_MAX];
    uint8_t version = 0;
    size_t size = 0;
    if (storage::load(STATS_FILE, version, data, lo::STATS_BYTES, size) && !lo::unpackStats(version, data, size, stats_)) {
      Serial.println("Ignoring saved Lights Out record");
    }
    if (storage::load(SAVE_FILE, version, data, sizeof(data), size) && !lo::unpackGame(version, data, size, game_)) {
      Serial.println("Ignoring saved Lights Out puzzle");
    }
  }

  // Only a puzzle in progress has a save, so a solved one is never offered again.
  void persist() {
    if (!storage::ready()) {
      saveFailed_ = false;
      return;
    }
    if (lo::solved(game_)) {
      saveFailed_ = !storage::erase(SAVE_FILE);
      return;
    }
    uint8_t data[lo::SAVE_MAX];
    const size_t size = lo::packGame(game_, data);
    saveFailed_ = !storage::save(SAVE_FILE, lo::SAVE_VERSION, data, size);
  }

  void persistStats() {
    if (!storage::ready()) return;
    uint8_t data[lo::STATS_BYTES];
    const size_t size = lo::packStats(stats_, data);
    if (!storage::save(STATS_FILE, lo::STATS_VERSION, data, size)) saveFailed_ = true;
  }

  lo::Game game_{};
  lo::Stats stats_{};
  Layout layout_{};
  int hint_ = -1;
  Message message_ = Message::Help;
  char levelText_[48] = {};
  bool loaded_ = false;
  bool saveFailed_ = false;
};

}  // namespace

Game& lightsOutGame() {
  static LightsOut game;
  return game;
}
