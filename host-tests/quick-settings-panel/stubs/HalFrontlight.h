#pragma once

struct StubFrontlight {
  bool presentFlag = false;
  bool tempFlag = false;
  bool present() const { return presentFlag; }
  bool hasColorTemperature() const { return tempFlag; }
};

inline StubFrontlight Frontlight{};
