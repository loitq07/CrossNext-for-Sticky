#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "sticky_app_api.h"
#include "PocketGamesCore.h"

namespace pg = pocketgames;

static const StickyHostApi* g_host = nullptr;
static pg::State g_state;
static uint32_t g_generation = 0;
static bool g_vs_cpu = true;

static constexpr char kConfigPath[] = "/.crosspoint/othello.cfg";
static constexpr char kStatePath[]  = "/.crosspoint/othello-a.bin";

static void save_game() {
    if (!g_host || !g_host->write_file) return;

    // Save config (3 bytes: humanColour, level, vs_cpu)
    uint8_t cfg[3] = { g_state.humanColour, g_state.level, static_cast<uint8_t>(g_vs_cpu ? 1 : 0) };
    g_host->write_file(kConfigPath, cfg, 3);

    // Save state (kSaveBytes)
    uint8_t buf[pg::kSaveBytes] = {};
    pg::encode(g_state, ++g_generation, buf);
    g_host->write_file(kStatePath, buf, pg::kSaveBytes);
}

static void load_game() {
    if (!g_host || !g_host->read_file || !g_host->file_exists) return;

    if (g_host->file_exists(kConfigPath)) {
        uint8_t cfg[3] = {};
        uint32_t read_bytes = 0;
        if (g_host->read_file(kConfigPath, cfg, 3, &read_bytes) && read_bytes == 3) {
            g_state.humanColour = (cfg[0] == 2) ? 2 : 1;
            g_state.level = (cfg[1] <= 2) ? cfg[1] : 1;
            g_vs_cpu = (cfg[2] != 0);
        }
    }

    if (g_host->file_exists(kStatePath)) {
        uint8_t buf[pg::kSaveBytes] = {};
        uint32_t read_bytes = 0;
        if (g_host->read_file(kStatePath, buf, pg::kSaveBytes, &read_bytes) && read_bytes == pg::kSaveBytes) {
            uint32_t gen = 0;
            if (pg::decode(buf, pg::Kind::Othello, g_state, gen)) {
                g_generation = gen;
                return;
            }
        }
    }

    uint32_t seed = (g_host) ? g_host->get_millis() : 12345;
    pg::start(g_state, pg::Kind::Othello, seed, 1);
}

static void new_game() {
    uint32_t seed = (g_host) ? g_host->get_millis() : 12345;
    pg::start(g_state, pg::Kind::Othello, seed, g_state.level);
    save_game();
}

static void app_init(const StickyHostApi* host) {
    g_host = host;
    load_game();
}

