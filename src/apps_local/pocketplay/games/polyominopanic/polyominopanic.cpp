#include "polyominopanic.h"

#include <Arduino.h>
#include <esp_random.h>

#include <cstdio>

#include "core/graphics.h"
#include "core/storage.h"
#include "core/text.h"
#include "polyominopanic_cards.h"
#include "polyominopanic_deck.h"
#include "polyominopanic_icon.h"
#include "polyominopanic_save.h"
#include "ui/ui.h"

namespace pp = polyominopanic;

namespace {

constexpr const char* SAVE_FILE = "polyominopanic";

// From the top: the status capsule, the cards, then the buttons along the bottom. The starter
// deck shows one tall card; play shows the round's two cards one above the other, wide, so the
// shapes stay big.
constexpr int MARGIN = 16;
constexpr int GAP = 12;
constexpr int INNER_WIDTH = SCREEN_WIDTH - 2 * MARGIN;
constexpr int STATUS_TOP = ui::CONTENT_TOP;
constexpr int STATUS_HEIGHT = 44;
constexpr int CARD_TOP = STATUS_TOP + STATUS_HEIGHT + GAP;
constexpr int BUTTON_TOP = SCREEN_HEIGHT - MARGIN - ui::PILL_HEIGHT;
constexpr int CARD_RADIUS = 18;
constexpr int SQUARE_GAP = 4;  // white between squares, so each one reads

// The starter card, shaped like the paper card (750 x 1050).
constexpr int STARTER_HEIGHT = BUTTON_TOP - GAP - CARD_TOP;
constexpr int STARTER_WIDTH = STARTER_HEIGHT * 750 / 1050;
constexpr int STARTER_LEFT = (SCREEN_WIDTH - STARTER_WIDTH) / 2;
constexpr int STARTER_CELL = 64;

// The round's pair, and the count of cards left under them.
constexpr int LEFT_HEIGHT = 30;
constexpr int LEFT_TOP = BUTTON_TOP - GAP - LEFT_HEIGHT;
constexpr int PAIR_HEIGHT = (LEFT_TOP - 8 - CARD_TOP - GAP) / 2;
constexpr int PAIR_CELL = 52;

static_assert(STARTER_CELL * pp::kMaxWidth + 2 * GAP <= STARTER_WIDTH, "a starter shape is wider than its card");
static_assert(STARTER_CELL * pp::kMaxHeight + 2 * GAP <= STARTER_HEIGHT, "a starter shape is taller than its card");
static_assert(PAIR_CELL * pp::kMaxWidth + 2 * GAP <= INNER_WIDTH, "a shape is wider than its card");
static_assert(PAIR_CELL * pp::kMaxHeight + 2 * GAP <= PAIR_HEIGHT, "a shape is taller than its card");

int pairTop(int which) { return CARD_TOP + which * (PAIR_HEIGHT + GAP); }

// The buttons. Starter: SHUFFLE, DEAL (wider), START. Play: NEW GAME, NEXT ROUND (wider), as
// Bingo Caller's NEW GAME and CALL. draw() and onTap() both ask.
struct Box {
  int left;
  int width;
};

enum StarterButton { SHUFFLE, DEAL, START };
constexpr int SIDE_WIDTH = 128;

Box starterBox(int b) {
  const int middle = INNER_WIDTH - 2 * SIDE_WIDTH - 2 * GAP;
  if (b == SHUFFLE) return {MARGIN, SIDE_WIDTH};
  if (b == DEAL) return {MARGIN + SIDE_WIDTH + GAP, middle};
  return {MARGIN + INNER_WIDTH - SIDE_WIDTH, SIDE_WIDTH};
}

constexpr int NEW_WIDTH = (INNER_WIDTH - GAP) / 3;
constexpr Box NEW_BOX = {MARGIN, NEW_WIDTH};
constexpr Box NEXT_BOX = {MARGIN + NEW_WIDTH + GAP, INNER_WIDTH - NEW_WIDTH - GAP};

bool onBox(int x, int y, Box box) { return ui::isInBox(x, y, box.left, BUTTON_TOP, box.width, ui::PILL_HEIGHT); }

// A card outline with its shape centred inside, every square solid black.
void drawCard(uint8_t* frame, int left, int top, int width, int height, int cell, int card, bool starter) {
  drawRoundRectOutline(frame, left, top, width, height, CARD_RADIUS, ui::RULE);
  if (card < 0) return;
  const pp::Shape& shape = starter ? pp::kStarter[card] : pp::kGame[card];
  const int w = pp::width(shape), h = pp::height(shape);
  const int x0 = left + (width - w * cell) / 2;
  const int y0 = top + (height - h * cell) / 2;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      if (!pp::filled(shape, x, y)) continue;
      fillRect(frame, x0 + x * cell + SQUARE_GAP / 2, y0 + y * cell + SQUARE_GAP / 2, cell - SQUARE_GAP,
               cell - SQUARE_GAP, true);
    }
  }
}

