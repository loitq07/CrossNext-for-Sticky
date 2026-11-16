#pragma once

#include <memory>
#include <string>

#include "Activity.h"
#include "../sticky_app_api.h"

struct esp_elf;

class DynamicAppActivity final : public Activity {
 public:
  DynamicAppActivity(std::string appPath, GfxRenderer& renderer, MappedInputManager& mappedInput);
  ~DynamicAppActivity() override;

  static std::unique_ptr<Activity> create(const std::string& appPath, GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
  void onExit() override;

  bool isLoaded() const { return loaded; }
  const std::string& getAppName() const { return appTitle; }

 private:
  std::string path;
  std::string appTitle;
  bool loaded = false;
  void* dlHandle = nullptr;
  const StickyAppPlugin* plugin = nullptr;

  // Active instance pointer for C host callbacks
  static DynamicAppActivity* activeInstance;
  static StickyHostApi hostApi;

  static void initHostApi();
  bool loadModule();
  void unloadModule();

  // Host API callback implementations
  static int16_t api_get_screen_width(void);
  static int16_t api_get_screen_height(void);
  static void api_clear(uint8_t color);
  static void api_draw_pixel(int16_t x, int16_t y, uint8_t color);
  static void api_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);
  static void api_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
  static void api_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
  static void api_draw_circle(int16_t x, int16_t y, int16_t r, uint8_t color);
  static void api_fill_circle(int16_t x, int16_t y, int16_t r, uint8_t color);
  static void api_draw_text(int16_t x, int16_t y, const char* text, int font_size, uint8_t color);
  static void api_get_text_bounds(const char* text, int font_size, int16_t* w, int16_t* h);
  static void api_draw_header(const char* title);
  static void api_request_update(bool immediate);
  static bool api_get_touch(int16_t* x, int16_t* y);
  static uint32_t api_get_millis(void);
  static void api_delay_ms(uint32_t ms);
  static void api_play_haptic(int pattern);
  static void api_exit_app(void);
  static void api_log_print(const char* msg);
  static bool api_read_file(const char* path, void* buf, uint32_t max_len, uint32_t* out_len);
  static bool api_write_file(const char* path, const void* buf, uint32_t len);
  static bool api_file_exists(const char* path);
};
