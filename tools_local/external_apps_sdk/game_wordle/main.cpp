#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "sticky_app_api.h"

#define WORD_LEN 5
#define MAX_GUESSES 6

enum LetterStatus { STATUS_EMPTY, STATUS_TYPED, STATUS_ABSENT, STATUS_PRESENT, STATUS_CORRECT };

static const char* kAnswerList[] = {
    "APPLE", "BEACH", "CHAIR", "DANCE", "EAGLE", "FLAME", "GRAPE", "HEART", "IMAGE", "JUICE",
    "KNIFE", "LEMON", "MUSIC", "NIGHT", "OCEAN", "PIANO", "QUEEN", "RIVER", "SMILE", "TRAIN",
    "UNCLE", "VOICE", "WATER", "YOUTH", "ZEBRA", "BREAD", "CLOUD", "DREAM", "EARTH", "FRUIT",
    "GLASS", "HOUSE", "LIGHT", "MONEY", "PLANT", "ROUND", "SPACE", "TABLE", "WORLD", "TIGER",
    "SMART", "BRAIN", "CLOCK", "PAPER", "STONE", "POWER", "MAGIC", "STORY", "SUGAR", "SWEET"
};
#define ANSWER_COUNT (sizeof(kAnswerList) / sizeof(kAnswerList[0]))

typedef struct {
    char answer[WORD_LEN + 1];
    char guesses[MAX_GUESSES][WORD_LEN + 1];
    uint8_t scores[MAX_GUESSES][WORD_LEN];
    uint8_t current_row;
    uint8_t current_col;
    bool game_over;
    bool game_won;
    uint32_t played;
    uint32_t won;
} WordleState;

static const StickyHostApi* g_host = nullptr;
static WordleState g_state;
static uint8_t g_key_status[26]; // A-Z

static constexpr char kSavePath[] = "/.crosspoint/wordle.sav";

static void save_state() {
    if (!g_host || !g_host->write_file) return;
    g_host->write_file(kSavePath, &g_state, sizeof(g_state));
}

static void new_game() {
    uint32_t played = g_state.played;
    uint32_t won = g_state.won;
    memset(&g_state, 0, sizeof(g_state));
    g_state.played = played;
    g_state.won = won;

    uint32_t seed = (g_host) ? g_host->get_millis() : 54321;
    int idx = seed % ANSWER_COUNT;
    strcpy(g_state.answer, kAnswerList[idx]);

    memset(g_key_status, 0, sizeof(g_key_status));
    save_state();
}

static void load_state() {
    if (g_host && g_host->read_file && g_host->file_exists && g_host->file_exists(kSavePath)) {
        uint32_t read_bytes = 0;
        if (g_host->read_file(kSavePath, &g_state, sizeof(g_state), &read_bytes) && read_bytes == sizeof(g_state)) {
            // Recompute key status
            memset(g_key_status, 0, sizeof(g_key_status));
            for (int r = 0; r < g_state.current_row; ++r) {
                for (int c = 0; c < WORD_LEN; ++c) {
                    char ch = g_state.guesses[r][c];
                    if (ch >= 'A' && ch <= 'Z') {
                        uint8_t s = g_state.scores[r][c];
                        if (s > g_key_status[ch - 'A']) {
                            g_key_status[ch - 'A'] = s;
                        }
                    }
                }
            }
            return;
        }
    }
    new_game();
}

static void score_guess(const char* guess, const char* answer, uint8_t* out_scores) {
    bool ans_used[WORD_LEN] = {};
    for (int i = 0; i < WORD_LEN; ++i) out_scores[i] = STATUS_ABSENT;

    // 1st pass: exact matches (Green / Correct)
    for (int i = 0; i < WORD_LEN; ++i) {
        if (guess[i] == answer[i]) {
            out_scores[i] = STATUS_CORRECT;
            ans_used[i] = true;
        }
    }

    // 2nd pass: misplaced matches (Present)
    for (int i = 0; i < WORD_LEN; ++i) {
        if (out_scores[i] == STATUS_CORRECT) continue;
        for (int j = 0; j < WORD_LEN; ++j) {
            if (!ans_used[j] && guess[i] == answer[j]) {
                out_scores[i] = STATUS_PRESENT;
                ans_used[j] = true;
                break;
            }
        }
    }
}

