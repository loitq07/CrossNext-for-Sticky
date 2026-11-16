#include "DynamicAppActivity.h"

#include <Logging.h>
#include <HalStorage.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "ActivityManager.h"
#include "GfxRenderer.h"
#include "MappedInputManager.h"
#include "../HapticFeedback.h"
#include "../apps_local/ui/ToyboxFonts.h"
#include "../apps_local/ui/PixelMenuTheme.h"
#include "../apps_local/modules/EspElf.h"
#include "esp_log.h"

static const char* TAG = "DynamicApp";

DynamicAppActivity* DynamicAppActivity::activeInstance = nullptr;
StickyHostApi DynamicAppActivity::hostApi = {};

namespace {

struct HalElfSourceContext {
  HalFile file;
  size_t size = 0;
};

static size_t hal_elf_size(void* ctx) {
  HalElfSourceContext* src = static_cast<HalElfSourceContext*>(ctx);
  return src->size;
}

static int hal_elf_read(void* ctx, size_t off, void* buf, size_t len) {
  HalElfSourceContext* src = static_cast<HalElfSourceContext*>(ctx);
  if (!src->file) return -1;
  src->file.seek(off);
  size_t readBytes = src->file.read(reinterpret_cast<uint8_t*>(buf), len);
  return readBytes == len ? 0 : -1;
}

static void hal_elf_close(void* ctx) {
  HalElfSourceContext* src = static_cast<HalElfSourceContext*>(ctx);
  if (src->file) {
    src->file.close();
  }
}

}  // namespace

