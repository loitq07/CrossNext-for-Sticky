#pragma once

#include <cstdint>
#include <memory>
#include <string>

class Activity;
class GfxRenderer;
class MappedInputManager;

// A built-in app compiled from its original source into an SD-card module
// (tools_local/native_modules/build_module.py). The module links against this
// exact firmware build, so the loader checks `abi` before running anything.
constexpr uint32_t kStickyNativeModuleMagic = 0x444F4D53;  // "SMOD"

struct StickyNativeModule {
  uint32_t magic;
  uint32_t abi;
  const char* title;
  std::unique_ptr<Activity> (*create)(GfxRenderer&, MappedInputManager&);
};

namespace native_module {

enum class Result : uint8_t { Ok, NotNative, AbiMismatch, LoadFailed };

// Loads (once per boot; modules stay resident in PSRAM so an Activity's code
// never disappears under it) and creates the module's Activity.
Result create(const std::string& path, GfxRenderer& renderer, MappedInputManager& mappedInput,
              std::unique_ptr<Activity>& out);

// This firmware's module ABI id; the App Store uses it to fetch matching builds.
uint32_t firmwareAbi();

// The ABI a module file on the card was built for (its 8-byte "SMAB" trailer),
// or 0 when the file is missing or is not a native module.
uint32_t fileAbi(const char* path);

}  // namespace native_module