static void submit_row() {
    if (g_state.current_col != WORD_LEN || g_state.game_over) return;

    char* curr_guess = g_state.guesses[g_state.current_row];
    curr_guess[WORD_LEN] = '\0';
    uint8_t* row_scores = g_state.scores[g_state.current_row];

    score_guess(curr_guess, g_state.answer, row_scores);

    // Update keys
    for (int i = 0; i < WORD_LEN; ++i) {
        char ch = curr_guess[i];
        if (ch >= 'A' && ch <= 'Z') {
            if (row_scores[i] > g_key_status[ch - 'A']) {
                g_key_status[ch - 'A'] = row_scores[i];
            }
        }
    }

    if (strcmp(curr_guess, g_state.answer) == 0) {
        g_state.game_over = true;
        g_state.game_won = true;
        g_state.played++;
        g_state.won++;
    } else {
        g_state.current_row++;
        g_state.current_col = 0;
        if (g_state.current_row >= MAX_GUESSES) {
            g_state.game_over = true;
            g_state.game_won = false;
            g_state.played++;
        }
    }

    if (g_host) g_host->play_haptic(1);
    save_state();
}

static void app_init(const StickyHostApi* host) {
    g_host = host;
    load_state();
}

static void app_render() {
    if (!g_host) return;

    g_host->clear(STICKY_COLOR_WHITE);
    g_host->draw_header("WORDLE");

    int16_t sw = g_host->get_screen_width();
    int16_t sh = g_host->get_screen_height();

    // Status banner
    int banner_y = 65;
    g_host->fill_rect(20, banner_y, sw - 40, 42, STICKY_COLOR_LIGHTGRAY);
    g_host->draw_rect(20, banner_y, sw - 40, 42, STICKY_COLOR_BLACK);

    const char* status_str = "GUESS THE 5-LETTER WORD";
    if (g_state.game_over) {
        status_str = g_state.game_won ? "BRILLIANT! YOU GUESSED IT" : g_state.answer;
    }
    int16_t tw, th;
    g_host->get_text_bounds(status_str, 2, &tw, &th);
    g_host->draw_text(sw / 2 - tw / 2, banner_y + 12, status_str, 2, STICKY_COLOR_BLACK);

    // 6 rows x 5 tiles layout
    int tile_size = 54;
    int gap = 8;
    int grid_w = tile_size * WORD_LEN + gap * (WORD_LEN - 1);
    int grid_left = (sw - grid_w) / 2;
    int grid_top = 120;

    for (int r = 0; r < MAX_GUESSES; ++r) {
        for (int c = 0; c < WORD_LEN; ++c) {
            int tx = grid_left + c * (tile_size + gap);
            int ty = grid_top + r * (tile_size + gap);

            uint8_t score = g_state.scores[r][c];
            char letter = g_state.guesses[r][c];

            if (r < g_state.current_row || (r == g_state.current_row && g_state.game_over)) {
                if (score == STATUS_CORRECT) {
                    // Correct: Solid black tile, white text
                    g_host->fill_rect(tx, ty, tile_size, tile_size, STICKY_COLOR_BLACK);
                    char lstr[2] = { letter, '\0' };
                    g_host->get_text_bounds(lstr, 3, &tw, &th);
                    g_host->draw_text(tx + (tile_size - tw) / 2, ty + (tile_size - th) / 2, lstr, 3, STICKY_COLOR_WHITE);
                } else if (score == STATUS_PRESENT) {
                    // Present: Dark gray / double line tile
                    g_host->fill_rect(tx, ty, tile_size, tile_size, STICKY_COLOR_LIGHTGRAY);
                    g_host->draw_rect(tx, ty, tile_size, tile_size, STICKY_COLOR_BLACK);
                    char lstr[2] = { letter, '\0' };
                    g_host->get_text_bounds(lstr, 3, &tw, &th);
                    g_host->draw_text(tx + (tile_size - tw) / 2, ty + (tile_size - th) / 2, lstr, 3, STICKY_COLOR_BLACK);
                } else {
                    // Absent: Light gray border, strike line
                    g_host->draw_rect(tx, ty, tile_size, tile_size, STICKY_COLOR_LIGHTGRAY);
                    char lstr[2] = { letter, '\0' };
                    g_host->get_text_bounds(lstr, 3, &tw, &th);
                    g_host->draw_text(tx + (tile_size - tw) / 2, ty + (tile_size - th) / 2, lstr, 3, STICKY_COLOR_DARKGRAY);
                }
            } else {
                // Empty / Currently typing row
                g_host->draw_rect(tx, ty, tile_size, tile_size, STICKY_COLOR_BLACK);
                if (letter) {
                    char lstr[2] = { letter, '\0' };
                    g_host->get_text_bounds(lstr, 3, &tw, &th);
                    g_host->draw_text(tx + (tile_size - tw) / 2, ty + (tile_size - th) / 2, lstr, 3, STICKY_COLOR_BLACK);
                }
            }
        }
    }

    // Keyboard layout (QWERTY, 3 rows)
    const char* kb_rows[3] = { "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM" };
    int kb_top = grid_top + MAX_GUESSES * (tile_size + gap) + 16;
    int key_h = 44;

    for (int row = 0; row < 3; ++row) {
        int count = strlen(kb_rows[row]);
        int key_w = 40;
        int row_w = count * (key_w + 4);
        if (row == 2) row_w += 2 * (54 + 4); // Include ENTER and DEL

        int rx = (sw - row_w) / 2;
        int ry = kb_top + row * (key_h + 6);

        if (row == 2) {
            // ENTER key
            g_host->draw_rect(rx, ry, 54, key_h, STICKY_COLOR_BLACK);
            g_host->get_text_bounds("ENT", 1, &tw, &th);
            g_host->draw_text(rx + (54 - tw) / 2, ry + (key_h - th) / 2, "ENT", 1, STICKY_COLOR_BLACK);
            rx += 58;
        }

        for (int i = 0; i < count; ++i) {
            char k = kb_rows[row][i];
            uint8_t ks = g_key_status[k - 'A'];
            if (ks == STATUS_CORRECT) {
                g_host->fill_rect(rx, ry, key_w, key_h, STICKY_COLOR_BLACK);
                char ks_str[2] = { k, '\0' };
                g_host->get_text_bounds(ks_str, 2, &tw, &th);
                g_host->draw_text(rx + (key_w - tw) / 2, ry + (key_h - th) / 2, ks_str, 2, STICKY_COLOR_WHITE);
            } else if (ks == STATUS_PRESENT) {
                g_host->fill_rect(rx, ry, key_w, key_h, STICKY_COLOR_LIGHTGRAY);
                g_host->draw_rect(rx, ry, key_w, key_h, STICKY_COLOR_BLACK);
                char ks_str[2] = { k, '\0' };
                g_host->get_text_bounds(ks_str, 2, &tw, &th);
                g_host->draw_text(rx + (key_w - tw) / 2, ry + (key_h - th) / 2, ks_str, 2, STICKY_COLOR_BLACK);
            } else if (ks == STATUS_ABSENT) {
                g_host->draw_rect(rx, ry, key_w, key_h, STICKY_COLOR_LIGHTGRAY);
                char ks_str[2] = { k, '\0' };
                g_host->get_text_bounds(ks_str, 2, &tw, &th);
                g_host->draw_text(rx + (key_w - tw) / 2, ry + (key_h - th) / 2, ks_str, 2, STICKY_COLOR_DARKGRAY);
            } else {
                g_host->draw_rect(rx, ry, key_w, key_h, STICKY_COLOR_BLACK);
                char ks_str[2] = { k, '\0' };
                g_host->get_text_bounds(ks_str, 2, &tw, &th);
                g_host->draw_text(rx + (key_w - tw) / 2, ry + (key_h - th) / 2, ks_str, 2, STICKY_COLOR_BLACK);
            }
            rx += key_w + 4;
        }

        if (row == 2) {
            // DEL key
            g_host->draw_rect(rx, ry, 54, key_h, STICKY_COLOR_BLACK);
            g_host->get_text_bounds("DEL", 1, &tw, &th);
            g_host->draw_text(rx + (54 - tw) / 2, ry + (key_h - th) / 2, "DEL", 1, STICKY_COLOR_BLACK);
        }
    }

    // New Game button if game over
    if (g_state.game_over) {
        int btn_y = kb_top + 3 * (key_h + 6) + 12;
        int btn_w = 180;
        int btn_h = 44;
        g_host->draw_rect(sw / 2 - btn_w / 2, btn_y, btn_w, btn_h, STICKY_COLOR_BLACK);
        g_host->get_text_bounds("PLAY AGAIN", 2, &tw, &th);
        g_host->draw_text(sw / 2 - tw / 2, btn_y + 13, "PLAY AGAIN", 2, STICKY_COLOR_BLACK);
    }

    g_host->request_update(false);
}

