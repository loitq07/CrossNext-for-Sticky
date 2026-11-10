#pragma once

// Font registry shared by the renderer and host regression tests.
#include <EpdFontFamily.h>

#include "../../fontIds.h"
#include "ToyboxFonts.h"
#include "fonts/baemin_10.h"
#include "fonts/baemin_12.h"
#include "fonts/baemin_14.h"
#include "fonts/baemin_18.h"
#include "fonts/book_bold_10.h"
#include "fonts/book_text_10.h"
#include "fonts/display_sans_22.h"
#include "fonts/display_sans_40.h"
#include "fonts/display_sans_56.h"
#include "fonts/instrument_10.h"
#include "fonts/instrument_13.h"
#include "fonts/instrument_24.h"
#include "fonts/reading_serif_11.h"
#include "fonts/reading_serif_14.h"
#include "fonts/reading_serif_bold_11.h"
#include "fonts/reading_serif_bold_12.h"
#include "fonts/reading_serif_bold_14.h"
#include "fonts/reading_serif_bold_16.h"
#include "fonts/toybox_10.h"
#include "fonts/toybox_14.h"
#include "fonts/toybox_20.h"
#include "fonts/toybox_30.h"
#include "fonts/ui_prose_10.h"
#include "fonts/ui_prose_14.h"
#include "fonts/ui_prose_bold_14.h"

extern EpdFontFamily smallFontFamily;
extern EpdFontFamily ui10FontFamily;
extern EpdFontFamily ui12FontFamily;

namespace toybox::fontdetail {
namespace {
// Daniel is one selectable family; the canonical Ubuntu/Noto UI fonts remain
// available as Unicode fallbacks when a display face lacks a character.
EpdFont prose14(&baemin_14);
EpdFont proseBold14(&baemin_14);
EpdFontFamily proseFamily(&prose14, &proseBold14);
EpdFont library10(&baemin_12);
EpdFont libraryBold10(&baemin_12);
EpdFontFamily libraryFamily(&library10, &libraryBold10);
EpdFont bookText10(&book_text_10);
EpdFontFamily bookListFamily(&bookText10, &libraryBold10);
EpdFont display30(&baemin_18);
EpdFont large44(&display_sans_40);
EpdFont clockLabel22(&display_sans_22);
EpdFont huge64(&display_sans_56);
EpdFont ui20(&baemin_14);
EpdFont carousel18(&baemin_14);
EpdFont tile10(&baemin_10);
// Noto Serif, converted at 1 bit by us rather than taken from builtinFonts/.
// The builtin cuts are --2bit, and GfxRenderer's BW path paints a pixel for ANY
// coverage above zero, so they flood to mush -- which is why Toybox has its own
// face at all. Converting the same TTF at 1 bit sidesteps the flooding entirely
// and preserves thin strokes and open counters in monochrome.
//
// Noto Serif specifically because it is what the EPUB reader sets books in. An
// app whose surface is a page of prose should look like the rest of this
// device's reading, not like its games.
EpdFont button14(&baemin_12);
EpdFont reading11(&reading_serif_11);
EpdFont reading14(&reading_serif_14);
EpdFont readingBold12(&reading_serif_bold_12);
EpdFont readingBold16(&reading_serif_bold_16);
EpdFont instrument10(&instrument_10);
EpdFont instrument13(&instrument_13);
EpdFont instrument24(&instrument_24);
EpdFontFamily displayFamily(&display30);
EpdFontFamily largeFamily(&large44);
EpdFontFamily clockLabelFamily(&clockLabel22);
EpdFontFamily hugeFamily(&huge64);
EpdFontFamily uiFamily(&ui20);
EpdFontFamily carouselFamily(&carousel18);
EpdFontFamily tileFamily(&tile10);
EpdFontFamily buttonFamily(&button14);
EpdFont readingBold11(&reading_serif_bold_11);
EpdFont readingUiBold14(&reading_serif_bold_14);
EpdFontFamily readingSmallFamily(&reading11, &readingBold11);
EpdFontFamily readingFamily(&reading14, &readingUiBold14);
EpdFontFamily readingBoldSmallFamily(&readingBold12);
EpdFontFamily readingBoldFamily(&readingBold16);
EpdFontFamily serifTileFamily(&instrument13);
EpdFontFamily serifSmallFamily(&instrument10);
EpdFontFamily serifTitleFamily(&instrument24);

EpdFont jersey10(&toybox_10), jersey14(&toybox_14), jersey20(&toybox_20), jersey30(&toybox_30);
EpdFontFamily jerseyTileFamily(&jersey10), jerseyButtonFamily(&jersey14), jerseyUiFamily(&jersey20),
    jerseyDisplayFamily(&jersey30);
EpdFont sans10(&book_text_10), sansBold10(&book_bold_10);
EpdFont sans14(&ui_prose_14), sansBold14(&ui_prose_bold_14);
EpdFontFamily sansSmallFamily(&sans10, &sansBold10), sansFamily(&sans14, &sansBold14);

inline const EpdFontFamily* menuFamily(menufont::Style style, int id) {
  const bool tile = id == kTileFontId || id == SMALL_FONT_ID;
  const bool small = tile || id == kButtonFontId || id == kLibraryFontId || id == kBookListFontId || id == UI_10_FONT_ID;
  const bool title = id == kDisplayFontId;
  if (!small && !title && id != kUiFontId && id != kProseFontId && id != kCarouselTitleFontId && id != UI_12_FONT_ID)
    return nullptr;  // Dedicated content fonts keep their own metrics.
  if (style == menufont::Style::Pixel)
    return tile ? &jerseyTileFamily : small ? &jerseyButtonFamily : title ? &jerseyDisplayFamily : &jerseyUiFamily;
  if (style == menufont::Style::Classic)
    return tile ? &smallFontFamily : small ? &ui10FontFamily : title ? &ui12FontFamily : &ui12FontFamily;
  if (style == menufont::Style::NotoSans)
    return tile ? &sansSmallFamily : small ? &sansSmallFamily : title ? &sansFamily : &sansFamily;
  if (style == menufont::Style::NotoSerif) {
    return tile ? &readingSmallFamily : small ? &readingSmallFamily : title ? &readingFamily : &readingFamily;
  }
  if (tile) return &tileFamily;
  if (title) return &displayFamily;
  if (small) return &buttonFamily;
  return &uiFamily;
}

}  // namespace
}  // namespace toybox::fontdetail
