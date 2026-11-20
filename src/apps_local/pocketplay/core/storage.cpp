#include "storage.h"

#include <Arduino.h>
#include <HalStorage.h>
#include <esp_heap_caps.h>

#include <cstdio>
#include <cstring>

namespace storage {
namespace {

constexpr const char* DIRECTORY = SAVE_FOLDER;
constexpr size_t PATH_BYTES = 64;

constexpr size_t HEADER_BYTES = 10;
constexpr uint8_t FORMAT = 1;

constexpr size_t BLOB_HEADER_BYTES = 11;
constexpr uint8_t BLOB_FORMAT = 1;

bool mounted = false;

void makePath(char* path, const char* name, const char* extension) {
  snprintf(path, PATH_BYTES, "%s/%s.%s", DIRECTORY, name, extension);
}

uint32_t updateChecksum(uint32_t crc, const uint8_t* data, size_t size) {
  for (size_t i = 0; i < size; i++) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; bit++) {
      crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
    }
  }
  return crc;
}

uint32_t checksumOf(uint8_t version, uint16_t length, const uint8_t* data) {
  const uint8_t fields[3] = {version, static_cast<uint8_t>(length & 0xFF), static_cast<uint8_t>(length >> 8)};
  uint32_t crc = updateChecksum(0xFFFFFFFFu, fields, sizeof(fields));
  crc = updateChecksum(crc, data, length);
  return ~crc;
}

void makeHeader(uint8_t* header, uint8_t version, const uint8_t* data, size_t size) {
  const auto length = static_cast<uint16_t>(size);
  const uint32_t crc = checksumOf(version, length, data);
  header[0] = 'B';
  header[1] = 'B';
  header[2] = FORMAT;
  header[3] = version;
  header[4] = static_cast<uint8_t>(length & 0xFF);
  header[5] = static_cast<uint8_t>(length >> 8);
  for (int i = 0; i < 4; i++) {
    header[6 + i] = static_cast<uint8_t>(crc >> (8 * i));
  }
}

bool fileMatches(const char* path, const uint8_t* header, size_t headerBytes, const uint8_t* data, size_t size) {
  HalFile file = Storage.open(path, O_RDONLY);
  if (!file || file.fileSize() != headerBytes + size) return false;

  uint8_t buffer[64];
  if (file.read(buffer, headerBytes) != static_cast<int>(headerBytes) || memcmp(buffer, header, headerBytes) != 0) return false;

  size_t offset = 0;
  while (offset < size) {
    const size_t chunk = min(size - offset, sizeof(buffer));
    if (file.read(buffer, chunk) != static_cast<int>(chunk) || memcmp(buffer, data + offset, chunk) != 0) return false;
    offset += chunk;
  }
  return true;
}

bool writeFile(const char* path, const uint8_t* header, size_t headerBytes, const uint8_t* data, size_t size) {
  HalFile file = Storage.open(path, O_WRONLY | O_CREAT | O_TRUNC);
  if (!file) return false;
  if (file.write(header, headerBytes) != headerBytes || file.write(data, size) != size) return false;
  return file.close();
}

bool readValid(const char* path, uint8_t& version, uint8_t* data, size_t capacity, size_t& size) {
  HalFile file = Storage.open(path, O_RDONLY);
  if (!file) return false;

  uint8_t header[HEADER_BYTES];
  if (file.read(header, HEADER_BYTES) != static_cast<int>(HEADER_BYTES)) return false;
  if (header[0] != 'B' || header[1] != 'B' || header[2] != FORMAT) return false;

  const auto length = static_cast<uint16_t>(header[4] | (header[5] << 8));
  if (length > capacity || file.fileSize() != HEADER_BYTES + length) return false;
  if (file.read(data, length) != static_cast<int>(length)) return false;

  uint32_t stored = 0;
  for (int i = 0; i < 4; i++) {
    stored |= static_cast<uint32_t>(header[6 + i]) << (8 * i);
  }
  if (checksumOf(header[3], length, data) != stored) return false;

  version = header[3];
  size = length;
  return true;
}

uint32_t readUint32(const uint8_t* in) {
  uint32_t value = 0;
  for (int i = 0; i < 4; i++) {
    value |= static_cast<uint32_t>(in[i]) << (8 * i);
  }
  return value;
}

void writeUint32(uint8_t* out, uint32_t value) {
  for (int i = 0; i < 4; i++) {
    out[i] = static_cast<uint8_t>(value >> (8 * i));
  }
}