uint32_t seed() {
  const uint32_t s = esp_random();
  return s != 0 ? s : 1;
}

class PolyominoPanic final : public Game {
 public:
  const char* name() const override { return "POLYOMINO PANIC"; }
  const uint8_t* icon() const override { return polyominopanicIcon::ICON; }

  void open() override {
    if (!loaded_) {
      loaded_ = true;
      load();
    }
    // A finished game isn't kept: it opens on a fresh starter deck.
    if (!fresh_ && pp::over(state_)) fresh_ = true;
    if (fresh_) {
      pp::newGame(state_, seed(), seed());
      fresh_ = false;
      persist();
    }
    newArmed_ = false;
  }

  TapResult onTap(int x, int y) override {
    if (ui::isOnBack(x, y)) {
      newArmed_ = false;
      return TapResult::Leave;
    }
    if (state_.phase == pp::Phase::Starter) {
      if (onBox(x, y, starterBox(SHUFFLE))) return apply(pp::shuffleStarter(state_, seed()));
      if (onBox(x, y, starterBox(DEAL))) return apply(pp::deal(state_));
      if (onBox(x, y, starterBox(START))) return apply(pp::startPlay(state_));
      return TapResult::Nothing;
    }
    if (onBox(x, y, NEXT_BOX)) return nextRound();
    if (onBox(x, y, NEW_BOX)) {
      // The game in play would be lost, so the first tap only asks.
      if (!newArmed_) {
        newArmed_ = true;
        return TapResult::Redraw;
      }
      newArmed_ = false;
      return apply(pp::newGame(state_, seed(), seed()));
    }
    return disarm();
  }

  // Down deals or plays the next round, for playing without looking.
  TapResult onDownPressed() override {
    if (state_.phase == pp::Phase::Starter) return apply(pp::deal(state_));
    return nextRound();
  }

  void draw(uint8_t* frame) override {
    ui::drawBand(frame, name(), nullptr, true);
    drawStatus(frame);
    if (state_.phase == pp::Phase::Starter) {
      drawStarter(frame);
    } else {
      drawPlay(frame);
    }
  }

  bool saveFailed() const override { return saveFailed_; }
  void retrySave() override { persist(); }

 private:
  TapResult nextRound() {
    const pp::Change change = pp::nextRound(state_);
    if (change == pp::Change::None) return disarm();
    newArmed_ = false;
    return apply(change);
  }

  // A tap anywhere else takes back a NEW GAME that was waiting for its second tap.
  TapResult disarm() {
    if (!newArmed_) return TapResult::Nothing;
    newArmed_ = false;
    return TapResult::Redraw;
  }

  TapResult apply(pp::Change change) {
    if (change == pp::Change::None) return TapResult::Nothing;
    persist();
    return change == pp::Change::Full ? TapResult::RedrawFull : TapResult::Redraw;
  }

  // Outlined while dealing, and solid once a deck is used up.
  void drawStatus(uint8_t* frame) const {
    char text[32];
    bool solid = false;
    if (state_.phase == pp::Phase::Starter) {
      if (state_.starter.drawn == 0) {
        std::snprintf(text, sizeof(text), "STARTING SHAPES");
      } else if (pp::starterEmpty(state_)) {
        std::snprintf(text, sizeof(text), "ALL %d DEALT", pp::kStarterCount);
        solid = true;
      } else {
        std::snprintf(text, sizeof(text), "STARTER %d OF %d", state_.starter.drawn, pp::kStarterCount);
      }
    } else if (pp::over(state_)) {
      std::snprintf(text, sizeof(text), "GAME OVER");
      solid = true;
    } else {
      std::snprintf(text, sizeof(text), "ROUND %d OF %d", pp::roundNumber(state_), pp::kRounds);
    }
    if (solid) {
      fillRect(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, true);
    } else {
      drawRectOutline(frame, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, 2);
    }
    drawTextInBox(frame, Font::Size20, MARGIN, STATUS_TOP, INNER_WIDTH, STATUS_HEIGHT, text, !solid);
  }

