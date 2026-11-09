#pragma once

#include "ClippingsLine.h"
#include "QuotesCore.h"

namespace highlightsleep {
// Same bounded collection and record keys as Quotes, including CRLF and a final
// separator without a newline. The file is streamed; no book-sized text buffer.
class Index {
 public:
  quotes::Entry entries[quotes::kMax]{};
  int count = 0;

  void consume(uint8_t c) {
    ++offset;
    key = quotes::hashByte(key, c);
    if (line.consume(char(c))) finishRecord();
  }
  void finish() {
    if (line.consume('\n')) finishRecord();
  }

 private:
  quotes::ClippingsLine line;
  uint32_t offset = 0, start = 0;
  uint64_t key = quotes::kHashStart;
  int next = 0;
  void finishRecord() {
    auto& entry = entries[next];
    entry = {};
    entry.start = start;
    entry.end = offset;
    entry.key = key;
    for (int n = 0; n < 4; ++n) entry.key = quotes::hashByte(entry.key, uint8_t(start >> (n * 8)));
    next = (next + 1) % quotes::kMax;
    if (count < quotes::kMax) ++count;
    start = offset;
    key = quotes::kHashStart;
  }
};
}  // namespace highlightsleep
