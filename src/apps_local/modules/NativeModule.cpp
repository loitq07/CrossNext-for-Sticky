#include "NativeModule.h"

#include <HalStorage.h>
#include <Logging.h>

#include <cstring>
#include <map>

#include "../../activities/Activity.h"
#include "NativeModuleAbi.generated.h"
#include "EspElf.h"
#include "NativeModuleExports.h"
#include "private/elf_symbol.h"

namespace native_module {
namespace {

const char* TAG = "NativeMod";

struct SourceCtx {
  HalFile file;
  size_t size = 0;
};

size_t srcSize(void* ctx) { return static_cast<SourceCtx*>(ctx)->size; }

int srcRead(void* ctx, size_t off, void* buf, size_t len) {
  auto* src = static_cast<SourceCtx*>(ctx);
  if (!src->file) return -1;
  src->file.seek(off);
  return src->file.read(reinterpret_cast<uint8_t*>(buf), len) == len ? 0 : -1;
}

void srcClose(void* ctx) {
  auto* src = static_cast<SourceCtx*>(ctx);
  if (src->file) src->file.close();
}

uint32_t fnv1a(const char* s, uint32_t h) {
  for (; *s; ++s) {
    h ^= static_cast<uint8_t>(*s);
    h *= 0x01000193u;
  }
  return h;
}

// Resolves a module's imports against the generated export table, then the
// loader's own default table (used by small host-API plugins).
uintptr_t resolveSymbol(const char* name) {
  const uint32_t a = fnv1a(name, 0x811C9DC5u);
  const uint32_t b = fnv1a(name, 0x2545F491u);
  size_t lo = 0, hi = kExportCount;
  while (lo < hi) {
    const size_t mid = (lo + hi) / 2;
    const ExportEntry& e = kExports[mid];
    if (e.a < a || (e.a == a && e.b < b)) {
      lo = mid + 1;
    } else {
      hi = mid;
    }
  }
  if (lo < kExportCount && kExports[lo].a == a && kExports[lo].b == b && kExports[lo].addr)
    return reinterpret_cast<uintptr_t>(kExports[lo].addr);
  return elf_find_sym_default(name);
}

void ensureResolver() {
  static bool installed = false;
  if (installed) return;
  installed = true;
  elf_set_symbol_resolver(&resolveSymbol);
}

// Never unloaded: an Activity created from a module runs its destructor after
// the shelf has already moved on, and that code lives in the module.
std::map<std::string, const StickyNativeModule*> loaded;

}  // namespace

uint32_t firmwareAbi() { return STICKY_MODULE_ABI; }

uint32_t fileAbi(const char* path) {
  HalFile f = Storage.open(path, O_RDONLY);
  if (!f) return 0;
  const size_t size = f.size();
  uint8_t tail[8] = {};
  bool ok = size > sizeof(tail) && f.seek(size - sizeof(tail)) && f.read(tail, sizeof(tail)) == static_cast<int>(sizeof(tail));
  f.close();
  if (!ok || std::memcmp(tail, "SMAB", 4) != 0) return 0;
  return static_cast<uint32_t>(tail[4]) | (static_cast<uint32_t>(tail[5]) << 8) |
         (static_cast<uint32_t>(tail[6]) << 16) | (static_cast<uint32_t>(tail[7]) << 24);
}

Result create(const std::string& path, GfxRenderer& renderer, MappedInputManager& mappedInput,
              std::unique_ptr<Activity>& out) {
  const StickyNativeModule* desc = nullptr;
  auto it = loaded.find(path);
  if (it != loaded.end()) {
    desc = it->second;
  } else {
    ensureResolver();
    SourceCtx ctx;
    ctx.file = Storage.open(path.c_str(), O_RDONLY);
    if (!ctx.file) return Result::LoadFailed;
    ctx.size = ctx.file.size();
    elf_source_t source = {.size = srcSize, .read = srcRead, .close = srcClose, .ctx = &ctx};

    auto* elf = new esp_elf_t();
    if (esp_elf_init(elf) != 0 || esp_elf_relocate_source(elf, &source) != 0) {
      LOG_ERR(TAG, "relocate failed: %s", path.c_str());
      esp_elf_deinit(elf);
      delete elf;
      return Result::LoadFailed;
    }

    using GetFn = const StickyNativeModule* (*)();
    GetFn get = nullptr;
    for (int i = 0; elf->symtab && i < elf->num; ++i) {
      if (elf->symtab[i].name && std::strcmp(elf->symtab[i].name, "sticky_native_module_get") == 0) {
        get = reinterpret_cast<GetFn>(elf->symtab[i].addr);
        break;
      }
    }
    if (!get) {
      esp_elf_deinit(elf);
      delete elf;
      return Result::NotNative;
    }

    // The descriptor is constant-initialized, so reading it runs no module
    // code that could call into a different firmware build. Check the ABI
    // before letting any static constructor run.
    desc = get();
    if (!desc || desc->magic != kStickyNativeModuleMagic || desc->abi != STICKY_MODULE_ABI || !desc->create) {
      LOG_ERR(TAG, "%s built for abi %08x, firmware is %08x", path.c_str(), desc ? desc->abi : 0,
              static_cast<unsigned>(STICKY_MODULE_ABI));
      esp_elf_deinit(elf);
      delete elf;
      return Result::AbiMismatch;
    }
    // Static constructors (the entry itself is a no-op).
    if (esp_elf_request(elf, 0, 0, nullptr) != 0) LOG_ERR(TAG, "no entry; static init skipped for %s", path.c_str());
    loaded[path] = desc;
    LOG_INF(TAG, "Loaded module %s from %s", desc->title, path.c_str());
  }

  out = desc->create(renderer, mappedInput);
  return out ? Result::Ok : Result::LoadFailed;
}

}  // namespace native_module
