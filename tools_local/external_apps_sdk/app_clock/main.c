#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "sticky_app_api.h"
#include "mini_format.h"

static const StickyHostApi* g_host = NULL;

typedef enum {
    TAB_CLOCK = 0,
    TAB_STOPWATCH = 1
} ClockTab;

static ClockTab g_tab = TAB_CLOCK;

// Stopwatch State
static bool g_sw_running = false;
static uint32_t g_sw_start_ms = 0;
static uint32_t g_sw_elapsed_ms = 0;
static uint32_t g_sw_lap_ms = 0;

static void app_init(const StickyHostApi* host) {
    g_host = host;
}

static void app_enter(void) {
    if (g_host) {
        g_host->request_update(true);
    }
}

static void app_loop(void) {
    if (!g_host) return;

    if (g_tab == TAB_STOPWATCH && g_sw_running) {
        uint32_t now = g_host->get_millis();
        g_sw_elapsed_ms += (now - g_sw_start_ms);
        g_sw_start_ms = now;
        // Periodic update every ~200ms
        static uint32_t last_update = 0;
        if (now - last_update >= 300) {
            last_update = now;
            g_host->request_update(true);
        }
    }
}

static void app_render(void) {
    if (!g_host) return;

    g_host->clear(STICKY_COLOR_WHITE);
    g_host->draw_header("CLOCK & STOPWATCH");

    int sw = g_host->get_screen_width();
    int cx = sw / 2;

    // Mode Tabs
    int tab_w = 170, tab_h = 38, tab_y = 65;
    int t1_x = cx - tab_w - 10;
    int t2_x = cx + 10;

    // Tab 1: CLOCK
    if (g_tab == TAB_CLOCK) {
        g_host->fill_rect(t1_x, tab_y, tab_w, tab_h, STICKY_COLOR_BLACK);
    } else {
        g_host->draw_rect(t1_x, tab_y, tab_w, tab_h, STICKY_COLOR_BLACK);
    }
    int16_t w1 = 0, h1 = 0;
    g_host->get_text_bounds("DIGITAL CLOCK", 0, &w1, &h1);
    g_host->draw_text(t1_x + (tab_w - w1) / 2, tab_y + (tab_h - h1) / 2, "DIGITAL CLOCK", 0,
                      g_tab == TAB_CLOCK ? STICKY_COLOR_WHITE : STICKY_COLOR_BLACK);

    // Tab 2: STOPWATCH
    if (g_tab == TAB_STOPWATCH) {
        g_host->fill_rect(t2_x, tab_y, tab_w, tab_h, STICKY_COLOR_BLACK);
    } else {
        g_host->draw_rect(t2_x, tab_y, tab_w, tab_h, STICKY_COLOR_BLACK);
    }
    int16_t w2 = 0, h2 = 0;
    g_host->get_text_bounds("STOPWATCH", 0, &w2, &h2);
    g_host->draw_text(t2_x + (tab_w - w2) / 2, tab_y + (tab_h - h2) / 2, "STOPWATCH", 0,
                      g_tab == TAB_STOPWATCH ? STICKY_COLOR_WHITE : STICKY_COLOR_BLACK);

    if (g_tab == TAB_CLOCK) {
        // Digital Clock Frame
        int box_w = 380, box_h = 200;
        int box_x = (sw - box_w) / 2;
        int box_y = 140;

        g_host->draw_rect(box_x, box_y, box_w, box_h, STICKY_COLOR_BLACK);
        g_host->draw_rect(box_x + 1, box_y + 1, box_w - 2, box_h - 2, STICKY_COLOR_BLACK);

        uint32_t total_sec = g_host->get_millis() / 1000;
        int hours = (total_sec / 3600) % 24;
        int mins = (total_sec / 60) % 60;
        int secs = total_sec % 60;

        char time_buf[32];
        char h_buf[4], m_buf[4], s_buf[4];
        mini_format_2d(hours, h_buf);
        mini_format_2d(mins, m_buf);
        mini_format_2d(secs, s_buf);
        mini_strcpy(time_buf, h_buf);
        mini_strcat(time_buf, ":");
        mini_strcat(time_buf, m_buf);
        mini_strcat(time_buf, ":");
        mini_strcat(time_buf, s_buf);

        int16_t tw = 0, th = 0;
        g_host->get_text_bounds(time_buf, 0, &tw, &th);
        g_host->draw_text(cx - tw / 2, box_y + 60, time_buf, 0, STICKY_COLOR_BLACK);

        // Date indicator
        const char* date_info = "Device Local Hardware Timer";
        int16_t dw = 0, dh = 0;
        g_host->get_text_bounds(date_info, 0, &dw, &dh);
        g_host->draw_text(cx - dw / 2, box_y + 130, date_info, 0, STICKY_COLOR_DARKGRAY);

        // Refresh hint
        int ref_w = 200, ref_h = 44;
        int ref_x = cx - ref_w / 2;
        int ref_y = box_y + box_h + 40;
        g_host->draw_rect(ref_x, ref_y, ref_w, ref_h, STICKY_COLOR_BLACK);
        int16_t rw = 0, rh = 0;
        g_host->get_text_bounds("REFRESH TIME", 0, &rw, &rh);
        g_host->draw_text(ref_x + (ref_w - rw) / 2, ref_y + (ref_h - rh) / 2, "REFRESH TIME", 0, STICKY_COLOR_BLACK);

    } else {
        // Stopwatch Frame
        int box_w = 380, box_h = 160;
        int box_x = (sw - box_w) / 2;
        int box_y = 140;

        g_host->draw_rect(box_x, box_y, box_w, box_h, STICKY_COLOR_BLACK);
        g_host->draw_rect(box_x + 1, box_y + 1, box_w - 2, box_h - 2, STICKY_COLOR_BLACK);

        uint32_t total_sec = g_sw_elapsed_ms / 1000;
        int ms_tens = (g_sw_elapsed_ms % 1000) / 100;
        int mins = total_sec / 60;
        int secs = total_sec % 60;

        char sw_buf[32];
        char sm_buf[4], ss_buf[4], ms_buf[4];
        mini_format_2d(mins, sm_buf);
        mini_format_2d(secs, ss_buf);
        mini_itoa(ms_tens, ms_buf);
        mini_strcpy(sw_buf, sm_buf);
        mini_strcat(sw_buf, ":");
        mini_strcat(sw_buf, ss_buf);
        mini_strcat(sw_buf, ".");
        mini_strcat(sw_buf, ms_buf);

        int16_t tw = 0, th = 0;
        g_host->get_text_bounds(sw_buf, 0, &tw, &th);
        g_host->draw_text(cx - tw / 2, box_y + (box_h - th) / 2, sw_buf, 0, STICKY_COLOR_BLACK);

        // Lap Display
        if (g_sw_lap_ms > 0) {
            char lap_buf[32];
            uint32_t l_sec = g_sw_lap_ms / 1000;
            char lm_buf[4], ls_buf[4], lms_buf[4];
            mini_format_2d(l_sec / 60, lm_buf);
            mini_format_2d(l_sec % 60, ls_buf);
            mini_itoa((g_sw_lap_ms % 1000) / 100, lms_buf);
            mini_strcpy(lap_buf, "LAP: ");
            mini_strcat(lap_buf, lm_buf);
            mini_strcat(lap_buf, ":");
            mini_strcat(lap_buf, ls_buf);
            mini_strcat(lap_buf, ".");
            mini_strcat(lap_buf, lms_buf);

            int16_t lw = 0, lh = 0;
            g_host->get_text_bounds(lap_buf, 0, &lw, &lh);
            g_host->draw_text(cx - lw / 2, box_y + box_h + 20, lap_buf, 0, STICKY_COLOR_BLACK);
        }

        // Controls
        int btn_w = 110, btn_h = 50;
        int btn_y = box_y + box_h + 60;

        // START / STOP
        int b1_x = cx - 170;
        g_host->fill_rect(b1_x, btn_y, btn_w, btn_h, STICKY_COLOR_BLACK);
        const char* s_label = g_sw_running ? "STOP" : "START";
        int16_t bw1 = 0, bh1 = 0;
        g_host->get_text_bounds(s_label, 0, &bw1, &bh1);
        g_host->draw_text(b1_x + (btn_w - bw1) / 2, btn_y + (btn_h - bh1) / 2, s_label, 0, STICKY_COLOR_WHITE);

        // LAP
        int b2_x = cx - 55;
        g_host->draw_rect(b2_x, btn_y, btn_w, btn_h, STICKY_COLOR_BLACK);
        int16_t bw2 = 0, bh2 = 0;
        g_host->get_text_bounds("LAP", 0, &bw2, &bh2);
        g_host->draw_text(b2_x + (btn_w - bw2) / 2, btn_y + (btn_h - bh2) / 2, "LAP", 0, STICKY_COLOR_BLACK);

        // RESET
        int b3_x = cx + 60;
        g_host->draw_rect(b3_x, btn_y, btn_w, btn_h, STICKY_COLOR_BLACK);
        int16_t bw3 = 0, bh3 = 0;
        g_host->get_text_bounds("RESET", 0, &bw3, &bh3);
        g_host->draw_text(b3_x + (btn_w - bw3) / 2, btn_y + (btn_h - bh3) / 2, "RESET", 0, STICKY_COLOR_BLACK);
    }
}

