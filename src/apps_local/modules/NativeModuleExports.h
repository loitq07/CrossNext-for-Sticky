#pragma once

#include <cstddef>
#include <cstdint>

namespace native_module {

// One firmware symbol a native module may import, keyed by two FNV-1a hashes
// of its linker name (seeds 0x811C9DC5 and 0x2545F491). Sorted by (a, b).
struct ExportEntry {
  uint32_t a;
  uint32_t b;
  const void* addr;  // null when this firmware does not define the symbol
};

extern const ExportEntry kExports[];
extern const size_t kExportCount;

}  // namespace native_module
