#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "sticky_app_api.h"

// Connect Four 7x6 rules & AI
#define COLS 7
#define ROWS 6
#define CELLS (COLS * ROWS)

enum Side { EMPTY = 0, PLAYER1 = 1, PLAYER2 = 2 };
enum Outcome { OUTCOME_RUNNING, OUTCOME_P1_WIN, OUTCOME_P2_WIN, OUTCOME_DRAW };
enum Mode { MODE_VS_CPU, MODE_VS_P2 };

typedef struct {
    uint8_t cells[COLS][ROWS]; // row 0 is bottom
    uint8_t turn;
    uint8_t outcome;
    uint8_t mode;
    int8_t last_col;
    int8_t last_row;
    uint32_t p1_wins;
    uint32_t p2_wins;
} C4State;

static const StickyHostApi* g_host = NULL;
static C4State g_state;

static void save_game(void) {
    if (g_host && g_host->write_file) {
        g_host->write_file("/.crosspoint/connectfour.sav", &g_state, sizeof(g_state));
    }
}

static void load_game(void) {
    if (g_host && g_host->read_file && g_host->file_exists && g_host->file_exists("/.crosspoint/connectfour.sav")) {
        uint32_t read_bytes = 0;
        if (g_host->read_file("/.crosspoint/connectfour.sav", &g_state, sizeof(g_state), &read_bytes)) {
            if (read_bytes == sizeof(g_state)) return;
        }
    }
    memset(&g_state, 0, sizeof(g_state));
    g_state.turn = PLAYER1;
    g_state.outcome = OUTCOME_RUNNING;
    g_state.mode = MODE_VS_CPU;
    g_state.last_col = -1;
    g_state.last_row = -1;
}

static int landing_row(int col) {
    if (col < 0 || col >= COLS) return -1;
    for (int r = 0; r < ROWS; ++r) {
        if (g_state.cells[col][r] == EMPTY) return r;
    }
    return -1;
}

