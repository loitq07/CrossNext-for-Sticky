#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "sticky_app_api.h"
#include "mini_format.h"

static const StickyHostApi* g_host = NULL;

typedef enum {
    MODE_FOCUS = 0,
    MODE_SHORT_BREAK = 1,
    MODE_LONG_BREAK = 2
} PomodoroMode;

static PomodoroMode g_mode = MODE_FOCUS;
static bool g_running = false;
static int32_t g_seconds_left = 25 * 60;
static int32_t g_total_seconds = 25 * 60;
static uint32_t g_last_tick = 0;
static int g_round = 1;
static const int g_max_rounds = 4;

static void set_mode(PomodoroMode mode) {
    g_mode = mode;
    g_running = false;
    if (mode == MODE_FOCUS) {
        g_total_seconds = 25 * 60;
    } else if (mode == MODE_SHORT_BREAK) {
        g_total_seconds = 5 * 60;
    } else {
        g_total_seconds = 15 * 60;
    }
    g_seconds_left = g_total_seconds;
}

static void app_init(const StickyHostApi* host) {
    g_host = host;
    set_mode(MODE_FOCUS);
}

static void app_enter(void) {
    if (g_host) {
        g_last_tick = g_host->get_millis();
        g_host->request_update(true);
    }
}

static void app_loop(void) {
    if (!g_host || !g_running) return;

    uint32_t now = g_host->get_millis();
    if (now - g_last_tick >= 1000) {
        uint32_t elapsed_secs = (now - g_last_tick) / 1000;
        g_last_tick += elapsed_secs * 1000;

        if (g_seconds_left > (int32_t)elapsed_secs) {
            g_seconds_left -= elapsed_secs;
        } else {
            g_seconds_left = 0;
            g_running = false;
            g_host->play_haptic(1);
            if (g_mode == MODE_FOCUS) {
                if (g_round < g_max_rounds) {
                    set_mode(MODE_SHORT_BREAK);
                } else {
                    set_mode(MODE_LONG_BREAK);
                }
            } else {
                if (g_mode == MODE_LONG_BREAK) {
                    g_round = 1;
                } else {
                    g_round++;
                }
                set_mode(MODE_FOCUS);
            }
        }
        g_host->request_update(true);
    }
}

static void app_render(void) {
    if (!g_host) return;

    g_host->clear(STICKY_COLOR_WHITE);
    g_host->draw_header("POMODORO TIMER");

    int sw = g_host->get_screen_width();
    int cx = sw / 2;

    // Mode Selector Pills
    int tab_w = 130, tab_h = 38, tab_y = 65;
    const char* tabs[3] = {"FOCUS", "SHORT", "LONG"};
    int start_x = (sw - (tab_w * 3 + 20)) / 2;

    for (int i = 0; i < 3; ++i) {
        int tx = start_x + i * (tab_w + 10);
        bool sel = (g_mode == (PomodoroMode)i);
        if (sel) {
            g_host->fill_rect(tx, tab_y, tab_w, tab_h, STICKY_COLOR_BLACK);
        } else {
            g_host->draw_rect(tx, tab_y, tab_w, tab_h, STICKY_COLOR_BLACK);
        }
        int16_t tw = 0, th = 0;
        g_host->get_text_bounds(tabs[i], 0, &tw, &th);
        g_host->draw_text(tx + (tab_w - tw) / 2, tab_y + (tab_h - th) / 2, tabs[i], 0,
                          sel ? STICKY_COLOR_WHITE : STICKY_COLOR_BLACK);
    }

    // Rounds counter
    char round_str[32];
    char r_num[8];
    mini_itoa(g_round, r_num);
    mini_strcpy(round_str, "ROUND ");
    mini_strcat(round_str, r_num);
    mini_strcat(round_str, " / 4");
    int16_t rw = 0, rh = 0;
    g_host->get_text_bounds(round_str, 0, &rw, &rh);
    g_host->draw_text(cx - rw / 2, 125, round_str, 0, STICKY_COLOR_BLACK);

    // Big Timer Box
    int box_w = 380, box_h = 160;
    int box_x = (sw - box_w) / 2;
    int box_y = 160;

    g_host->draw_rect(box_x, box_y, box_w, box_h, STICKY_COLOR_BLACK);
    g_host->draw_rect(box_x + 1, box_y + 1, box_w - 2, box_h - 2, STICKY_COLOR_BLACK);

    char time_str[16];
    int mins = g_seconds_left / 60;
    int secs = g_seconds_left % 60;
    char m_buf[4], s_buf[4];
    mini_format_2d(mins, m_buf);
    mini_format_2d(secs, s_buf);
    mini_strcpy(time_str, m_buf);
    mini_strcat(time_str, ":");
    mini_strcat(time_str, s_buf);

    int16_t mw = 0, mh = 0;
    g_host->get_text_bounds(time_str, 0, &mw, &mh);
    g_host->draw_text(cx - mw / 2, box_y + (box_h - mh) / 2 - 10, time_str, 0, STICKY_COLOR_BLACK);

    // Progress Bar
    int bar_x = box_x + 20;
    int bar_y = box_y + box_h - 30;
    int bar_w = box_w - 40;
    int bar_h = 14;
    g_host->draw_rect(bar_x, bar_y, bar_w, bar_h, STICKY_COLOR_BLACK);
    if (g_total_seconds > 0) {
        int progress = (g_total_seconds - g_seconds_left) * bar_w / g_total_seconds;
        if (progress > bar_w) progress = bar_w;
        if (progress > 0) {
            g_host->fill_rect(bar_x, bar_y, progress, bar_h, STICKY_COLOR_BLACK);
        }
    }

    // Controls
    // START / PAUSE Button
    int btn_w = 170, btn_h = 60;
    int btn1_x = cx - btn_w - 10;
    int btn_y = 350;

    g_host->fill_rect(btn1_x, btn_y, btn_w, btn_h, STICKY_COLOR_BLACK);
    const char* label = g_running ? "PAUSE" : "START";
    int16_t lw = 0, lh = 0;
    g_host->get_text_bounds(label, 0, &lw, &lh);
    g_host->draw_text(btn1_x + (btn_w - lw) / 2, btn_y + (btn_h - lh) / 2, label, 0, STICKY_COLOR_WHITE);

    // RESET Button
    int btn2_x = cx + 10;
    g_host->draw_rect(btn2_x, btn_y, btn_w, btn_h, STICKY_COLOR_BLACK);
    g_host->draw_rect(btn2_x + 1, btn_y + 1, btn_w - 2, btn_h - 2, STICKY_COLOR_BLACK);
    int16_t rew = 0, reh = 0;
    g_host->get_text_bounds("RESET", 0, &rew, &reh);
    g_host->draw_text(btn2_x + (btn_w - rew) / 2, btn_y + (btn_h - reh) / 2, "RESET", 0, STICKY_COLOR_BLACK);

    // SKIP Button
    int skip_w = 200, skip_h = 44;
    int skip_x = cx - skip_w / 2;
    int skip_y = btn_y + btn_h + 25;
    g_host->draw_rect(skip_x, skip_y, skip_w, skip_h, STICKY_COLOR_BLACK);
    int16_t skw = 0, skh = 0;
    g_host->get_text_bounds("SKIP STAGE >", 0, &skw, &skh);
    g_host->draw_text(skip_x + (skip_w - skw) / 2, skip_y + (skip_h - skh) / 2, "SKIP STAGE >", 0, STICKY_COLOR_BLACK);
}

