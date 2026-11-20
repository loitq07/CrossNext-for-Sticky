#include "islandmaps_save.h"

#include "islandmaps_maps.h"

namespace islandmaps {

size_t pack(int index, uint8_t* out) {
  out[0] = static_cast<uint8_t>(index);
  return SAVE_BYTES;
}

bool unpack(uint8_t version, const uint8_t* in, size_t size, int& index) {
  if (version != SAVE_VERSION || size != SAVE_BYTES || in[0] >= kIslandCount) return false;
  index = in[0];
  return true;
}

}  // namespace islandmaps
