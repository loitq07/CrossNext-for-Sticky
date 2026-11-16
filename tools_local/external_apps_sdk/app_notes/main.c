#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "sticky_app_api.h"

static const StickyHostApi* g_host = NULL;

typedef struct {
    char title[64];
    bool is_todo;
    bool checked;
} NoteItem;

#define MAX_ITEMS 8
static NoteItem g_items[MAX_ITEMS];
static int g_item_count = 6;
static int g_view_mode = 0; // 0 = Todo List, 1 = Quick Notes

static void init_default_notes(void) {
    strncpy(g_items[0].title, "Buy groceries & coffee beans", sizeof(g_items[0].title));
    g_items[0].is_todo = true;
    g_items[0].checked = false;

    strncpy(g_items[1].title, "Review pull requests on GitHub", sizeof(g_items[1].title));
    g_items[1].is_todo = true;
    g_items[1].checked = true;

    strncpy(g_items[2].title, "Sync e-paper reading highlights", sizeof(g_items[2].title));
    g_items[2].is_todo = true;
    g_items[2].checked = false;

    strncpy(g_items[3].title, "Idea: Add weather widget to home", sizeof(g_items[3].title));
    g_items[3].is_todo = false;
    g_items[3].checked = false;

    strncpy(g_items[4].title, "Meeting notes: Q4 device roadmap", sizeof(g_items[4].title));
    g_items[4].is_todo = false;
    g_items[4].checked = false;

    strncpy(g_items[5].title, "Pack charger & USB cable for trip", sizeof(g_items[5].title));
    g_items[5].is_todo = true;
    g_items[5].checked = false;
}

static void save_notes(void) {
    if (g_host) {
        g_host->write_file("/apps/notes.dat", g_items, sizeof(g_items));
    }
}

static void load_notes(void) {
    if (g_host && g_host->file_exists("/apps/notes.dat")) {
        uint32_t len = 0;
        g_host->read_file("/apps/notes.dat", g_items, sizeof(g_items), &len);
    } else {
        init_default_notes();
    }
}

