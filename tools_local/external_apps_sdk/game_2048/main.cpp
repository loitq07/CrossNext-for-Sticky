#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sticky_app_api.h"

// ----------------- 2048 Core Logic -----------------
enum Direction { DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT };

typedef struct {
    uint8_t cells[16];
    uint32_t score;
    uint32_t best;
    uint32_t random_seed;
    bool game_over;
} GameState;

static const StickyHostApi* g_host = NULL;
static GameState g_state;
static int16_t g_touch_start_x = -1;
static int16_t g_touch_start_y = -1;

static uint32_t next_random(void) {
    uint32_t v = g_state.random_seed ? g_state.random_seed : 1;
    v ^= v << 13;
    v ^= v >> 17;
    v ^= v << 5;
    g_state.random_seed = v;
    return v;
}

static bool spawn_tile(void) {
    int free_indices[16];
    int count = 0;
    for (int i = 0; i < 16; ++i) {
        if (g_state.cells[i] == 0) {
            free_indices[count++] = i;
        }
    }
    if (count == 0) return false;
    int idx = free_indices[next_random() % count];
    g_state.cells[idx] = (next_random() % 10 == 0) ? 2 : 1; // 10% chance for 4 (2^2), 90% for 2 (2^1)
    return true;
}

static void start_new_game(void) {
    uint32_t preserved_best = g_state.best;
    memset(g_state.cells, 0, sizeof(g_state.cells));
    g_state.score = 0;
    g_state.best = preserved_best;
    g_state.game_over = false;
    if (g_host) {
        g_state.random_seed = g_host->get_millis();
    }
    spawn_tile();
    spawn_tile();
}

static int cell_index(int dir, int line, int pos) {
    switch (dir) {
        case DIR_LEFT:  return line * 4 + pos;
        case DIR_RIGHT: return line * 4 + (3 - pos);
        case DIR_UP:    return pos * 4 + line;
        case DIR_DOWN:  return (3 - pos) * 4 + line;
    }
    return 0;
}

static bool can_move(void) {
    for (int i = 0; i < 16; ++i) {
        if (g_state.cells[i] == 0) return true;
        int row = i / 4;
        int col = i % 4;
        if (col < 3 && g_state.cells[i] == g_state.cells[i + 1]) return true;
        if (row < 3 && g_state.cells[i] == g_state.cells[i + 4]) return true;
    }
    return false;
}

static bool move_tiles(int dir) {
    bool moved = false;
    for (int line = 0; line < 4; ++line) {
        uint8_t target[4] = {0};
        int target_pos = 0;

        for (int pos = 0; pos < 4; ++pos) {
            int idx = cell_index(dir, line, pos);
            if (g_state.cells[idx] == 0) continue;

            if (target_pos > 0 && target[target_pos - 1] == g_state.cells[idx]) {
                target[target_pos - 1]++;
                uint32_t val = 1u << target[target_pos - 1];
                g_state.score += val;
                if (g_state.score > g_state.best) {
                    g_state.best = g_state.score;
                }
                target_pos++; // Merged tile cannot merge again this turn
                moved = true;
            } else {
                if (target_pos > 0 && target[target_pos - 1] == 0) {
                    target_pos--;
                }
                target[target_pos++] = g_state.cells[idx];
            }
        }

        // Clean trailing zero merge slots
        uint8_t final_row[4] = {0};
        int fpos = 0;
        for (int i = 0; i < 4; ++i) {
            if (target[i] > 0) final_row[fpos++] = target[i];
        }

        for (int pos = 0; pos < 4; ++pos) {
            int idx = cell_index(dir, line, pos);
            if (g_state.cells[idx] != final_row[pos]) {
                moved = true;
            }
            g_state.cells[idx] = final_row[pos];
        }
    }

    if (moved) {
        spawn_tile();
        if (!can_move()) {
            g_state.game_over = true;
        }
    }
    return moved;
}

// ----------------- App Lifecycle -----------------

static void app_init(const StickyHostApi* host) {
    g_host = host;
}

static void app_enter(void) {
    if (g_host) {
        start_new_game();
        g_host->request_update(true);
    }
}

static void app_loop(void) {
}

