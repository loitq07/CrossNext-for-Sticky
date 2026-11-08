#pragma once

// Persistence is replaced at the IO boundary; tests compile the real recent-book store.
template <typename Derived>
class PersistableStore {
 public:
  bool resaveRequested = false;
  int saves = 0;
  static Derived& getLoadedInstance() {
    static Derived value;
    return value;
  }
  bool saveToFile() { ++saves; return true; }

 protected:
  void requestResave() { resaveRequested = true; }
};
