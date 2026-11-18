#include "BookProgress.h"

#include <Epub.h>
#include <FsHelpers.h>
#include <HalMemory.h>
#include <HalStorage.h>
#include <KOReaderDocumentId.h>
#include <LibraryBuilder.h>
#include <Logging.h>
#include <Memory.h>
#include <TrustedTime.h>
#include <Xtc.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <functional>
#include <new>

#include "BookProgressCache.h"
#include "apps_local/home/CoverSceneCache.h"

#include "CrossPointSettings.h"
#include "RecentBooksStore.h"
#include "activities/reader/EpubReaderUtils.h"
#include "apps_local/reading/ReadingStatsStore.h"

namespace {
uint32_t readLe32(const uint8_t* p) {
  return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

struct ProgressCache {
  char path[512]{};
  uint8_t progress[10]{};
  uint64_t sourceBytes = 0, metadataBytes = 0;
  uint32_t sourceModified = 0, metadataModified = 0;
  int progressBytes = 0, percent = -1;
} cached;
}  // namespace

namespace {
bookprogress::Cache* progressCache() {
  // Sixty-four validated results cover the entire shelf, replacing its per-activity path table.
  // Static internal storage and the render-task stack cannot afford this table.
  static HalMemory::PsramBuffer memory;
  static bookprogress::Cache* cache = nullptr;
  static bool attempted = false;
  if (!attempted) {
    attempted = true;
    if (HalMemory::getPsramHeap().freeBytes >= sizeof(bookprogress::Cache) + 512 * 1024)
      memory = HalMemory::allocatePsram(sizeof(bookprogress::Cache));
    if (memory)
      cache = new (memory.get()) bookprogress::Cache;
    else
      LOG_ERR("HOME", "PSRAM progress cache unavailable (%u bytes)", unsigned(sizeof(bookprogress::Cache)));
  }
  return cache;
}
bookprogress::LiveStamp liveProgressStamp(const char* progressPath) {
  return {Storage.stateFileRevision(progressPath), Storage.resourceRevision(), Storage.mediaRevision(),
          library::libraryContentRevision()};
}
}  // namespace

static int loadBookProgressUncached(const std::string& path) {
  uint8_t data[10]{};
  if (FsHelpers::hasReflowableBookExtension(path)) {
    // Metadata objects exceed the render-task stack budget.
    auto epub = makeUniqueNoThrow<Epub>(path, "/.crosspoint");
    if (!epub) {
      LOG_ERR("HOME", "OOM: progress metadata");
      return -1;
    }
    if (!epub->load(false, true)) return -1;
    HalFile file;
    if (!Storage.openFileForRead("HOME", epub->getCachePath() + "/progress.bin", file)) return -1;
    const int size = file.read(data, sizeof(data));
    if (size != 4 && size != 6 && size != 10) return -1;
    const int spine = data[0] | (data[1] << 8);
    const int page = data[2] | (data[3] << 8);
    const int total = size >= 6 ? data[4] | (data[5] << 8) : 0;
    if (epub->getSpineItemsCount() <= 0 || epub->getBookSize() == 0) return -1;
    if (spine == epub->getSpineItemsCount()) return 100;
    if (spine > epub->getSpineItemsCount()) return -1;
    const float fraction = total > 0 && page != UINT16_MAX ? std::clamp(float(page) / total, 0.0f, 1.0f) : 0;
    return std::clamp(static_cast<int>(epub->calculateProgress(spine, fraction) * 100 + 0.5f), 0, 100);
  }
  if (FsHelpers::hasXtcExtension(path)) {
    auto xtc = makeUniqueNoThrow<Xtc>(path, "/.crosspoint");
    if (!xtc) {
      LOG_ERR("HOME", "OOM: XTC progress metadata");
      return -1;
    }
    if (!xtc->load()) return -1;
    HalFile file;
    if (!Storage.openFileForRead("HOME", xtc->getCachePath() + "/progress.bin", file) || file.read(data, 4) != 4)
      return -1;
    const uint32_t page = readLe32(data);
    if (xtc->getPageCount() == 0) return -1;
    if (page >= xtc->getPageCount()) return 100;
    return xtc->calculateProgress(page);
  }
  return -1;
}

int loadBookProgress(const std::string& path) {
  auto* cache = progressCache();
  const bool epub = FsHelpers::hasReflowableBookExtension(path);
  if ((!epub && !FsHelpers::hasXtcExtension(path)) || path.size() >= bookprogress::Cache::PATH_SIZE)
    return loadBookProgressUncached(path);
  char filePath[96];
  const auto hash = static_cast<unsigned long>(std::hash<std::string>{}(path));
  std::snprintf(filePath, sizeof(filePath), "/.crosspoint/%s_%lu/progress.bin", epub ? "epub" : "xtc", hash);
  const auto live = liveProgressStamp(filePath);
  if (cache) {
    const int percent = cache->findLive(path.c_str(), live);
    if (percent >= 0 && live == liveProgressStamp(filePath)) return percent;
  }
  const auto libraryRevision = library::libraryContentRevision();
  const auto artRevision = homecover::sceneRevision();
  uint8_t progress[10]{};
  int length = 0;
  uint64_t sourceBytes = 0, metadataBytes = 0;
  uint32_t sourceModified = 0, metadataModified = 0;
  {
    HalFile file;
    if (!Storage.openFileForRead("HOME", filePath, file)) return -1;
    length = file.read(progress, sizeof(progress));
    if (length != 4 && length != 6 && length != 10) return -1;
  }
  if (cache) {
    const int percent =
        cache->findProgress(path.c_str(), progress, length, libraryRevision, artRevision, live.resource, live.media);
    if (percent >= 0) {
      if (live == liveProgressStamp(filePath)) cache->bindLive(path.c_str(), live);
      return percent;
    }
  }
  {
    HalFile file;
    if (!Storage.openFileForRead("HOME", path, file)) return -1;
    sourceBytes = file.fileSize64();
    sourceModified = file.modificationTime();
  }
  if (epub) {
    std::snprintf(filePath, sizeof(filePath), "/.crosspoint/epub_%lu/book.bin", hash);
    HalFile file;
    if (!Storage.openFileForRead("HOME", filePath, file)) return -1;
    metadataBytes = file.fileSize64();
    metadataModified = file.modificationTime();
  }
  bookprogress::Stamp stamp;
  stamp.libraryRevision = libraryRevision;
  stamp.artRevision = artRevision;
  stamp.resource = live.resource;
  stamp.media = live.media;
  std::memcpy(stamp.progress.data(), progress, sizeof(progress));
  stamp.progressBytes = length;
  stamp.sourceBytes = sourceBytes;
  stamp.sourceModified = sourceModified;
  stamp.metadataBytes = metadataBytes;
  stamp.metadataModified = metadataModified;
  if (cache) {
    const int percent = cache->find(path.c_str(), stamp);
    if (percent >= 0) {
      std::snprintf(filePath, sizeof(filePath), "/.crosspoint/%s_%lu/progress.bin", epub ? "epub" : "xtc", hash);
      if (live == liveProgressStamp(filePath)) cache->bindLive(path.c_str(), live);
      return percent;
    }
  }
  const int percent = loadBookProgressUncached(path);
  if (cache) {
    cache->store(path.c_str(), stamp, percent);
    std::snprintf(filePath, sizeof(filePath), "/.crosspoint/%s_%lu/progress.bin", epub ? "epub" : "xtc", hash);
    if (live == liveProgressStamp(filePath)) cache->bindLive(path.c_str(), live);
  }
  return percent;
}

bool isBookRead(const std::string& path) {
  const int progress = loadBookProgress(path);
  if (progress >= 100) return true;
  const auto doc = KOReaderDocumentId::calculate(path);
  bool completed = false, manuallyUnfinished = false;
  if (readingstats::validDocument(doc.c_str()) &&
      readingstats::completionStatus(doc.c_str(), completed, manuallyUnfinished)) {
    return completed;
  }
  return false;
}

bool setBookReadStatus(const std::string& path, const std::string& title, const std::string& author, bool markAsRead) {
  cached.percent = -1;
  cached.path[0] = '\0';

  const bool isEpub = FsHelpers::hasReflowableBookExtension(path);
  const bool isXtc = FsHelpers::hasXtcExtension(path);

  if (markAsRead) {
    if (isEpub) {
      auto epub = makeUniqueNoThrow<Epub>(path, "/.crosspoint");
      if (epub && epub->load(false, true) && epub->getSpineItemsCount() > 0) {
        EpubReaderUtils::saveProgress(*epub, epub->getSpineItemsCount(), 0, 0);
      }
    } else if (isXtc) {
      auto xtc = makeUniqueNoThrow<Xtc>(path, "/.crosspoint");
      if (xtc && xtc->load() && xtc->getPageCount() > 0) {
        const uint32_t page = xtc->getPageCount();
        uint8_t data[4];
        for (int n = 0; n < 4; ++n) data[n] = static_cast<uint8_t>(page >> (8 * n));
        ProgressFile::writeAtomic(xtc->getCachePath(), data, sizeof(data));
      }
    }

    const auto doc = KOReaderDocumentId::calculate(path);
    auto now = trustedtime::trustedNow();
    if (!now) now = static_cast<int64_t>(std::time(nullptr));
    if (readingstats::validDocument(doc.c_str())) {
      readingstats::recordCompletion(doc.c_str(), title.c_str(), author.c_str(), now);
    }

    if (SETTINGS.removeReadBooksFromRecents) {
      if (RECENT_BOOKS.removeByPath(path)) {
        RECENT_BOOKS.saveToFile();
      }
    }
  } else {
    // Mark as unread
    if (isEpub) {
      auto epub = makeUniqueNoThrow<Epub>(path, "/.crosspoint");
      if (epub && epub->load(false, true)) {
        EpubReaderUtils::saveProgress(*epub, 0, 0, 0);
      } else {
        const auto hash = static_cast<unsigned long>(std::hash<std::string>{}(path));
        char progressPath[96];
        std::snprintf(progressPath, sizeof(progressPath), "/.crosspoint/epub_%lu/progress.bin", hash);
        Storage.remove(progressPath);
      }
    } else if (isXtc) {
      auto xtc = makeUniqueNoThrow<Xtc>(path, "/.crosspoint");
      if (xtc && xtc->load()) {
        uint8_t data[4]{0, 0, 0, 0};
        ProgressFile::writeAtomic(xtc->getCachePath(), data, sizeof(data));
      } else {
        const auto hash = static_cast<unsigned long>(std::hash<std::string>{}(path));
        char progressPath[96];
        std::snprintf(progressPath, sizeof(progressPath), "/.crosspoint/xtc_%lu/progress.bin", hash);
        Storage.remove(progressPath);
      }
    }

    const auto doc = KOReaderDocumentId::calculate(path);
    auto now = trustedtime::trustedNow();
    if (!now) now = static_cast<int64_t>(std::time(nullptr));
    if (readingstats::validDocument(doc.c_str())) {
      readingstats::recordReopening(doc.c_str(), title.c_str(), author.c_str(), now, 0);
    }
  }
  return true;
}

void publishBookProgress(const std::string& path, int percent) {
  const bool epub = FsHelpers::hasReflowableBookExtension(path);
  if (!epub && !FsHelpers::hasXtcExtension(path)) return;
  char progressPath[96];
  std::snprintf(progressPath, sizeof(progressPath), "/.crosspoint/%s_%lu/progress.bin", epub ? "epub" : "xtc",
                static_cast<unsigned long>(std::hash<std::string>{}(path)));
  if (auto* cache = progressCache()) cache->publish(path.c_str(), liveProgressStamp(progressPath), percent);
}
