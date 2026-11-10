#include "ToyboxFonts.h"

#include <EpdFont.h>
#include <EpdFontFamily.h>
#include <Logging.h>

#include "../../CrossPointSettings.h"
#include "../../activities/Activity.h"
#include "../../fontIds.h"
#include "../../util/MenuFontStyle.h"
#include "ToyboxFontFamilies.h"
#include "ToyboxTokens.h"
namespace toybox {
using namespace fontdetail;
namespace {

bool registered = false;
menufont::Style registeredStyle = menufont::kDefault;

bool bindMenuFonts(GfxRenderer& renderer, menufont::Style style) {
  static constexpr int ids[] = {kLibraryFontId, kBookListFontId, kCarouselTitleFontId, kDisplayFontId,
                                kUiFontId,      kTileFontId,     kButtonFontId,        kProseFontId};
  bool ok = true;
  for (int id : ids) {
    if (const auto* family = menuFamily(style, id)) {
      ok = renderer.replaceBuiltinFont(id, *family) && ok;
    }
  }
  if (const auto* ui = menuFamily(style, kUiFontId)) {
    ok = renderer.replaceBuiltinFont(UI_12_FONT_ID, *ui) && ok;
  }
  if (const auto* button = menuFamily(style, kButtonFontId)) {
    ok = renderer.replaceBuiltinFont(UI_10_FONT_ID, *button) && ok;
  }
  if (const auto* tile = menuFamily(style, kTileFontId)) {
    ok = renderer.replaceBuiltinFont(SMALL_FONT_ID, *tile) && ok;
  }
  return ok;
}

}  // namespace

FontMetrics metricsFor(const int fontId) {
  if (const auto* family = menuFamily(registeredStyle, fontId)) {
    const auto* data = family->getData();
    FontMetrics metrics;
    metrics.ascender = data->ascender;
    if (const auto* glyph = family->getGlyph('H')) {
      metrics.capTop = glyph->top;
      metrics.capHeight = glyph->height;
    }
    return metrics;
  }
  const EpdFontFamily& family = fontId == kHugeFontId               ? hugeFamily
                                : fontId == kClockLabelFontId       ? clockLabelFamily
                                : fontId == kLargeFontId            ? largeFamily
                                : fontId == kDisplayFontId          ? displayFamily
                                : fontId == kTileFontId             ? tileFamily
                                : fontId == kButtonFontId           ? buttonFamily
                                : fontId == kReadingBoldFontId      ? readingBoldFamily
                                : fontId == kReadingFontId          ? readingFamily
                                : fontId == kReadingSmallFontId     ? readingSmallFamily
                                : fontId == kReadingBoldSmallFontId ? readingBoldSmallFamily
                                                                    : uiFamily;
  const EpdFontData* data = fontId == kHugeFontId               ? &display_sans_56
                            : fontId == kClockLabelFontId       ? &display_sans_22
                            : fontId == kLargeFontId            ? &display_sans_40
                            : fontId == kDisplayFontId          ? &baemin_18
                            : fontId == kTileFontId             ? &baemin_10
                            : fontId == kButtonFontId           ? &baemin_12
                            : fontId == kReadingBoldFontId      ? &reading_serif_bold_16
                            : fontId == kReadingFontId          ? &reading_serif_14
                            : fontId == kReadingSmallFontId     ? &reading_serif_11
                            : fontId == kReadingBoldSmallFontId ? &reading_serif_bold_12
                                                                : &baemin_14;
  FontMetrics metrics;
  metrics.ascender = data->ascender;
  // 'H' stands in for cap height: a flat-topped capital with no overshoot, so it
  // measures the band the eye actually aligns to.
  if (const EpdGlyph* glyph = family.getGlyph('H'); glyph != nullptr) {
    metrics.capTop = glyph->top;
    metrics.capHeight = glyph->height;
  }
  return metrics;
}

int drawCapsCentered(const GfxRenderer& renderer, const int fontId, const int x, const int boxY, const int boxH,
                     const char* text, const bool black) {
  const FontMetrics metrics = metricsFor(fontId);
  // Ink top on screen is (y + ascender) - capTop. Solve for the y that puts ink
  // top at the box's centred position.
  const int y = boxY + (boxH - metrics.capHeight) / 2 - metrics.ascender + metrics.capTop;
  renderer.drawText(fontId, x, y, text, black);
  return x;
}

// The cuts, paired with the constants ToyboxTokens.h carries for them.
// Screens are freestanding and cannot see a font, so those constants are the
// only thing standing between a regenerated cut and every centred label in the
// fork drifting at once. Checked here, where the real data is in scope.
void verifyCutMetrics() {
  struct Pair {
    const char* name;
    const EpdFontData* data;
    const EpdFontFamily* family;
    const toybox::CutMetrics* cut;
  };
  const Pair pairs[] = {
      {"baemin_10", &baemin_10, &tileFamily, &toybox::kTileCut},
      {"baemin_12", &baemin_12, &buttonFamily, &toybox::kButtonCut},
      {"baemin_14", &baemin_14, &uiFamily, &toybox::kUiCut},
      {"baemin_18", &baemin_18, &displayFamily, &toybox::kDisplayCut},
      {"display_sans_40", &display_sans_40, &largeFamily, &toybox::kLargeCut},
      {"display_sans_56", &display_sans_56, &hugeFamily, &toybox::kHugeCut},
      {"instrument_10", &instrument_10, &serifSmallFamily, &toybox::kSerifSmallCut},
      {"instrument_13", &instrument_13, &serifTileFamily, &toybox::kSerifTileCut},
      {"instrument_24", &instrument_24, &serifTitleFamily, &toybox::kSerifTitleCut},
      {"reading_serif_11", &reading_serif_11, &readingSmallFamily, &toybox::kReadingSmallCut},
      {"reading_serif_14", &reading_serif_14, &readingFamily, &toybox::kReadingCut},
      {"reading_serif_bold_12", &reading_serif_bold_12, &readingBoldSmallFamily, &toybox::kReadingBoldSmallCut},
      {"reading_serif_bold_16", &reading_serif_bold_16, &readingBoldFamily, &toybox::kReadingBoldCut},
  };
  for (const Pair& pair : pairs) {
    const EpdGlyph* glyph = pair.family->getGlyph('H');
    const int ink = glyph != nullptr ? glyph->height : 0;
    if (pair.data->advanceY == pair.cut->lineHeight && pair.data->ascender == pair.cut->ascender &&
        ink == pair.cut->inkHeight) {
      continue;
    }
    LOG_ERR("TOYBOX", "%s moved: advanceY %d/%d ascender %d/%d ink %d/%d -- update CutMetrics in ToyboxTokens.h",
            pair.name, pair.data->advanceY, pair.cut->lineHeight, pair.data->ascender, pair.cut->ascender, ink,
            pair.cut->inkHeight);
  }
}

menufont::Style interfaceFontStyle() {
  const auto section = activityManager.currentInterfaceFontSection();
  const auto item = activityManager.currentInterfaceFontItem();
  return menufont::resolve(SETTINGS.menuFontStyle,
                           section == menufont::Section::General ? 0 : SETTINGS.menuFontOverrides[int(section)],
                           item < menufont::kItemCount ? SETTINGS.menuFontItemOverrides[item] : 0);
}

void syncInterfaceFonts(GfxRenderer& renderer) {
  if (!registered) {
    ensureFonts(renderer);
    return;
  }
  const auto style = interfaceFontStyle();
  if (registeredStyle != style && bindMenuFonts(renderer, style)) registeredStyle = style;
}

constexpr int kSystemFallbackFontId = 0x70B0'0099;

void ensureFonts(GfxRenderer& renderer) {
  // Font settings can clear this registry while built-in faces remain loaded.
  // Shared map entries are reused; no image or per-label allocation.
  renderer.insertFont(kSystemFallbackFontId, ui12FontFamily);
  renderer.setFallbackFont(kButtonFontId, kSystemFallbackFontId);
  renderer.setFallbackFont(kUiFontId, kSystemFallbackFontId);
  renderer.setFallbackFont(kDisplayFontId, kSystemFallbackFontId);
  renderer.setFallbackFont(kTileFontId, kSystemFallbackFontId);
  renderer.setFallbackFont(kLibraryFontId, kSystemFallbackFontId);
  renderer.setFallbackFont(kBookListFontId, kSystemFallbackFontId);
  renderer.setFallbackFont(kProseFontId, kSystemFallbackFontId);
  renderer.setFallbackFont(kCarouselTitleFontId, kSystemFallbackFontId);
  renderer.setFallbackFont(UI_12_FONT_ID, kSystemFallbackFontId);
  renderer.setFallbackFont(UI_10_FONT_ID, kSystemFallbackFontId);
  renderer.setFallbackFont(SMALL_FONT_ID, kSystemFallbackFontId);
  const auto style = interfaceFontStyle();
  if (registered) {
    syncInterfaceFonts(renderer);
    return;
  }
  verifyCutMetrics();
  // Registered from here rather than main.cpp so the design language costs zero
  // upstream surface. insertFont is idempotent per id, but the guard keeps this
  // free to call from every activity's onEnter().
  renderer.insertFont(kProseFontId, proseFamily);
  renderer.insertFont(kLibraryFontId, libraryFamily);
  renderer.insertFont(kBookListFontId, bookListFamily);
  renderer.insertFont(kCarouselTitleFontId, carouselFamily);
  renderer.insertFont(kDisplayFontId, displayFamily);
  renderer.insertFont(kLargeFontId, largeFamily);
  renderer.insertFont(kClockLabelFontId, clockLabelFamily);
  renderer.insertFont(kHugeFontId, hugeFamily);
  renderer.insertFont(kUiFontId, uiFamily);
  renderer.insertFont(kTileFontId, tileFamily);
  renderer.insertFont(kButtonFontId, buttonFamily);
  renderer.insertFont(kReadingFontId, readingFamily);
  renderer.insertFont(kReadingSmallFontId, readingSmallFamily);
  renderer.insertFont(kReadingBoldSmallFontId, readingBoldSmallFamily);
  renderer.insertFont(kReadingBoldFontId, readingBoldFamily);
  renderer.insertFont(kSerifTileFontId, serifTileFamily);
  renderer.insertFont(kSerifSmallFontId, serifSmallFamily);
  renderer.insertFont(kSerifTitleFontId, serifTitleFamily);
  if (bindMenuFonts(renderer, style)) registeredStyle = style;
  registered = true;
}

}  // namespace toybox
