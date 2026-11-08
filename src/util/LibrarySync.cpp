#include "LibrarySync.h"

#include <ContentVisibility.h>
#include <Epub.h>
#include <FsHelpers.h>
#include <LibraryBuilder.h>
#include <LibraryIndexFile.h>
#include <Logging.h>
#include <Memory.h>
#include <Xtc.h>

#include <algorithm>
#include <vector>

#include "../CrossPointSettings.h"
#include "../RecentBooksStore.h"
#include "../apps_local/library/LibraryPreferences.h"

namespace library_sync {

void syncIndexToRecents(int maxBooks) {
  library::LibraryIndexFile index;
  if (!index.open(library::libraryIndexPath())) {
    return;
  }

  library::ClixRecord record;
  const auto& existing = RECENT_BOOKS.getBooks();
  std::vector<RecentBook> toAdd;
  const uint16_t count = index.bookCount();

  for (uint16_t row = 0; row < count && static_cast<int>(toAdd.size()) < maxBooks; ++row) {
    uint16_t ordinal = index.ordinalForRow(library::SortOrder::DateAddedDesc, row);
    if (ordinal == 0xFFFF || !index.readRecord(ordinal, record)) break;

    std::string path;
    if (!index.readPath(record, path) || content::readLaterContent(path)) continue;
    if (!libraryui::acceptsFormat(path, SETTINGS.libraryFileTypes)) continue;

    // Skip if already in RECENT_BOOKS
    if (std::any_of(existing.begin(), existing.end(), [&](const RecentBook& b) { return b.path == path; })) {
      continue;
    }

    RecentBook book;
    book.path = std::move(path);
    if (!index.readTitle(record, book.title) && !index.readName(record, book.title)) continue;
    index.readAuthor(record, book.author);

    if (FsHelpers::hasEpubExtension(book.path)) {
      auto epub = makeUniqueNoThrow<Epub>(book.path, "/.crosspoint");
      if (epub) book.coverBmpPath = epub->getThumbBmpPath();
    } else if (FsHelpers::hasXtcExtension(book.path)) {
      auto xtc = makeUniqueNoThrow<Xtc>(book.path, "/.crosspoint", SETTINGS.xtcCoverRotation != 0);
      if (xtc) book.coverBmpPath = xtc->getThumbBmpPath();
    }

    toAdd.push_back(std::move(book));
  }
  index.close();

  // Insert in reverse order so the most newly added book is at index 0 of RECENT_BOOKS
  for (auto it = toAdd.rbegin(); it != toAdd.rend(); ++it) {
    RECENT_BOOKS.addBook(it->path, it->title, it->author, it->coverBmpPath);
  }
}

int scanAndSync(bool useMetadata) {
  library::BuildStats stats;
  bool ok = library::buildLibraryIndex("/", stats, useMetadata);
  if (!ok) return -1;
  syncIndexToRecents(10);
  return static_cast<int>(stats.books);
}

}  // namespace library_sync
