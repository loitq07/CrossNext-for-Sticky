#pragma once

#include <cstddef>
#include <cstdint>

// Interface language for the Sticky build: English is built into the firmware,
// every other language (Vietnamese first) is an upstream 2.0 `.cplang` pack in
// /languages on the microSD, loaded once into PSRAM.
//
// FORK CHANGE: upstream restarts after every language change and only ever
// loads a pack at boot. This fork has always switched language in place, so a
// pack is loaded on first use and stays resident; choosing another pack after
// that asks for a restart (the PSRAM arena is never freed).
namespace uilanguage {

// True when `language` (Language enum index) is compiled into this firmware.
bool builtIn(uint8_t language);

// The pack file a non-built-in language is looked up under when settings name
// none: "<bcp47>.cplang", e.g. "vi.cplang". False if it does not fit.
bool defaultPackFilename(uint8_t language, char* out, size_t size);

// Applies SETTINGS.language / SETTINGS.languagePack to I18N at boot. A saved
// non-built-in language with no pack named (a settings file from a firmware
// that had Vietnamese built in) adopts its default pack file when present.
// If the pack is missing or invalid the interface runs in English for this
// boot without touching the saved choice, so installing the pack restores it.
// Returns true when SETTINGS changed and should be saved.
bool applyAtBoot();

enum class Result : uint8_t { Applied, PackMissing, RestartRequired };

// Switch to a built-in language (pack == nullptr) or to a pack file. Updates
// I18N and SETTINGS.language/languagePack; the caller saves settings.
Result select(uint8_t language, const char* pack);

}  // namespace uilanguage
