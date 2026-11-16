#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "sticky_app_api.h"
#include "PicrossCore.h"

namespace px = picross;

static const StickyHostApi* g_host = nullptr;
static px::Board g_board;
static int g_puzzle_idx = 0;
static bool g_cross_mode = false; // false = FILL, true = CROSS ('X')

static constexpr char kSavePath[] = "/.crosspoint/picross.sav";

static void save_state() {
    if (!g_host || !g_host->write_file) return;
    uint32_t buf[2] = { static_cast<uint32_t>(g_puzzle_idx), 0 };
    g_host->write_file(kSavePath, buf, sizeof(buf));
}

static void load_state() {
    if (g_host && g_host->read_file && g_host->file_exists && g_host->file_exists(kSavePath)) {
        uint32_t buf[2] = {};
        uint32_t read_bytes = 0;
        if (g_host->read_file(kSavePath, buf, sizeof(buf), &read_bytes) && read_bytes == sizeof(buf)) {
            if (buf[0] < px::kPuzzleCount) {
                g_puzzle_idx = buf[0];
            }
        }
    }
    g_board.load(g_puzzle_idx);
}

static void select_puzzle(int idx) {
    if (idx < 0) idx = px::kPuzzleCount - 1;
    if (idx >= px::kPuzzleCount) idx = 0;
    g_puzzle_idx = idx;
    g_board.load(g_puzzle_idx);
    save_state();
}

static void app_init(const StickyHostApi* host) {
    g_host = host;
    load_state();
}