static void app_init(const StickyHostApi* host) {
    g_host = host;
    load_notes();
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
    g_host->draw_header(g_view_mode == 0 ? "TO-DO LIST" : "QUICK NOTES");

    int sw = g_host->get_screen_width();
    int cx = sw / 2;

    // View Tabs
    int tab_w = 170, tab_h = 38, tab_y = 65;
    int t1_x = cx - tab_w - 10;
    int t2_x = cx + 10;

    // Tab 0: To-Do
    if (g_view_mode == 0) {
        g_host->fill_rect(t1_x, tab_y, tab_w, tab_h, STICKY_COLOR_BLACK);
    } else {
        g_host->draw_rect(t1_x, tab_y, tab_w, tab_h, STICKY_COLOR_BLACK);
    }
    int16_t w1 = 0, h1 = 0;
    g_host->get_text_bounds("TO-DO TASKS", 0, &w1, &h1);
    g_host->draw_text(t1_x + (tab_w - w1) / 2, tab_y + (tab_h - h1) / 2, "TO-DO TASKS", 0,
                      g_view_mode == 0 ? STICKY_COLOR_WHITE : STICKY_COLOR_BLACK);

    // Tab 1: Notes
    if (g_view_mode == 1) {
        g_host->fill_rect(t2_x, tab_y, tab_w, tab_h, STICKY_COLOR_BLACK);
    } else {
        g_host->draw_rect(t2_x, tab_y, tab_w, tab_h, STICKY_COLOR_BLACK);
    }
    int16_t w2 = 0, h2 = 0;
    g_host->get_text_bounds("QUICK NOTES", 0, &w2, &h2);
    g_host->draw_text(t2_x + (tab_w - w2) / 2, tab_y + (tab_h - h2) / 2, "QUICK NOTES", 0,
                      g_view_mode == 1 ? STICKY_COLOR_WHITE : STICKY_COLOR_BLACK);

    // List Items
    int start_y = 125;
    int row_h = 68;
    int visible_idx = 0;

    for (int i = 0; i < g_item_count; ++i) {
        if (g_view_mode == 0 && !g_items[i].is_todo) continue;
        if (g_view_mode == 1 && g_items[i].is_todo) continue;

        int ry = start_y + visible_idx * (row_h + 10);
        visible_idx++;

        // Row box
        g_host->draw_rect(20, ry, sw - 40, row_h, STICKY_COLOR_BLACK);

        if (g_view_mode == 0) {
            // Checkbox
            int cb_size = 32;
            int cb_x = 35;
            int cb_y = ry + (row_h - cb_size) / 2;
            g_host->draw_rect(cb_x, cb_y, cb_size, cb_size, STICKY_COLOR_BLACK);
            if (g_items[i].checked) {
                g_host->fill_rect(cb_x + 4, cb_y + 4, cb_size - 8, cb_size - 8, STICKY_COLOR_BLACK);
            }

            int text_x = cb_x + cb_size + 16;
            g_host->draw_text(text_x, ry + 22, g_items[i].title, 0,
                              g_items[i].checked ? STICKY_COLOR_DARKGRAY : STICKY_COLOR_BLACK);

            if (g_items[i].checked) {
                // Strike-through line
                int16_t tw = 0, th = 0;
                g_host->get_text_bounds(g_items[i].title, 0, &tw, &th);
                g_host->draw_line(text_x, ry + 32, text_x + tw, ry + 32, STICKY_COLOR_DARKGRAY);
            }
        } else {
            // Note Bullet
            g_host->fill_circle(45, ry + row_h / 2, 4, STICKY_COLOR_BLACK);
            g_host->draw_text(65, ry + 22, g_items[i].title, 0, STICKY_COLOR_BLACK);
        }
    }

    if (visible_idx == 0) {
        g_host->draw_text(cx - 80, start_y + 100, "(No items found)", 0, STICKY_COLOR_DARKGRAY);
    }

    // Bottom Action Buttons
    int act_y = start_y + 5 * (row_h + 10) + 15;
    int btn_w = 180, btn_h = 44;

    if (g_view_mode == 0) {
        int b1_x = cx - btn_w / 2;
        g_host->draw_rect(b1_x, act_y, btn_w, btn_h, STICKY_COLOR_BLACK);
        int16_t cw = 0, ch = 0;
        g_host->get_text_bounds("UNCHECK ALL", 0, &cw, &ch);
        g_host->draw_text(b1_x + (btn_w - cw) / 2, act_y + (btn_h - ch) / 2, "UNCHECK ALL", 0, STICKY_COLOR_BLACK);
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
        g_view_mode = 0;
        g_host->request_update(true);
        return;
    }
    if (x >= t2_x && x < t2_x + tab_w && y >= tab_y && y < tab_y + tab_h) {
        g_host->play_haptic(0);
        g_view_mode = 1;
        g_host->request_update(true);
        return;
    }

    // Toggle Items
    int start_y = 125;
    int row_h = 68;
    int visible_idx = 0;

    for (int i = 0; i < g_item_count; ++i) {
        if (g_view_mode == 0 && !g_items[i].is_todo) continue;
        if (g_view_mode == 1 && g_items[i].is_todo) continue;

        int ry = start_y + visible_idx * (row_h + 10);
        visible_idx++;

        if (x >= 20 && x < sw - 20 && y >= ry && y < ry + row_h) {
            g_host->play_haptic(0);
            if (g_view_mode == 0) {
                g_items[i].checked = !g_items[i].checked;
                save_notes();
                g_host->request_update(true);
            }
            return;
        }
    }

    // Uncheck all
    if (g_view_mode == 0) {
        int act_y = start_y + 5 * (row_h + 10) + 15;
        int btn_w = 180, btn_h = 44;
        int b1_x = cx - btn_w / 2;
        if (x >= b1_x && x < b1_x + btn_w && y >= act_y && y < act_y + btn_h) {
            g_host->play_haptic(0);
            for (int i = 0; i < g_item_count; ++i) {
                if (g_items[i].is_todo) g_items[i].checked = false;
            }
            save_notes();
            g_host->request_update(true);
            return;
        }
    }
}

static void app_exit(void) {
    save_notes();
}

static void app_destroy(void) {}

static const StickyAppPlugin g_plugin = {
    .api_version = STICKY_APP_API_VERSION,
    .app_name = "Notes",
    .app_version = "1.0.0",
    .app_author = "Sticky",
    .icon_name = "notes",
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