static bool check_win_for(uint8_t side) {
    const int dirs[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    for (int c = 0; c < COLS; ++c) {
        for (int r = 0; r < ROWS; ++r) {
            if (g_state.cells[c][r] != side) continue;
            for (int d = 0; d < 4; ++d) {
                int dc = dirs[d][0], dr = dirs[d][1];
                int count = 0;
                for (int step = 0; step < 4; ++step) {
                    int nc = c + dc * step;
                    int nr = r + dr * step;
                    if (nc >= 0 && nc < COLS && nr >= 0 && nr < ROWS && g_state.cells[nc][nr] == side) {
                        count++;
                    } else break;
                }
                if (count == 4) return true;
            }
        }
    }
    return false;
}

static bool is_full(void) {
    for (int c = 0; c < COLS; ++c) {
        if (g_state.cells[c][ROWS - 1] == EMPTY) return false;
    }
    return true;
}

static bool drop_disc(int col) {
    if (g_state.outcome != OUTCOME_RUNNING) return false;
    int r = landing_row(col);
    if (r < 0) return false;

    g_state.cells[col][r] = g_state.turn;
    g_state.last_col = col;
    g_state.last_row = r;

    if (check_win_for(g_state.turn)) {
        g_state.outcome = (g_state.turn == PLAYER1) ? OUTCOME_P1_WIN : OUTCOME_P2_WIN;
        if (g_state.outcome == OUTCOME_P1_WIN) g_state.p1_wins++;
        else g_state.p2_wins++;
    } else if (is_full()) {
        g_state.outcome = OUTCOME_DRAW;
    } else {
        g_state.turn = (g_state.turn == PLAYER1) ? PLAYER2 : PLAYER1;
    }

    if (g_host) g_host->play_haptic(1);
    save_game();
    return true;
}

// Simple heuristic bot for CPU moves
static int bot_choose_column(void) {
    // 1. Can CPU win in one move?
    for (int c = 0; c < COLS; ++c) {
        int r = landing_row(c);
        if (r < 0) continue;
        g_state.cells[c][r] = PLAYER2;
        bool win = check_win_for(PLAYER2);
        g_state.cells[c][r] = EMPTY;
        if (win) return c;
    }

    // 2. Can Player win in one move? Block them!
    for (int c = 0; c < COLS; ++c) {
        int r = landing_row(c);
        if (r < 0) continue;
        g_state.cells[c][r] = PLAYER1;
        bool win = check_win_for(PLAYER1);
        g_state.cells[c][r] = EMPTY;
        if (win) return c;
    }

    // 3. Prefer center outward: 3, 2, 4, 1, 5, 0, 6
    const int order[COLS] = {3, 2, 4, 1, 5, 0, 6};
    for (int i = 0; i < COLS; ++i) {
        int c = order[i];
        if (landing_row(c) >= 0) return c;
    }
    return 3;
}

static void new_game(void) {
    for (int c = 0; c < COLS; ++c) {
        for (int r = 0; r < ROWS; ++r) {
            g_state.cells[c][r] = EMPTY;
        }
    }
    g_state.turn = PLAYER1;
    g_state.outcome = OUTCOME_RUNNING;
    g_state.last_col = -1;
    g_state.last_row = -1;
    save_game();
}

static void app_init(const StickyHostApi* host) {
    g_host = host;
    load_game();
}

static void app_render(void) {
    if (!g_host) return;

    g_host->clear(STICKY_COLOR_WHITE);
    g_host->draw_header("CONNECT FOUR");

    int16_t sw = g_host->get_screen_width();
    int16_t sh = g_host->get_screen_height();

    // Status banner
    int status_y = 65;
    g_host->fill_rect(20, status_y, sw - 40, 50, STICKY_COLOR_LIGHTGRAY);
    g_host->draw_rect(20, status_y, sw - 40, 50, STICKY_COLOR_BLACK);

    const char* status_str = "";
    if (g_state.outcome == OUTCOME_P1_WIN) {
        status_str = (g_state.mode == MODE_VS_CPU) ? "YOU WIN!" : "PLAYER 1 WINS!";
    } else if (g_state.outcome == OUTCOME_P2_WIN) {
        status_str = (g_state.mode == MODE_VS_CPU) ? "CPU WINS!" : "PLAYER 2 WINS!";
    } else if (g_state.outcome == OUTCOME_DRAW) {
        status_str = "GAME DRAW!";
    } else {
        if (g_state.turn == PLAYER1) {
            status_str = (g_state.mode == MODE_VS_CPU) ? "YOUR TURN (LIGHT)" : "PLAYER 1 (LIGHT)";
        } else {
            status_str = (g_state.mode == MODE_VS_CPU) ? "CPU THINKING..." : "PLAYER 2 (DARK)";
        }
    }
    int16_t tw, th;
    g_host->get_text_bounds(status_str, 2, &tw, &th);
    g_host->draw_text(sw / 2 - tw / 2, status_y + 16, status_str, 2, STICKY_COLOR_BLACK);

    // Board layout
    int board_top = 135;
    int col_width = (sw - 40) / COLS;
    int board_left = (sw - col_width * COLS) / 2;
    int row_height = col_width; // square cells
    int radius = col_width / 2 - 4;

    // Draw grid background frame
    g_host->draw_rect(board_left, board_top, col_width * COLS, row_height * ROWS, STICKY_COLOR_BLACK);

    for (int c = 0; c < COLS; ++c) {
        for (int r = 0; r < ROWS; ++r) {
            // Screen row 0 is top, board row 0 is bottom
            int scr_r = ROWS - 1 - r;
            int cx = board_left + c * col_width + col_width / 2;
            int cy = board_top + scr_r * row_height + row_height / 2;

            uint8_t cell = g_state.cells[c][r];
            if (cell == EMPTY) {
                g_host->draw_circle(cx, cy, radius, STICKY_COLOR_LIGHTGRAY);
            } else if (cell == PLAYER1) {
                // Light disc: white circle with black ring
                g_host->fill_circle(cx, cy, radius, STICKY_COLOR_WHITE);
                g_host->draw_circle(cx, cy, radius, STICKY_COLOR_BLACK);
                g_host->draw_circle(cx, cy, radius - 3, STICKY_COLOR_BLACK);
            } else {
                // Dark disc: solid black
                g_host->fill_circle(cx, cy, radius, STICKY_COLOR_BLACK);
            }

            // Indicator for last move
            if (c == g_state.last_col && r == g_state.last_row) {
                uint8_t mark_color = (cell == PLAYER2) ? STICKY_COLOR_WHITE : STICKY_COLOR_BLACK;
                g_host->fill_circle(cx, cy, 4, mark_color);
            }
        }
    }

    // Controls at bottom
    int btn_y = board_top + row_height * ROWS + 25;
    int btn_w = (sw - 60) / 2;
    int btn_h = 50;

    // New Game Button
    g_host->draw_rect(20, btn_y, btn_w, btn_h, STICKY_COLOR_BLACK);
    g_host->get_text_bounds("NEW GAME", 2, &tw, &th);
    g_host->draw_text(20 + btn_w / 2 - tw / 2, btn_y + 16, "NEW GAME", 2, STICKY_COLOR_BLACK);

    // Mode Toggle Button
    g_host->draw_rect(sw - 20 - btn_w, btn_y, btn_w, btn_h, STICKY_COLOR_BLACK);
    const char* mode_label = (g_state.mode == MODE_VS_CPU) ? "MODE: CPU" : "MODE: 2P";
    g_host->get_text_bounds(mode_label, 2, &tw, &th);
    g_host->draw_text(sw - 20 - btn_w + btn_w / 2 - tw / 2, btn_y + 16, mode_label, 2, STICKY_COLOR_BLACK);

    // Score display
    char score_str[64];
    int len = 0;
    // Simple integer to string helper without snprintf
    auto append_int = [](char* b, int& pos, uint32_t val) {
        char temp[12];
        int tp = 0;
        if (val == 0) temp[tp++] = '0';
        else {
            while (val > 0) {
                temp[tp++] = '0' + (val % 10);
                val /= 10;
            }
        }
        for (int i = tp - 1; i >= 0; --i) b[pos++] = temp[i];
    };
    const char* s_pre = "SCORE: P1 (";
    while (*s_pre) score_str[len++] = *s_pre++;
    append_int(score_str, len, g_state.p1_wins);
    const char* s_mid = ") - P2 (";
    while (*s_mid) score_str[len++] = *s_mid++;
    append_int(score_str, len, g_state.p2_wins);
    score_str[len++] = ')';
    score_str[len] = '\0';

    g_host->get_text_bounds(score_str, 2, &tw, &th);
    g_host->draw_text(sw / 2 - tw / 2, btn_y + btn_h + 20, score_str, 2, STICKY_COLOR_BLACK);

    g_host->request_update(false);
}

static void app_touch(int event, int16_t x, int16_t y) {
    if (event != STICKY_TOUCH_CLICK || !g_host) return;

    int16_t sw = g_host->get_screen_width();
    int col_width = (sw - 40) / COLS;
    int board_left = (sw - col_width * COLS) / 2;
    int board_top = 135;
    int board_height = col_width * ROWS;

    // Check board tap
    if (x >= board_left && x < board_left + col_width * COLS &&
        y >= board_top && y < board_top + board_height) {
        int col = (x - board_left) / col_width;
        if (g_state.outcome == OUTCOME_RUNNING) {
            if (drop_disc(col)) {
                app_render();
                // If vs CPU and still running, execute bot move after brief render
                if (g_state.mode == MODE_VS_CPU && g_state.outcome == OUTCOME_RUNNING && g_state.turn == PLAYER2) {
                    g_host->delay_ms(150);
                    int bot_col = bot_choose_column();
                    drop_disc(bot_col);
                    app_render();
                }
            }
        }
        return;
    }

    // Check Bottom Buttons
    int btn_y = board_top + board_height + 25;
    int btn_w = (sw - 60) / 2;
    int btn_h = 50;

    // New Game
    if (x >= 20 && x <= 20 + btn_w && y >= btn_y && y <= btn_y + btn_h) {
        g_host->play_haptic(1);
        new_game();
        app_render();
        return;
    }

    // Mode Toggle
    if (x >= sw - 20 - btn_w && x <= sw - 20 && y >= btn_y && y <= btn_y + btn_h) {
        g_host->play_haptic(1);
        g_state.mode = (g_state.mode == MODE_VS_CPU) ? MODE_VS_P2 : MODE_VS_CPU;
        new_game();
        app_render();
        return;
    }
}

static const StickyAppPlugin g_plugin = {
    .api_version = STICKY_APP_API_VERSION,
    .app_name    = "Connect Four",
    .app_version = "1.0",
    .app_author  = "CrossNext",
    .icon_name   = "icon_connectfour_32",
    .on_init     = app_init,
    .on_enter    = app_render,
    .on_loop     = NULL,
    .on_render   = app_render,
    .on_touch    = app_touch,
    .on_exit     = NULL,
    .on_destroy  = NULL,
};

extern "C" const StickyAppPlugin* sticky_app_get_plugin(void) {
    return &g_plugin;
}
