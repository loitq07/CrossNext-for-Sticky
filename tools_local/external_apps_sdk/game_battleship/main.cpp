#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "sticky_app_api.h"
#include "BattleshipCore.h"

namespace bs = bship;

static const StickyHostApi* g_host = nullptr;
static bs::Game g_game;
static uint32_t g_rng = 987654321;
static char g_notice[48] = "TAP TO FIRE AT ENEMY FLEET";

static constexpr char kSavePath[] = "/.crosspoint/bship.sav";

static void save_game() {
    if (!g_host || !g_host->write_file) return;
    g_host->write_file(kSavePath, &g_game, sizeof(g_game));
}

static void load_game() {
    if (g_host && g_host->read_file && g_host->file_exists && g_host->file_exists(kSavePath)) {
        uint32_t read_bytes = 0;
        if (g_host->read_file(kSavePath, &g_game, sizeof(g_game), &read_bytes) && read_bytes == sizeof(g_game)) {
            return;
        }
    }

    // Initialize fresh game
    bs::reset(g_game);
    bs::Fleet f0, f1;
    bs::randomFleet(f0, g_rng);
    bs::randomFleet(f1, g_rng);
    bs::place(g_game, 0, f0);
    bs::place(g_game, 1, f1);
    save_game();
}

static void new_game() {
    bs::reset(g_game);
    bs::Fleet f0, f1;
    bs::randomFleet(f0, g_rng);
    bs::randomFleet(f1, g_rng);
    bs::place(g_game, 0, f0);
    bs::place(g_game, 1, f1);
    strcpy(g_notice, "NEW BATTLE READY! FIRE AT WILL");
    save_game();
}

static void app_init(const StickyHostApi* host) {
    g_host = host;
    if (g_host) g_rng = g_host->get_millis() ^ 0xDEADBEEF;
    load_game();
}

static void app_render() {
    if (!g_host) return;

    g_host->clear(STICKY_COLOR_WHITE);
    g_host->draw_header("BATTLESHIP");

    int16_t sw = g_host->get_screen_width();
    int16_t sh = g_host->get_screen_height();

    // Status banner
    int banner_y = 65;
    g_host->fill_rect(20, banner_y, sw - 40, 48, STICKY_COLOR_LIGHTGRAY);
    g_host->draw_rect(20, banner_y, sw - 40, 48, STICKY_COLOR_BLACK);

    const char* status_str = g_notice;
    if (bs::over(g_game)) {
        status_str = (bs::winner(g_game) == 0) ? "VICTORY! YOU SANK THE ENEMY FLEET" : "DEFEAT! YOUR FLEET WAS DESTROYED";
    }

    int16_t tw, th;
    g_host->get_text_bounds(status_str, 1, &tw, &th);
    g_host->draw_text(sw / 2 - tw / 2, banner_y + 16, status_str, 1, STICKY_COLOR_BLACK);

    // Target Grid (Side 1 - Enemy Fleet): 10x10
    int cell_size = 40;
    int grid_w = cell_size * bs::kSize;
    int grid_left = (sw - grid_w) / 2;
    int grid_top = 125;

    g_host->draw_rect(grid_left, grid_top, grid_w, grid_w, STICKY_COLOR_BLACK);
    g_host->draw_rect(grid_left - 1, grid_top - 1, grid_w + 2, grid_w + 2, STICKY_COLOR_BLACK);

    const bs::Side& enemy_side = g_game.side[1];

    for (int r = 0; r < bs::kSize; ++r) {
        for (int c = 0; c < bs::kSize; ++c) {
            int cell = bs::cellOf(r, c);
            int cx = grid_left + c * cell_size;
            int cy = grid_top + r * cell_size;

            g_host->draw_rect(cx, cy, cell_size, cell_size, STICKY_COLOR_LIGHTGRAY);

            if (bs::shotAt(enemy_side, cell)) {
                int hit_ship = bs::shipAt(enemy_side.fleet, cell);
                if (hit_ship >= 0) {
                    // Hit: Solid black cross
                    g_host->draw_line(cx + 6, cy + 6, cx + cell_size - 6, cy + cell_size - 6, STICKY_COLOR_BLACK);
                    g_host->draw_line(cx + 7, cy + 6, cx + cell_size - 5, cy + cell_size - 6, STICKY_COLOR_BLACK);
                    g_host->draw_line(cx + cell_size - 6, cy + 6, cx + 6, cy + cell_size - 6, STICKY_COLOR_BLACK);
                    g_host->draw_line(cx + cell_size - 5, cy + 6, cx + 7, cy + cell_size - 6, STICKY_COLOR_BLACK);
                } else {
                    // Miss: small dot
                    g_host->fill_circle(cx + cell_size / 2, cy + cell_size / 2, 4, STICKY_COLOR_DARKGRAY);
                }
            } else if (bs::over(g_game)) {
                // If game over, reveal remaining enemy ships
                if (bs::shipAt(enemy_side.fleet, cell) >= 0) {
                    g_host->fill_rect(cx + 4, cy + 4, cell_size - 8, cell_size - 8, STICKY_COLOR_LIGHTGRAY);
                }
            }
        }
    }

    // Friendly fleet summary
    int fleet_y = grid_top + grid_w + 20;
    int enemy_sunk = bs::sunkCount(g_game.side[1]);
    int player_sunk = bs::sunkCount(g_game.side[0]);

    char fleet_str[64];
    int fpos = 0;
    auto append_int = [](char* b, int& pos, int val) {
        char temp[12]; int tp = 0;
        if (val == 0) temp[tp++] = '0';
        else { while (val > 0) { temp[tp++] = '0' + (val % 10); val /= 10; } }
        for (int i = tp - 1; i >= 0; --i) b[pos++] = temp[i];
    };
    const char* s1 = "ENEMY SUNK: "; while (*s1) fleet_str[fpos++] = *s1++;
    append_int(fleet_str, fpos, enemy_sunk);
    const char* s2 = "/5   YOUR FLEET LOST: "; while (*s2) fleet_str[fpos++] = *s2++;
    append_int(fleet_str, fpos, player_sunk);
    const char* s3 = "/5"; while (*s3) fleet_str[fpos++] = *s3++;
    fleet_str[fpos] = '\0';

    g_host->get_text_bounds(fleet_str, 1, &tw, &th);
    g_host->draw_text(sw / 2 - tw / 2, fleet_y, fleet_str, 1, STICKY_COLOR_BLACK);

    // Controls at bottom
    int btn_y = fleet_y + 45;
    int btn_w = 200;
    int btn_h = 50;
    g_host->draw_rect(sw / 2 - btn_w / 2, btn_y, btn_w, btn_h, STICKY_COLOR_BLACK);
    g_host->get_text_bounds("NEW BATTLE", 2, &tw, &th);
    g_host->draw_text(sw / 2 - tw / 2, btn_y + 16, "NEW BATTLE", 2, STICKY_COLOR_BLACK);

    g_host->request_update(false);
}