static void uint_to_str(uint32_t val, char* buf) {
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    char tmp[16];
    int i = 0;
    while (val > 0) {
        tmp[i++] = '0' + (val % 10);
        val /= 10;
    }
    int j = 0;
    while (i > 0) {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
}

static void draw_tile(int x, int y, int size, uint8_t power) {
    if (!g_host) return;

    if (power == 0) {
        g_host->draw_rect(x, y, size, size, STICKY_COLOR_LIGHTGRAY);
        return;
    }

    // High contrast styling for e-paper
    bool inverted = (power >= 3); // 8 and above are black tiles with white text
    if (inverted) {
        g_host->fill_rect(x, y, size, size, STICKY_COLOR_BLACK);
    } else {
        g_host->fill_rect(x, y, size, size, STICKY_COLOR_WHITE);
        g_host->draw_rect(x, y, size, size, STICKY_COLOR_BLACK);
        g_host->draw_rect(x + 1, y + 1, size - 2, size - 2, STICKY_COLOR_BLACK);
    }

    uint32_t val = 1u << power;
    char text[16];
    uint_to_str(val, text);

    int16_t tw = 0, th = 0;
    g_host->get_text_bounds(text, 0, &tw, &th);
    int tx = x + (size - tw) / 2;
    int ty = y + (size - th) / 2;

    g_host->draw_text(tx, ty, text, 0, inverted ? STICKY_COLOR_WHITE : STICKY_COLOR_BLACK);
}

static void app_render(void) {
    if (!g_host) return;

    g_host->clear(STICKY_COLOR_WHITE);
    g_host->draw_header("2048");

    int screen_w = g_host->get_screen_width();

    // Draw Score and Best
    char score_str[48] = "SCORE: ";
    char num_buf[16];
    uint_to_str(g_state.score, num_buf);
    strcat(score_str, num_buf);
    strcat(score_str, "   BEST: ");
    uint_to_str(g_state.best, num_buf);
    strcat(score_str, num_buf);
    int16_t sw = 0, sh = 0;
    g_host->get_text_bounds(score_str, 0, &sw, &sh);
    g_host->draw_text((screen_w - sw) / 2, 70, score_str, 0, STICKY_COLOR_BLACK);

    // Board Layout
    int board_size = 400;
    int start_x = (screen_w - board_size) / 2;
    int start_y = 130;
    int gap = 8;
    int tile_size = (board_size - gap * 5) / 4;

    // Outer board border
    g_host->draw_rect(start_x, start_y, board_size, board_size, STICKY_COLOR_BLACK);
    g_host->draw_rect(start_x + 1, start_y + 1, board_size - 2, board_size - 2, STICKY_COLOR_BLACK);

    // Draw Tiles
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int tx = start_x + gap + col * (tile_size + gap);
            int ty = start_y + gap + row * (tile_size + gap);
            draw_tile(tx, ty, tile_size, g_state.cells[row * 4 + col]);
        }
    }

    // Touch D-Pad buttons for e-paper accessibility (under board)
    int btn_y = start_y + board_size + 30;
    int cx = screen_w / 2;

    // Up, Down, Left, Right buttons
    int bw = 80, bh = 50;
    // Up
    g_host->draw_rect(cx - bw / 2, btn_y, bw, bh, STICKY_COLOR_BLACK);
    g_host->draw_text(cx - 10, btn_y + 15, "UP", 0, STICKY_COLOR_BLACK);

    // Left, Down, Right
    int row2_y = btn_y + bh + 15;
    g_host->draw_rect(cx - bw / 2 - bw - 15, row2_y, bw, bh, STICKY_COLOR_BLACK);
    g_host->draw_text(cx - bw / 2 - bw - 10, row2_y + 15, "LEFT", 0, STICKY_COLOR_BLACK);

    g_host->draw_rect(cx - bw / 2, row2_y, bw, bh, STICKY_COLOR_BLACK);
    g_host->draw_text(cx - 20, row2_y + 15, "DOWN", 0, STICKY_COLOR_BLACK);

    g_host->draw_rect(cx + bw / 2 + 15, row2_y, bw, bh, STICKY_COLOR_BLACK);
    g_host->draw_text(cx + bw / 2 + 25, row2_y + 15, "RIGHT", 0, STICKY_COLOR_BLACK);

    // New Game button
    int nbtn_y = row2_y + bh + 25;
    int nbw = 160, nbh = 45;
    g_host->fill_rect(cx - nbw / 2, nbtn_y, nbw, nbh, STICKY_COLOR_BLACK);
    g_host->draw_text(cx - 50, nbtn_y + 12, "NEW GAME", 0, STICKY_COLOR_WHITE);

    if (g_state.game_over) {
        g_host->fill_rect(start_x + 30, start_y + 150, board_size - 60, 100, STICKY_COLOR_WHITE);
        g_host->draw_rect(start_x + 30, start_y + 150, board_size - 60, 100, STICKY_COLOR_BLACK);
        g_host->draw_rect(start_x + 32, start_y + 152, board_size - 64, 96, STICKY_COLOR_BLACK);
        const char* msg = "GAME OVER!";
        int16_t mw = 0, mh = 0;
        g_host->get_text_bounds(msg, 0, &mw, &mh);
        g_host->draw_text(start_x + (board_size - mw) / 2, start_y + 185, msg, 0, STICKY_COLOR_BLACK);
    }
}