DynamicAppActivity::DynamicAppActivity(std::string appPath, GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("DynamicApp", renderer, mappedInput), path(std::move(appPath)) {
  initHostApi();
}

DynamicAppActivity::~DynamicAppActivity() {
  unloadModule();
  if (activeInstance == this) {
    activeInstance = nullptr;
  }
}

std::unique_ptr<Activity> DynamicAppActivity::create(const std::string& appPath, GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<DynamicAppActivity>(appPath, renderer, mappedInput);
}

void DynamicAppActivity::initHostApi() {
  if (hostApi.api_version == STICKY_APP_API_VERSION) {
    return;
  }
  hostApi.api_version       = STICKY_APP_API_VERSION;
  hostApi.get_screen_width  = api_get_screen_width;
  hostApi.get_screen_height = api_get_screen_height;
  hostApi.clear             = api_clear;
  hostApi.draw_pixel        = api_draw_pixel;
  hostApi.draw_line         = api_draw_line;
  hostApi.draw_rect         = api_draw_rect;
  hostApi.fill_rect         = api_fill_rect;
  hostApi.draw_circle       = api_draw_circle;
  hostApi.fill_circle       = api_fill_circle;
  hostApi.draw_text         = api_draw_text;
  hostApi.get_text_bounds   = api_get_text_bounds;
  hostApi.draw_header       = api_draw_header;
  hostApi.request_update    = api_request_update;
  hostApi.get_touch         = api_get_touch;
  hostApi.get_millis        = api_get_millis;
  hostApi.delay_ms          = api_delay_ms;
  hostApi.play_haptic       = api_play_haptic;
  hostApi.exit_app          = api_exit_app;
  hostApi.log_print         = api_log_print;
  hostApi.read_file         = api_read_file;
  hostApi.write_file        = api_write_file;
  hostApi.file_exists       = api_file_exists;
}

bool DynamicAppActivity::loadModule() {
  if (loaded) return true;

  activeInstance = this;
  toybox::ensureFonts(renderer);

  HalFile f = Storage.open(path.c_str(), O_RDONLY);
  if (!f) {
    ESP_LOGE(TAG, "Cannot open ELF file: %s", path.c_str());
    return false;
  }

  size_t fSize = f.size();
  if (fSize == 0) {
    ESP_LOGE(TAG, "ELF file is empty: %s", path.c_str());
    f.close();
    return false;
  }

  HalElfSourceContext srcCtx;
  srcCtx.file = std::move(f);
  srcCtx.size = fSize;

  elf_source_t source = {
      .size = hal_elf_size,
      .read = hal_elf_read,
      .close = hal_elf_close,
      .ctx = &srcCtx,
  };

  esp_elf_t* elf = new esp_elf_t();
  if (esp_elf_init(elf) != 0) {
    ESP_LOGE(TAG, "esp_elf_init failed");
    delete elf;
    return false;
  }

  if (esp_elf_relocate_source(elf, &source) != 0) {
    ESP_LOGE(TAG, "esp_elf_relocate_source failed for %s", path.c_str());
    esp_elf_deinit(elf);
    delete elf;
    return false;
  }

  dlHandle = elf;

  // Search for sticky_app_get_plugin
  sticky_app_entry_fn getPluginFn = nullptr;

#ifdef CONFIG_ELF_DYNAMIC_LOAD_SHARED_OBJECT
  if (elf->symtab && elf->num > 0) {
    for (int i = 0; i < elf->num; i++) {
      if (elf->symtab[i].name && strcmp(elf->symtab[i].name, "sticky_app_get_plugin") == 0) {
        getPluginFn = reinterpret_cast<sticky_app_entry_fn>(elf->symtab[i].addr);
        break;
      }
    }
  }
#endif

  if (!getPluginFn && elf->entry) {
    // If no get_plugin exported, use entry point to exchange API
    typedef const StickyAppPlugin* (*entry_fn_t)(const StickyHostApi*);
    entry_fn_t entryCall = reinterpret_cast<entry_fn_t>(elf->entry);
    plugin = entryCall(&hostApi);
  } else if (getPluginFn) {
    plugin = getPluginFn();
    if (plugin && plugin->on_init) {
      plugin->on_init(&hostApi);
    }
  }

  if (!plugin) {
    ESP_LOGE(TAG, "App plugin descriptor not found in %s", path.c_str());
    unloadModule();
    return false;
  }

  appTitle = plugin->app_name ? plugin->app_name : "App";
  name = appTitle;
  loaded = true;
  ESP_LOGI(TAG, "Loaded external app: %s (%s)", appTitle.c_str(), plugin->app_version ? plugin->app_version : "1.0");
  return true;
}

void DynamicAppActivity::unloadModule() {
  if (plugin && plugin->on_destroy) {
    plugin->on_destroy();
  }
  plugin = nullptr;

  if (dlHandle) {
    esp_elf_t* elf = static_cast<esp_elf_t*>(dlHandle);
    esp_elf_deinit(elf);
    delete elf;
    dlHandle = nullptr;
  }
  loaded = false;
}

void DynamicAppActivity::onEnter() {
  Activity::onEnter();
  activeInstance = this;

  if (!loaded) {
    if (!loadModule()) {
      activityManager.popActivity();
      return;
    }
  }

  if (plugin && plugin->on_enter) {
    plugin->on_enter();
  }
  requestUpdate(true);
}

void DynamicAppActivity::loop() {
  if (!loaded || !plugin) return;

  activeInstance = this;

  // Process touch events
  if (plugin->on_touch && mappedInput.hasTouch()) {
    int tx = 0, ty = 0;
    if (mappedInput.wasScreenTapped(tx, ty)) {
      plugin->on_touch(STICKY_TOUCH_CLICK, tx, ty);
    }
  }

  if (plugin->on_loop) {
    plugin->on_loop();
  }
}

void DynamicAppActivity::render(RenderLock&&) {
  if (!loaded || !plugin) return;

  activeInstance = this;
  if (plugin->on_render) {
    plugin->on_render();
  }
}

void DynamicAppActivity::onExit() {
  if (loaded && plugin && plugin->on_exit) {
    plugin->on_exit();
  }
  Activity::onExit();
}

// ----------------- Host API Callbacks -----------------

int16_t DynamicAppActivity::api_get_screen_width(void) {
  return activeInstance ? activeInstance->renderer.getScreenWidth() : 480;
}

int16_t DynamicAppActivity::api_get_screen_height(void) {
  return activeInstance ? activeInstance->renderer.getScreenHeight() : 800;
}

void DynamicAppActivity::api_clear(uint8_t color) {
  if (activeInstance) {
    activeInstance->renderer.clearScreen(static_cast<Color>(color));
  }
}

void DynamicAppActivity::api_draw_pixel(int16_t x, int16_t y, uint8_t color) {
  if (activeInstance) {
    activeInstance->renderer.drawPixel(x, y, color != STICKY_COLOR_WHITE);
  }
}

void DynamicAppActivity::api_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color) {
  if (activeInstance) {
    activeInstance->renderer.drawLine(x0, y0, x1, y1, static_cast<Color>(color));
  }
}

void DynamicAppActivity::api_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
  if (activeInstance) {
    activeInstance->renderer.drawRect(x, y, w, h, static_cast<Color>(color));
  }
}

void DynamicAppActivity::api_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
  if (activeInstance) {
    activeInstance->renderer.fillRect(x, y, w, h, static_cast<Color>(color));
  }
}

