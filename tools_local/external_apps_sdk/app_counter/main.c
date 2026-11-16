#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "sticky_app_api.h"

static const StickyHostApi* g_host = NULL;
static int32_t g_count = 0;

static void uint_to_str(int32_t val, char* buf) {
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    bool neg = false;
    if (val < 0) {
        neg = true;
        val = -val;
    }
    char tmp[16];
    int i = 0;
    while (val > 0) {
        tmp[i++] = '0' + (val % 10);
        val /= 10;
    }
    int j = 0;
    if (neg) buf[j++] = '-';
    while (i > 0) {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
}

static void app_init(const StickyHostApi* host) {
    g_host = host;
}

static void app_enter(void) {
    if (g_host) {
        g_host->request_update(true);
    }
}

static void app_loop(void) {}

static void app_render(void) {
    if (!g_host) return;

    g_host->clear(STICKY_COLOR_WHITE);
    g_host->draw_header("COUNTER");

    int screen_w = g_host->get_screen_width();
    int screen_h = g_host->get_screen_height();
    int cx = screen_w / 2;

    // Center Display Count Box
    int box_w = 340, box_h = 160;
    int box_x = (screen_w - box_w) / 2;
    int box_y = 150;

    g_host->fill_rect(box_x, box_y, box_w, box_h, STICKY_COLOR_WHITE);
    g_host->draw_rect(box_x, box_y, box_w, box_h, STICKY_COLOR_BLACK);
    g_host->draw_rect(box_x + 1, box_y + 1, box_w - 2, box_h - 2, STICKY_COLOR_BLACK);
    g_host->draw_rect(box_x + 2, box_y + 2, box_w - 4, box_h - 4, STICKY_COLOR_BLACK);

    char num_str[32];
    uint_to_str(g_count, num_str);
    int16_t tw = 0, th = 0;
    g_host->get_text_bounds(num_str, 0, &tw, &th);
    g_host->draw_text(cx - tw / 2, box_y + (box_h - th) / 2, num_str, 0, STICKY_COLOR_BLACK);

    // Large PLUS Button (+1)
    int btn_w = 160, btn_h = 100;
    int plus_x = cx + 10;
    int plus_y = box_y + box_h + 40;
    g_host->fill_rect(plus_x, plus_y, btn_w, btn_h, STICKY_COLOR_BLACK);
    int16_t pw = 0, ph = 0;
    g_host->get_text_bounds("+1", 0, &pw, &ph);
    g_host->draw_text(plus_x + (btn_w - pw) / 2, plus_y + (btn_h - ph) / 2, "+1", 0, STICKY_COLOR_WHITE);

    // Large MINUS Button (-1)
    int minus_x = cx - 10 - btn_w;
    int minus_y = plus_y;
    g_host->fill_rect(minus_x, minus_y, btn_w, btn_h, STICKY_COLOR_WHITE);
    g_host->draw_rect(minus_x, minus_y, btn_w, btn_h, STICKY_COLOR_BLACK);
    g_host->draw_rect(minus_x + 1, minus_y + 1, btn_w - 2, btn_h - 2, STICKY_COLOR_BLACK);
    int16_t mw = 0, mh = 0;
    g_host->get_text_bounds("-1", 0, &mw, &mh);
    g_host->draw_text(minus_x + (btn_w - mw) / 2, minus_y + (btn_h - mh) / 2, "-1", 0, STICKY_COLOR_BLACK);

    // RESET Button
    int rbtn_w = 200, rbtn_h = 50;
    int rx = cx - rbtn_w / 2;
    int ry = plus_y + btn_h + 40;
    g_host->draw_rect(rx, ry, rbtn_w, rbtn_h, STICKY_COLOR_BLACK);
    int16_t rw = 0, rh = 0;
    g_host->get_text_bounds("RESET", 0, &rw, &rh);
    g_host->draw_text(rx + (rbtn_w - rw) / 2, ry + (rbtn_h - rh) / 2, "RESET", 0, STICKY_COLOR_BLACK);
}

static void app_touch(int event_type, int16_t x, int16_t y) {
    if (!g_host) return;

    int screen_w = g_host->get_screen_width();
    int cx = screen_w / 2;
    int box_y = 150, box_h = 160;
    int btn_w = 160, btn_h = 100;
    int plus_x = cx + 10;
    int plus_y = box_y + box_h + 40;
    int minus_x = cx - 10 - btn_w;
    int minus_y = plus_y;
    int rbtn_w = 200, rbtn_h = 50;
    int rx = cx - rbtn_w / 2;
    int ry = plus_y + btn_h + 40;

    if (event_type == STICKY_TOUCH_CLICK) {
        // +1
        if (x >= plus_x && x <= plus_x + btn_w && y >= plus_y && y <= plus_y + btn_h) {
            g_count++;
            g_host->play_haptic(1);
            g_host->request_update(true);
            return;
        }

        // -1
        if (x >= minus_x && x <= minus_x + btn_w && y >= minus_y && y <= minus_y + btn_h) {
            g_count--;
            g_host->play_haptic(1);
            g_host->request_update(true);
            return;
        }

        // RESET
        if (x >= rx && x <= rx + rbtn_w && y >= ry && y <= ry + rbtn_h) {
            g_count = 0;
            g_host->play_haptic(1);
            g_host->request_update(true);
            return;
        }
    }
}

static void app_exit(void) {}
static void app_destroy(void) {}

// ----------------- Plugin Definition -----------------

static const StickyAppPlugin s_counter_plugin = {
    .api_version = STICKY_APP_API_VERSION,
    .app_name = "Counter",
    .app_version = "1.0.0",
    .app_author = "CrossNext",
    .icon_name = "calculator",
    .on_init = app_init,
    .on_enter = app_enter,
    .on_loop = app_loop,
    .on_render = app_render,
    .on_touch = app_touch,
    .on_exit = app_exit,
    .on_destroy = app_destroy,
};

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((visibility("default")))
const StickyAppPlugin* sticky_app_get_plugin(void) {
    return &s_counter_plugin;
}

__attribute__((visibility("default")))
const StickyAppPlugin* main_entry(const StickyHostApi* host) {
    app_init(host);
    return &s_counter_plugin;
}

#ifdef __cplusplus
}
#endif
