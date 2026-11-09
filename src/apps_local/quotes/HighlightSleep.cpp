#include "HighlightSleep.h"

#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>
#include <Memory.h>
#if !defined(SIMULATOR)
#include <esp_attr.h>
#endif
#include <esp_random.h>

#include <algorithm>
#include <cctype>

#include "CrossPointSettings.h"
#include "HighlightSleepIndex.h"
#include "QuotesStore.h"
#include "clippings/ClippingsManager.h"
#include "fontIds.h"

namespace highlightsleep {
namespace {
// RTC memory survives timer wakes without an SD write on every rotation.
#if defined(SIMULATOR)
bool armed = false;
uint64_t previousKey = 0;
#else
RTC_DATA_ATTR bool armed = false;
RTC_DATA_ATTR uint64_t previousKey = 0;
#endif
constexpr size_t kRecordBytes = 4096;
constexpr uint8_t kInvalid = 0x80;

struct Workspace {
  Index index;
  char record[kRecordBytes];
  uint8_t scan[512];
  quotes::Quote quote;
};

bool readQuote(HalFile& source, const quotes::Entry& entry, Workspace& work) {
  if (!source.seekSet(entry.start)) return false;
  const size_t want = std::min(size_t(entry.end - entry.start), sizeof(work.record) - 1);
  if (source.read(work.record, want) != int(want)) return false;
  work.record[want] = 0;
  // Keep a truncated record valid UTF-8, even for unusually long imported highlights.
  size_t length = want;
  if (entry.end - entry.start > want) {
    while (length && (uint8_t(work.record[length - 1]) & 0xc0) == 0x80) --length;
    if (length && uint8_t(work.record[length - 1]) >= 0xc0) --length;
    work.record[length] = 0;
  }
  // Ignore incomplete records and bookmarks/notes in imported Kindle exports.
  const char* meta = std::strchr(work.record, '\n');
  const char* body = meta ? std::strchr(meta + 1, '\n') : nullptr;
  if (!body) return false;
  const char* highlight = std::strstr(meta, "Highlight");
  if (!highlight || highlight >= body) return false;
  if (char* separator = std::strstr(work.record, "\n==========")) *separator = 0;
  quotes::parse(work.record, work.quote);
  // A sleep quote is one flowing passage. Fold CRLF/paragraph whitespace so
  // wrappedText cannot truncate an entire paragraph at a hard line break.
  char* out = work.quote.body;
  bool space = false;
  for (const char* text = work.quote.body; *text; ++text) {
    if (std::isspace(static_cast<unsigned char>(*text))) {
      space = out != work.quote.body;
    } else {
      if (space) *out++ = ' ';
      *out++ = *text;
      space = false;
    }
  }
  *out = 0;
  return work.quote.title[0] && work.quote.body[0];
}
}  // namespace

void disarm() { armed = false; }

uint64_t timerMicros() {
  return armed && SETTINGS.sleepScreen == CrossPointSettings::HIGHLIGHTS
             ? uint64_t(SETTINGS.sleepHighlightMinutes) * 60ULL * 1000000ULL
             : 0;
}

static bool draw(GfxRenderer& renderer) {
  armed = false;
  auto work = makeUniqueNoThrow<Workspace>();
  HalFile source;
  if (!work || !Storage.openFileForRead("SLP", ClippingsManager::CLIPPINGS_PATH, source) ||
      source.fileSize64() > UINT32_MAX)
    return false;

  while (source.available()) {
    const int bytes = source.read(work->scan, sizeof(work->scan));
    if (bytes <= 0) return false;
    for (int n = 0; n < bytes; ++n) work->index.consume(work->scan[n]);
  }
  work->index.finish();
  auto& index = work->index;
  if (quotes::loadFlags(index.entries, index.count) == quotes::Load::Error) return false;

  int valid = 0, alternatives = 0;
  for (int n = 0; n < index.count; ++n) {
    auto& entry = index.entries[n];
    if ((entry.flags & quotes::kHidden) || !readQuote(source, entry, *work)) {
      entry.flags |= kInvalid;
      continue;
    }
    ++valid;
    if (entry.key != previousKey) ++alternatives;
  }
  if (!valid) return false;
  uint32_t choice = esp_random() % uint32_t(alternatives ? alternatives : valid);
  for (int n = 0; n < index.count; ++n) {
    const auto& entry = index.entries[n];
    if ((entry.flags & kInvalid) || (alternatives && entry.key == previousKey)) continue;
    if (choice--) continue;
    if (!readQuote(source, entry, *work)) return false;

    const int width = renderer.getScreenWidth();
    const int height = renderer.getScreenHeight();
    const int margin = std::max(20, width / 12);
    const int bodyFont = NOTOSERIF_14_FONT_ID;
    const int lineHeight = renderer.getLineHeight(bodyFont) + 6;
    const int top = 90;
    const int footer = height - 130;
    auto lines = renderer.wrappedText(bodyFont, work->quote.body, width - margin * 2,
                                      std::max(1, (footer - top - 30) / lineHeight));
    renderer.clearScreen();
    renderer.drawCenteredText(SMALL_FONT_ID, 28, tr(STR_SLEEP_HIGHLIGHTS));
    int y = top + std::max(0, (footer - top - int(lines.size()) * lineHeight) / 2);
    for (const auto& line : lines) {
      renderer.drawText(bodyFont, margin, y, line.c_str());
      y += lineHeight;
    }
    const auto title =
        renderer.wrappedText(UI_10_FONT_ID, work->quote.title, width - margin * 2, 2, EpdFontFamily::BOLD);
    y = footer;
    for (const auto& line : title) {
      renderer.drawCenteredText(UI_10_FONT_ID, y, line.c_str(), true, EpdFontFamily::BOLD);
      y += renderer.getLineHeight(UI_10_FONT_ID) + 3;
    }
    const auto author = renderer.truncatedText(SMALL_FONT_ID, work->quote.author, width - margin * 2);
    renderer.drawCenteredText(SMALL_FONT_ID, y + 8, author.c_str());
    previousKey = entry.key;
    armed = valid > 1;
    return true;
  }
  return false;
}

void render(GfxRenderer& renderer) {
  if (!draw(renderer)) {
    renderer.clearScreen();
    const int width = renderer.getScreenWidth();
    const int height = renderer.getScreenHeight();
    renderer.drawCenteredText(UI_10_FONT_ID, height / 3, tr(STR_SLEEP_HIGHLIGHTS));
    const auto lines = renderer.wrappedText(UI_10_FONT_ID, tr(STR_SLEEP_HIGHLIGHTS_EMPTY), width - 60, 4);
    int y = height / 2;
    for (const auto& line : lines) {
      renderer.drawCenteredText(UI_10_FONT_ID, y, line.c_str());
      y += renderer.getLineHeight(UI_10_FONT_ID) + 4;
    }
  }
  renderer.displayBuffer(HalDisplay::HALF_REFRESH);
}
}  // namespace highlightsleep
