#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STICKY_APP_API_VERSION 1

// Color tokens for e-paper display
enum StickyColor {
    STICKY_COLOR_CLEAR     = 0x00,
    STICKY_COLOR_WHITE     = 0x01,
    STICKY_COLOR_LIGHTGRAY = 0x05,
    STICKY_COLOR_DARKGRAY  = 0x0A,
    STICKY_COLOR_BLACK     = 0x10
};

// Touch events
enum StickyTouchEvent {
    STICKY_TOUCH_DOWN  = 1,
    STICKY_TOUCH_MOVE  = 2,
    STICKY_TOUCH_UP    = 3,
    STICKY_TOUCH_CLICK = 4
};

// Host API provided by CrossNext / Sticky OS to the dynamic app
typedef struct StickyHostApi {
    uint32_t api_version;

    // Screen dimensions
    int16_t (*get_screen_width)(void);
    int16_t (*get_screen_height)(void);

    // Drawing primitives
    void (*clear)(uint8_t color);
    void (*draw_pixel)(int16_t x, int16_t y, uint8_t color);
    void (*draw_line)(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);
    void (*draw_rect)(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
    void (*fill_rect)(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
    void (*draw_circle)(int16_t x, int16_t y, int16_t r, uint8_t color);
    void (*fill_circle)(int16_t x, int16_t y, int16_t r, uint8_t color);
    void (*draw_text)(int16_t x, int16_t y, const char* text, int font_size, uint8_t color);
    void (*get_text_bounds)(const char* text, int font_size, int16_t* w, int16_t* h);

    // Standard system UI chrome (header with title and back button)
    void (*draw_header)(const char* title);

    // Trigger e-paper display refresh
    void (*request_update)(bool immediate);

    // Touch & Input
    bool (*get_touch)(int16_t* x, int16_t* y);

    // System Utilities
    uint32_t (*get_millis)(void);
    void (*delay_ms)(uint32_t ms);
    void (*play_haptic)(int pattern);
    void (*exit_app)(void);
    void (*log_print)(const char* msg);

    // Storage Utilities
    bool (*read_file)(const char* path, void* buf, uint32_t max_len, uint32_t* out_len);
    bool (*write_file)(const char* path, const void* buf, uint32_t len);
    bool (*file_exists)(const char* path);
} StickyHostApi;

// App plugin descriptor exported by the dynamic .elf / .so app
typedef struct StickyAppPlugin {
    uint32_t api_version;
    const char* app_name;
    const char* app_version;
    const char* app_author;
    const char* icon_name;

    // Callbacks
    void (*on_init)(const StickyHostApi* host);
    void (*on_enter)(void);
    void (*on_loop)(void);
    void (*on_render)(void);
    void (*on_touch)(int event_type, int16_t x, int16_t y);
    void (*on_exit)(void);
    void (*on_destroy)(void);
} StickyAppPlugin;

// Export function signature expected in .elf / .so
typedef const StickyAppPlugin* (*sticky_app_entry_fn)(void);

#ifdef __cplusplus
}
#endif