static void app_touch(int event, int16_t x, int16_t y) {
    if (event != STICKY_TOUCH_CLICK || !g_host) return;

    int16_t sw = g_host->get_screen_width();
    int cell_size = 40;
    int grid_w = cell_size * bs::kSize;
    int grid_left = (sw - grid_w) / 2;
    int grid_top = 125;

    // Grid tap
    if (x >= grid_left && x < grid_left + grid_w && y >= grid_top && y < grid_top + grid_w) {
        if (bs::over(g_game) || g_game.turn != 0) return;

        int col = (x - grid_left) / cell_size;
        int row = (y - grid_top) / cell_size;
        int cell = bs::cellOf(row, col);

        if (!bs::shotAt(g_game.side[1], cell)) {
            bs::fire(g_game, cell);
            if (g_host) g_host->play_haptic(1);

            int sank = bs::lastShotSank(g_game);
            if (sank >= 0) {
                // Sunk enemy ship!
                strcpy(g_notice, "HIT! YOU SANK ENEMY ");
                strcat(g_notice, bs::shipName(sank));
            } else if (bs::lastShotHit(g_game)) {
                strcpy(g_notice, "DIRECT HIT ON ENEMY VESSEL!");
            } else {
                strcpy(g_notice, "SHOT MISSED!");
            }

            save_game();
            app_render();

            // CPU turn
            if (!bs::over(g_game) && g_game.turn == 1) {
                g_host->delay_ms(200);
                // CPU aims using gunner
                int cpu_shot = bs::chooseShot(g_game.side[0], g_rng);
                bs::fire(g_game, cpu_shot);
                save_game();
                app_render();
            }
        }
        return;
    }

    // Button tap: NEW BATTLE
    int fleet_y = grid_top + grid_w + 20;
    int btn_y = fleet_y + 45;
    int btn_w = 200;
    int btn_h = 50;

    if (x >= sw / 2 - btn_w / 2 && x <= sw / 2 + btn_w / 2 && y >= btn_y && y <= btn_y + btn_h) {
        if (g_host) g_host->play_haptic(1);
        new_game();
        app_render();
    }
}

static const StickyAppPlugin g_plugin = {
    .api_version = STICKY_APP_API_VERSION,
    .app_name    = "Battleship",
    .app_version = "1.0",
    .app_author  = "CrossNext",
    .icon_name   = "icon_battleship_32",
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
