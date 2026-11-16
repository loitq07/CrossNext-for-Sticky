#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "sticky_app_api.h"

// ----------------- Minesweeper Rules -----------------
#define COLS 8
#define ROWS 10
#define TOTAL_MINES 10

#define CELL_MINE     (1 << 0)
#define CELL_REVEALED (1 << 1)
#define CELL_FLAGGED  (1 << 2)

enum GameStatus { STATUS_FRESH, STATUS_PLAYING, STATUS_WON, STATUS_LOST };

typedef struct {
    uint8_t cells[COLS][ROWS];
    uint32_t rng_state;
    enum GameStatus status;
    bool flag_mode; // false = DIG, true = FLAG
} MinesweeperState;

static const StickyHostApi* g_host = NULL;
static MinesweeperState g_game;

static uint32_t next_rng(void) {
    if (g_game.rng_state == 0) g_game.rng_state = 0x9E3779B9u;
    g_game.rng_state ^= g_game.rng_state << 13;
    g_game.rng_state ^= g_game.rng_state >> 17;
    g_game.rng_state ^= g_game.rng_state << 5;
    return g_game.rng_state;
}

static void start_game(void) {
    memset(g_game.cells, 0, sizeof(g_game.cells));
    if (g_host) {
        g_game.rng_state = g_host->get_millis();
    }
    g_game.status = STATUS_FRESH;
    g_game.flag_mode = false;
}

static int count_neighbour_mines(int col, int row) {
    int count = 0;
    for (int dc = -1; dc <= 1; ++dc) {
        for (int dr = -1; dr <= 1; ++dr) {
            if (dc == 0 && dr == 0) continue;
            int c = col + dc;
            int r = row + dr;
            if (c >= 0 && c < COLS && r >= 0 && r < ROWS) {
                if (g_game.cells[c][r] & CELL_MINE) count++;
            }
        }
    }
    return count;
}

static void lay_mines(int safe_col, int safe_row) {
    int placed = 0;
    while (placed < TOTAL_MINES) {
        uint32_t roll = next_rng() % (COLS * ROWS);
        int c = (int)(roll % COLS);
        int r = (int)(roll / COLS);
        if (g_game.cells[c][r] & CELL_MINE) continue;
        int dc = abs(c - safe_col);
        int dr = abs(r - safe_row);
        if (dc <= 1 && dr <= 1) continue; // Safe zone around first tap
        g_game.cells[c][r] |= CELL_MINE;
        placed++;
    }
}

static bool check_win(void) {
    for (int c = 0; c < COLS; ++c) {
        for (int r = 0; r < ROWS; ++r) {
            uint8_t cell = g_game.cells[c][r];
            if ((cell & CELL_MINE) == 0 && (cell & CELL_REVEALED) == 0) {
                return false;
            }
        }
    }
    return true;
}

static void reveal_cell(int c, int r) {
    if (c < 0 || c >= COLS || r < 0 || r >= ROWS) return;
    if (g_game.cells[c][r] & (CELL_REVEALED | CELL_FLAGGED)) return;

    if (g_game.status == STATUS_FRESH) {
        lay_mines(c, r);
        g_game.status = STATUS_PLAYING;
    }

    g_game.cells[c][r] |= CELL_REVEALED;

    if (g_game.cells[c][r] & CELL_MINE) {
        g_game.status = STATUS_LOST;
        return;
    }

    if (check_win()) {
        g_game.status = STATUS_WON;
        return;
    }

    // Flood fill zeros
    if (count_neighbour_mines(c, r) == 0) {
        for (int dc = -1; dc <= 1; ++dc) {
            for (int dr = -1; dr <= 1; ++dr) {
                if (dc != 0 || dr != 0) {
                    reveal_cell(c + dc, r + dr);
                }
            }
        }
    }
}

static void toggle_flag(int c, int r) {
    if (c < 0 || c >= COLS || r < 0 || r >= ROWS) return;
    if (g_game.cells[c][r] & CELL_REVEALED) return;
    g_game.cells[c][r] ^= CELL_FLAGGED;
}

// ----------------- App Lifecycle -----------------

