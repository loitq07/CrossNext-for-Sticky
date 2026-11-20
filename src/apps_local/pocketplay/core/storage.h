#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// Small save files on the SD card, one per name, in /.pocketplay/. A power cut at
// any moment leaves either the old save or the new one, never a damaged one.
namespace storage {

constexpr size_t MAX_DATA_BYTES = 512;

// Mounts the card and creates the folder. Returns false if there's no usable card.
bool begin();
bool ready();

bool save(const char* name, uint8_t version, const uint8_t* data, size_t size);

// Returns false if there's no valid save, which is normal on first use.
bool load(const char* name, uint8_t& version, uint8_t* data, size_t capacity, size_t& size);

// Returns true once no save by this name is left on the card.
bool erase(const char* name);

// For data too big for a save, such as a word list. These work the same way, and the
// data lives in PSRAM. A blob named "x" is a separate file from a save named "x".
struct BlobDeleter {
  void operator()(uint8_t* data) const;
};
using Blob = std::unique_ptr<uint8_t[], BlobDeleter>;

bool saveBlob(const char* name, const uint8_t* data, size_t size);

// Returns nothing if there's no valid blob or no memory for it.
Blob loadBlob(const char* name, size_t& size);

// Returns true once no blob by this name is left on the card.
bool eraseBlob(const char* name);

// The folder blobs and saves are kept in, for listFolder(): a blob named "x" is the file
// "x.bin" there, and "x.bit" while it's being replaced.
constexpr const char* SAVE_FOLDER = "/.pocketplay";

// Files the player copies onto the card, outside the save folder, such as a game's story
// files. Read only: nothing here writes or removes them.
//
// Creates the folder if it's missing, so the player can find where files go. Folder names
// are full paths from the card's root, such as "/textadv".
bool ensureFolder(const char* folder);

// The names of the files in a folder, not its subfolders, up to `max`, in the card's order.
std::vector<std::string> listFolder(const char* folder, int max);

// A whole file into PSRAM. Returns nothing if it's missing, bigger than `maxSize`, or
// there's no memory for it.
Blob readFile(const char* path, size_t maxSize, size_t& size);

}  // namespace storage
