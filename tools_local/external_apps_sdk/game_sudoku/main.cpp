#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "sticky_app_api.h"
#include "SudokuCore.h"
#include "SudokuGame.h"

namespace sk = sudoku;

static const StickyHostApi* g_host = nullptr;
static sk::Game g_game;
static sk::Workspace g_work;
static uint32_t g_rng = 123456789;
static int8_t g_selected_cell = 0;
static bool g_notes_mode = false;
static sk::Level g_level = sk::Level::Easy;

static constexpr char kStatePath[] = "/.crosspoint/sudoku.sav";
static constexpr int kStateBytes = 768;
static constexpr int kStateVersion = 1;

static void save_state() {
    if (!g_host || !g_host->write_file) return;

    char buffer[kStateBytes];
    int used = 0;

    auto append_str = [&](const char* s) {
        while (*s && used < kStateBytes - 1) buffer[used++] = *s++;
    };
    auto append_int = [&](long val) {
        char temp[16];
        int tp = 0;
        if (val == 0) temp[tp++] = '0';
        else {
            if (val < 0) { buffer[used++] = '-'; val = -val; }
            while (val > 0) {
                temp[tp++] = '0' + (val % 10);
                val /= 10;
            }
        }
        for (int i = tp - 1; i >= 0 && used < kStateBytes - 1; --i) buffer[used++] = temp[i];
    };

    // Header: version, level, elapsedMs, hintsUsed, armed, ...
    append_int(kStateVersion); buffer[used++] = ' ';
    append_int(static_cast<long>(g_level)); buffer[used++] = ' ';
    append_int(g_game.elapsedMs); buffer[used++] = ' ';
    append_int(g_game.hintsUsed); buffer[used++] = ' ';
    append_int(g_game.armed); buffer[used++] = ' ';
    // Pad remaining header elements to 18
    for (int i = 5; i < 18; ++i) {
        append_int(0);
        buffer[used++] = (i == 17) ? '\n' : ' ';
    }

    // Clues (81 digits)
    for (int cell = 0; cell < sk::kCells; ++cell) {
        buffer[used++] = '0' + g_game.puzzle.given[cell];
    }
    buffer[used++] = '\n';

    // Entries (81 digits)
    for (int cell = 0; cell < sk::kCells; ++cell) {
        buffer[used++] = '0' + (g_game.entry[cell] <= 9 ? g_game.entry[cell] : 0);
    }
    buffer[used++] = '\n';

    // Notes (81 x 3 hex chars)
    static const char kHex[] = "0123456789ABCDEF";
    for (int cell = 0; cell < sk::kCells; ++cell) {
        sk::Mask m = g_game.note[cell];
        buffer[used++] = kHex[(m >> 8) & 0xF];
        buffer[used++] = kHex[(m >> 4) & 0xF];
        buffer[used++] = kHex[m & 0xF];
    }
    buffer[used++] = '\n';
    buffer[used] = '\0';

    g_host->write_file(kStatePath, buffer, used);
}

