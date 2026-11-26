#pragma once
#include <cstdint>
class MappedInputManager {
 public:
  enum class EdgeSwipe : uint8_t { None, Left, Right, Top, Bottom };
  struct EdgeSwipeContext {
    bool allowBack = false;
    bool allowHome = false;
    bool allowReaderSettings = false;
    bool allowLightPanel = false;
  };
};
