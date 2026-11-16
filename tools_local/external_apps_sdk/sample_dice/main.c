#include "sticky_app_api.h"
#include <stddef.h>

static const StickyHostApi* g_host = NULL;
static int g_dice_value = 1;
static unsigned int g_seed = 12345;

static int simple_rand(void) {
    g_seed = (1103515245 * g_seed + 12345) & 0x7fffffff;
    return (int)(g_seed % 6) + 1;
}

static void app_init(const StickyHostApi* host) {
    g_host = host;
}

static void app_enter(void) {
    if (g_host) {
        g_seed = g_host->get_millis();
        g_dice_value = simple_rand();
        g_host->request_update(true);
    }
}

static void app_loop(void) {
}

static void draw_dice_dot(int cx, int cy, int radius) {
    if (g_host) {
        g_host->fill_circle(cx, cy, radius, STICKY_COLOR_BLACK);
    }
}

static void app_render(void) {
    if (!g_host) return;

    // Clear background to white
    g_host->clear(STICKY_COLOR_WHITE);

    // Draw the standard system header
    g_host->draw_header("DICE ROLLER");

    int screen_w = g_host->get_screen_width();
    int screen_h = g_host->get_screen_height();

    // Draw center Dice Box (200x200 px)
    int box_size = 200;
    int box_x = (screen_w - box_size) / 2;
    int box_y = (screen_h - box_size) / 2 - 40;

    // Outer border
    g_host->draw_rect(box_x, box_y, box_size, box_size, STICKY_COLOR_BLACK);
    g_host->draw_rect(box_x + 1, box_y + 1, box_size - 2, box_size - 2, STICKY_COLOR_BLACK);
    g_host->draw_rect(box_x + 2, box_y + 2, box_size - 4, box_size - 4, STICKY_COLOR_BLACK);

    // Draw Dice Dots
    int dot_r = 14;
    int left_x = box_x + 45;
    int mid_x  = box_x + box_size / 2;
    int right_x = box_x + box_size - 45;

    int top_y = box_y + 45;
    int mid_y = box_y + box_size / 2;
    int bot_y = box_y + box_size - 45;

    switch (g_dice_value) {
        case 1:
            draw_dice_dot(mid_x, mid_y, dot_r);
            break;
        case 2:
            draw_dice_dot(left_x, top_y, dot_r);
            draw_dice_dot(right_x, bot_y, dot_r);
            break;
        case 3:
            draw_dice_dot(left_x, top_y, dot_r);
            draw_dice_dot(mid_x, mid_y, dot_r);
            draw_dice_dot(right_x, bot_y, dot_r);
            break;
        case 4:
            draw_dice_dot(left_x, top_y, dot_r);
            draw_dice_dot(right_x, top_y, dot_r);
            draw_dice_dot(left_x, bot_y, dot_r);
            draw_dice_dot(right_x, bot_y, dot_r);
            break;
        case 5:
            draw_dice_dot(left_x, top_y, dot_r);
            draw_dice_dot(right_x, top_y, dot_r);
            draw_dice_dot(mid_x, mid_y, dot_r);
            draw_dice_dot(left_x, bot_y, dot_r);
            draw_dice_dot(right_x, bot_y, dot_r);
            break;
        case 6:
            draw_dice_dot(left_x, top_y, dot_r);
            draw_dice_dot(right_x, top_y, dot_r);
            draw_dice_dot(left_x, mid_y, dot_r);
            draw_dice_dot(right_x, mid_y, dot_r);
            draw_dice_dot(left_x, bot_y, dot_r);
            draw_dice_dot(right_x, bot_y, dot_r);
            break;
    }

    // Hint text at the bottom
    const char* hint = "Tap screen to roll!";
    int16_t tw = 0, th = 0;
    g_host->get_text_bounds(hint, 0, &tw, &th);
    int text_x = (screen_w - tw) / 2;
    g_host->draw_text(text_x, box_y + box_size + 60, hint, 0, STICKY_COLOR_BLACK);
}

static void app_touch(int event_type, int16_t x, int16_t y) {
    (void)x;
    (void)y;
    (void)event_type;

    if (g_host) {
        g_dice_value = simple_rand();
        g_host->play_haptic(1);
        g_host->request_update(true);
    }
}

static void app_exit(void) {
}

static void app_destroy(void) {
}

// Plugin Descriptor
static const StickyAppPlugin s_dice_plugin = {
    .api_version = STICKY_APP_API_VERSION,
    .app_name = "Dice Roller",
    .app_version = "1.0.0",
    .app_author = "Community",
    .icon_name = "dice",
    .on_init = app_init,
    .on_enter = app_enter,
    .on_loop = app_loop,
    .on_render = app_render,
    .on_touch = app_touch,
    .on_exit = app_exit,
    .on_destroy = app_destroy,
};

// Exported symbol for dynamic loading
#ifdef __cplusplus
extern "C" {
#endif

__attribute__((visibility("default")))
const StickyAppPlugin* sticky_app_get_plugin(void) {
    return &s_dice_plugin;
}

// Optional entry point if invoked via esp_elf_request
__attribute__((visibility("default")))
const StickyAppPlugin* main_entry(const StickyHostApi* host) {
    app_init(host);
    return &s_dice_plugin;
}

#ifdef __cplusplus
}
#endif
