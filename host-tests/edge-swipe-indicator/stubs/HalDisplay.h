#pragma once
class HalDisplay {
 public:
  bool fastWindow = true;
  bool inverted = false;
  bool supportsFastWindow() const { return fastWindow; }
  bool isInverted() const { return inverted; }
};