void DynamicAppActivity::api_draw_circle(int16_t cx, int16_t cy, int16_t r, uint8_t color) {
  if (!activeInstance || r <= 0) return;
  bool is_black = (color != STICKY_COLOR_WHITE);
  int x = r, y = 0;
  int err = 0;
  while (x >= y) {
    activeInstance->renderer.drawPixel(cx + x, cy + y, is_black);
    activeInstance->renderer.drawPixel(cx + y, cy + x, is_black);
    activeInstance->renderer.drawPixel(cx - y, cy + x, is_black);
    activeInstance->renderer.drawPixel(cx - x, cy + y, is_black);
    activeInstance->renderer.drawPixel(cx - x, cy - y, is_black);
    activeInstance->renderer.drawPixel(cx - y, cy - x, is_black);
    activeInstance->renderer.drawPixel(cx + y, cy - x, is_black);
    activeInstance->renderer.drawPixel(cx + x, cy - y, is_black);
    y++;
    if (err <= 0) {
      err += 2 * y + 1;
    }
    if (err > 0) {
      x--;
      err -= 2 * x + 1;
    }
  }
}

void DynamicAppActivity::api_fill_circle(int16_t cx, int16_t cy, int16_t r, uint8_t color) {
  if (!activeInstance || r <= 0) return;
  for (int16_t dy = -r; dy <= r; ++dy) {
    int16_t half = 0;
    while ((half + 1) * (half + 1) + dy * dy <= r * r) ++half;
    if (half <= 0) continue;
    activeInstance->renderer.drawLine(cx - half, cy + dy, cx + half, cy + dy, static_cast<Color>(color));
  }
}

void DynamicAppActivity::api_draw_text(int16_t x, int16_t y, const char* text, int font_size, uint8_t color) {
  if (activeInstance && text) {
    int fontId = font_size > 0 ? font_size : toybox::kUiFontId;
    activeInstance->renderer.drawText(fontId, x, y, text, color != STICKY_COLOR_WHITE);
  }
}

void DynamicAppActivity::api_get_text_bounds(const char* text, int font_size, int16_t* w, int16_t* h) {
  if (activeInstance && text) {
    int fontId = font_size > 0 ? font_size : toybox::kUiFontId;
    if (w) *w = activeInstance->renderer.getTextWidth(fontId, text);
    if (h) *h = activeInstance->renderer.getTextHeight(fontId);
  }
}

void DynamicAppActivity::api_draw_header(const char* title) {
  if (activeInstance && title) {
    pixelui::drawHeader(activeInstance->renderer, title, true);
  }
}

void DynamicAppActivity::api_request_update(bool immediate) {
  if (activeInstance) {
    activeInstance->requestUpdate(immediate);
  }
}

bool DynamicAppActivity::api_get_touch(int16_t* x, int16_t* y) {
  if (!activeInstance) return false;
  int tx = 0, ty = 0;
  if (activeInstance->mappedInput.wasScreenTapped(tx, ty)) {
    if (x) *x = tx;
    if (y) *y = ty;
    return true;
  }
  return false;
}

uint32_t DynamicAppActivity::api_get_millis(void) {
  return static_cast<uint32_t>(esp_log_timestamp());
}

void DynamicAppActivity::api_delay_ms(uint32_t ms) {
  vTaskDelay(pdMS_TO_TICKS(ms));
}

void DynamicAppActivity::api_play_haptic(int pattern) {
  haptic_feedback::touchAction(pattern > 1);
}

void DynamicAppActivity::api_exit_app(void) {
  activityManager.popActivity();
}

void DynamicAppActivity::api_log_print(const char* msg) {
  if (msg) {
    ESP_LOGI("EXT_APP", "%s", msg);
  }
}

bool DynamicAppActivity::api_read_file(const char* path, void* buf, uint32_t max_len, uint32_t* out_len) {
  if (!path || !buf) return false;
  HalFile f = Storage.open(path, O_RDONLY);
  if (!f) return false;
  size_t sz = f.size();
  size_t to_read = sz > max_len ? max_len : sz;
  int read_bytes = f.read(buf, to_read);
  if (out_len) *out_len = read_bytes > 0 ? static_cast<uint32_t>(read_bytes) : 0;
  f.close();
  return read_bytes >= 0;
}

bool DynamicAppActivity::api_write_file(const char* path, const void* buf, uint32_t len) {
  if (!path || !buf) return false;
  Storage.ensureDirectoryExists("/.crosspoint");
  HalFile f = Storage.open(path, O_WRONLY | O_CREAT | O_TRUNC);
  if (!f) return false;
  size_t written = f.write(reinterpret_cast<const uint8_t*>(buf), len);
  f.flush();
  f.close();
  return written == len;
}

bool DynamicAppActivity::api_file_exists(const char* path) {
  if (!path) return false;
  return Storage.exists(path);
}
