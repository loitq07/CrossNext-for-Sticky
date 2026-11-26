#pragma once

#define FREEINK_CAP_BUZZER 1

struct BoardConfig {
  static inline bool sticky = true;
  static inline bool homeKey = false;
  static inline bool buzzer = true;
  static bool isSticky() { return sticky; }
  static bool hasHomeKey() { return homeKey; }
  static bool hasBuzzer() { return buzzer; }
};
