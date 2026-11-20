#pragma once

#include <cstdint>

#include "core/text.h"

// The parts every screen shares, so they all line up and behave the same.
namespace ui {

constexpr int MARGIN = 12;
constexpr int GAP = 12;

constexpr int HEADER_TOP = 8;
constexpr int HEADER_BUTTON_WIDTH = 150;
constexpr int HEADER_BUTTON_HEIGHT = 64;

// The warning line, under the header. Content starts below it, so a warning never
// has to move anything.
constexpr int WARNING_TOP = 76;
constexpr int CONTENT_TOP = 108;

bool isInBox(int x, int y, int left, int top, int width, int height);

void drawButton(uint8_t* frame, int left, int top, int width, int height, const char* label);

// BACK on the left, and the title centred in the space beside it. Set hasRightButton
// if the screen draws a button of its own in the top right corner.
void drawHeader(uint8_t* frame, const char* title, bool hasRightButton = false);
bool isOnBack(int x, int y);

void drawWarning(uint8_t* frame, const char* text);

// CrossPlay's Toybox look, for screens moved over to it. Values from its
// ToyboxMetrics.h.
constexpr int HAIRLINE = 1;
constexpr int RULE = 3;
constexpr int FRAME = 4;
constexpr int BOARD_FRAME = 9;
constexpr int ROW_HEIGHT = 62;
constexpr int ROW_GAP = 4;
constexpr int PILL_HEIGHT = 52;
constexpr int PILL_RADIUS = 20;
constexpr int ICON_SIZE = 32;  // a game's home-page icon: 4 bytes a row

// The solid black header, then a white gap and a rule. It ends above WARNING_TOP,
// so the warning line and CONTENT_TOP are where they always are. The title is the
// largest cut that fits, never shortened. rightLabel, such as "2/3", can be null.
// With hasBack, BACK sits where isOnBack() looks.
constexpr int BAND_HEIGHT = 66;
constexpr int BAND_RULE_GAP = 3;
void drawBand(uint8_t* frame, const char* title, const char* rightLabel, bool hasBack, Font labelFont = Font::Size30);

// In landscape the band is slimmer, as CrossPlay's Solitaire has it, because 480 px
// is the height the cards need. Its content starts at LANDSCAPE_CONTENT_TOP.
constexpr int LANDSCAPE_BAND_HEIGHT = 56;
constexpr int LANDSCAPE_CONTENT_TOP = LANDSCAPE_BAND_HEIGHT + BAND_RULE_GAP + RULE;

// Landscape has no free line under the band, so the shell's warning is drawn in the
// band itself, in this slot. drawBand() keeps the title left of it, and a game's own
// band buttons go to its right.
constexpr int LANDSCAPE_WARNING_LEFT = 428;
constexpr int LANDSCAPE_WARNING_WIDTH = 144;

// A rounded button. A filled one is black with white type.
void drawPill(uint8_t* frame, int left, int top, int width, const char* label, bool filled, int height = PILL_HEIGHT);

// A button that can't act: dimmed, never removed, so the layout doesn't jump and it
// still says what it would do. A light grey fill inside a thin outline, with the
// label still black, because type can't be drawn grey.
void drawDimmedPill(uint8_t* frame, int left, int top, int width, const char* label, int height = PILL_HEIGHT);

// A list row: the label on the left and a 32 px icon (or none) on the right.
void drawRow(uint8_t* frame, int top, const char* label, const uint8_t* icon, int height = ROW_HEIGHT);

// Left-aligned text with its capitals centred in a box of the given height.
void drawTextLeft(uint8_t* frame, Font font, int left, int top, int height, const char* text, bool black = true);

// A small battery, its percentage, and a bolt while charging, centred in a band of
// the given height. A percent below 0 means unknown: an empty battery and "--%".
void drawBattery(uint8_t* frame, int left, int top, int height, int percent, bool charging);

}  // namespace ui