Blob readValidBlob(const char* path, size_t& size) {
  HalFile file = Storage.open(path, O_RDONLY);
  if (!file) return nullptr;

  uint8_t header[BLOB_HEADER_BYTES];
  if (file.read(header, BLOB_HEADER_BYTES) != static_cast<int>(BLOB_HEADER_BYTES)) return nullptr;
  if (header[0] != 'B' || header[1] != 'L' || header[2] != BLOB_FORMAT) return nullptr;

  const uint32_t length = readUint32(header + 3);
  if (file.fileSize() != BLOB_HEADER_BYTES + length) return nullptr;

  Blob data(static_cast<uint8_t*>(heap_caps_malloc(length + 1, MALLOC_CAP_SPIRAM)));
  if (!data) return nullptr;
  if (file.read(data.get(), length) != static_cast<int>(length)) return nullptr;
  if (~updateChecksum(0xFFFFFFFFu, data.get(), length) != readUint32(header + 7)) return nullptr;

  size = length;
  return data;
}

}  // namespace

void BlobDeleter::operator()(uint8_t* data) const { heap_caps_free(data); }

bool begin() {
  mounted = Storage.ready();
  if (mounted) {
    Storage.ensureDirectoryExists(DIRECTORY);
  }
  return mounted;
}

bool ready() { return mounted; }

bool save(const char* name, uint8_t version, const uint8_t* data, size_t size) {
  if (!mounted || size > MAX_DATA_BYTES) return false;

  char savePath[PATH_BYTES];
  char tempPath[PATH_BYTES];
  makePath(savePath, name, "sav");
  makePath(tempPath, name, "tmp");

  uint8_t header[HEADER_BYTES];
  makeHeader(header, version, data, size);

  Storage.remove(tempPath);
  if (!writeFile(tempPath, header, HEADER_BYTES, data, size) || !fileMatches(tempPath, header, HEADER_BYTES, data, size)) {
    return false;
  }
  if (!Storage.replaceFile(tempPath, savePath)) {
    return false;
  }
  return true;
}

bool load(const char* name, uint8_t& version, uint8_t* data, size_t capacity, size_t& size) {
  if (!mounted) return false;

  char path[PATH_BYTES];
  makePath(path, name, "sav");
  if (readValid(path, version, data, capacity, size)) return true;

  makePath(path, name, "tmp");
  if (readValid(path, version, data, capacity, size)) {
    return true;
  }
  return false;
}

bool erase(const char* name) {
  if (!mounted) return false;

  char path[PATH_BYTES];
  bool erased = true;
  for (const char* extension : {"sav", "tmp"}) {
    makePath(path, name, extension);
    if (Storage.exists(path) && !Storage.remove(path)) erased = false;
  }
  return erased;
}

bool eraseBlob(const char* name) {
  if (!mounted) return false;

  char path[PATH_BYTES];
  bool erased = true;
  for (const char* extension : {"bin", "bit"}) {
    makePath(path, name, extension);
    if (Storage.exists(path) && !Storage.remove(path)) erased = false;
  }
  return erased;
}

bool saveBlob(const char* name, const uint8_t* data, size_t size) {
  if (!mounted) return false;

  char blobPath[PATH_BYTES];
  char tempPath[PATH_BYTES];
  makePath(blobPath, name, "bin");
  makePath(tempPath, name, "bit");

  uint8_t header[BLOB_HEADER_BYTES] = {'B', 'L', BLOB_FORMAT};
  writeUint32(header + 3, static_cast<uint32_t>(size));
  writeUint32(header + 7, ~updateChecksum(0xFFFFFFFFu, data, size));

  Storage.remove(tempPath);
  if (!writeFile(tempPath, header, BLOB_HEADER_BYTES, data, size) || !fileMatches(tempPath, header, BLOB_HEADER_BYTES, data, size)) {
    return false;
  }
  if (!Storage.replaceFile(tempPath, blobPath)) {
    return false;
  }
  return true;
}

Blob loadBlob(const char* name, size_t& size) {
  if (!mounted) return nullptr;

  char path[PATH_BYTES];
  makePath(path, name, "bin");
  Blob data = readValidBlob(path, size);
  if (data) return data;

  makePath(path, name, "bit");
  data = readValidBlob(path, size);
  return data;
}

bool ensureFolder(const char* folder) { return mounted && Storage.ensureDirectoryExists(folder); }

std::vector<std::string> listFolder(const char* folder, int max) {
  std::vector<std::string> names;
  if (!mounted) return names;
  for (const String& name : Storage.listFiles(folder, max)) names.emplace_back(name.c_str());
  return names;
}

Blob readFile(const char* path, size_t maxSize, size_t& size) {
  if (!mounted) return nullptr;
  HalFile file = Storage.open(path, O_RDONLY);
  if (!file) return nullptr;
  const size_t length = file.fileSize();
  if (length > maxSize) return nullptr;
  Blob data(static_cast<uint8_t*>(heap_caps_malloc(length + 1, MALLOC_CAP_SPIRAM)));
  if (!data) return nullptr;
  if (file.read(data.get(), length) != static_cast<int>(length)) return nullptr;
  size = length;
  return data;
}

}  // namespace storage