static void app_render() {
    if (!g_host) return;

    g_host->clear(STICKY_COLOR_WHITE);
    g_host->draw_header("PICROSS");

    int16_t sw = g_host->get_screen_width();
    int16_t sh = g_host->get_screen_height();
    int size = g_board.size();

    // Banner
    int banner_y = 65;
    g_host->fill_rect(20, banner_y, sw - 40, 42, STICKY_COLOR_LIGHTGRAY);
    g_host->draw_rect(20, banner_y, sw - 40, 42, STICKY_COLOR_BLACK);

    char banner_str[64];
    int bpos = 0;
    auto append_int = [](char* b, int& pos, int val) {
        char temp[12]; int tp = 0;
        if (val == 0) temp[tp++] = '0';
        else { while (val > 0) { temp[tp++] = '0' + (val % 10); val /= 10; } }
        for (int i = tp - 1; i >= 0; --i) b[pos++] = temp[i];
    };

    if (g_board.solved()) {
        const char* name = g_board.puzzle().name;
        if (name && *name) {
            const char* s = "SOLVED: "; while (*s) banner_str[bpos++] = *s++;
            while (*name) banner_str[bpos++] = *name++;
            banner_str[bpos] = '\0';
        } else {
            strcpy(banner_str, "PUZZLE SOLVED!");
        }
    } else {
        const char* s = "PUZZLE "; while (*s) banner_str[bpos++] = *s++;
        append_int(banner_str, bpos, g_puzzle_idx + 1);
        const char* s2 = "/"; while (*s2) banner_str[bpos++] = *s2++;
        append_int(banner_str, bpos, px::kPuzzleCount);
        const char* s3 = " ("; while (*s3) banner_str[bpos++] = *s3++;
        append_int(banner_str, bpos, size);
        const char* s4 = "x"; while (*s4) banner_str[bpos++] = *s4++;
        append_int(banner_str, bpos, size);
        banner_str[bpos++] = ')';
        banner_str[bpos] = '\0';
    }

    int16_t tw, th;
    g_host->get_text_bounds(banner_str, 2, &tw, &th);
    g_host->draw_text(sw / 2 - tw / 2, banner_y + 12, banner_str, 2, STICKY_COLOR_BLACK);

    // Clues and Grid geometry
    int clue_area = (size <= 10) ? 90 : 70;
    int max_grid_w = sw - 40 - clue_area;
    int cell_size = max_grid_w / size;
    if (cell_size > 42) cell_size = 42;

    int grid_w = cell_size * size;
    int grid_left = sw - 20 - grid_w;
    int grid_top = 130 + clue_area;

    // Outer grid border
    g_host->draw_rect(grid_left, grid_top, grid_w, grid_w, STICKY_COLOR_BLACK);
    g_host->draw_rect(grid_left - 1, grid_top - 1, grid_w + 2, grid_w + 2, STICKY_COLOR_BLACK);

    // Draw Column clues above grid
    for (int c = 0; c < size; ++c) {
        uint8_t runs[px::kMaxSize];
        int count = g_board.colClues(c, runs);
        int cx = grid_left + c * cell_size;
        int bottom_y = grid_top - 6;

        if (count == 0) {
            g_host->get_text_bounds("0", 0, &tw, &th);
            g_host->draw_text(cx + (cell_size - tw) / 2, bottom_y - 12, "0", 0, STICKY_COLOR_DARKGRAY);
        } else {
            for (int i = count - 1; i >= 0; --i) {
                char num_str[4]; int np = 0;
                append_int(num_str, np, runs[i]);
                num_str[np] = '\0';
                g_host->get_text_bounds(num_str, 0, &tw, &th);
                g_host->draw_text(cx + (cell_size - tw) / 2, bottom_y - 14, num_str, 0, STICKY_COLOR_BLACK);
                bottom_y -= 14;
            }
        }
    }

    // Draw Row clues to the left of grid
    for (int r = 0; r < size; ++r) {
        uint8_t runs[px::kMaxSize];
        int count = g_board.rowClues(r, runs);
        int cy = grid_top + r * cell_size;
        int right_x = grid_left - 8;

        if (count == 0) {
            g_host->get_text_bounds("0", 0, &tw, &th);
            g_host->draw_text(right_x - tw, cy + (cell_size - th) / 2, "0", 0, STICKY_COLOR_DARKGRAY);
        } else {
            for (int i = count - 1; i >= 0; --i) {
                char num_str[4]; int np = 0;
                append_int(num_str, np, runs[i]);
                num_str[np] = '\0';
                g_host->get_text_bounds(num_str, 0, &tw, &th);
                g_host->draw_text(right_x - tw, cy + (cell_size - th) / 2, num_str, 0, STICKY_COLOR_BLACK);
                right_x -= (tw + 6);
            }
        }
    }

    // Draw Grid cells
    for (int r = 0; r < size; ++r) {
        for (int c = 0; c < size; ++c) {
            int cx = grid_left + c * cell_size;
            int cy = grid_top + r * cell_size;

            g_host->draw_rect(cx, cy, cell_size, cell_size, STICKY_COLOR_LIGHTGRAY);

            px::Cell cell_type = g_board.cell(r, c);
            if (cell_type == px::Cell::Filled) {
                g_host->fill_rect(cx + 2, cy + 2, cell_size - 3, cell_size - 3, STICKY_COLOR_BLACK);
            } else if (cell_type == px::Cell::Crossed) {
                g_host->draw_line(cx + 4, cy + 4, cx + cell_size - 4, cy + cell_size - 4, STICKY_COLOR_DARKGRAY);
                g_host->draw_line(cx + cell_size - 4, cy + 4, cx + 4, cy + cell_size - 4, STICKY_COLOR_DARKGRAY);
            } else if (cell_type == px::Cell::Mistake) {
                // Mistake cross
                g_host->draw_line(cx + 3, cy + 3, cx + cell_size - 3, cy + cell_size - 3, STICKY_COLOR_BLACK);
                g_host->draw_line(cx + 4, cy + 3, cx + cell_size - 3, cy + cell_size - 4, STICKY_COLOR_BLACK);
                g_host->draw_line(cx + cell_size - 3, cy + 3, cx + 3, cy + cell_size - 3, STICKY_COLOR_BLACK);
                g_host->draw_line(cx + cell_size - 4, cy + 3, cx + 3, cy + cell_size - 4, STICKY_COLOR_BLACK);
            }
        }
    }

    // Bold lines every 5 cells
    for (int i = 5; i < size; i += 5) {
        int x = grid_left + i * cell_size;
        int y = grid_top + i * cell_size;
        g_host->draw_line(x, grid_top, x, grid_top + grid_w, STICKY_COLOR_BLACK);
        g_host->draw_line(grid_left, y, grid_left + grid_w, y, STICKY_COLOR_BLACK);
    }

    // Controls at bottom
    int btn_y = grid_top + grid_w + 30;
    int btn_w = (sw - 60) / 3;
    int btn_h = 50;

    // PREV
    g_host->draw_rect(20, btn_y, btn_w, btn_h, STICKY_COLOR_BLACK);
    g_host->get_text_bounds("< PREV", 2, &tw, &th);
    g_host->draw_text(20 + (btn_w - tw) / 2, btn_y + 16, "< PREV", 2, STICKY_COLOR_BLACK);

    // MODE TOGGLE (FILL vs CROSS)
    int mode_x = 20 + btn_w + 10;
    if (g_cross_mode) {
        g_host->fill_rect(mode_x, btn_y, btn_w, btn_h, STICKY_COLOR_LIGHTGRAY);
    }
    g_host->draw_rect(mode_x, btn_y, btn_w, btn_h, STICKY_COLOR_BLACK);
    const char* m_str = g_cross_mode ? "MODE: [X]" : "MODE: [■]";
    g_host->get_text_bounds(m_str, 2, &tw, &th);
    g_host->draw_text(mode_x + (btn_w - tw) / 2, btn_y + 16, m_str, 2, STICKY_COLOR_BLACK);

    // NEXT
    int next_x = sw - 20 - btn_w;
    g_host->draw_rect(next_x, btn_y, btn_w, btn_h, STICKY_COLOR_BLACK);
    g_host->get_text_bounds("NEXT >", 2, &tw, &th);
    g_host->draw_text(next_x + (btn_w - tw) / 2, btn_y + 16, "NEXT >", 2, STICKY_COLOR_BLACK);

    g_host->request_update(false);
}