static void app_touch(int event, int16_t x, int16_t y) {
    if (event != STICKY_TOUCH_CLICK || !g_host) return;

    int16_t sw = g_host->get_screen_width();
    int tile_size = 54;
    int gap = 8;
    int grid_top = 120;
    int kb_top = grid_top + MAX_GUESSES * (tile_size + gap) + 16;
    int key_h = 44;

    // Check Play Again button
    if (g_state.game_over) {
        int btn_y = kb_top + 3 * (key_h + 6) + 12;
        int btn_w = 180;
        int btn_h = 44;
        if (x >= sw / 2 - btn_w / 2 && x <= sw / 2 + btn_w / 2 && y >= btn_y && y <= btn_y + btn_h) {
            if (g_host) g_host->play_haptic(1);
            new_game();
            app_render();
            return;
        }
    }

    // Keyboard taps
    const char* kb_rows[3] = { "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM" };
    for (int row = 0; row < 3; ++row) {
        int count = strlen(kb_rows[row]);
        int key_w = 40;
        int row_w = count * (key_w + 4);
        if (row == 2) row_w += 2 * (54 + 4);

        int rx = (sw - row_w) / 2;
        int ry = kb_top + row * (key_h + 6);

        if (y >= ry && y < ry + key_h) {
            if (row == 2) {
                // Check ENTER
                if (x >= rx && x < rx + 54) {
                    submit_row();
                    app_render();
                    return;
                }
                rx += 58;
            }

            for (int i = 0; i < count; ++i) {
                if (x >= rx && x < rx + key_w) {
                    char ch = kb_rows[row][i];
                    if (!g_state.game_over && g_state.current_col < WORD_LEN) {
                        g_state.guesses[g_state.current_row][g_state.current_col++] = ch;
                        if (g_host) g_host->play_haptic(1);
                        app_render();
                    }
                    return;
                }
                rx += key_w + 4;
            }

            if (row == 2) {
                // Check DEL
                if (x >= rx && x < rx + 54) {
                    if (!g_state.game_over && g_state.current_col > 0) {
                        g_state.current_col--;
                        g_state.guesses[g_state.current_row][g_state.current_col] = '\0';
                        if (g_host) g_host->play_haptic(1);
                        app_render();
                    }
                    return;
                }
            }
        }
    }
}

static const StickyAppPlugin g_plugin = {
    .api_version = STICKY_APP_API_VERSION,
    .app_name    = "Wordle",
    .app_version = "1.0",
    .app_author  = "CrossNext",
    .icon_name   = "icon_wordle_32",
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
