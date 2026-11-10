#pragma once
class MappedInputManager {
 public:
  enum class Button { NavNext, NavPrevious, Confirm };
  enum class SwipeDir { None, Left, Right, Up, Down };
  Button released = Button::NavNext;
  bool key = false, touch = false, pressed = false;
  int touchX = 15, touchY = 25;
  mutable int suppressed = 0;
  bool wasReleased(Button b) const { return key && b == released; }
  bool wasPressed(Button b) const { return pressed && b == released; }
  bool isPressed(Button) const { return false; }
  unsigned getHeldTime() const { return 0; }
  bool wasLongPressed(Button, unsigned) const { return false; }
  SwipeDir wasSwipe() const { return SwipeDir::None; }
  void suppressNextRelease(Button) const { ++suppressed; }
  bool wasScreenTapped(int& x, int& y) const { x = touchX; y = touchY; return touch; }
};