static bool load_state() {
    if (!g_host || !g_host->read_file || !g_host->file_exists || !g_host->file_exists(kStatePath)) return false;

    char buffer[kStateBytes];
    uint32_t read_bytes = 0;
    if (!g_host->read_file(kStatePath, buffer, kStateBytes, &read_bytes) || read_bytes == 0) return false;
    buffer[read_bytes < kStateBytes ? read_bytes : kStateBytes - 1] = '\0';

    char* cursor = buffer;
    long header[18] = {};
    for (int i = 0; i < 18; ++i) {
        char* next = nullptr;
        long val = strtol(cursor, &next, 10);
        if (next == cursor) return false;
        header[i] = val;
        cursor = next;
    }
    if (header[0] != kStateVersion) return false;

    while (*cursor == ' ' || *cursor == '\n' || *cursor == '\r') ++cursor;
    for (int cell = 0; cell < sk::kCells; ++cell) {
        if (*cursor < '0' || *cursor > '9') return false;
        g_game.puzzle.given[cell] = *cursor++ - '0';
    }

    while (*cursor == ' ' || *cursor == '\n' || *cursor == '\r') ++cursor;
    for (int cell = 0; cell < sk::kCells; ++cell) {
        if (*cursor < '0' || *cursor > '9') return false;
        g_game.entry[cell] = g_game.puzzle.given[cell] != 0 ? 0 : (*cursor++ - '0');
    }

    while (*cursor == ' ' || *cursor == '\n' || *cursor == '\r') ++cursor;
    for (int cell = 0; cell < sk::kCells; ++cell) {
        char hex[4] = {cursor[0], cursor[1], cursor[2], '\0'};
        cursor += 3;
        g_game.note[cell] = static_cast<sk::Mask>(strtol(hex, nullptr, 16) & sk::kAllDigits);
    }

    g_level = static_cast<sk::Level>(header[1] >= 0 && header[1] < sk::kLevelCount ? header[1] : 0);
    g_game.elapsedMs = static_cast<uint32_t>(header[2]);
    g_game.hintsUsed = static_cast<uint8_t>(header[3]);
    g_game.armed = static_cast<uint8_t>(header[4] >= 1 && header[4] <= sk::kSize ? header[4] : 1);

    // Derive solution
    sk::Grid grid;
    if (sk::load(grid, g_game.puzzle.given)) {
        sk::SolveReport rep = sk::solve(grid, sk::ceilingFor(sk::Level::Expert));
        if (rep.solved && !rep.broken) {
            for (int c = 0; c < sk::kCells; ++c) g_game.puzzle.solution[c] = grid.value[c];
        }
    }
    return true;
}

static void new_puzzle(sk::Level level) {
    g_level = level;
    memset(&g_game, 0, sizeof(g_game));
    g_game.armed = 1;

    for (int att = 0; att < 30; ++att) {
        if (sk::generate(g_game.puzzle, level, g_work, g_rng, 10)) {
            break;
        }
    }
    save_state();
}

static void app_init(const StickyHostApi* host) {
    g_host = host;
    if (g_host) {
        g_rng = g_host->get_millis() ^ 0x5DEECE66DULL;
    }
    if (!load_state()) {
        new_puzzle(sk::Level::Easy);
    }
}