static void app_render() {
    if (!g_host) return;

    g_host->clear(STICKY_COLOR_WHITE);
    g_host->draw_header("OTHELLO");

    int16_t sw = g_host->get_screen_width();
    int16_t sh = g_host->get_screen_height();

    int black_disks = pg::disks(g_state.othello, 2);
    int white_disks = pg::disks(g_state.othello, 1);

    // Status bar
    int banner_y = 65;
    g_host->fill_rect(20, banner_y, sw - 40, 48, STICKY_COLOR_LIGHTGRAY);
    g_host->draw_rect(20, banner_y, sw - 40, 48, STICKY_COLOR_BLACK);

    const char* status_str = "";
    if (g_state.othello.over) {
        if (black_disks > white_disks) status_str = "BLACK WINS!";
        else if (white_disks > black_disks) status_str = "WHITE WINS!";
        else status_str = "GAME TIED!";
    } else {
        if (g_vs_cpu) {
            status_str = (g_state.othello.turn == g_state.humanColour) ? "YOUR TURN" : "CPU THINKING...";
        } else {
            status_str = (g_state.othello.turn == 1) ? "WHITE'S TURN" : "BLACK'S TURN";
        }
    }
    int16_t tw, th;
    g_host->get_text_bounds(status_str, 2, &tw, &th);
    g_host->draw_text(sw / 2 - tw / 2, banner_y + 15, status_str, 2, STICKY_COLOR_BLACK);

    // Board layout: 8x8
    int cell_size = (sw - 40) / 8;
    int board_w = cell_size * 8;
    int board_left = (sw - board_w) / 2;
    int board_top = 125;
    int radius = cell_size * 38 / 100;

    g_host->draw_rect(board_left, board_top, board_w, board_w, STICKY_COLOR_BLACK);
    g_host->draw_rect(board_left - 1, board_top - 1, board_w + 2, board_w + 2, STICKY_COLOR_BLACK);

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            int sq = r * 8 + c;
            int bx = board_left + c * cell_size;
            int by = board_top + r * cell_size;

            g_host->draw_rect(bx, by, cell_size, cell_size, STICKY_COLOR_LIGHTGRAY);

            uint8_t cell = g_state.othello.board[sq];
            int cx = bx + cell_size / 2;
            int cy = by + cell_size / 2;

            if (cell == 1) { // White disk
                g_host->fill_circle(cx, cy, radius, STICKY_COLOR_WHITE);
                g_host->draw_circle(cx, cy, radius, STICKY_COLOR_BLACK);
                g_host->draw_circle(cx, cy, radius - 2, STICKY_COLOR_BLACK);
            } else if (cell == 2) { // Black disk
                g_host->fill_circle(cx, cy, radius, STICKY_COLOR_BLACK);
            } else {
                // If legal move for current turn, show dot
                if (!g_state.othello.over && pg::flips(g_state.othello, sq, g_state.othello.turn) > 0) {
                    g_host->fill_circle(cx, cy, 4, STICKY_COLOR_DARKGRAY);
                }
            }
        }
    }

    // Star dots on corners (2,2; 2,6; 6,2; 6,6)
    int star_coords[4][2] = {{2, 2}, {2, 6}, {6, 2}, {6, 6}};
    for (int i = 0; i < 4; ++i) {
        int sx = board_left + star_coords[i][1] * cell_size;
        int sy = board_top + star_coords[i][0] * cell_size;
        g_host->fill_circle(sx, sy, 3, STICKY_COLOR_BLACK);
    }

    // Disk count bar
    int score_y = board_top + board_w + 20;
    // Helper to format string
    char score_str[64];
    int spos = 0;
    auto append_int = [](char* b, int& pos, int val) {
        char temp[12];
        int tp = 0;
        if (val == 0) temp[tp++] = '0';
        else {
            while (val > 0) { temp[tp++] = '0' + (val % 10); val /= 10; }
        }
        for (int i = tp - 1; i >= 0; --i) b[pos++] = temp[i];
    };
    const char* s1 = "WHITE: ";
    while (*s1) score_str[spos++] = *s1++;
    append_int(score_str, spos, white_disks);
    const char* s2 = "   BLACK: ";
    while (*s2) score_str[spos++] = *s2++;
    append_int(score_str, spos, black_disks);
    score_str[spos] = '\0';

    g_host->get_text_bounds(score_str, 2, &tw, &th);
    g_host->draw_text(sw / 2 - tw / 2, score_y, score_str, 2, STICKY_COLOR_BLACK);

    // Controls at bottom
    int btn_y = score_y + 45;
    int btn_w = (sw - 60) / 2;
    int btn_h = 50;

    // New Game
    g_host->draw_rect(20, btn_y, btn_w, btn_h, STICKY_COLOR_BLACK);
    g_host->get_text_bounds("NEW GAME", 2, &tw, &th);
    g_host->draw_text(20 + (btn_w - tw) / 2, btn_y + 16, "NEW GAME", 2, STICKY_COLOR_BLACK);

    // Mode Toggle
    g_host->draw_rect(sw - 20 - btn_w, btn_y, btn_w, btn_h, STICKY_COLOR_BLACK);
    const char* mode_str = g_vs_cpu ? "MODE: CPU" : "MODE: 2P";
    g_host->get_text_bounds(mode_str, 2, &tw, &th);
    g_host->draw_text(sw - 20 - btn_w + (btn_w - tw) / 2, btn_y + 16, mode_str, 2, STICKY_COLOR_BLACK);

    g_host->request_update(false);
}

static void execute_bot_if_needed() {
    if (!g_vs_cpu || g_state.othello.over) return;
    if (g_state.othello.turn != g_state.humanColour) {
        g_host->delay_ms(150);
        pg::bot(g_state.othello, g_state.level);
        save_game();
        app_render();

        // If player has no moves and game isn't over, bot moves again
        if (!g_state.othello.over && !pg::hasMove(g_state.othello, g_state.humanColour)) {
            g_state.othello.turn = (g_state.humanColour == 1) ? 2 : 1;
            execute_bot_if_needed();
        }
    }
}

static void app_touch(int event, int16_t x, int16_t y) {
    if (event != STICKY_TOUCH_CLICK || !g_host) return;

    int16_t sw = g_host->get_screen_width();
    int cell_size = (sw - 40) / 8;
    int board_w = cell_size * 8;
    int board_left = (sw - board_w) / 2;
    int board_top = 125;

    // Board tap
    if (x >= board_left && x < board_left + board_w && y >= board_top && y < board_top + board_w) {
        if (g_state.othello.over) return;
        if (g_vs_cpu && g_state.othello.turn != g_state.humanColour) return;

        int col = (x - board_left) / cell_size;
        int row = (y - board_top) / cell_size;
        int sq = row * 8 + col;

        if (pg::flips(g_state.othello, sq, g_state.othello.turn) > 0) {
            pg::play(g_state.othello, sq);
            if (g_host) g_host->play_haptic(1);
            save_game();
            app_render();

            execute_bot_if_needed();
        }
        return;
    }

    // Button taps
    int score_y = board_top + board_w + 20;
    int btn_y = score_y + 45;
    int btn_w = (sw - 60) / 2;
    int btn_h = 50;

    if (y >= btn_y && y <= btn_y + btn_h) {
        if (x >= 20 && x <= 20 + btn_w) { // NEW GAME
            if (g_host) g_host->play_haptic(1);
            new_game();
            app_render();
        } else if (x >= sw - 20 - btn_w && x <= sw - 20) { // MODE TOGGLE
            if (g_host) g_host->play_haptic(1);
            g_vs_cpu = !g_vs_cpu;
            new_game();
            app_render();
        }
    }
}

static const StickyAppPlugin g_plugin = {
    .api_version = STICKY_APP_API_VERSION,
    .app_name    = "Othello",
    .app_version = "1.0",
    .app_author  = "CrossNext",
    .icon_name   = "extra_circle_dot_32",
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
