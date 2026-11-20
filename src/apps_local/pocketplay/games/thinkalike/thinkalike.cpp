#include "thinkalike.h"

#include <Arduino.h>
#include <esp_random.h>

#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <string>
#include <vector>

#include "core/graphics.h"
#include "core/storage.h"
#include "core/text.h"
#include "thinkalike_deck.h"
#include "thinkalike_icon.h"
#include "thinkalike_questions.h"
#include "thinkalike_save.h"
#include "ui/ui.h"

namespace ta = thinkalike;

namespace {

constexpr const char* SAVE_FILE = "thinkalike";
constexpr const char* CARD_FOLDER = "/thinkalike";
constexpr const char* QUESTION_FILE = "questions.txt";

// The question in a card filling the screen under the band, and the two buttons
// along the bottom. The rules are in the README, so the question gets the room.
constexpr int MARGIN = 16;
constexpr int GAP = 12;
constexpr int INNER_WIDTH = SCREEN_WIDTH - 2 * MARGIN;
constexpr int CARD_PADDING = 24;
constexpr int CARD_TEXT_WIDTH = INNER_WIDTH - 2 * CARD_PADDING;
constexpr int BUTTON_TOP = SCREEN_HEIGHT - MARGIN - ui::PILL_HEIGHT;
constexpr int BUTTON_WIDTH = (INNER_WIDTH - GAP) / 2;

// The buttons where draw() put them, so a tap reads the same geometry.
struct Layout {
  int newLeft = MARGIN;
  int nextLeft = MARGIN + BUTTON_WIDTH + GAP;
  int top = BUTTON_TOP;
  int width = BUTTON_WIDTH;
  int height = ui::PILL_HEIGHT;
};

// True if no word is wider than the line, so drawWrapped never splits one.
bool wordsFit(Font font, int width, const std::string& text) {
  size_t start = 0;
  while (start < text.size()) {
    size_t end = text.find(' ', start);
    if (end == std::string::npos) end = text.size();
    if (end > start && textWidth(font, text.substr(start, end - start).c_str()) > width) return false;
    start = end + 1;
  }
  return true;
}

// The biggest cut that fits the question in the card, whole words and all.
Font questionFont(const std::string& question, int height) {
  for (const Font font : {Font::Size44, Font::Size30, Font::Size20}) {
    if (wordsFit(font, CARD_TEXT_WIDTH, question) && wrappedHeight(font, CARD_TEXT_WIDTH, question.c_str()) <= height) {
      return font;
    }
  }
  return Font::Size14;
}

class ThinkAlike final : public Game {
 public:
  const char* name() const override { return "THINK ALIKE"; }
  const uint8_t* icon() const override { return thinkalikeIcon::ICON; }

  // Reads the card's file every time, so questions edited on a computer show up
  // the next time the game opens. The deck carries on unless the file changed.
  void open() override {
    readQuestions();
    if (questions_.list.empty()) return;
    const uint16_t count = static_cast<uint16_t>(questions_.list.size());
    if (deck_.count == count && deckSum_ == fileSum_) return;
    ta::Saved saved;
    if (load(saved) && saved.deck.count == count && saved.fileSum == fileSum_) {
      deck_ = saved.deck;
      deckSum_ = saved.fileSum;
      order_ = ta::order(deck_);
    } else {
      shuffle();
    }
  }

  TapResult onTap(int x, int y) override {
    if (ui::isOnBack(x, y)) return TapResult::Leave;
    if (questions_.list.empty()) return TapResult::Nothing;
    if (ui::isInBox(x, y, layout_.nextLeft, layout_.top, layout_.width, layout_.height)) return nextQuestion();
    if (ui::isInBox(x, y, layout_.newLeft, layout_.top, layout_.width, layout_.height)) {
      shuffle();
      return TapResult::RedrawFull;
    }
    return TapResult::Nothing;
  }

  TapResult onDownPressed() override {
    if (questions_.list.empty()) return TapResult::Nothing;
    return nextQuestion();
  }

  void draw(uint8_t* frame) override {
    if (questions_.list.empty()) {
      drawMissing(frame);
      return;
    }

    char count[16];
    std::snprintf(count, sizeof(count), "%d/%d", deck_.drawn, deck_.count);
    ui::drawBand(frame, name(), count, true, Font::Size20);

    // The card runs down to the buttons, above a note of lines left out if there are
    // any. Its size never changes between questions, so the layout doesn't jump.
    const bool done = ta::finished(deck_);
    int cardBottom = BUTTON_TOP - 2 * GAP;
    const std::string skipped = skippedNote();
    if (!skipped.empty()) {
      const int noteTop = BUTTON_TOP - 2 * GAP - wrappedHeight(Font::Size10, INNER_WIDTH, skipped.c_str());
      drawWrapped(frame, Font::Size10, MARGIN, noteTop, INNER_WIDTH, skipped.c_str());
      cardBottom = noteTop - 2 * GAP;
    }

    const int cardTop = ui::CONTENT_TOP;
    const int cardHeight = cardBottom - cardTop;
    drawRectOutline(frame, MARGIN, cardTop, INNER_WIDTH, cardHeight, ui::RULE);
    const std::string& question = questions_.list[order_[deck_.drawn - 1]];
    const Font font = questionFont(question, cardHeight - 2 * CARD_PADDING);
    const int textHeight = wrappedHeight(font, CARD_TEXT_WIDTH, question.c_str());
    drawWrapped(frame, font, MARGIN + CARD_PADDING, cardTop + (cardHeight - textHeight) / 2, CARD_TEXT_WIDTH,
                question.c_str(), Align::Center);

    ui::drawPill(frame, layout_.newLeft, layout_.top, layout_.width, "NEW GAME", false);
    if (done) {
      ui::drawDimmedPill(frame, layout_.nextLeft, layout_.top, layout_.width, "NEXT QUESTION");
    } else {
      ui::drawPill(frame, layout_.nextLeft, layout_.top, layout_.width, "NEXT QUESTION", true);
    }
  }

