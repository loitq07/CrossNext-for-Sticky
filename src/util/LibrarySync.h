#pragma once

#include <cstdint>

namespace library_sync {

// Scan SD card (rebuild library index), and sync newly added books into recent books
// so they appear immediately on the home screen, respecting SETTINGS.libraryFileTypes.
// Returns book count on success, or -1 on failure.
int scanAndSync(bool useMetadata = true);

// Sync up to maxBooks newest books from library.idx into RECENT_BOOKS,
// filtered by SETTINGS.libraryFileTypes.
void syncIndexToRecents(int maxBooks = 10);

}  // namespace library_sync
