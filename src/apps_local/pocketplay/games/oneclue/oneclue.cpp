#include "oneclue.h"

#include <Arduino.h>
#include <esp_random.h>

#include <cstdio>
#include <initializer_list>

#include "core/graphics.h"
#include "core/storage.h"
#include "core/text.h"
#include "oneclue_deck.h"
#include "oneclue_icon.h"
#include "oneclue_save.h"
#include "ui/ui.h"

namespace oc = oneclue;

namespace {

constexpr const char* SAVE_FILE = "oneclue";

// A card as five numbered rows under the band, the rules under them, and the two
// buttons along the bottom.
constexpr int MARGIN = 16;
constexpr int GAP = 12;
constexpr int INNER_WIDTH = SCREEN_WIDTH - 2 * MARGIN;
constexpr int ROW_HEIGHT = 76;
constexpr int NUMBER_WIDTH = 40;
constexpr int WORD_LEFT = MARGIN + NUMBER_WIDTH + GAP;
constexpr int WORD_WIDTH = SCREEN_WIDTH - MARGIN - WORD_LEFT;
constexpr int WORD_PADDING = 16;
constexpr int RULES_TOP = ui::CONTENT_TOP + oc::kWords * (ROW_HEIGHT + GAP) + 8;
constexpr int BUTTON_TOP = SCREEN_HEIGHT - MARGIN - ui::PILL_HEIGHT;
constexpr int BUTTON_WIDTH = (INNER_WIDTH - GAP) / 2;

constexpr const char* RULES =
    "THE GUESSER PICKS A NUMBER AND LOOKS AWAY. EVERYONE ELSE WRITES A ONE-WORD CLUE. "
    "MATCHING CLUES ARE CANCELLED. THE GUESSER GETS ONE GUESS.";
constexpr const char* USED_UP = "ALL 110 CARDS USED. NEW GAME RESHUFFLES THE DECK.";

// The buttons where draw() put them, so a tap reads the same geometry.
struct Layout {
  int newLeft = MARGIN;
  int nextLeft = MARGIN + BUTTON_WIDTH + GAP;
  int top = BUTTON_TOP;
  int width = BUTTON_WIDTH;
  int height = ui::PILL_HEIGHT;
};

// The words share one cut across the deck, so the type doesn't change size from
// card to card: the biggest that fits the widest word.
Font wordFont() {
  static bool chosen = false;
  static Font font = Font::Size30;
  if (!chosen) {
    chosen = true;
    for (const Font candidate : {Font::Size44, Font::Size30, Font::Size20}) {
      bool fits = true;
      for (const auto& card : oc::CARDS) {
        for (const char* word : card) {
          if (textWidth(candidate, word) > WORD_WIDTH - 2 * WORD_PADDING) fits = false;
        }
      }
      if (fits) {
        font = candidate;
        break;
      }
    }
  }
  return font;
}

class OneClue final : public Game {
 public:
  const char* name() const override { return "ONE CLUE"; }
  const uint8_t* icon() const override { return oneclueIcon::ICON; }

  // Memory is the truth once loaded, so leaving and coming back keeps the card.
  void open() override {
    if (loaded_) return;
    loaded_ = true;
    if (!load()) {
      oc::shuffle(deck_, esp_random());
      persist();
    }
  }

  TapResult onTap(int x, int y) override {
    if (ui::isOnBack(x, y)) return TapResult::Leave;
    if (ui::isInBox(x, y, layout_.nextLeft, layout_.top, layout_.width, layout_.height)) return nextCard();
    if (ui::isInBox(x, y, layout_.newLeft, layout_.top, layout_.width, layout_.height)) {
      // The chip's hardware random numbers, so each new game is a different order.
      oc::shuffle(deck_, esp_random());
      persist();
      return TapResult::RedrawFull;
    }
    return TapResult::Nothing;
  }

  TapResult onDownPressed() override { return nextCard(); }

  void draw(uint8_t* frame) override {
    char count[12];
    std::snprintf(count, sizeof(count), "%d/%d", deck_.drawn, oc::kCards);
    ui::drawBand(frame, name(), count, true, Font::Size20);

    const Font font = wordFont();
    const auto& card = oc::CARDS[oc::current(deck_)];
    for (int i = 0; i < oc::kWords; i++) {
      const int top = ui::CONTENT_TOP + i * (ROW_HEIGHT + GAP);
      char number[2] = {static_cast<char>('1' + i), '\0'};
      drawTextInBox(frame, Font::Size44, MARGIN, top, NUMBER_WIDTH, ROW_HEIGHT, number);
      drawRectOutline(frame, WORD_LEFT, top, WORD_WIDTH, ROW_HEIGHT, ui::RULE);
      drawTextInBox(frame, font, WORD_LEFT, top, WORD_WIDTH, ROW_HEIGHT, card[i]);
    }

    drawWrapped(frame, Font::Size14, MARGIN, RULES_TOP, INNER_WIDTH, oc::finished(deck_) ? USED_UP : RULES);

    ui::drawPill(frame, layout_.newLeft, layout_.top, layout_.width, "NEW GAME", false);
    if (oc::finished(deck_)) {
      ui::drawDimmedPill(frame, layout_.nextLeft, layout_.top, layout_.width, "NEXT CARD");
    } else {
      ui::drawPill(frame, layout_.nextLeft, layout_.top, layout_.width, "NEXT CARD", true);
    }
  }

  bool saveFailed() const override { return saveFailed_; }
  void retrySave() override { persist(); }

 private:
  TapResult nextCard() {
    if (!oc::next(deck_)) return TapResult::Nothing;
    persist();
    return TapResult::Redraw;
  }

  bool load() {
    if (!storage::ready()) return false;
    uint8_t data[oc::SAVE_BYTES];
    uint8_t version = 0;
    size_t size = 0;
    if (!storage::load(SAVE_FILE, version, data, sizeof(data), size)) return false;
    if (oc::unpackDeck(version, data, size, deck_)) return true;
    Serial.println("Ignoring saved One Clue deck");
    return false;
  }

  // The deck has no ending, so its save is never erased: it always holds the card
  // on the table and the ones already used.
  void persist() {
    if (!storage::ready()) {
      saveFailed_ = false;
      return;
    }
    uint8_t data[oc::SAVE_BYTES];
    const size_t size = oc::packDeck(deck_, data);
    saveFailed_ = !storage::save(SAVE_FILE, oc::SAVE_VERSION, data, size);
  }

  oc::Deck deck_{};
  Layout layout_{};
  bool loaded_ = false;
  bool saveFailed_ = false;
};

}  // namespace

Game& oneClueGame() {
  static OneClue game;
  return game;
}