  bool saveFailed() const override { return saveFailed_; }
  void retrySave() override { persist(); }

 private:
  // Why there are no questions to show: no file, a file too big, or one with no
  // questions in it.
  enum class Missing { NoFile, TooBig, Empty };

  void readQuestions() {
    questions_ = ta::Questions{};
    fileSum_ = 0;
    missing_ = Missing::NoFile;
    if (!storage::ready()) return;
    storage::ensureFolder(CARD_FOLDER);
    const std::string path = std::string(CARD_FOLDER) + "/" + QUESTION_FILE;
    size_t size = 0;
    const storage::Blob data = storage::readFile(path.c_str(), ta::MAX_FILE_BYTES, size);
    if (!data) {
      if (fileListed()) missing_ = Missing::TooBig;
      return;
    }
    questions_ = ta::parse(data.get(), size);
    fileSum_ = ta::checksum(data.get(), size);
    missing_ = Missing::Empty;
  }

  // Whether the folder holds the question file, so a file readFile refused can be
  // told apart from one that isn't there. The card's names ignore case.
  bool fileListed() const {
    for (const std::string& file : storage::listFolder(CARD_FOLDER, 200)) {
      if (file.size() != std::strlen(QUESTION_FILE)) continue;
      bool same = true;
      for (size_t i = 0; i < file.size(); i++) {
        const char c = file[i] >= 'A' && file[i] <= 'Z' ? static_cast<char>(file[i] - 'A' + 'a') : file[i];
        same = same && c == QUESTION_FILE[i];
      }
      if (same) return true;
    }
    return false;
  }

  std::string skippedNote() const {
    std::string note;
    char text[96];
    if (questions_.tooLong > 0) {
      std::snprintf(text, sizeof(text), "%d %s OVER %d LETTERS LEFT OUT. ", questions_.tooLong,
                    questions_.tooLong == 1 ? "LINE" : "LINES", static_cast<int>(ta::MAX_QUESTION_CHARS));
      note += text;
    }
    if (questions_.overLimit > 0) {
      std::snprintf(text, sizeof(text), "ONLY THE FIRST %d QUESTIONS ARE USED.", ta::MAX_QUESTIONS);
      note += text;
    }
    while (!note.empty() && note.back() == ' ') note.pop_back();
    return note;
  }

  void drawMissing(uint8_t* frame) {
    ui::drawBand(frame, name(), nullptr, true);
    const char* headline = "NO QUESTIONS";
    const char* body = "COPY QUESTIONS.TXT INTO THE /THINKALIKE FOLDER ON THE CARD, THEN OPEN THINK ALIKE AGAIN.";
    if (missing_ == Missing::TooBig) {
      headline = "FILE TOO BIG";
      body = "QUESTIONS.TXT IN /THINKALIKE IS OVER 64 KB. SPLIT IT, OR TAKE SOME QUESTIONS OUT.";
    } else if (missing_ == Missing::Empty) {
      headline = "NO QUESTIONS";
      body = "QUESTIONS.TXT IN /THINKALIKE HAS NO QUESTIONS IN IT YET.";
    }
    int top = ui::CONTENT_TOP;
    top += drawWrapped(frame, Font::Size30, MARGIN, top, INNER_WIDTH, headline) + GAP;
    top += drawWrapped(frame, Font::Size20, MARGIN, top, INNER_WIDTH, body) + 2 * GAP;
    drawWrapped(frame, Font::Size14, MARGIN, top, INNER_WIDTH,
                "ONE QUESTION PER LINE, UP TO 1000. BLANK LINES AND LINES STARTING WITH # ARE SKIPPED.");
  }

  TapResult nextQuestion() {
    if (!ta::next(deck_)) return TapResult::Nothing;
    persist();
    return TapResult::Redraw;
  }

  // The chip's hardware random numbers, so each new game is a different order.
  void shuffle() {
    deck_ = ta::shuffled(esp_random(), static_cast<uint16_t>(questions_.list.size()));
    deckSum_ = fileSum_;
    order_ = ta::order(deck_);
    persist();
  }

  bool load(ta::Saved& saved) {
    if (!storage::ready()) return false;
    uint8_t data[ta::SAVE_BYTES];
    uint8_t version = 0;
    size_t size = 0;
    if (!storage::load(SAVE_FILE, version, data, sizeof(data), size)) return false;
    if (ta::unpack(version, data, size, saved)) return true;
    Serial.println("Ignoring saved Think Alike deck");
    return false;
  }

  // The deck has no ending, so its save is never erased: it always holds the
  // question on the table and how many have been used.
  void persist() {
    if (!storage::ready()) {
      saveFailed_ = false;
      return;
    }
    ta::Saved saved;
    saved.deck = deck_;
    saved.fileSum = deckSum_;
    uint8_t data[ta::SAVE_BYTES];
    const size_t size = ta::pack(saved, data);
    saveFailed_ = !storage::save(SAVE_FILE, ta::SAVE_VERSION, data, size);
  }

  ta::Questions questions_;
  uint32_t fileSum_ = 0;  // the file on the card
  Missing missing_ = Missing::NoFile;
  ta::Deck deck_{};
  uint32_t deckSum_ = 0;  // the file the deck was shuffled for
  std::vector<uint16_t> order_;
  Layout layout_{};
  bool saveFailed_ = false;
};

}  // namespace

Game& thinkAlikeGame() {
  static ThinkAlike game;
  return game;
}
