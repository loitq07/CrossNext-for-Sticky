#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// The question file the player copies onto the card, turned into questions. No
// screen or card, so it can be tested off the device.
namespace thinkalike {

constexpr size_t MAX_FILE_BYTES = 64 * 1024;
constexpr int MAX_QUESTIONS = 1000;
constexpr size_t MAX_QUESTION_CHARS = 200;  // longer won't fit the card at the smallest cut

struct Questions {
  std::vector<std::string> list;
  int tooLong = 0;    // lines over MAX_QUESTION_CHARS, left out
  int overLimit = 0;  // questions past MAX_QUESTIONS, left out
};

// One question per line. Blank lines and lines starting with '#' are skipped, spaces
// round a line are trimmed, and "\r\n" works as well as "\n". Letters come out as
// capitals. The curly quotes, dashes and ellipsis a word processor puts in are
// turned into ASCII, and any other byte outside ASCII is dropped, since the font
// would draw it as a box.
Questions parse(const uint8_t* data, size_t size);

// FNV-1a over the whole file, so a save can tell the file has been edited.
uint32_t checksum(const uint8_t* data, size_t size);

}  // namespace thinkalike
