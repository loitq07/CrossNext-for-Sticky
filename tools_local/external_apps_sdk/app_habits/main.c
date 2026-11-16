#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "sticky_app_api.h"
#include "mini_format.h"

static const StickyHostApi* g_host = NULL;

typedef struct {
    char name[48];
    bool completed;
    int streak;
} Habit;

#define MAX_HABITS 6
static Habit g_habits[MAX_HABITS];
static int g_habit_count = 5;

static void init_default_habits(void) {
    strncpy(g_habits[0].name, "Read 20 pages of book", sizeof(g_habits[0].name));
    g_habits[0].completed = false;
    g_habits[0].streak = 5;

    strncpy(g_habits[1].name, "Morning exercise 20m", sizeof(g_habits[1].name));
    g_habits[1].completed = true;
    g_habits[1].streak = 12;

    strncpy(g_habits[2].name, "Drink 2L fresh water", sizeof(g_habits[2].name));
    g_habits[2].completed = false;
    g_habits[2].streak = 8;

    strncpy(g_habits[3].name, "Study English / Code", sizeof(g_habits[3].name));
    g_habits[3].completed = true;
    g_habits[3].streak = 19;

    strncpy(g_habits[4].name, "Sleep before 11:00 PM", sizeof(g_habits[4].name));
    g_habits[4].completed = false;
    g_habits[4].streak = 4;
}

static void save_habits(void) {
    if (g_host) {
        g_host->write_file("/apps/habits.dat", g_habits, sizeof(g_habits));
    }
}

static void load_habits(void) {
    if (g_host && g_host->file_exists("/apps/habits.dat")) {
        uint32_t len = 0;
        g_host->read_file("/apps/habits.dat", g_habits, sizeof(g_habits), &len);
    } else {
        init_default_habits();
    }
}

