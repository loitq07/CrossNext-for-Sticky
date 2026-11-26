#pragma once

struct StubTiltSensor {
  bool available = true;
  bool isAvailable() const { return available; }
};

inline StubTiltSensor halTiltSensor{};
