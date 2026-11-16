#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "sticky_app_api.h"
#include "mini_format.h"

static const StickyHostApi* g_host = NULL;

static int g_year = 2026;
static int g_month = 11; // 1-12
static int g_selected_day = 16;

static const char* kMonthNames[] = {
    "JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE",
    "JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"
};

static bool is_leap_year(int y) {
    return ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0));
}

static int days_in_month(int y, int m) {
    static const int kDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m == 2 && is_leap_year(y)) return 29;
    if (m >= 1 && m <= 12) return kDays[m - 1];
    return 30;
}

// 0 = Sunday, 1 = Monday, ..., 6 = Saturday
static int day_of_week(int y, int m, int d) {
    if (m < 3) {
        m += 12;
        y -= 1;
    }
    int k = y % 100;
    int j = y / 100;
    int f = d + 13 * (m + 1) / 5 + k + k / 4 + j / 4 + 5 * j;
    int dow = (f + 6) % 7; // Convert Zeller: 0=Sunday
    return dow;
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
    g_host->draw_header("CALENDAR");

    int sw = g_host->get_screen_width();
    int cx = sw / 2;

    // Month Navigation Header: [<] NOVEMBER 2026 [>]
    int nav_y = 65;
    int btn_w = 48, btn_h = 40;

    // Prev Button [<]
    int prev_x = 30;
    g_host->draw_rect(prev_x, nav_y, btn_w, btn_h, STICKY_COLOR_BLACK);
    int16_t bw1 = 0, bh1 = 0;
    g_host->get_text_bounds("<", 0, &bw1, &bh1);
    g_host->draw_text(prev_x + (btn_w - bw1) / 2, nav_y + (btn_h - bh1) / 2, "<", 0, STICKY_COLOR_BLACK);

    // Title: e.g. "NOVEMBER 2026"
    char title[32];
    char yr_buf[8];
    mini_itoa(g_year, yr_buf);
    mini_strcpy(title, kMonthNames[g_month - 1]);
    mini_strcat(title, " ");
    mini_strcat(title, yr_buf);
    int16_t tw = 0, th = 0;
    g_host->get_text_bounds(title, 0, &tw, &th);
    g_host->draw_text(cx - tw / 2, nav_y + (btn_h - th) / 2, title, 0, STICKY_COLOR_BLACK);

    // Next Button [>]
    int next_x = sw - 30 - btn_w;
    g_host->draw_rect(next_x, nav_y, btn_w, btn_h, STICKY_COLOR_BLACK);
    int16_t bw2 = 0, bh2 = 0;
    g_host->get_text_bounds(">", 0, &bw2, &bh2);
    g_host->draw_text(next_x + (btn_w - bw2) / 2, nav_y + (btn_h - bh2) / 2, ">", 0, STICKY_COLOR_BLACK);

    // Days of week header
    static const char* kDayHeaders[] = {"SU", "MO", "TU", "WE", "TH", "FR", "SA"};
    int grid_top = 125;
    int col_w = (sw - 40) / 7; // ~62px
    int start_x = 20;

    for (int col = 0; col < 7; ++col) {
        int x = start_x + col * col_w;
        int16_t hw = 0, hh = 0;
        g_host->get_text_bounds(kDayHeaders[col], 0, &hw, &hh);
        g_host->draw_text(x + (col_w - hw) / 2, grid_top, kDayHeaders[col], 0,
                          (col == 0 || col == 6) ? STICKY_COLOR_DARKGRAY : STICKY_COLOR_BLACK);
    }

    g_host->draw_line(20, grid_top + 28, sw - 20, grid_top + 28, STICKY_COLOR_BLACK);

    // Grid Calendar
    int first_dow = day_of_week(g_year, g_month, 1);
    int total_days = days_in_month(g_year, g_month);

    int row_h = 58;
    int cell_y_start = grid_top + 35;

    for (int day = 1; day <= total_days; ++day) {
        int pos = first_dow + day - 1;
        int col = pos % 7;
        int row = pos / 7;

        int cell_x = start_x + col * col_w;
        int cell_y = cell_y_start + row * row_h;

        bool is_sel = (day == g_selected_day);
        if (is_sel) {
            int sel_pad = 4;
            g_host->fill_rect(cell_x + sel_pad, cell_y + sel_pad, col_w - sel_pad * 2, row_h - sel_pad * 2,
                              STICKY_COLOR_BLACK);
        }

        char day_str[8];
        mini_itoa(day, day_str);
        int16_t dw = 0, dh = 0;
        g_host->get_text_bounds(day_str, 0, &dw, &dh);
        g_host->draw_text(cell_x + (col_w - dw) / 2, cell_y + (row_h - dh) / 2, day_str, 0,
                          is_sel ? STICKY_COLOR_WHITE : STICKY_COLOR_BLACK);
    }

    // Bottom Selected Day details card
    int card_y = cell_y_start + 6 * row_h + 10;
    int card_w = sw - 40;
    g_host->draw_rect(20, card_y, card_w, 75, STICKY_COLOR_BLACK);
    g_host->draw_rect(21, card_y + 1, card_w - 2, 73, STICKY_COLOR_BLACK);

    char detail_buf[64];
    char sd_buf[4], sm_buf[4], sy_buf[8];
    mini_format_2d(g_selected_day, sd_buf);
    mini_format_2d(g_month, sm_buf);
    mini_format_4d(g_year, sy_buf);
    mini_strcpy(detail_buf, "SELECTED: ");
    mini_strcat(detail_buf, sd_buf);
    mini_strcat(detail_buf, " / ");
    mini_strcat(detail_buf, sm_buf);
    mini_strcat(detail_buf, " / ");
    mini_strcat(detail_buf, sy_buf);
    g_host->draw_text(35, card_y + 16, detail_buf, 0, STICKY_COLOR_BLACK);
    g_host->draw_text(35, card_y + 44, "Tap any date to select or plan activities.", 0, STICKY_COLOR_DARKGRAY);
}

