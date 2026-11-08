#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

#include "ContentVisibility.h"
#include "apps_local/home/HomeMenuCore.h"

static void write32(uint8_t* p, uint32_t n) {
  for (int i = 0; i < 4; ++i) p[i] = n >> (8 * i);
}

int main() {
  using namespace homemenu;
  assert(content::readLaterContent("/books/Wallabag"));
  assert(content::readLaterContent("/BOOKS/WALLABAG/article.epub"));
  assert(content::readLaterContent("books/wallabag/folder/article.epub"));
  for (const char* path : {"/books/Wallabag2/book.epub", "/books/Wallabag.epub", "/other/books/Wallabag/a.epub",
                           "/books/Novels/a.epub", "/Wallabag/a.epub", ""})
    assert(!content::readLaterContent(path));
  Config defaults;
  assert(visibleCount(defaults) == 8);
  assert(at(defaults, 2) == Id::ReadLater);
  assert(!isShortcut(Id::ReadLater));
  assert(!shortcut(defaults, Id::ReadLater));
  assert(!setVisible(defaults, Id::Services, true));

  // Construct real old layouts independently of the new encoder. In particular,
  // labels and the shortcut table had different offsets before ReadLater existed.
  for (char version : {'1', '2', '3', '4'}) {
    const size_t size = version == '1' ? kLegacySaveBytes : kPreviousSaveBytes;
    std::vector<uint8_t> bytes(size);
    std::memcpy(bytes.data(), "HMC", 3);
    bytes[3] = version;
    write32(bytes.data() + 4, 123);
    write32(bytes.data() + 8, version == '1' ? 0x3f : 0x13b); // Settings plus CHESS shortcut.
    std::strcpy(reinterpret_cast<char*>(bytes.data() + 12 + 3 * kLabelBytes), "My settings");
    if (version != '1') {
      std::strcpy(reinterpret_cast<char*>(bytes.data() + 12 + 8 * kLabelBytes), "My chess");
      auto* entries = bytes.data() + 12 + 13 * kLabelBytes;
      std::strcpy(reinterpret_cast<char*>(entries + 1), "CHESS");
      const uint8_t order[] = {0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 3};
      std::memcpy(entries + 5 * (kItemBytes + 1), order, sizeof(order));
    }
    write32(bytes.data() + size - 4, checksum(bytes.data(), size - 4));
    Config migrated;
    uint32_t generation = 0;
    assert(decode(bytes.data(), migrated, generation, size));
    assert(generation == 123);
    assert(indexOf(migrated, Id::ReadLater) == indexOf(migrated, Id::Recent) + 1);
    assert(!std::strcmp(migrated.labels[3], "My settings"));
    if (version != '1') {
      assert(!std::strcmp(shortcut(migrated, Id::Shortcut0)->title, "CHESS"));
      assert(!std::strcmp(migrated.labels[8], "My chess"));
      assert(indexOf(migrated, Id::Shortcut0) >= 0);
    }
    assert(visibleCount(migrated) <= kMaxVisible);
    uint8_t saved[kSaveBytes];
    encode(migrated, 124, saved);
    Config restored;
    assert(decode(saved, restored, generation));
    assert(restored.visible == migrated.visible && generation == 124);
    assert(!std::memcmp(restored.order, migrated.order, sizeof(restored.order)));
    assert(!std::memcmp(restored.shortcuts, migrated.shortcuts, sizeof(restored.shortcuts)));
    for (size_t i = 0; i < sizeof(saved); ++i) {
      saved[i] ^= 1;
      assert(!decode(saved, restored, generation));
      saved[i] ^= 1;
    }
  }
  assert(setVisible(defaults, Id::ReadLater, false));
  uint8_t saved[kSaveBytes];
  encode(defaults, 4, saved);
  Config restored;
  uint32_t generation;
  assert(decode(saved, restored, generation));
  assert(indexOf(restored, Id::ReadLater) == -1); // Respect an intentional hide after upgrade.
  std::puts("Read-later path and HMC1-HMC5 migration checks passed");
}