static void app_touch(int event_type, int16_t x, int16_t y) {
    if (!g_host) return;

    int screen_w = g_host->get_screen_width();
    int cx = screen_w / 2;
    int start_y = 130;
    int board_size = 400;
    int btn_y = start_y + board_size + 30;
    int bw = 80, bh = 50;
    int row2_y = btn_y + bh + 15;
    int nbtn_y = row2_y + bh + 25;
    int nbw = 160, nbh = 45;

    if (event_type == STICKY_TOUCH_DOWN) {
        g_touch_start_x = x;
        g_touch_start_y = y;
        return;
    }

    if (event_type == STICKY_TOUCH_CLICK || event_type == STICKY_TOUCH_UP) {
        // Check New Game Button
        if (x >= cx - nbw / 2 && x <= cx + nbw / 2 && y >= nbtn_y && y <= nbtn_y + nbh) {
            start_new_game();
            g_host->play_haptic(1);
            g_host->request_update(true);
            return;
        }

        if (g_state.game_over) return;

        // Check D-Pad Buttons
        // Up
        if (x >= cx - bw / 2 && x <= cx + bw / 2 && y >= btn_y && y <= btn_y + bh) {
            if (move_tiles(DIR_UP)) {
                g_host->play_haptic(1);
                g_host->request_update(true);
            }
            return;
        }

        // Left
        if (x >= cx - bw / 2 - bw - 15 && x <= cx - bw / 2 - 15 && y >= row2_y && y <= row2_y + bh) {
            if (move_tiles(DIR_LEFT)) {
                g_host->play_haptic(1);
                g_host->request_update(true);
            }
            return;
        }

        // Down
        if (x >= cx - bw / 2 && x <= cx + bw / 2 && y >= row2_y && y <= row2_y + bh) {
            if (move_tiles(DIR_DOWN)) {
                g_host->play_haptic(1);
                g_host->request_update(true);
            }
            return;
        }

        // Right
        if (x >= cx + bw / 2 + 15 && x <= cx + bw / 2 + bw + 15 && y >= row2_y && y <= row2_y + bh) {
            if (move_tiles(DIR_RIGHT)) {
                g_host->play_haptic(1);
                g_host->request_update(true);
            }
            return;
        }

        // Swipe Detection on board
        if (g_touch_start_x >= 0 && g_touch_start_y >= 0) {
            int16_t dx = x - g_touch_start_x;
            int16_t dy = y - g_touch_start_y;
            g_touch_start_x = -1;
            g_touch_start_y = -1;

            if (abs(dx) > 30 || abs(dy) > 30) {
                bool moved = false;
                if (abs(dx) > abs(dy)) {
                    moved = move_tiles(dx > 0 ? DIR_RIGHT : DIR_LEFT);
                } else {
                    moved = move_tiles(dy > 0 ? DIR_DOWN : DIR_UP);
                }
                if (moved) {
                    g_host->play_haptic(1);
                    g_host->request_update(true);
                }
            }
        }
    }
}

static void app_exit(void) {
}

static void app_destroy(void) {
}

// ----------------- Plugin Definition -----------------

static const StickyAppPlugin s_2048_plugin = {
    .api_version = STICKY_APP_API_VERSION,
    .app_name = "2048",
    .app_version = "1.0.0",
    .app_author = "CrossNext",
    .icon_name = "connections",
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
    return &s_2048_plugin;
}

__attribute__((visibility("default")))
const StickyAppPlugin* main_entry(const StickyHostApi* host) {
    app_init(host);
    return &s_2048_plugin;
}

#ifdef __cplusplus
}
#endif
