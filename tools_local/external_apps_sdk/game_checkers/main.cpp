#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "sticky_app_api.h"
#include "DraughtsCore.h"

namespace dr = draughts;

static const StickyHostApi* g_host = nullptr;
static dr::Game g_game;
static dr::Generator g_gen;
static int8_t g_selected = -1;
static uint32_t g_rng = 123456789;
static uint8_t g_difficulty = 1;
static bool g_vs_cpu = true;
static bool g_human_black = false;

static constexpr char kConfigPath[] = "/.crosspoint/draughts.cfg";

static void save_config() {
    if (!g_host || !g_host->write_file) return;
    uint8_t bytes[8] = {
        'D', 'R', 2,
        static_cast<uint8_t>(g_game.rules),
        g_difficulty,
        static_cast<uint8_t>(g_vs_cpu ? 0 : 1),
        static_cast<uint8_t>(g_human_black ? 1 : 0),
        static_cast<uint8_t>(static_cast<uint8_t>(g_game.rules) ^ g_difficulty ^ (g_vs_cpu ? 0 : 1) ^ (g_human_black ? 1 : 0) ^ 0xa5)
    };
    g_host->write_file(kConfigPath, bytes, 8);
}

static void load_config() {
    if (!g_host || !g_host->read_file || !g_host->file_exists || !g_host->file_exists(kConfigPath)) return;
    uint8_t bytes[8] = {};
    uint32_t read_bytes = 0;
    if (g_host->read_file(kConfigPath, bytes, 8, &read_bytes) && read_bytes == 8) {
        if (bytes[0] == 'D' && bytes[1] == 'R') {
            if (bytes[3] < 4 && bytes[4] < 3 && bytes[5] < 2 && bytes[6] < 2) {
                g_game.rules = static_cast<dr::Rules>(bytes[3]);
                g_difficulty = bytes[4];
                g_vs_cpu = (bytes[5] == 0);
                g_human_black = (bytes[6] == 1);
            }
        }
    }
}

static void new_game() {
    dr::start(g_game, dr::Rules::English); // 8x8 standard English checkers
    g_gen.begin(g_game);
    while (!g_gen.ready()) g_gen.tick(1024);
    g_selected = -1;
    save_config();
}

static void app_init(const StickyHostApi* host) {
    g_host = host;
    if (g_host) g_rng = g_host->get_millis() | 1;
    load_config();
    new_game();
}