static void app_render() {
    if (!g_host) return;

    g_host->clear(STICKY_COLOR_WHITE);
    g_host->draw_header("SUDOKU");

    int16_t sw = g_host->get_screen_width();
    int16_t sh = g_host->get_screen_height();

    // Difficulty and status banner
    int banner_y = 65;
    g_host->fill_rect(20, banner_y, sw - 40, 40, STICKY_COLOR_LIGHTGRAY);
    g_host->draw_rect(20, banner_y, sw - 40, 40, STICKY_COLOR_BLACK);

    const char* diff_str = sk::levelName(g_level);
    int16_t tw, th;
    g_host->get_text_bounds(diff_str, 2, &tw, &th);
    g_host->draw_text(30, banner_y + 11, diff_str, 2, STICKY_COLOR_BLACK);

    bool is_solved = true;
    for (int c = 0; c < sk::kCells; ++c) {
        uint8_t v = sk::valueAt(g_game, c);
        if (v == 0 || v != g_game.puzzle.solution[c]) {
            is_solved = false;
            break;
        }
    }

    if (is_solved) {
        g_host->get_text_bounds("SOLVED!", 2, &tw, &th);
        g_host->draw_text(sw - 30 - tw, banner_y + 11, "SOLVED!", 2, STICKY_COLOR_BLACK);
    } else {
        const char* mode_str = g_notes_mode ? "NOTE MODE" : "PEN MODE";
        g_host->get_text_bounds(mode_str, 2, &tw, &th);
        g_host->draw_text(sw - 30 - tw, banner_y + 11, mode_str, 2, STICKY_COLOR_BLACK);
    }

    // Grid layout: 9x9 cells
    int cell_size = (sw - 40) / 9; // ~48px
    int grid_w = cell_size * 9;
    int grid_left = (sw - grid_w) / 2;
    int grid_top = 115;

    // Outer border
    g_host->draw_rect(grid_left, grid_top, grid_w, grid_w, STICKY_COLOR_BLACK);
    g_host->draw_rect(grid_left - 1, grid_top - 1, grid_w + 2, grid_w + 2, STICKY_COLOR_BLACK);

    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            int cell_idx = r * 9 + c;
            int cx = grid_left + c * cell_size;
            int cy = grid_top + r * cell_size;

            // Selected cell highlight
            if (cell_idx == g_selected_cell) {
                g_host->fill_rect(cx + 1, cy + 1, cell_size - 1, cell_size - 1, STICKY_COLOR_LIGHTGRAY);
            }

            // Cell border
            g_host->draw_rect(cx, cy, cell_size, cell_size, STICKY_COLOR_LIGHTGRAY);

            // Draw digit or notes
            uint8_t val = sk::valueAt(g_game, cell_idx);
            if (val != 0) {
                char d_str[2] = { static_cast<char>('0' + val), '\0' };
                g_host->get_text_bounds(d_str, 3, &tw, &th);
                // Given clues are drawn bolder or black; player entries normal
                g_host->draw_text(cx + (cell_size - tw) / 2, cy + (cell_size - th) / 2, d_str, 3, STICKY_COLOR_BLACK);
                if (sk::isGiven(g_game, cell_idx)) {
                    // Double strike to indicate given clue
                    g_host->draw_text(cx + (cell_size - tw) / 2 + 1, cy + (cell_size - th) / 2, d_str, 3, STICKY_COLOR_BLACK);
                }
            } else {
                // Draw notes
                sk::Mask m = sk::visibleNotes(g_game, cell_idx);
                if (m != 0) {
                    for (int d = 1; d <= 9; ++d) {
                        if (m & sk::bitFor(d)) {
                            int nr = (d - 1) / 3;
                            int nc = (d - 1) % 3;
                            char nd[2] = { static_cast<char>('0' + d), '\0' };
                            g_host->draw_text(cx + 4 + nc * 14, cy + 2 + nr * 14, nd, 0, STICKY_COLOR_DARKGRAY);
                        }
                    }
                }
            }
        }
    }

    // Draw thick lines for 3x3 box boundaries
    for (int i = 1; i < 3; ++i) {
        int x = grid_left + i * 3 * cell_size;
        int y = grid_top + i * 3 * cell_size;
        g_host->draw_line(x, grid_top, x, grid_top + grid_w, STICKY_COLOR_BLACK);
        g_host->draw_line(x - 1, grid_top, x - 1, grid_top + grid_w, STICKY_COLOR_BLACK);
        g_host->draw_line(grid_left, y, grid_left + grid_w, y, STICKY_COLOR_BLACK);
        g_host->draw_line(grid_left, y - 1, grid_left + grid_w, y - 1, STICKY_COLOR_BLACK);
    }

    // Keypad: digits 1 to 9
    int keypad_y = grid_top + grid_w + 20;
    int key_w = (sw - 40) / 9;
    int key_h = 52;
    for (int d = 1; d <= 9; ++d) {
        int kx = 20 + (d - 1) * key_w;
        g_host->draw_rect(kx, keypad_y, key_w - 2, key_h, STICKY_COLOR_BLACK);
        char d_str[2] = { static_cast<char>('0' + d), '\0' };
        g_host->get_text_bounds(d_str, 3, &tw, &th);
        g_host->draw_text(kx + (key_w - 2 - tw) / 2, keypad_y + (key_h - th) / 2, d_str, 3, STICKY_COLOR_BLACK);
    }

    // Action buttons: ERASE | NOTE | HINT | NEW
    int act_y = keypad_y + key_h + 16;
    int act_w = (sw - 50) / 4;
    int act_h = 48;

    const char* actions[4] = { "ERASE", g_notes_mode ? "PEN" : "NOTE", "HINT", "NEW" };
    for (int i = 0; i < 4; ++i) {
        int ax = 20 + i * (act_w + 3);
        if (i == 1 && g_notes_mode) {
            g_host->fill_rect(ax, act_y, act_w, act_h, STICKY_COLOR_LIGHTGRAY);
        }
        g_host->draw_rect(ax, act_y, act_w, act_h, STICKY_COLOR_BLACK);
        g_host->get_text_bounds(actions[i], 1, &tw, &th);
        g_host->draw_text(ax + (act_w - tw) / 2, act_y + (act_h - th) / 2, actions[i], 1, STICKY_COLOR_BLACK);
    }

    g_host->request_update(false);
}

