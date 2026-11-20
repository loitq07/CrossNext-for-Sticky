#pragma once

#include <cstdint>

// An on-screen keyboard. It covers all printable ASCII, shows what's typed, and only
// offers its action key once the text is long enough. By default it's set up for a
// Wi-Fi password, and knows nothing about Wi-Fi beyond those length limits.
namespace keyboard {

constexpr int MIN_LENGTH = 8;   // WPA's shortest password
constexpr int MAX_LENGTH = 64;  // WPA's longest password, and the most it can hold

// Join is the action key, whatever its label.
enum class Event { None, Changed, Back, Join };

struct Options {
  int minLength = MIN_LENGTH;  // the action key works from this many characters
  int maxLength = MAX_LENGTH;  // up to MAX_LENGTH
  const char* action = "JOIN";
  const char* hint = "PASSWORD: AT LEAST 8 CHARACTERS";  // shown while too short, or null
  const char* initial = "";                              // the text to start with
};

// The title names what's being typed. Everything is copied.
void begin(const char* title, const Options& options = Options());
const char* text();

Event onTap(int x, int y);
void draw(uint8_t* frame);

}  // namespace keyboard
