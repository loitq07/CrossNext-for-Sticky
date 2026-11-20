#include "keyboard.h"

#include <cstdio>
#include <cstring>

#include "core/graphics.h"
#include "core/text.h"

namespace keyboard {

namespace {

constexpr int KEY_WIDTH = 46;
constexpr int KEY_GAP = 2;
constexpr int KEY_PITCH = KEY_WIDTH + KEY_GAP;
constexpr int ROW_PITCH = 64;
constexpr int KEY_HEIGHT = ROW_PITCH - KEY_GAP;
constexpr int CHARACTER_ROWS = 4;
constexpr int ROW_COUNT = CHARACTER_ROWS + 1;
constexpr int KEYS_TOP = SCREEN_HEIGHT - ROW_COUNT * ROW_PITCH;
constexpr int CONTROL_TOP = KEYS_TOP + CHARACTER_ROWS * ROW_PITCH;

constexpr int MARGIN = 12;
constexpr int BACK_WIDTH = 120;
constexpr int BACK_HEIGHT = 64;
constexpr int BACK_TOP = 8;
constexpr int TYPED_TOP = 84;
constexpr int HINT_TOP = 220;
constexpr int BORDER = 2;

constexpr const char* LOWER[CHARACTER_ROWS] = {"1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm"};
constexpr const char* UPPER[CHARACTER_ROWS] = {"1234567890", "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM"};
constexpr const char* SYMBOLS[CHARACTER_ROWS] = {"!@#$%^&*()", "-_=+[]{}\\|", ";:'\",.<>/", "?`~"};

enum class Control { Shift, Symbols, Space, Delete, Join };

struct ControlKey {
  Control control;
  const char* label;
  int width;
};

constexpr ControlKey CONTROLS[] = {
    {Control::Shift, "SHIFT", 94},
    {Control::Symbols, "SYM", 72},
    {Control::Space, "SPACE", 142},
    {Control::Delete, "DEL", 72},
    {Control::Join, "JOIN", 90},
};

constexpr int CONTROLS_LEFT = 1;

char typed[MAX_LENGTH + 1] = "";
int typedLength = 0;
char titleText[64] = "";
char actionText[16] = "JOIN";
char hintText[64] = "";
int minLength = MIN_LENGTH;
int maxLength = MAX_LENGTH;
bool shift = false;
bool symbols = false;

const char* const* currentRows() {
  if (symbols) return SYMBOLS;
  return shift ? UPPER : LOWER;
}

int rowLeft(int keys) { return (SCREEN_WIDTH - (keys * KEY_WIDTH + (keys - 1) * KEY_GAP)) / 2; }

bool canJoin() { return typedLength >= minLength; }

void addCharacter(char character) {
  if (typedLength >= maxLength) return;
  typed[typedLength++] = character;
  typed[typedLength] = '\0';
}

Event handleControl(Control control) {
  switch (control) {
    case Control::Shift:
      if (symbols) return Event::None;
      shift = !shift;
      return Event::Changed;
    case Control::Symbols:
      symbols = !symbols;
      shift = false;
      return Event::Changed;
    case Control::Space:
      addCharacter(' ');
      return Event::Changed;
    case Control::Delete:
      if (typedLength == 0) return Event::None;
      typed[--typedLength] = '\0';
      return Event::Changed;
    case Control::Join:
      return canJoin() ? Event::Join : Event::None;
  }
  return Event::None;
}

Event tapControlRow(int x) {
  int left = CONTROLS_LEFT;
  for (const ControlKey& key : CONTROLS) {
    if (x >= left && x < left + key.width + KEY_GAP) return handleControl(key.control);
    left += key.width + KEY_GAP;
  }
  return Event::None;
}

Event tapCharacterRow(int row, int x) {
  const char* keys = currentRows()[row];
  const int count = static_cast<int>(strlen(keys));
  const int offset = x - rowLeft(count);
  if (offset < 0 || offset >= count * KEY_PITCH) return Event::None;

  addCharacter(keys[offset / KEY_PITCH]);
  shift = false;  // applies to one letter, like a phone keyboard
  return Event::Changed;
}

void drawKey(uint8_t* frame, Font font, int left, int top, int width, const char* label, bool on, int border) {
  if (on) {
    fillRect(frame, left, top, width, KEY_HEIGHT, true);
  } else {
    drawRectOutline(frame, left, top, width, KEY_HEIGHT, border);
  }
  drawTextInBox(frame, font, left, top, width, KEY_HEIGHT, label, !on);
}

void drawCharacterKeys(uint8_t* frame) {
  const char* const* rows = currentRows();
  for (int row = 0; row < CHARACTER_ROWS; row++) {
    const int count = static_cast<int>(strlen(rows[row]));
    const int left = rowLeft(count);
    const int top = KEYS_TOP + row * ROW_PITCH;
    for (int i = 0; i < count; i++) {
      const char label[2] = {rows[row][i], '\0'};
      drawKey(frame, Font::Size20, left + i * KEY_PITCH, top, KEY_WIDTH, label, false, BORDER);
    }
  }
}

void drawControlKeys(uint8_t* frame) {
  int left = CONTROLS_LEFT;
  for (const ControlKey& key : CONTROLS) {
    const bool on = (key.control == Control::Shift && shift) || (key.control == Control::Symbols && symbols);
    const char* label = key.control == Control::Symbols && symbols ? "ABC" : key.control == Control::Join ? actionText : key.label;
    // A thin border and no effect on tap until there are enough characters.
    const int border = key.control == Control::Join && !canJoin() ? 1 : BORDER + 1;
    drawKey(frame, Font::Size14, left, CONTROL_TOP, key.width, label, on, border);
    left += key.width + KEY_GAP;
  }
}

}  // namespace

void begin(const char* title, const Options& options) {
  snprintf(titleText, sizeof(titleText), "%s", title);
  snprintf(actionText, sizeof(actionText), "%s", options.action);
  snprintf(hintText, sizeof(hintText), "%s", options.hint ? options.hint : "");
  minLength = options.minLength;
  maxLength = options.maxLength < MAX_LENGTH ? options.maxLength : MAX_LENGTH;
  snprintf(typed, static_cast<size_t>(maxLength) + 1, "%s", options.initial ? options.initial : "");
  typedLength = static_cast<int>(strlen(typed));
  shift = false;
  symbols = false;
}

const char* text() { return typed; }

Event onTap(int x, int y) {
  if (x >= MARGIN && x < MARGIN + BACK_WIDTH && y >= BACK_TOP && y < BACK_TOP + BACK_HEIGHT) return Event::Back;
  if (y >= CONTROL_TOP && y < SCREEN_HEIGHT) return tapControlRow(x);
  if (y >= KEYS_TOP && y < CONTROL_TOP) return tapCharacterRow((y - KEYS_TOP) / ROW_PITCH, x);
  return Event::None;
}

void draw(uint8_t* frame) {
  drawRectOutline(frame, MARGIN, BACK_TOP, BACK_WIDTH, BACK_HEIGHT, BORDER);
  drawTextInBox(frame, Font::Size20, MARGIN, BACK_TOP, BACK_WIDTH, BACK_HEIGHT, "BACK");

  const int titleLeft = MARGIN + BACK_WIDTH + MARGIN;
  char title[sizeof(titleText)];
  fitText(Font::Size20, titleText, SCREEN_WIDTH - titleLeft - MARGIN, title, sizeof(title));
  drawTextInBox(frame, Font::Size20, titleLeft, BACK_TOP, SCREEN_WIDTH - titleLeft - MARGIN, BACK_HEIGHT, title);

  char shown[MAX_LENGTH + 2];
  snprintf(shown, sizeof(shown), "%s_", typed);
  drawWrapped(frame, Font::Size20, MARGIN, TYPED_TOP, SCREEN_WIDTH - 2 * MARGIN, shown);

  if (!canJoin() && hintText[0]) drawTextInBox(frame, Font::Size14, 0, HINT_TOP, SCREEN_WIDTH, lineHeight(Font::Size14), hintText);

  drawCharacterKeys(frame);
  drawControlKeys(frame);
}

}  // namespace keyboard