  void drawStarter(uint8_t* frame) const {
    const int card = pp::starterCard(state_);
    drawCard(frame, STARTER_LEFT, CARD_TOP, STARTER_WIDTH, STARTER_HEIGHT, STARTER_CELL, card, true);
    if (card < 0) {
      drawTextInBox(frame, Font::Size20, STARTER_LEFT, CARD_TOP, STARTER_WIDTH, STARTER_HEIGHT, "TAP DEAL");
    }

    const Box shuffle = starterBox(SHUFFLE), deal = starterBox(DEAL), start = starterBox(START);
    if (state_.starter.drawn == 0) {
      ui::drawDimmedPill(frame, shuffle.left, BUTTON_TOP, shuffle.width, "SHUFFLE");
    } else {
      ui::drawPill(frame, shuffle.left, BUTTON_TOP, shuffle.width, "SHUFFLE", false);
    }
    if (pp::starterEmpty(state_)) {
      ui::drawDimmedPill(frame, deal.left, BUTTON_TOP, deal.width, "DEAL");
    } else {
      ui::drawPill(frame, deal.left, BUTTON_TOP, deal.width, "DEAL", true);
    }
    ui::drawPill(frame, start.left, BUTTON_TOP, start.width, "START", false);
  }

  void drawPlay(uint8_t* frame) const {
    for (int which = 0; which < pp::kPerRound; which++) {
      drawCard(frame, MARGIN, pairTop(which), INNER_WIDTH, PAIR_HEIGHT, PAIR_CELL, pp::gameCard(state_, which), false);
    }

    char left[24];
    const int n = pp::cardsLeft(state_);
    if (n == 0) {
      std::snprintf(left, sizeof(left), "NO CARDS LEFT");
    } else {
      std::snprintf(left, sizeof(left), "%d CARDS LEFT", n);
    }
    drawTextInBox(frame, Font::Size20, MARGIN, LEFT_TOP, INNER_WIDTH, LEFT_HEIGHT, left);

    ui::drawPill(frame, NEW_BOX.left, BUTTON_TOP, NEW_BOX.width, newArmed_ ? "SURE?" : "NEW GAME", false);
    if (pp::over(state_)) {
      ui::drawDimmedPill(frame, NEXT_BOX.left, BUTTON_TOP, NEXT_BOX.width, "NEXT ROUND");
    } else {
      ui::drawPill(frame, NEXT_BOX.left, BUTTON_TOP, NEXT_BOX.width, "NEXT ROUND", true);
    }
  }

  // ---- Saves ----

  void load() {
    fresh_ = true;
    if (!storage::ready()) return;
    uint8_t data[pp::SAVE_SIZE];
    uint8_t version = 0;
    size_t size = 0;
    if (!storage::load(SAVE_FILE, version, data, sizeof(data), size)) return;
    if (pp::unpack(version, data, size, state_)) {
      fresh_ = false;
    } else {
      Serial.println("Ignoring saved Polyomino Panic game");
    }
  }

  // Every deal and round is saved as it happens. The end of the deck ends the game, so its save
  // is erased.
  void persist() {
    if (!storage::ready()) {
      saveFailed_ = false;
      return;
    }
    if (pp::over(state_)) {
      saveFailed_ = !storage::erase(SAVE_FILE);
      return;
    }
    uint8_t data[pp::SAVE_SIZE];
    const size_t size = pp::pack(state_, data);
    saveFailed_ = !storage::save(SAVE_FILE, pp::SAVE_VERSION, data, size);
  }

  pp::State state_{};
  bool loaded_ = false;
  bool fresh_ = true;
  bool newArmed_ = false;
  bool saveFailed_ = false;
};

}  // namespace

Game& polyominoPanicGame() {
  static PolyominoPanic game;
  return game;
}