static void app_init(const StickyHostApi* host) {
    g_host = host;
    load_habits();
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
    g_host->draw_header("HABITS TRACKER");

    int sw = g_host->get_screen_width();

    // Summary Card
    int done_count = 0;
    for (int i = 0; i < g_habit_count; ++i) {
        if (g_habits[i].completed) done_count++;
    }

    int cy = 60;
    g_host->draw_rect(20, cy, sw - 40, 65, STICKY_COLOR_BLACK);
    g_host->draw_rect(21, cy + 1, sw - 42, 63, STICKY_COLOR_BLACK);

    char sum_buf[64];
    char d_buf[8], c_buf[8];
    mini_itoa(done_count, d_buf);
    mini_itoa(g_habit_count, c_buf);
    mini_strcpy(sum_buf, "TODAY'S PROGRESS: ");
    mini_strcat(sum_buf, d_buf);
    mini_strcat(sum_buf, " / ");
    mini_strcat(sum_buf, c_buf);
    mini_strcat(sum_buf, " COMPLETED");
    g_host->draw_text(35, cy + 14, sum_buf, 0, STICKY_COLOR_BLACK);

    // Progress bar inside summary
    int bar_w = sw - 80;
    g_host->draw_rect(35, cy + 42, bar_w, 10, STICKY_COLOR_BLACK);
    if (g_habit_count > 0 && done_count > 0) {
        int fill = done_count * bar_w / g_habit_count;
        g_host->fill_rect(35, cy + 42, fill, 10, STICKY_COLOR_BLACK);
    }

    // Habit Rows
    int start_y = 140;
    int row_h = 76;

    for (int i = 0; i < g_habit_count; ++i) {
        int ry = start_y + i * (row_h + 10);

        // Row box
        g_host->draw_rect(20, ry, sw - 40, row_h, STICKY_COLOR_BLACK);

        // Checkbox button
        int cb_size = 36;
        int cb_x = 35;
        int cb_y = ry + (row_h - cb_size) / 2;

        g_host->draw_rect(cb_x, cb_y, cb_size, cb_size, STICKY_COLOR_BLACK);
        g_host->draw_rect(cb_x + 1, cb_y + 1, cb_size - 2, cb_size - 2, STICKY_COLOR_BLACK);

        if (g_habits[i].completed) {
            g_host->fill_rect(cb_x + 4, cb_y + 4, cb_size - 8, cb_size - 8, STICKY_COLOR_BLACK);
        }

        // Habit Name
        g_host->draw_text(cb_x + cb_size + 16, ry + 16, g_habits[i].name, 0, STICKY_COLOR_BLACK);

        // Streak info
        char strk_buf[32];
        char sk_buf[8];
        mini_itoa(g_habits[i].streak, sk_buf);
        mini_strcpy(strk_buf, "Streak: ");
        mini_strcat(strk_buf, sk_buf);
        mini_strcat(strk_buf, " days active");
        g_host->draw_text(cb_x + cb_size + 16, ry + 44, strk_buf, 0, STICKY_COLOR_DARKGRAY);

        // Status badge on right
        if (g_habits[i].completed) {
            int bw = 65, bh = 28;
            int bx = sw - 35 - bw;
            int by = ry + (row_h - bh) / 2;
            g_host->fill_rect(bx, by, bw, bh, STICKY_COLOR_BLACK);
            int16_t dw = 0, dh = 0;
            g_host->get_text_bounds("DONE", 0, &dw, &dh);
            g_host->draw_text(bx + (bw - dw) / 2, by + (bh - dh) / 2, "DONE", 0, STICKY_COLOR_WHITE);
        }
    }

    // Reset Day Checkmarks button at bottom
    int reset_w = 220, reset_h = 44;
    int reset_x = (sw - reset_w) / 2;
    int reset_y = start_y + g_habit_count * (row_h + 10) + 15;
    g_host->draw_rect(reset_x, reset_y, reset_w, reset_h, STICKY_COLOR_BLACK);
    int16_t rtw = 0, rth = 0;
    g_host->get_text_bounds("RESET TODAY MARKS", 0, &rtw, &rth);
    g_host->draw_text(reset_x + (reset_w - rtw) / 2, reset_y + (reset_h - rth) / 2, "RESET TODAY MARKS", 0,
                      STICKY_COLOR_BLACK);
}

static void app_touch(int event_type, int16_t x, int16_t y) {
    if (!g_host || event_type != STICKY_TOUCH_CLICK) return;

    int sw = g_host->get_screen_width();
    int start_y = 140;
    int row_h = 76;

    // Check row clicks
    for (int i = 0; i < g_habit_count; ++i) {
        int ry = start_y + i * (row_h + 10);
        if (x >= 20 && x < sw - 20 && y >= ry && y < ry + row_h) {
            g_host->play_haptic(0);
            g_habits[i].completed = !g_habits[i].completed;
            if (g_habits[i].completed) {
                g_habits[i].streak++;
            } else {
                if (g_habits[i].streak > 0) g_habits[i].streak--;
            }
            save_habits();
            g_host->request_update(true);
            return;
        }
    }

    // Reset Marks Button
    int reset_w = 220, reset_h = 44;
    int reset_x = (sw - reset_w) / 2;
    int reset_y = start_y + g_habit_count * (row_h + 10) + 15;
    if (x >= reset_x && x < reset_x + reset_w && y >= reset_y && y < reset_y + reset_h) {
        g_host->play_haptic(0);
        for (int i = 0; i < g_habit_count; ++i) {
            g_habits[i].completed = false;
        }
        save_habits();
        g_host->request_update(true);
        return;
    }
}

static void app_exit(void) {
    save_habits();
}

static void app_destroy(void) {}

static const StickyAppPlugin g_plugin = {
    .api_version = STICKY_APP_API_VERSION,
    .app_name = "Habits",
    .app_version = "1.0.0",
    .app_author = "Sticky",
    .icon_name = "habits",
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