static void app_init(const StickyHostApi* host) {
    g_host = host;
}

static void app_enter(void) {
    start_game();
    if (g_host) {
        g_host->request_update(true);
    }
}

static void app_loop(void) {
}

static void app_render(void) {
    if (!g_host) return;

    g_host->clear(STICKY_COLOR_WHITE);
    g_host->draw_header("MINESWEEPER");

    int screen_w = g_host->get_screen_width();

    // Mine status bar
    int flagged_count = 0;
    for (int c = 0; c < COLS; ++c) {
        for (int r = 0; r < ROWS; ++r) {
            if (g_game.cells[c][r] & CELL_FLAGGED) flagged_count++;
        }
    }

    char status_bar[64] = "MINES: ";
    char num_buf[16];
    int remaining = TOTAL_MINES - flagged_count;
    if (remaining < 0) remaining = 0;
    num_buf[0] = '0' + (remaining / 10);
    num_buf[1] = '0' + (remaining % 10);
    num_buf[2] = '\0';
    strcat(status_bar, num_buf);
    strcat(status_bar, "   MODE: ");
    strcat(status_bar, g_game.flag_mode ? "[FLAG]" : "[DIG]");

    int16_t sb_w = 0, sb_h = 0;
    g_host->get_text_bounds(status_bar, 0, &sb_w, &sb_h);
    g_host->draw_text((screen_w - sb_w) / 2, 70, status_bar, 0, STICKY_COLOR_BLACK);

    // Board Layout: 8 columns x 10 rows
    // 480 wide -> cell width = 56px, board width = 448px, margin = 16px
    int cell_w = 56;
    int cell_h = 52;
    int start_x = (screen_w - (cell_w * COLS)) / 2;
    int start_y = 115;

    for (int c = 0; c < COLS; ++c) {
        for (int r = 0; r < ROWS; ++r) {
            int cx = start_x + c * cell_w;
            int cy = start_y + r * cell_h;
            uint8_t cell = g_game.cells[c][r];

            if (cell & CELL_REVEALED) {
                // Revealed cell: clear white background, thin border
                g_host->fill_rect(cx, cy, cell_w, cell_h, STICKY_COLOR_WHITE);
                g_host->draw_rect(cx, cy, cell_w, cell_h, STICKY_COLOR_LIGHTGRAY);

                if (cell & CELL_MINE) {
                    // Draw Mine
                    g_host->fill_circle(cx + cell_w / 2, cy + cell_h / 2, 12, STICKY_COLOR_BLACK);
                } else {
                    int n = count_neighbour_mines(c, r);
                    if (n > 0) {
                        char digit[2] = { (char)('0' + n), '\0' };
                        int16_t dw = 0, dh = 0;
                        g_host->get_text_bounds(digit, 0, &dw, &dh);
                        g_host->draw_text(cx + (cell_w - dw) / 2, cy + (cell_h - dh) / 2, digit, 0, STICKY_COLOR_BLACK);
                    }
                }
            } else {
                // Hidden cell: embossed 3D bevel box
                g_host->fill_rect(cx + 2, cy + 2, cell_w - 4, cell_h - 4, STICKY_COLOR_WHITE);
                g_host->draw_rect(cx, cy, cell_w, cell_h, STICKY_COLOR_BLACK);
                g_host->draw_rect(cx + 1, cy + 1, cell_w - 2, cell_h - 2, STICKY_COLOR_BLACK);

                if (cell & CELL_FLAGGED) {
                    // Draw Flag indicator
                    g_host->draw_text(cx + 20, cy + 15, "P", 0, STICKY_COLOR_BLACK);
                }
            }
        }
    }

    // Bottom Controls: Toggle Mode & New Game
    int btn_y = start_y + (cell_h * ROWS) + 30;
    int bw = 180, bh = 55;

    // Toggle Mode Button
    int mode_x = screen_w / 2 - bw - 15;
    g_host->draw_rect(mode_x, btn_y, bw, bh, STICKY_COLOR_BLACK);
    g_host->draw_rect(mode_x + 1, btn_y + 1, bw - 2, bh - 2, STICKY_COLOR_BLACK);
    const char* mlabel = g_game.flag_mode ? "MODE: FLAG" : "MODE: DIG";
    int16_t mlw = 0, mlh = 0;
    g_host->get_text_bounds(mlabel, 0, &mlw, &mlh);
    g_host->draw_text(mode_x + (bw - mlw) / 2, btn_y + 18, mlabel, 0, STICKY_COLOR_BLACK);

    // New Game Button
    int new_x = screen_w / 2 + 15;
    g_host->fill_rect(new_x, btn_y, bw, bh, STICKY_COLOR_BLACK);
    const char* nlabel = "NEW GAME";
    int16_t nlw = 0, nlh = 0;
    g_host->get_text_bounds(nlabel, 0, &nlw, &nlh);
    g_host->draw_text(new_x + (bw - nlw) / 2, btn_y + 18, nlabel, 0, STICKY_COLOR_WHITE);

    // Game Over Overlay
    if (g_game.status == STATUS_WON || g_game.status == STATUS_LOST) {
        int banner_w = 340, banner_h = 80;
        int bx = (screen_w - banner_w) / 2;
        int by = start_y + 180;
        g_host->fill_rect(bx, by, banner_w, banner_h, STICKY_COLOR_WHITE);
        g_host->draw_rect(bx, by, banner_w, banner_h, STICKY_COLOR_BLACK);
        g_host->draw_rect(bx + 2, by + 2, banner_w - 4, banner_h - 4, STICKY_COLOR_BLACK);

        const char* msg = (g_game.status == STATUS_WON) ? "VICTORY! YOU WON!" : "BOOM! GAME OVER!";
        int16_t mw = 0, mh = 0;
        g_host->get_text_bounds(msg, 0, &mw, &mh);
        g_host->draw_text(bx + (banner_w - mw) / 2, by + 28, msg, 0, STICKY_COLOR_BLACK);
    }
}

