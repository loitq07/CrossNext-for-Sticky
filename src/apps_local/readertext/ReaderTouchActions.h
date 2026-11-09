#pragma once

namespace readertext {

// Logical screen coordinates follow the reader's current orientation.
inline bool isBookmarkCorner(const int x, const int y, const int width, const int height) {
  const int size = 64;
  return width > 0 && height > 0 && x >= 0 && x >= width - size && x < width && y >= 0 && y < size && y < height;
}

}  // namespace readertext