static void app_touch(int event, int16_t x, int16_t y) {
    if (event != STICKY_TOUCH_CLICK || !g_host) return;

    int16_t sw = g_host->get_screen_width();
    int cell_size = (sw - 40) / 9;
    int grid_w = cell_size * 9;
    int grid_left = (sw - grid_w) / 2;
    int grid_top = 115;

    // Check grid tap
    if (x >= grid_left && x < grid_left + grid_w && y >= grid_top && y < grid_top + grid_w) {
        int c = (x - grid_left) / cell_size;
        int r = (y - grid_top) / cell_size;
        g_selected_cell = r * 9 + c;
        if (g_host) g_host->play_haptic(1);
        app_render();
        return;
    }

    // Keypad tap (1 to 9)
    int keypad_y = grid_top + grid_w + 20;
    int key_w = (sw - 40) / 9;
    int key_h = 52;
    if (y >= keypad_y && y < keypad_y + key_h && x >= 20 && x < sw - 20) {
        int digit = 1 + (x - 20) / key_w;
        if (digit >= 1 && digit <= 9 && g_selected_cell >= 0 && g_selected_cell < sk::kCells) {
            if (!sk::isGiven(g_game, g_selected_cell)) {
                if (g_notes_mode) {
                    // Toggle note candidate
                    g_game.note[g_selected_cell] ^= sk::bitFor(digit);
                } else {
                    // Place or clear digit
                    if (g_game.entry[g_selected_cell] == digit) {
                        g_game.entry[g_selected_cell] = 0;
                    } else {
                        g_game.entry[g_selected_cell] = digit;
                    }
                }
                if (g_host) g_host->play_haptic(1);
                save_state();
                app_render();
            }
        }
        return;
    }

    // Action buttons tap
    int act_y = keypad_y + key_h + 16;
    int act_w = (sw - 50) / 4;
    int act_h = 48;
    if (y >= act_y && y < act_y + act_h) {
        for (int i = 0; i < 4; ++i) {
            int ax = 20 + i * (act_w + 3);
            if (x >= ax && x < ax + act_w) {
                if (g_host) g_host->play_haptic(1);
                if (i == 0) { // ERASE
                    if (g_selected_cell >= 0 && !sk::isGiven(g_game, g_selected_cell)) {
                        g_game.entry[g_selected_cell] = 0;
                        g_game.note[g_selected_cell] = 0;
                        save_state();
                        app_render();
                    }
                } else if (i == 1) { // TOGGLE NOTE MODE
                    g_notes_mode = !g_notes_mode;
                    app_render();
                } else if (i == 2) { // HINT
                    if (g_selected_cell >= 0 && !sk::isGiven(g_game, g_selected_cell)) {
                        g_game.entry[g_selected_cell] = g_game.puzzle.solution[g_selected_cell];
                        g_game.hintsUsed++;
                        save_state();
                        app_render();
                    }
                } else if (i == 3) { // NEW PUZZLE (cycles difficulty)
                    sk::Level next_lvl = static_cast<sk::Level>((static_cast<int>(g_level) + 1) % sk::kLevelCount);
                    new_puzzle(next_lvl);
                    app_render();
                }
                return;
            }
        }
    }
}

static const StickyAppPlugin g_plugin = {
    .api_version = STICKY_APP_API_VERSION,
    .app_name    = "Sudoku",
    .app_version = "1.0",
    .app_author  = "CrossNext",
    .icon_name   = "icon_sudoku_32",
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
