#pragma once

// esp_elf_t's layout depends on the CONFIG_ELF_* switches the elf_loader
// library is compiled with (lib/elf_loader/library.json "flags"). Those flags
// are private to the library build, so firmware code that includes esp_elf.h
// directly sees a smaller struct and `new esp_elf_t()` under-allocates what the
// library then writes into. Mirror them here; keep in step with library.json.
#ifndef CONFIG_ELF_LOADER
#define CONFIG_ELF_LOADER 1
#endif
#ifndef CONFIG_ELF_DYNAMIC_LOAD_SHARED_OBJECT
#define CONFIG_ELF_DYNAMIC_LOAD_SHARED_OBJECT 1
#endif
#ifndef CONFIG_ELF_LOADER_LOAD_PSRAM
#define CONFIG_ELF_LOADER_LOAD_PSRAM 1
#endif
#ifndef CONFIG_ELF_LOADER_BUS_ADDRESS_MIRROR
#define CONFIG_ELF_LOADER_BUS_ADDRESS_MIRROR 1
#endif
#ifndef CONFIG_ELF_LOADER_CACHE_OFFSET
#define CONFIG_ELF_LOADER_CACHE_OFFSET 1
#endif
#ifndef CONFIG_ELF_LOADER_CUSTOMER_SYMBOLS
#define CONFIG_ELF_LOADER_CUSTOMER_SYMBOLS 1
#endif

#include "esp_elf.h"