static void app_touch(int event_type, int16_t x, int16_t y) {
    if (!g_host || event_type != STICKY_TOUCH_CLICK) return;

    int sw = g_host->get_screen_width();
    int cx = sw / 2;

    // Tabs
    int tab_w = 130, tab_h = 38, tab_y = 65;
    int start_x = (sw - (tab_w * 3 + 20)) / 2;
    for (int i = 0; i < 3; ++i) {
        int tx = start_x + i * (tab_w + 10);
        if (x >= tx && x < tx + tab_w && y >= tab_y && y < tab_y + tab_h) {
            g_host->play_haptic(0);
            set_mode((PomodoroMode)i);
            g_host->request_update(true);
            return;
        }
    }

    // START / PAUSE
    int btn_w = 170, btn_h = 60;
    int btn1_x = cx - btn_w - 10;
    int btn_y = 350;
    if (x >= btn1_x && x < btn1_x + btn_w && y >= btn_y && y < btn_y + btn_h) {
        g_host->play_haptic(0);
        g_running = !g_running;
        g_last_tick = g_host->get_millis();
        g_host->request_update(true);
        return;
    }

    // RESET
    int btn2_x = cx + 10;
    if (x >= btn2_x && x < btn2_x + btn_w && y >= btn_y && y < btn_y + btn_h) {
        g_host->play_haptic(0);
        set_mode(g_mode);
        g_host->request_update(true);
        return;
    }

    // SKIP
    int skip_w = 200, skip_h = 44;
    int skip_x = cx - skip_w / 2;
    int skip_y = btn_y + btn_h + 25;
    if (x >= skip_x && x < skip_x + skip_w && y >= skip_y && y < skip_y + skip_h) {
        g_host->play_haptic(0);
        if (g_mode == MODE_FOCUS) {
            set_mode(MODE_SHORT_BREAK);
        } else {
            g_round++;
            set_mode(MODE_FOCUS);
        }
        g_host->request_update(true);
        return;
    }
}

static void app_exit(void) {
    g_running = false;
}

static void app_destroy(void) {}

static const StickyAppPlugin g_plugin = {
    .api_version = STICKY_APP_API_VERSION,
    .app_name = "Pomodoro",
    .app_version = "1.0.0",
    .app_author = "Sticky",
    .icon_name = "pomodoro",
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
