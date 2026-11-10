#include <cstdio>
#include <initializer_list>
#include <map>

#include "../../lib/GfxRenderer/FontFallback.h"

#include "../../lib/EpdFont/builtinFonts/notosans_8_regular.h"
#include "../../lib/EpdFont/builtinFonts/ubuntu_10_regular.h"
#include "../../lib/EpdFont/builtinFonts/ubuntu_10_bold.h"
#include "../../lib/EpdFont/builtinFonts/ubuntu_12_regular.h"
#include "../../lib/EpdFont/builtinFonts/ubuntu_12_bold.h"
#include "../../src/apps_local/ui/ToyboxFontFamilies.h"

EpdFont small(&notosans_8_regular), ubuntu10(&ubuntu_10_regular), ubuntu10Bold(&ubuntu_10_bold),
    ubuntu12(&ubuntu_12_regular), ubuntu12Bold(&ubuntu_12_bold);
EpdFontFamily smallFontFamily(&small), ui10FontFamily(&ubuntu10, &ubuntu10Bold),
    ui12FontFamily(&ubuntu12, &ubuntu12Bold);

int main() {
  int checks = 0, failures = 0;
  const auto check = [&](bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::printf("FAIL: %s\n", label); }
  };
  using namespace toybox;
  using fontdetail::menuFamily;
  using menufont::Style;
  const EpdFontData* body[] = {&toybox_20, &ubuntu_12_regular, &ui_prose_14, &reading_serif_14, &baemin_14};
  const EpdFontData* button[] = {&toybox_14, &ubuntu_10_regular, &book_text_10, &reading_serif_11, &baemin_12};
  const EpdFontData* title[] = {&toybox_30, &ubuntu_12_regular, &ui_prose_14, &reading_serif_14, &baemin_18};
  for (int style = 0; style < int(Style::Count); ++style) {
    const auto face = Style(style);
    check(menuFamily(face, kUiFontId)->getData() == body[style], "body uses selected font");
    check(menuFamily(face, kButtonFontId)->getData() == button[style], "button uses selected font");
    check(menuFamily(face, kDisplayFontId)->getData() == title[style], "title uses selected font");
    for (int id : {UI_12_FONT_ID, UI_10_FONT_ID, SMALL_FONT_ID})
      check(menuFamily(face, id) == nullptr, "canonical Unicode fallback faces are never rebound");
    for (int id : {kLibraryFontId, kBookListFontId, kCarouselTitleFontId, kProseFontId, kTileFontId})
      check(menuFamily(face, id) != nullptr, "all interface aliases bind");
    for (int id : {kReadingFontId, kHugeFontId, kSerifTileFontId})
      check(menuFamily(face, id) == nullptr, "content font metrics stay independent");
    int w = 0, h = 0;
    menuFamily(face, kUiFontId)->getTextDimensions("Power / Games", &w, &h);
    check(w > 0 && h > 0, "selected font has real renderable glyph metrics");
    for (int section = 0; section < int(menufont::Override::Count); ++section)
      for (int item = 0; item < int(menufont::Override::Count); ++item)
        check(menufont::resolve(style, section, item) ==
                  (item ? Style(item - 1) : section ? Style(section - 1) : face),
              "general, section and content font inheritance");
  }
  check(menufont::kDefault == Style::Daniel, "Daniel is the new-install default");
  check(menufont::normalized(0) == Style::Pixel, "saved Jersey id stays stable");
  check(menufont::normalized(1) == Style::Classic, "saved Ubuntu id stays stable");
  check(menufont::normalized(2) == Style::NotoSans, "saved Sans id stays stable");
  check(menufont::normalized(3) == Style::NotoSerif, "saved Serif id stays stable");
  check(menufont::normalized(255) == Style::Daniel, "invalid id uses default");
  for (auto style : {Style::Pixel, Style::Daniel}) {
    const std::map<int, EpdFontFamily> fonts = {
        {kUiFontId, *menuFamily(style, kUiFontId)}, {UI_12_FONT_ID, ui12FontFamily}};
    const std::map<int, int> fallbacks = {{kUiFontId, UI_12_FONT_ID}};
    const int resolved = fontfallback::resolve(fonts, fallbacks, kUiFontId, "Cài đặt", EpdFontFamily::REGULAR);
    check(resolved == (style == Style::Pixel ? UI_12_FONT_ID : kUiFontId),
          "Vietnamese falls back from Jersey while Daniel retains its own accents");
  }
  std::printf("menu-fonts: %d checks, %d failed\n", checks, failures);
  return failures ? 1 : 0;
}
