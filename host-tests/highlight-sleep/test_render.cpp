#include <CrossPointSettings.h>
#include <GfxRenderer.h>
#include <HalStorage.h>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <string>

#include "../../src/apps_local/quotes/HighlightSleep.h"
#include "../../src/apps_local/quotes/HighlightSleepIndex.h"
#include "../../src/apps_local/quotes/QuotesStore.h"

std::string record(const char* title, const char* body, const char* type = "Highlight") {
  return std::string(title) + " (Author)\n- Your " + type + " on Page 1\n\n" + body + "\n==========\n";
}
void put(const std::string& file) { testStorage.files["/My Clippings.txt"] = {file.begin(), file.end()}; }
bool contains(const GfxRenderer& renderer, const char* value) {
  return std::find(renderer.text.begin(), renderer.text.end(), value) != renderer.text.end();
}
int main() {
  GfxRenderer renderer;
  highlightsleep::render(renderer);
  assert(contains(renderer, "No highlights") && !highlightsleep::timerMicros());
  const std::string file = record("Sách một", "Nội dung tiếng Việt") + record("Book two", "Second passage");
  put(file);
  highlightsleep::render(renderer);
  assert(contains(renderer, "Sách một") && contains(renderer, "Nội dung tiếng Việt"));
  assert(highlightsleep::timerMicros() == 300000000ULL);
  // Model a timer wake: RTC state remains and the previous quote is excluded.
  highlightsleep::disarm();
  highlightsleep::render(renderer);
  assert(contains(renderer, "Book two"));
  SETTINGS.sleepHighlightMinutes = 240;
  assert(highlightsleep::timerMicros() == 14400000000ULL);
  SETTINGS.sleepHighlightMinutes = 0;
  assert(!highlightsleep::timerMicros());
  SETTINGS.sleepHighlightMinutes = 5;
  SETTINGS.sleepScreen = CrossPointSettings::DARK;
  assert(!highlightsleep::timerMicros());
  SETTINGS.sleepScreen = CrossPointSettings::HIGHLIGHTS;
  const auto original = testStorage.files["/My Clippings.txt"];
  assert(testStorage.writes == 0);
  assert(original == std::vector<uint8_t>(file.begin(), file.end()));

  highlightsleep::Index index;
  for (uint8_t c : file) index.consume(c);
  index.finish();
  index.entries[0].flags = quotes::kHidden;
  assert(quotes::saveFlags(index.entries, index.count));
  testStorage.writes = 0;
  highlightsleep::render(renderer);
  assert(contains(renderer, "Book two") && !contains(renderer, "Sách một"));
  assert(!highlightsleep::timerMicros());  // Only one visible quote: no pointless wakeups.
  index.entries[1].flags = quotes::kHidden;
  assert(quotes::saveFlags(index.entries, index.count));
  highlightsleep::render(renderer);
  assert(contains(renderer, "No highlights") && !highlightsleep::timerMicros());

  testStorage.files.clear();
  put(record("Empty", "") + record("Blank", "   ") + record("Bookmark", "Ignored", "Bookmark") + "Incomplete\nText");
  highlightsleep::render(renderer);
  assert(contains(renderer, "No highlights") && !highlightsleep::timerMicros());
  put(record("Paragraphs", "First paragraph\r\n\r\nSecond paragraph"));
  highlightsleep::render(renderer);
  assert(contains(renderer, "First paragraph Second paragraph"));
  put(record("Long", (std::string(6000, 'x') + " tiếng Việt").c_str()));
  highlightsleep::render(renderer);
  assert(contains(renderer, "Long") && !highlightsleep::timerMicros());
  testStorage.files["/.crosspoint/quotes.bin"] = {'x'};
  highlightsleep::render(renderer);
  assert(contains(renderer, "No highlights") && !highlightsleep::timerMicros());
  testStorage.files.erase("/.crosspoint/quotes.bin");
  testStorage.refuseReads = true;
  highlightsleep::render(renderer);
  assert(contains(renderer, "No highlights") && !highlightsleep::timerMicros());
  std::puts(
      "Highlight sleep: rendering, rotation, timer bounds, hidden/empty/malformed records and read-only storage "
      "passed.");
}
