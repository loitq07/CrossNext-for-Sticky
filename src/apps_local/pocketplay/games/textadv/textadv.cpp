#include "textadv.h"

#include <Arduino.h>
#include <esp_random.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "core/graphics.h"
#include "core/storage.h"
#include "core/text.h"
#include "textadv_icon.h"
#include "textadv_text.h"
#include "ui/keyboard.h"
#include "ui/ui.h"
#include "zmachine.h"
#include "zork_stories.h"

namespace ta = textadv;

namespace {

constexpr const char* SETTINGS_FILE = "textadv";
constexpr const char* CARD_FOLDER = "/textadv";
constexpr size_t MAX_STORY = 512 * 1024;  // version 8's largest
constexpr size_t TAIL_LINES = 60;         // the transcript an autosave keeps
constexpr size_t MAX_COMMAND = 60;

// The game screen, from the top: the band, the status line, the transcript, the command being
// built, then five rows of buttons anchored to the bottom: directions, actions, two rows of word
// chips, and the command keys.
constexpr int MARGIN = 16;
constexpr int INNER_WIDTH = SCREEN_WIDTH - 2 * MARGIN;
constexpr int STATUS_TOP = ui::CONTENT_TOP;
constexpr int STATUS_HEIGHT = 24;
constexpr int TEXT_TOP = STATUS_TOP + STATUS_HEIGHT + 6;
constexpr int ROW_H = 44;
constexpr int ROW_GAP = 6;
constexpr int KEY_GAP = 4;
constexpr int ROWS_TOP = SCREEN_HEIGHT - MARGIN - 5 * ROW_H - 4 * ROW_GAP;
constexpr int CMD_HEIGHT = 38;
constexpr int CMD_TOP = ROWS_TOP - 6 - CMD_HEIGHT;
constexpr int TEXT_BOTTOM = CMD_TOP - 6;
constexpr int BUTTON_RADIUS = 10;
constexpr int MORE_WIDTH = 44;
constexpr unsigned long SLICE_MS = 40;
constexpr unsigned long THINKING_MS = 400;
constexpr int SLICE_STEPS = 5000;
constexpr int SLICE_ROUNDS = 200;  // a cap as well as the clock, so a slice always ends

int rowTop(int r) { return ROWS_TOP + r * (ROW_H + ROW_GAP); }

struct Key {
  const char* label;  // as in DIR_LABELS and ACT_LABELS
  const char* command;
};
constexpr const char* DIR_LABELS[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
constexpr const char* ACT_LABELS[] = {"UP", "DOWN", "IN", "OUT", "LOOK", "INV", "WAIT"};
constexpr Key DIRS[] = {{"N", "north"}, {"NE", "northeast"}, {"E", "east"}, {"SE", "southeast"},
                        {"S", "south"}, {"SW", "southwest"}, {"W", "west"}, {"NW", "northwest"}};
constexpr Key ACTS[] = {{"UP", "up"},     {"DOWN", "down"},      {"IN", "in"},    {"OUT", "out"},
                        {"LOOK", "look"}, {"INV", "inventory"}, {"WAIT", "wait"}};
enum CommandKey { KEYS, DEL, CLEAR, AGAIN, SEND, MENU };
constexpr const char* COMMAND_LABELS[] = {"KEYS", "DEL", "CLEAR", "AGAIN", "SEND", "MENU"};

// A row of keys across the screen, each as wide as its label needs at Size14 and the rest
// shared out, as the source's row_buttons does. draw and tap both lay out from here.
struct RowLayout {
  int left[8];
  int width[8];
};

RowLayout layoutRow(const char* const* labels, int count) {
  RowLayout r{};
  int need[8];
  int total = 0;
  for (int k = 0; k < count; k++) {
    need[k] = textWidth(Font::Size14, labels[k]) + 12;
    total += need[k];
  }
  const int room = INNER_WIDTH - (count - 1) * KEY_GAP;
  const int spare = room > total ? room - total : 0;
  int x = MARGIN;
  for (int k = 0; k < count; k++) {
    r.width[k] = need[k] + spare / count + (k < spare % count ? 1 : 0);
    r.left[k] = x;
    x += r.width[k] + KEY_GAP;
  }
  return r;
}

// The library: a row a game, then a note on where more go.
constexpr int LIBRARY_ROWS = 8;
constexpr int ROW_STEP = ui::ROW_HEIGHT + ui::ROW_GAP;
constexpr int NOTE_TOP = ui::CONTENT_TOP + LIBRARY_ROWS * ROW_STEP + 8;

// The menu's pills.
constexpr int MENU_TOP = ui::CONTENT_TOP + 150;  // under the note, which wraps to four lines
constexpr int MENU_STEP = ui::PILL_HEIGHT + 12;

std::string upper(const std::string& s) {
  std::string out = s;
  for (char& c : out) {
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
  }
  return out;
}

void drawTextRight(uint8_t* frame, Font font, int right, int top, int height, const char* text) {
  ui::drawTextLeft(frame, font, right - textWidth(font, text), top, height, text);
}

// This screen's keys: rounder than a pill's corners would allow at this size, with Size14 labels
// (Size10 only for one that won't fit), so a two-letter direction stays readable.
void drawKey(uint8_t* frame, int left, int top, int width, const char* label, bool filled) {
  if (filled) {
    fillRoundRect(frame, left, top, width, ROW_H, BUTTON_RADIUS, true);
  } else {
    drawRoundRectOutline(frame, left, top, width, ROW_H, BUTTON_RADIUS, 2);
  }
  const Font font = textWidth(Font::Size14, label) <= width - 4 ? Font::Size14 : Font::Size10;
  drawTextInBox(frame, font, left, top, width, ROW_H, label, !filled);
}

struct Entry {
  std::string name;  // as shown, without the extension
  std::string file;  // as on the card, or the built-in one's name; saves are keyed by it
  const uint8_t* data = nullptr;  // a built-in story, or null for one on the card
  size_t size = 0;
  bool inProgress = false;
};

// Where a chip went, for taps; drawn and hit-tested from the same layout.
struct Chip {
  int left;
  int top;
  int width;
  int index;
};

enum class View : uint8_t { Library, Game, Menu, Keyboard };

class TextAdventures final : public Game {
 public:
  const char* name() const override { return "TEXT ADVENTURES"; }
  const uint8_t* icon() const override { return textadvIcon::ICON; }

  // The last game played opens at once if it's in progress, as in the source.
  void open() override {
    if (loaded_) {
      if (view_ == View::Keyboard || view_ == View::Menu) view_ = game_ >= 0 ? View::Game : View::Library;
      return;
    }
    loaded_ = true;
    loadSettings();
    buildLibrary();
    for (size_t i = 0; i < library_.size(); i++) {
      if (library_[i].file == last_ && library_[i].inProgress) {
        openGame(static_cast<int>(i));
        return;
      }
    }
  }

  TapResult onTap(int x, int y) override {
    switch (view_) {
      case View::Library:
        return tapLibrary(x, y);
      case View::Menu:
        return tapMenu(x, y);
      case View::Keyboard:
        return tapKeyboard(x, y);
      case View::Game:
        break;
    }
    if (ui::isOnBack(x, y)) return leaveGame();
    if (running_) return TapResult::Nothing;

    if (y >= TEXT_TOP && y < TEXT_BOTTOM) {
      // The left third pages back, the rest on.
      return transcript_.page(x < SCREEN_WIDTH / 3 ? -1 : 1, pageLines()) ? TapResult::Redraw : TapResult::Nothing;
    }
    const RowLayout dirs = layoutRow(DIR_LABELS, 8);
    for (int k = 0; k < 8; k++) {
      if (ui::isInBox(x, y, dirs.left[k], rowTop(0), dirs.width[k], ROW_H)) return send(DIRS[k].command);
    }
    const RowLayout acts = layoutRow(ACT_LABELS, 7);
    for (int k = 0; k < 7; k++) {
      if (ui::isInBox(x, y, acts.left[k], rowTop(1), acts.width[k], ROW_H)) return send(ACTS[k].command);
    }
    std::vector<Chip> page;
    int pages = 0;
    layoutChips(page, pages);
    for (const Chip& c : page) {
      if (ui::isInBox(x, y, c.left, c.top, c.width, ROW_H)) {
        const std::string before = cmd_;
        ta::addWord(cmd_, chips_[c.index], MAX_COMMAND);
        return cmd_ == before ? TapResult::Nothing : TapResult::Redraw;
      }
    }
    if (pages > 1 && ui::isInBox(x, y, MARGIN + INNER_WIDTH - MORE_WIDTH, rowTop(2), MORE_WIDTH, 2 * ROW_H + ROW_GAP)) {
      chipPage_ = (chipPage_ + 1) % pages;
      return TapResult::Redraw;
    }
    const RowLayout keys = layoutRow(COMMAND_LABELS, 6);
    for (int k = 0; k < 6; k++) {
      if (ui::isInBox(x, y, keys.left[k], rowTop(4), keys.width[k], ROW_H)) return tapCommandKey(static_cast<CommandKey>(k));
    }
    return TapResult::Nothing;
  }

  TapResult onUpPressed() override { return step(-1); }
  TapResult onDownPressed() override { return step(1); }

  // The game runs in slices between taps, so a long turn never stops the buttons being read.
  bool update() override {
    if (!running_) return false;
    const unsigned long start = millis();
    zmachine::Result r = zmachine::Result::More;
    for (int round = 0; round < SLICE_ROUNDS && r == zmachine::Result::More; round++) {
      r = machine_.run(SLICE_STEPS);
      if (millis() - start >= SLICE_MS) break;
    }
    if (r == zmachine::Result::More) {
      // Say so once a turn is slow enough to notice.
      if (!thinking_ && millis() - runStart_ >= THINKING_MS) {
        thinking_ = true;
        return true;
      }
      return false;
    }
    running_ = false;
    thinking_ = false;
    result_ = r;
    takeReply();
    if (r == zmachine::Result::Input || r == zmachine::Result::Char) {
      autosave();
    } else if (r == zmachine::Result::Quit) {
      // The game is over, so it's no longer offered to continue.
      eraseAutosave();
    } else {
      Serial.printf("Text adventure stopped: %s\n", machine_.error().c_str());
    }
    return true;
  }

  bool busy() const override { return running_; }

  void draw(uint8_t* frame) override {
    switch (view_) {
      case View::Library:
        drawLibrary(frame);
        return;
      case View::Menu:
        drawMenu(frame);
        return;
      case View::Keyboard:
        keyboard::draw(frame);
        return;
      case View::Game:
        break;
    }
    drawGame(frame);
  }

  bool saveFailed() const override { return saveFailed_; }

  void retrySave() override {
    saveFailed_ = false;
    saveSettings();
    if (game_ >= 0 && !running_) autosave();
  }

 private:
  // ---------------------------------------------------------------- the library

  void buildLibrary() {
    library_.clear();
    const struct {
      const char* name;
      const uint8_t* data;
      size_t size;
    } builtins[] = {{"Zork I", ta::ZORK_I, sizeof(ta::ZORK_I)},
                    {"Zork II", ta::ZORK_II, sizeof(ta::ZORK_II)},
                    {"Zork III", ta::ZORK_III, sizeof(ta::ZORK_III)}};
    std::vector<Entry> card;
    if (storage::ready()) {
      storage::ensureFolder(CARD_FOLDER);
      for (const std::string& file : storage::listFolder(CARD_FOLDER, 100)) {
        std::string base;
        if (ta::isStoryFile(file, base)) card.push_back(Entry{base, file, nullptr, 0, false});
      }
    }
    // A Zork on the card takes the built-in one's place, with the same progress.
    for (const auto& b : builtins) {
      bool onCard = false;
      for (const Entry& e : card) onCard = onCard || upper(e.name) == upper(b.name);
      if (!onCard) library_.push_back(Entry{b.name, std::string(b.name) + ".z3", b.data, b.size, false});
    }
    std::sort(card.begin(), card.end(), [](const Entry& a, const Entry& b) { return upper(a.name) < upper(b.name); });
    for (Entry& e : card) library_.push_back(e);
    for (Entry& e : library_) {
      size_t size = 0;
      // An erased autosave is an empty blob.
      e.inProgress = storage::ready() && storage::loadBlob(blobName(e.file, "").c_str(), size) && size > 0;
    }
    if (libraryTop_ >= library_.size()) libraryTop_ = 0;
  }

  TapResult tapLibrary(int x, int y) {
    if (ui::isOnBack(x, y)) return TapResult::Leave;
    for (int r = 0; r < LIBRARY_ROWS; r++) {
      const size_t i = libraryTop_ + r;
      if (i >= library_.size()) break;
      if (ui::isInBox(x, y, ui::MARGIN, ui::CONTENT_TOP + r * ROW_STEP, SCREEN_WIDTH - 2 * ui::MARGIN, ui::ROW_HEIGHT)) {
        openGame(static_cast<int>(i));
        return view_ == View::Game ? TapResult::RedrawFull : TapResult::Redraw;
      }
    }
    return TapResult::Nothing;
  }

  void drawLibrary(uint8_t* frame) {
    ui::drawBand(frame, name(), nullptr, true);
    const int width = SCREEN_WIDTH - 2 * ui::MARGIN;
    for (int r = 0; r < LIBRARY_ROWS; r++) {
      const size_t i = libraryTop_ + r;
      if (i >= library_.size()) break;
      const Entry& e = library_[i];
      const int top = ui::CONTENT_TOP + r * ROW_STEP;
      ui::drawRow(frame, top, upper(e.name).c_str(), nullptr);
      if (e.file == last_) drawRectOutline(frame, ui::MARGIN, top, width, ui::ROW_HEIGHT, ui::FRAME);
      std::string note;
      if (e.inProgress) {
        note = "IN PROGRESS";
      } else if (e.data) {
        note = "BUILT IN";
      } else {
        note = upper(e.file.substr(e.file.size() - 2)) + " FILE";
      }
      drawTextRight(frame, Font::Size14, ui::MARGIN + width - 14, top, ui::ROW_HEIGHT, note.c_str());
    }
    char text[160];
    if (library_.size() > LIBRARY_ROWS) {
      std::snprintf(text, sizeof(text), "GAMES %u-%u OF %u: UP AND DOWN FOR MORE. ", static_cast<unsigned>(libraryTop_ + 1),
                    static_cast<unsigned>(std::min(library_.size(), libraryTop_ + LIBRARY_ROWS)), static_cast<unsigned>(library_.size()));
    } else {
      text[0] = '\0';
    }
    std::snprintf(text + std::strlen(text), sizeof(text) - std::strlen(text),
                  "MORE GAMES: COPY .Z3, .Z5 OR .Z8 STORY FILES INTO %s ON THE CARD.", upper(CARD_FOLDER).c_str());
    int top = ui::CONTENT_TOP + static_cast<int>(std::min<size_t>(library_.size(), LIBRARY_ROWS)) * ROW_STEP + 8;
    // Why the last game tapped didn't open, above the note.
    if (!notice_.empty()) top += drawWrapped(frame, Font::Size20, MARGIN, top, INNER_WIDTH, notice_.c_str()) + 8;
    drawWrapped(frame, Font::Size14, MARGIN, top, INNER_WIDTH, text);
  }

  // ---------------------------------------------------------------- a game

  void openGame(int i) {
    Entry& e = library_[i];
    const uint8_t* data = e.data;
    size_t size = e.size;
    story_.reset();
    if (!data) {
      const std::string path = std::string(CARD_FOLDER) + "/" + e.file;
      story_ = storage::readFile(path.c_str(), MAX_STORY, size);
      data = story_.get();
    }
    std::string error;
    if (!data || !machine_.load(data, size, error)) {
      notice_ = data ? error : "COULDN'T READ " + upper(e.file) + ".";
      view_ = View::Library;
      return;
    }
    notice_.clear();
    machine_.seed(esp_random());
    const std::string slot = blobName(e.file, "-s");
    machine_.onSave = [this, slot](const std::vector<uint8_t>& s) {
      if (!storage::ready()) return false;
      return storage::saveBlob(slot.c_str(), s.data(), s.size());
    };
    machine_.onRestore = [slot](std::vector<uint8_t>& s) {
      size_t n = 0;
      const storage::Blob b = storage::loadBlob(slot.c_str(), n);
      if (!b) return false;
      s.assign(b.get(), b.get() + n);
      return true;
    };
    game_ = i;
    view_ = View::Game;
    transcript_.clear();
    cmd_.clear();
    chips_.clear();
    known_.clear();
    chipPage_ = 0;
    result_ = zmachine::Result::More;
    last_ = e.file;
    saveSettings();

    // Pick up where it was left, as the source does, with the last of the transcript.
    size_t n = 0;
    const storage::Blob snap = storage::loadBlob(blobName(e.file, "").c_str(), n);
    if (snap && n > 0 && machine_.restore(snap.get(), n)) {
      size_t tn = 0;
      const storage::Blob tail = storage::loadBlob(blobName(e.file, "-t").c_str(), tn);
      if (tail) transcript_.setLines(std::string(reinterpret_cast<const char*>(tail.get()), tn));
      transcript_.add("\n[CONTINUED]\n>", measure(), INNER_WIDTH);
      chips_ = ta::makeChips(transcript_.tail(30), known_, [this](const std::string& w) { return machine_.lookup(w) != 0; });
    } else if (snap && n > 0) {
      Serial.println("Ignoring a text adventure autosave that didn't fit its game");
    }
    startRunning();
  }

  void startRunning() {
    running_ = true;
    thinking_ = false;
    runStart_ = millis();
  }

  TapResult send(const std::string& line) {
    if (running_ || game_ < 0) return TapResult::Nothing;
    if (result_ == zmachine::Result::Char) {
      machine_.sendChar(line.empty() ? 13 : static_cast<uint8_t>(line[0]));
    } else if (result_ == zmachine::Result::Input) {
      machine_.sendLine(line);
    } else {
      return TapResult::Nothing;
    }
    cmd_.clear();
    startRunning();
    // Nothing yet: update() redraws once the reply is in, or says THINKING if it's slow.
    return TapResult::Nothing;
  }

  void takeReply() {
    const std::string out = machine_.takeOutput();
    const size_t firstNew = transcript_.size();
    transcript_.add(out, measure(), INNER_WIDTH);
    transcript_.showReply(firstNew, pageLines());
    chips_ = ta::makeChips(ta::withoutEcho(out), known_, [this](const std::string& w) { return machine_.lookup(w) != 0; });
    chipPage_ = 0;
  }

  TapResult tapCommandKey(CommandKey k) {
    switch (k) {
      case KEYS: {
        keyboard::Options options;
        options.minLength = result_ == zmachine::Result::Char ? 0 : 1;
        options.maxLength = static_cast<int>(MAX_COMMAND);
        options.action = "SEND";
        options.hint = nullptr;
        options.initial = cmd_.c_str();
        keyboard::begin("COMMAND", options);
        view_ = View::Keyboard;
        return TapResult::Redraw;
      }
      case DEL:
        return ta::deleteWord(cmd_) ? TapResult::Redraw : TapResult::Nothing;
      case CLEAR:
        if (cmd_.empty()) return TapResult::Nothing;
        cmd_.clear();
        return TapResult::Redraw;
      case AGAIN:
        return send("again");
      case SEND:
        if (result_ != zmachine::Result::Char && cmd_.empty()) return TapResult::Nothing;
        return send(cmd_);
      case MENU:
        view_ = View::Menu;
        return TapResult::Redraw;
    }
    return TapResult::Nothing;
  }

  TapResult tapKeyboard(int x, int y) {
    switch (keyboard::onTap(x, y)) {
      case keyboard::Event::None:
        return TapResult::Nothing;
      case keyboard::Event::Changed:
        return TapResult::Redraw;
      case keyboard::Event::Back:
        cmd_ = keyboard::text();
        view_ = View::Game;
        return TapResult::Redraw;
      case keyboard::Event::Join:
        cmd_ = keyboard::text();
        view_ = View::Game;
        send(cmd_);
        return TapResult::Redraw;
    }
    return TapResult::Nothing;
  }

  // BACK on the board goes to the library; the game is already saved at its last read.
  TapResult leaveGame() {
    if (running_) return TapResult::Nothing;
    view_ = View::Library;
    buildLibrary();
    return TapResult::RedrawFull;
  }

  TapResult step(int delta) {
    if (view_ == View::Game) return transcript_.page(delta, pageLines()) ? TapResult::Redraw : TapResult::Nothing;
    if (view_ != View::Library || library_.size() <= LIBRARY_ROWS) return TapResult::Nothing;
    const long next = static_cast<long>(libraryTop_) + delta * LIBRARY_ROWS;
    if (next < 0 || next >= static_cast<long>(library_.size())) return TapResult::Nothing;
    libraryTop_ = static_cast<size_t>(next);
    return TapResult::Redraw;
  }

  // ---------------------------------------------------------------- the menu

  TapResult tapMenu(int x, int y) {
    if (ui::isOnBack(x, y)) {
      view_ = View::Game;
      return TapResult::Redraw;
    }
    for (int p = 0; p < 3; p++) {
      if (!ui::isInBox(x, y, MARGIN, MENU_TOP + p * MENU_STEP, INNER_WIDTH, ui::PILL_HEIGHT)) continue;
      if (p == 0) {
        small_ = !small_;
        saveSettings();
        transcript_.rewrap(measure(), INNER_WIDTH);
        transcript_.showEnd(pageLines());
        view_ = View::Game;
        return TapResult::RedrawFull;
      }
      if (p == 1) return restartGame();
      view_ = View::Library;
      buildLibrary();
      return TapResult::RedrawFull;
    }
    return TapResult::Nothing;
  }

  TapResult restartGame() {
    if (running_ || game_ < 0) return TapResult::Nothing;
    eraseAutosave();
    const int g = game_;
    openGame(g);
    return TapResult::RedrawFull;
  }

  void drawMenu(uint8_t* frame) {
    ui::drawBand(frame, upper(library_[game_].name).c_str(), nullptr, true);
    drawWrapped(frame, Font::Size14, MARGIN, ui::CONTENT_TOP, INNER_WIDTH,
                "THE GAME IS SAVED AFTER EVERY TURN. SAVE AND RESTORE IN THE GAME KEEP ONE MORE SLOT. BACK RETURNS TO THE GAME.");
    const char* labels[3] = {small_ ? "TEXT SIZE: SMALL" : "TEXT SIZE: LARGE", "RESTART", "GAMES"};
    for (int p = 0; p < 3; p++) ui::drawPill(frame, MARGIN, MENU_TOP + p * MENU_STEP, INNER_WIDTH, labels[p], p != 1);
  }

  // ---------------------------------------------------------------- drawing the game

  Font textFont() const { return small_ ? Font::Size14 : Font::Size20; }
  int textLineHeight() const { return lineHeight(textFont()) + 2; }
  int pageLines() const { return (TEXT_BOTTOM - TEXT_TOP) / textLineHeight(); }
  ta::Measure measure() const {
    const Font font = textFont();
    return [font](const std::string& s) { return textWidth(font, s.c_str()); };
  }

  // The chips on this page in two rows, each as wide as its word, with room kept for the page
  // key when there's more than one page.
  void layoutChips(std::vector<Chip>& page, int& pages) const {
    page.clear();
    pages = 0;
    if (chips_.empty()) return;
    int x = MARGIN;
    int row = 0;
    int p = 0;
    const int right = MARGIN + INNER_WIDTH - MORE_WIDTH - KEY_GAP;
    for (size_t i = 0; i < chips_.size(); i++) {
      int w = textWidth(Font::Size14, upper(chips_[i]).c_str()) + 20;
      if (w < 44) w = 44;
      if (x + w > right) {
        row++;
        x = MARGIN;
        if (row == 2) {
          p++;
          row = 0;
        }
      }
      if (p == chipPage_) page.push_back(Chip{x, rowTop(2 + row), w, static_cast<int>(i)});
      x += w + KEY_GAP;
    }
    pages = p + 1;
  }

  void drawGame(uint8_t* frame) {
    const Entry& e = library_[game_];
    ui::drawBand(frame, upper(e.name).c_str(), nullptr, true);

    std::string left, right;
    machine_.status(left, right);
    if (left.empty()) left = upper(e.name);
    drawTextRight(frame, Font::Size14, MARGIN + INNER_WIDTH, STATUS_TOP, STATUS_HEIGHT, right.c_str());
    ui::drawTextLeft(frame, Font::Size14, MARGIN, STATUS_TOP, STATUS_HEIGHT, upper(left).c_str());
    fillRect(frame, MARGIN, TEXT_TOP - 4, INNER_WIDTH, ui::HAIRLINE, true);

    const int lh = textLineHeight();
    const int n = pageLines();
    const auto& lines = transcript_.lines();
    for (int k = 0; k < n && transcript_.top() + k < lines.size(); k++) {
      ui::drawTextLeft(frame, textFont(), MARGIN, TEXT_TOP + k * lh, lh, lines[transcript_.top() + k].c_str());
    }

    fillRect(frame, MARGIN, CMD_TOP - 3, INNER_WIDTH, ui::HAIRLINE, true);
    std::string prompt;
    if (running_) {
      prompt = thinking_ ? "THINKING..." : "";
    } else if (result_ == zmachine::Result::Quit) {
      prompt = "THE GAME HAS ENDED. MENU: RESTART, OR GAMES.";
    } else if (result_ == zmachine::Result::Error) {
      prompt = "GAME ERROR: " + machine_.error();
    } else if (result_ == zmachine::Result::Char) {
      prompt = "[PRESS SEND TO GO ON]";
    } else {
      prompt = "> " + cmd_ + "_";
    }
    if (lines.size() > static_cast<size_t>(n)) {
      char pos[24];
      std::snprintf(pos, sizeof(pos), "%u/%u", static_cast<unsigned>(std::min(lines.size(), transcript_.top() + n)),
                    static_cast<unsigned>(lines.size()));
      drawTextRight(frame, Font::Size14, MARGIN + INNER_WIDTH, CMD_TOP, CMD_HEIGHT, pos);
    }
    const Font promptFont = textWidth(Font::Size20, prompt.c_str()) <= INNER_WIDTH - 70 ? Font::Size20 : Font::Size14;
    ui::drawTextLeft(frame, promptFont, MARGIN, CMD_TOP, CMD_HEIGHT, prompt.c_str());

    const RowLayout dirs = layoutRow(DIR_LABELS, 8);
    for (int k = 0; k < 8; k++) drawKey(frame, dirs.left[k], rowTop(0), dirs.width[k], DIR_LABELS[k], true);
    const RowLayout acts = layoutRow(ACT_LABELS, 7);
    for (int k = 0; k < 7; k++) drawKey(frame, acts.left[k], rowTop(1), acts.width[k], ACT_LABELS[k], true);
    std::vector<Chip> page;
    int pages = 0;
    layoutChips(page, pages);
    for (const Chip& c : page) drawKey(frame, c.left, c.top, c.width, upper(chips_[c.index]).c_str(), false);
    if (pages > 1) {
      fillRoundRect(frame, MARGIN + INNER_WIDTH - MORE_WIDTH, rowTop(2), MORE_WIDTH, 2 * ROW_H + ROW_GAP, BUTTON_RADIUS, true);
      drawTextInBox(frame, Font::Size20, MARGIN + INNER_WIDTH - MORE_WIDTH, rowTop(2), MORE_WIDTH, 2 * ROW_H + ROW_GAP, ">", false);
    }
    const RowLayout keys = layoutRow(COMMAND_LABELS, 6);
    for (int k = 0; k < 6; k++) drawKey(frame, keys.left[k], rowTop(4), keys.width[k], COMMAND_LABELS[k], k == SEND || k == MENU);
  }

  // ---------------------------------------------------------------- saves

  static std::string blobName(const std::string& file, const char* suffix) {
    char name[40];
    std::snprintf(name, sizeof(name), "textadv-%08lx%s", static_cast<unsigned long>(ta::keyOf(file)), suffix);
    return name;
  }

  // At a read, the whole machine and the last of the transcript.
  void autosave() {
    if (!storage::ready() || game_ < 0 || !machine_.reading()) return;
    std::vector<uint8_t> snap;
    machine_.snapshot(snap);
    const std::string& file = library_[game_].file;
    const std::string tail = transcript_.tail(TAIL_LINES);
    bool ok = storage::saveBlob(blobName(file, "").c_str(), snap.data(), snap.size());
    ok = storage::saveBlob(blobName(file, "-t").c_str(), reinterpret_cast<const uint8_t*>(tail.data()), tail.size()) && ok;
    saveFailed_ = !ok;
  }

  // An empty blob is no save: loadBlob still finds it, so it's written as nothing a restore
  // would take.
  void eraseAutosave() {
    if (!storage::ready() || game_ < 0) return;
    const std::string& file = library_[game_].file;
    const uint8_t none = 0;
    storage::saveBlob(blobName(file, "").c_str(), &none, 0);
  }

  void loadSettings() {
    if (!storage::ready()) return;
    uint8_t data[2 + ta::kMaxFileName];
    uint8_t version = 0;
    size_t size = 0;
    if (storage::load(SETTINGS_FILE, version, data, sizeof(data), size) && !ta::unpackSettings(version, data, size, small_, last_)) {
      Serial.println("Ignoring saved text adventure settings");
    }
  }

  void saveSettings() {
    if (!storage::ready()) return;
    uint8_t data[2 + ta::kMaxFileName];
    const size_t size = ta::packSettings(small_, last_, data);
    if (!storage::save(SETTINGS_FILE, ta::SETTINGS_VERSION, data, size)) saveFailed_ = true;
  }

  std::vector<Entry> library_;
  size_t libraryTop_ = 0;
  int game_ = -1;
  View view_ = View::Library;
  zmachine::Machine machine_;
  storage::Blob story_;
  ta::Transcript transcript_;
  std::string cmd_;
  std::vector<std::string> chips_;
  std::vector<std::string> known_;
  int chipPage_ = 0;
  zmachine::Result result_ = zmachine::Result::More;
  bool running_ = false;
  bool thinking_ = false;
  unsigned long runStart_ = 0;
  std::string notice_;
  std::string last_;
  bool small_ = false;
  bool loaded_ = false;
  bool saveFailed_ = false;
};

}  // namespace

Game& textAdventuresGame() {
  static TextAdventures game;
  return game;
}