static void app_touch(int event, int16_t x, int16_t y) {
    if (event != STICKY_TOUCH_CLICK || !g_host) return;

    int16_t sw = g_host->get_screen_width();
    int size = g_board.size();
    int clue_area = (size <= 10) ? 90 : 70;
    int max_grid_w = sw - 40 - clue_area;
    int cell_size = max_grid_w / size;
    if (cell_size > 42) cell_size = 42;

    int grid_w = cell_size * size;
    int grid_left = sw - 20 - grid_w;
    int grid_top = 130 + clue_area;

    // Grid tap
    if (x >= grid_left && x < grid_left + grid_w && y >= grid_top && y < grid_top + grid_w) {
        if (!g_board.solved()) {
            int col = (x - grid_left) / cell_size;
            int row = (y - grid_top) / cell_size;

            if (g_cross_mode) {
                g_board.mark(row, col);
            } else {
                g_board.fill(row, col);
            }

            if (g_host) g_host->play_haptic(1);
            save_state();
            app_render();
        }
        return;
    }

    // Button taps
    int btn_y = grid_top + grid_w + 30;
    int btn_w = (sw - 60) / 3;
    int btn_h = 50;

    if (y >= btn_y && y <= btn_y + btn_h) {
        if (x >= 20 && x <= 20 + btn_w) { // PREV
            if (g_host) g_host->play_haptic(1);
            select_puzzle(g_puzzle_idx - 1);
            app_render();
        } else if (x >= 20 + btn_w + 10 && x <= 20 + 2 * btn_w + 10) { // MODE
            if (g_host) g_host->play_haptic(1);
            g_cross_mode = !g_cross_mode;
            app_render();
        } else if (x >= sw - 20 - btn_w && x <= sw - 20) { // NEXT
            if (g_host) g_host->play_haptic(1);
            select_puzzle(g_puzzle_idx + 1);
            app_render();
        }
    }
}

static const StickyAppPlugin g_plugin = {
    .api_version = STICKY_APP_API_VERSION,
    .app_name    = "Picross",
    .app_version = "1.0",
    .app_author  = "CrossNext",
    .icon_name   = "icon_picross_32",
    .on_init     = app_init,
    .on_enter    = app_render,
    .on_loop     = nullptr,
    .on_render   = app_render,
    .on_touch    = app_touch,
    .on_exit     = nullptr,
    .on_destroy  = nullptr,
};

extern "C" const StickyAppPlugin* sticky_app_get_plugin(void) {
    return &g_plugin;
}
