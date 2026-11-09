#pragma once
#include <string>
#include <vector>
namespace EpdFontFamily {
enum Style { REGULAR, BOLD };
}
namespace HalDisplay {
enum { HALF_REFRESH };
}
class GfxRenderer {
 public:
  std::vector<std::string> text;
  int paints = 0;
  int getScreenWidth() { return 480; }
  int getScreenHeight() { return 800; }
  int getLineHeight(int) { return 24; }
  void clearScreen() { text.clear(); }
  std::vector<std::string> wrappedText(int, const char* s, int, int, EpdFontFamily::Style = EpdFontFamily::REGULAR) {
    return {s};
  }
  std::string truncatedText(int, const char* s, int) { return s; }
  void drawText(int, int, int, const char* s) { text.emplace_back(s); }
  void drawCenteredText(int, int, const char* s, bool = true, EpdFontFamily::Style = EpdFontFamily::REGULAR) {
    text.emplace_back(s);
  }
  void displayBuffer(int) { ++paints; }
};