static void app_touch(int event_type, int16_t x, int16_t y) {
    if (!g_host || event_type != STICKY_TOUCH_CLICK) return;

    int sw = g_host->get_screen_width();
    int nav_y = 65;
    int btn_w = 48, btn_h = 40;

    // Prev Month [<]
    if (x >= 30 && x < 30 + btn_w && y >= nav_y && y < nav_y + btn_h) {
        g_host->play_haptic(0);
        g_month--;
        if (g_month < 1) {
            g_month = 12;
            g_year--;
        }
        int max_d = days_in_month(g_year, g_month);
        if (g_selected_day > max_d) g_selected_day = max_d;
        g_host->request_update(true);
        return;
    }

    // Next Month [>]
    int next_x = sw - 30 - btn_w;
    if (x >= next_x && x < next_x + btn_w && y >= nav_y && y < nav_y + btn_h) {
        g_host->play_haptic(0);
        g_month++;
        if (g_month > 12) {
            g_month = 1;
            g_year++;
        }
        int max_d = days_in_month(g_year, g_month);
        if (g_selected_day > max_d) g_selected_day = max_d;
        g_host->request_update(true);
        return;
    }

    // Day Selection in Grid
    int grid_top = 125;
    int col_w = (sw - 40) / 7;
    int start_x = 20;
    int row_h = 58;
    int cell_y_start = grid_top + 35;

    if (x >= start_x && x < start_x + 7 * col_w && y >= cell_y_start && y < cell_y_start + 6 * row_h) {
        int col = (x - start_x) / col_w;
        int row = (y - cell_y_start) / row_h;
        int pos = row * 7 + col;
        int first_dow = day_of_week(g_year, g_month, 1);
        int day = pos - first_dow + 1;
        int total_days = days_in_month(g_year, g_month);

        if (day >= 1 && day <= total_days) {
            g_host->play_haptic(0);
            g_selected_day = day;
            g_host->request_update(true);
            return;
        }
    }
}

static void app_exit(void) {}
static void app_destroy(void) {}

static const StickyAppPlugin g_plugin = {
    .api_version = STICKY_APP_API_VERSION,
    .app_name = "Calendar",
    .app_version = "1.0.0",
    .app_author = "Sticky",
    .icon_name = "calendar",
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
