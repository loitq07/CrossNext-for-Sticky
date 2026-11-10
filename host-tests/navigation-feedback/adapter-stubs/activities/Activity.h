#pragma once
class Activity {
 public:
  int repaints = 0;
  void requestUpdate() { ++repaints; }
};
