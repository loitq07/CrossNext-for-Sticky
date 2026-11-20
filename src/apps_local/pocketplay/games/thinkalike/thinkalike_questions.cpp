#include "thinkalike_questions.h"

namespace thinkalike {

namespace {

// The UTF-8 punctuation that editors substitute for plain ASCII: E2 80 xx.
const char* punctuation(uint8_t third) {
  switch (third) {
    case 0x98:  // left single quote
    case 0x99:  // right single quote, the curly apostrophe
      return "'";
    case 0x9C:  // left double quote
    case 0x9D:  // right double quote
      return "\"";
    case 0x93:  // en dash
    case 0x94:  // em dash
      return "-";
    case 0xA6:  // ellipsis
      return "...";
    default:
      return "";
  }
}

void finishLine(std::string& line, Questions& out) {
  const size_t first = line.find_first_not_of(' ');
  if (first == std::string::npos) return;
  const size_t last = line.find_last_not_of(' ');
  std::string question = line.substr(first, last - first + 1);
  if (question[0] == '#') return;
  if (question.size() > MAX_QUESTION_CHARS) {
    out.tooLong++;
    return;
  }
  if (static_cast<int>(out.list.size()) >= MAX_QUESTIONS) {
    out.overLimit++;
    return;
  }
  out.list.push_back(question);
}

}  // namespace

Questions parse(const uint8_t* data, size_t size) {
  Questions out;
  std::string line;
  size_t i = 0;
  while (i < size) {
    const uint8_t c = data[i];
    if (c == '\n') {
      finishLine(line, out);
      line.clear();
      i++;
    } else if (c < 0x80) {
      if (c >= 'a' && c <= 'z') {
        line += static_cast<char>(c - 'a' + 'A');
      } else if (c == '\t') {
        line += ' ';
      } else if (c >= 0x20 && c < 0x7F) {
        line += static_cast<char>(c);
      }
      // '\r' and other control bytes are dropped.
      i++;
    } else if (c == 0xE2 && i + 2 < size && data[i + 1] == 0x80) {
      line += punctuation(data[i + 2]);
      i += 3;
    } else {
      // Skip the whole UTF-8 character: its lead byte and continuation bytes.
      i++;
      while (i < size && (data[i] & 0xC0) == 0x80) i++;
    }
  }
  finishLine(line, out);
  return out;
}

uint32_t checksum(const uint8_t* data, size_t size) {
  uint32_t hash = 2166136261u;
  for (size_t i = 0; i < size; i++) {
    hash ^= data[i];
    hash *= 16777619u;
  }
  return hash;
}

}  // namespace thinkalike