static void app_touch(int event_type, int16_t x, int16_t y) {
    if (!g_host || event_type != STICKY_TOUCH_CLICK) return;

    int sw = g_host->get_screen_width();
    int cx = sw / 2;

    // Tabs
    int tab_w = 170, tab_h = 38, tab_y = 65;
    int t1_x = cx - tab_w - 10;
    int t2_x = cx + 10;

    if (x >= t1_x && x < t1_x + tab_w && y >= tab_y && y < tab_y + tab_h) {
        g_host->play_haptic(0);
        g_tab = TAB_CLOCK;
        g_host->request_update(true);
        return;
    }
    if (x >= t2_x && x < t2_x + tab_w && y >= tab_y && y < tab_y + tab_h) {
        g_host->play_haptic(0);
        g_tab = TAB_STOPWATCH;
        g_host->request_update(true);
        return;
    }

    if (g_tab == TAB_CLOCK) {
        // Refresh Time
        int ref_w = 200, ref_h = 44;
        int ref_x = cx - ref_w / 2;
        int ref_y = 140 + 200 + 40;
        if (x >= ref_x && x < ref_x + ref_w && y >= ref_y && y < ref_y + ref_h) {
            g_host->play_haptic(0);
            g_host->request_update(true);
        }
    } else {
        int btn_w = 110, btn_h = 50;
        int btn_y = 140 + 160 + 60;
        int b1_x = cx - 170;
        int b2_x = cx - 55;
        int b3_x = cx + 60;

        // START / STOP
        if (x >= b1_x && x < b1_x + btn_w && y >= btn_y && y < btn_y + btn_h) {
            g_host->play_haptic(0);
            g_sw_running = !g_sw_running;
            if (g_sw_running) {
                g_sw_start_ms = g_host->get_millis();
            }
            g_host->request_update(true);
            return;
        }

        // LAP
        if (x >= b2_x && x < b2_x + btn_w && y >= btn_y && y < btn_y + btn_h) {
            g_host->play_haptic(0);
            g_sw_lap_ms = g_sw_elapsed_ms;
            g_host->request_update(true);
            return;
        }

        // RESET
        if (x >= b3_x && x < b3_x + btn_w && y >= btn_y && y < btn_y + btn_h) {
            g_host->play_haptic(0);
            g_sw_running = false;
            g_sw_elapsed_ms = 0;
            g_sw_lap_ms = 0;
            g_host->request_update(true);
            return;
        }
    }
}

static void app_exit(void) {
    g_sw_running = false;
}

static void app_destroy(void) {}

static const StickyAppPlugin g_plugin = {
    .api_version = STICKY_APP_API_VERSION,
    .app_name = "Clock",
    .app_version = "1.0.0",
    .app_author = "Sticky",
    .icon_name = "clock",
    .on_init = app_init,
    .on_enter = app_enter,
    .on_loop = app_loop,
    .on_render = app_render,
    .on_touch = app_touch,
    .on_exit = app_exit,
    .on_destroy = app_destroy,
};

const StickyAppPlugin* sticky_app_entry(void) {
    return &g_plugin;
}