static void app_render() {
    if (!g_host) return;

    g_host->clear(STICKY_COLOR_WHITE);
    g_host->draw_header("CHECKERS");

    int16_t sw = g_host->get_screen_width();
    int16_t sh = g_host->get_screen_height();
    int size = g_game.size; // 8 for English

    // Status bar
    int banner_y = 65;
    g_host->fill_rect(20, banner_y, sw - 40, 42, STICKY_COLOR_LIGHTGRAY);
    g_host->draw_rect(20, banner_y, sw - 40, 42, STICKY_COLOR_BLACK);

    const char* turn_str = "";
    if (g_gen.count() == 0 || g_game.draw) {
        turn_str = g_game.draw ? "DRAW GAME!" : (g_game.turn == 0 ? "BLACK WINS!" : "WHITE WINS!");
    } else {
        if (g_vs_cpu) {
            turn_str = (g_game.turn == (g_human_black ? 1 : 0)) ? "YOUR TURN" : "CPU THINKING...";
        } else {
            turn_str = (g_game.turn == 0) ? "WHITE TURN" : "BLACK TURN";
        }
    }
    int16_t tw, th;
    g_host->get_text_bounds(turn_str, 2, &tw, &th);
    g_host->draw_text(sw / 2 - tw / 2, banner_y + 12, turn_str, 2, STICKY_COLOR_BLACK);

    // Board layout
    int cell_size = (sw - 40) / size;
    int board_w = cell_size * size;
    int board_left = (sw - board_w) / 2;
    int board_top = 120;

    g_host->draw_rect(board_left, board_top, board_w, board_w, STICKY_COLOR_BLACK);
    g_host->draw_rect(board_left - 1, board_top - 1, board_w + 2, board_w + 2, STICKY_COLOR_BLACK);

    int radius = cell_size * 36 / 100;

    for (int display = 0; display < size * size; ++display) {
        int n = g_human_black ? (size * size - 1 - display) : display;
        int row = display / size;
        int col = display % size;
        int bx = board_left + col * cell_size;
        int by = board_top + row * cell_size;

        bool dark_square = ((n % size) + (n / size)) % 2 != 0;
        if (dark_square) {
            g_host->fill_rect(bx, by, cell_size, cell_size, STICKY_COLOR_LIGHTGRAY);
        }

        // Selection highlight
        if (n == g_selected) {
            g_host->draw_rect(bx + 2, by + 2, cell_size - 4, cell_size - 4, STICKY_COLOR_BLACK);
            g_host->draw_rect(bx + 3, by + 3, cell_size - 6, cell_size - 6, STICKY_COLOR_BLACK);
        } else if (g_selected >= 0 && g_gen.permits(g_selected, n)) {
            // Legal move destination
            g_host->fill_circle(bx + cell_size / 2, by + cell_size / 2, 6, STICKY_COLOR_DARKGRAY);
        }

        uint8_t piece = g_game.cells[n];
        if (piece != 0 && !(piece & dr::kTaken)) {
            bool is_black = dr::owner(piece) == 1;
            bool is_king = dr::king(piece);
            int cx = bx + cell_size / 2;
            int cy = by + cell_size / 2;

            if (is_black) {
                g_host->fill_circle(cx, cy, radius, STICKY_COLOR_BLACK);
                if (is_king) {
                    g_host->draw_circle(cx, cy, radius / 2, STICKY_COLOR_WHITE);
                    g_host->draw_circle(cx, cy, radius / 2 - 1, STICKY_COLOR_WHITE);
                }
            } else {
                g_host->fill_circle(cx, cy, radius, STICKY_COLOR_WHITE);
                g_host->draw_circle(cx, cy, radius, STICKY_COLOR_BLACK);
                g_host->draw_circle(cx, cy, radius - 2, STICKY_COLOR_BLACK);
                if (is_king) {
                    g_host->draw_circle(cx, cy, radius / 2, STICKY_COLOR_BLACK);
                }
            }
        }
    }

    // Controls at bottom
    int btn_y = board_top + board_w + 30;
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
    if (!g_vs_cpu || g_gen.count() == 0 || g_game.draw) return;
    bool is_bot_turn = (g_game.turn != (g_human_black ? 1 : 0));
    if (!is_bot_turn) return;

    g_host->delay_ms(150);
    int chosen_idx = dr::choose(g_game, g_gen, g_difficulty, g_rng);
    if (chosen_idx >= 0 && chosen_idx < g_gen.count()) {
        const auto& opt = g_gen.at(chosen_idx);
        dr::play(g_game, g_gen, opt.move.from, opt.move.to);
        g_gen.begin(g_game);
        while (!g_gen.ready()) g_gen.tick(1024);
        g_selected = -1;
        save_config();
        app_render();
    }
}

static void app_touch(int event, int16_t x, int16_t y) {
    if (event != STICKY_TOUCH_CLICK || !g_host) return;

    int16_t sw = g_host->get_screen_width();
    int size = g_game.size;
    int cell_size = (sw - 40) / size;
    int board_w = cell_size * size;
    int board_left = (sw - board_w) / 2;
    int board_top = 120;

    // Board tap
    if (x >= board_left && x < board_left + board_w && y >= board_top && y < board_top + board_w) {
        int col = (x - board_left) / cell_size;
        int row = (y - board_top) / cell_size;
        int display = row * size + col;
        int n = g_human_black ? (size * size - 1 - display) : display;

        if (g_selected >= 0 && g_gen.permits(g_selected, n)) {
            // Play selected move
            dr::play(g_game, g_gen, g_selected, n);
            g_gen.begin(g_game);
            while (!g_gen.ready()) g_gen.tick(1024);
            g_selected = -1;
            if (g_host) g_host->play_haptic(1);
            save_config();
            app_render();

            execute_bot_if_needed();
            return;
        }

        // Check if tapping a piece owned by current turn
        uint8_t piece = g_game.cells[n];
        if (piece != 0 && dr::owner(piece) == g_game.turn) {
            g_selected = n;
            if (g_host) g_host->play_haptic(1);
            app_render();
            return;
        }

        g_selected = -1;
        app_render();
        return;
    }

    // Button taps
    int btn_y = board_top + board_w + 30;
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
    .app_name    = "Checkers",
    .app_version = "1.0",
    .app_author  = "CrossNext",
    .icon_name   = "icon_checkers_32",
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
