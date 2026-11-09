#include <ClippingStore.h>
#include <HalStorage.h>
#include <apps_local/readertext/ReaderTouchActions.h>

#include <cassert>
#include <cstring>
#include <string>

static void testCorners() {
  for (const auto width : {480, 800}) {
    const int height = width == 480 ? 800 : 480;
    assert(readertext::isBookmarkCorner(width - 1, 0, width, height));
    assert(readertext::isBookmarkCorner(width - 64, 63, width, height));
    assert(!readertext::isBookmarkCorner(width - 65, 0, width, height));
    assert(!readertext::isBookmarkCorner(width - 1, 64, width, height));
    assert(!readertext::isBookmarkCorner(width, 0, width, height));
    assert(!readertext::isBookmarkCorner(width - 1, -1, width, height));
    assert(!readertext::isBookmarkCorner(-1, 0, width, height));
  }
}

static void testEdit(const int failure) {
  fake::reset();
  ClippingStore store;
  assert(store.loadForBook("/book.epub", "Book", "Author", "epub"));
  for (const char* text : {"first", "original", "last"}) {
    assert(store.addClipping(0, 1, 1, 3, 0, 1, 2, "Chapter", 0, text, 42, 5, 15) == ClippingStore::AddResult::Added);
  }
  assert(store.prepareSync());
  assert(store.finishUploads());
  const Clipping original = *store.clippingAt(1);
  Clipping edited = original;
  edited.startPage = 2;
  edited.endPage = 2;
  edited.startOffset = 20;
  edited.endOffset = 36;
  // Local callers cannot accidentally change the record's sync identity.
  std::strcpy(edited.id, "different");
  edited.timestamp = 99;
  if (failure == 0) fake::failWrite = 0;
  if (failure == 1) fake::failRename = 0;
  if (failure == 2) fake::failRename = 1;
  if (failure == 3) fake::shortWrite = 0;
  const bool saved = store.updateClippingAt(1, edited, "edited selection");
  assert(saved == (failure < 0));
  assert(store.clippingCount() == 3);
  assert(std::strcmp(store.clippingAt(1)->id, original.id) == 0);
  assert(store.clippingAt(1)->timestamp == original.timestamp);
  assert(store.clippingAt(1)->syncRevision == original.syncRevision);
  assert(store.clippingAt(1)->pendingUpload == saved);
  assert(store.clippingAt(1)->startOffset == (saved ? 20u : 5u));
  std::string text;
  assert(store.readClippingText(1, text));
  assert(text == (saved ? "edited selection" : "original"));
  assert(!store.updateClippingAt(3, edited, "invalid"));
  assert(!store.updateClippingAt(1, edited, ""));
  assert(!store.updateClippingAt(1, edited, std::string(CLIPPING_TEXT_MAX + 1, 'x')));
  store.unload();
  assert(store.loadForBook("/book.epub", "Book", "Author", "epub"));
  assert(store.clippingCount() == 3);
  assert(store.readClippingText(0, text) && text == "first");
  assert(store.readClippingText(1, text) && text == (saved ? "edited selection" : "original"));
  assert(store.readClippingText(2, text) && text == "last");
  uint32_t offset = 0;
  char id[65];
  assert(store.nextDeletion(offset, id) && id[0] == '\0');
}

int main() {
  testCorners();
  for (int failure = -1; failure <= 3; ++failure) testEdit(failure);
}
