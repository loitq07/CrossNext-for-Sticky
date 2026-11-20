#pragma once

#include <cstddef>
#include <cstdint>

// Jersey 25, at CrossPlay's six sizes. Capitals are 13, 18, 25, 38, 57 and 82 px tall.
enum class Font { Size10, Size14, Size20, Size30, Size44, Size64 };

enum class Align { Left, Center, Right };

// Text is ASCII. Any other byte draws as an outlined box, so a missing character
// can be seen rather than silently vanishing.
int textWidth(Font font, const char* text);
int lineHeight(Font font);
int capHeight(Font font);

// x is the left edge, centre or right edge, depending on align.
void drawText(uint8_t* frame, Font font, int x, int baseline, const char* text, Align align = Align::Left, bool black = true);

// Centres the capitals in the box on both axes. Centring on the font's line box
// instead would make all-caps text hang low.
void drawTextInBox(uint8_t* frame, Font font, int left, int top, int width, int height, const char* text, bool black = true);

// Copies text to out, shortened with ".." if it's wider than maxWidth.
void fitText(Font font, const char* text, int maxWidth, char* out, size_t outSize);

// Wraps at spaces and '\n'. A word wider than the box breaks between characters.
// Returns the height used, so the next thing can be drawn below it.
int drawWrapped(uint8_t* frame, Font font, int left, int top, int width, const char* text, Align align = Align::Left, bool black = true);

// The height drawWrapped() would use, without drawing. It breaks lines the same
// way, so a size chosen with it is the size that gets drawn.
int wrappedHeight(Font font, int width, const char* text);