static void app_touch(int event_type, int16_t x, int16_t y) {
    if (!g_host) return;

    int screen_w = g_host->get_screen_width();
    int cell_w = 56;
    int cell_h = 52;
    int start_x = (screen_w - (cell_w * COLS)) / 2;
    int start_y = 115;
    int btn_y = start_y + (cell_h * ROWS) + 30;
    int bw = 180, bh = 55;
    int mode_x = screen_w / 2 - bw - 15;
    int new_x = screen_w / 2 + 15;

    if (event_type == STICKY_TOUCH_CLICK) {
        // Mode toggle button
        if (x >= mode_x && x <= mode_x + bw && y >= btn_y && y <= btn_y + bh) {
            g_game.flag_mode = !g_game.flag_mode;
            g_host->play_haptic(1);
            g_host->request_update(true);
            return;
        }

        // New game button
        if (x >= new_x && x <= new_x + bw && y >= btn_y && y <= btn_y + bh) {
            start_game();
            g_host->play_haptic(1);
            g_host->request_update(true);
            return;
        }

        if (g_game.status == STATUS_WON || g_game.status == STATUS_LOST) {
            return;
        }

        // Board Tap
        if (x >= start_x && x < start_x + (cell_w * COLS) &&
            y >= start_y && y < start_y + (cell_h * ROWS)) {
            int col = (x - start_x) / cell_w;
            int row = (y - start_y) / cell_h;

            if (g_game.flag_mode) {
                toggle_flag(col, row);
            } else {
                reveal_cell(col, row);
            }
            g_host->play_haptic(1);
            g_host->request_update(true);
        }
    }
}

static void app_exit(void) {}
static void app_destroy(void) {}

// ----------------- Plugin Definition -----------------

static const StickyAppPlugin s_minesweeper_plugin = {
    .api_version = STICKY_APP_API_VERSION,
    .app_name = "Minesweeper",
    .app_version = "1.0.0",
    .app_author = "CrossNext",
    .icon_name = "minesweeper",
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
    return &s_minesweeper_plugin;
}

__attribute__((visibility("default")))
const StickyAppPlugin* main_entry(const StickyHostApi* host) {
    app_init(host);
    return &s_minesweeper_plugin;
}

#ifdef __cplusplus
}
#endif
