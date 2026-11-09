#include <cassert>
#include <string>

#include "../../src/apps_local/quotes/HighlightSleepIndex.h"

int main() {
  const std::string record =
      "Sách (Tác giả)\r\n- Your Highlight on Page 1\r\n\r\nNội dung tiếng Việt\r\n==========\r\n";
  highlightsleep::Index index;
  std::string file;
  for (int n = 0; n < quotes::kMax + 17; ++n) {
    file += record;
    for (uint8_t c : record) index.consume(c);
  }
  index.finish();
  assert(index.count == quotes::kMax);
  // Retain the latest records and exactly the same keys as the Quotes library.
  for (int n = 0; n < index.count; ++n) {
    const auto& e = index.entries[n];
    assert(e.start >= 17 * record.size());
    assert(e.end - e.start == record.size());
    uint64_t key = quotes::kHashStart;
    for (size_t k = e.start; k < e.end; ++k) key = quotes::hashByte(key, uint8_t(file[k]));
    for (int k = 0; k < 4; ++k) key = quotes::hashByte(key, uint8_t(e.start >> (k * 8)));
    assert(e.key == key);
  }
  highlightsleep::Index tail;
  for (uint8_t c : std::string("Book\n- Your Highlight\n\nText\n==========")) tail.consume(c);
  tail.finish();
  assert(tail.count == 1);
  const auto end = tail.entries[0].end;
  tail.finish();
  assert(tail.count == 1 && tail.entries[0].end == end);
  highlightsleep::Index incomplete;
  for (uint8_t c : std::string("Book\n- Your Highlight\n\n========== extra\nPartial")) incomplete.consume(c);
  incomplete.finish();
  assert(incomplete.count == 0);
}
