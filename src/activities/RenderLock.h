#pragma once

class Activity;  // forward declaration

// RAII helper to lock rendering mutex for the duration of a scope.
class RenderLock {
  bool isLocked = false;

 public:
  enum class Mode { Blocking, Try };
  explicit RenderLock(Mode mode = Mode::Blocking);
  explicit RenderLock(Activity&);  // Activity argument retained for compatibility.
  RenderLock(const RenderLock&) = delete;
  RenderLock& operator=(const RenderLock&) = delete;
  ~RenderLock();
  bool ownsLock() const { return isLocked; }
  void unlock();
  static bool peek();
  // True when the calling task already owns the rendering mutex (upstream 2.0.1).
  static bool heldByCurrentTask();
};
