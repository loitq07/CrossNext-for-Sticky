#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "sticky_app_api.h"
#include "mini_format.h"

static const StickyHostApi* g_host = NULL;

typedef struct {
    int hunger;    // 0-100 (100 = full)
    int happiness; // 0-100
    int energy;    // 0-100
    int level;
    int xp;
} PetState;

static PetState g_pet = {
    .hunger = 70,
    .happiness = 85,
    .energy = 60,
    .level = 3,
    .xp = 45
};

static const char* g_pet_msg = "Meow! Ready to explore today!";

static void save_pet(void) {
    if (g_host) {
        g_host->write_file("/apps/pet.dat", &g_pet, sizeof(g_pet));
    }
}

static void load_pet(void) {
    if (g_host && g_host->file_exists("/apps/pet.dat")) {
        uint32_t len = 0;
        g_host->read_file("/apps/pet.dat", &g_pet, sizeof(g_pet), &len);
    }
}

static void app_init(const StickyHostApi* host) {
    g_host = host;
    load_pet();
}

static void app_enter(void) {
    if (g_host) {
        g_host->request_update(true);
    }
}

static void app_loop(void) {}

static void draw_meter(int x, int y, int w, int h, const char* label, int val) {
    if (val < 0) val = 0;
    if (val > 100) val = 100;

    g_host->draw_text(x, y, label, 0, STICKY_COLOR_BLACK);
    int bar_y = y + 22;
    g_host->draw_rect(x, bar_y, w, h, STICKY_COLOR_BLACK);
    int fill = val * w / 100;
    if (fill > 0) {
        g_host->fill_rect(x, bar_y, fill, h, STICKY_COLOR_BLACK);
    }

    char pct[16];
    char p_buf[8];
    mini_itoa(val, p_buf);
    mini_strcpy(pct, p_buf);
    mini_strcat(pct, "%");
    g_host->draw_text(x + w + 12, bar_y - 2, pct, 0, STICKY_COLOR_BLACK);
}

static void app_render(void) {
    if (!g_host) return;

    g_host->clear(STICKY_COLOR_WHITE);
    g_host->draw_header("POCKET PET");

    int sw = g_host->get_screen_width();
    int cx = sw / 2;

    // Pet Display Box
    int box_w = sw - 40;
    int box_h = 240;
    int box_x = 20;
    int box_y = 65;

    g_host->draw_rect(box_x, box_y, box_w, box_h, STICKY_COLOR_BLACK);
    g_host->draw_rect(box_x + 1, box_y + 1, box_w - 2, box_h - 2, STICKY_COLOR_BLACK);

    // ASCII Art Cat
    static const char* kPetArt[] = {
        "   /\\_____/\\   ",
        "  /  o   o  \\  ",
        " ( ==  ^  == ) ",
        "  )         (  ",
        " (           ) ",
        "  ( (  \"  ) )  ",
        "   \\_(__)_/    "
    };

    int art_y = box_y + 25;
    for (int i = 0; i < 7; ++i) {
        int16_t aw = 0, ah = 0;
        g_host->get_text_bounds(kPetArt[i], 0, &aw, &ah);
        g_host->draw_text(cx - aw / 2, art_y + i * 22, kPetArt[i], 0, STICKY_COLOR_BLACK);
    }

    // Message bubble
    int bubble_y = box_y + box_h - 45;
    int16_t mw = 0, mh = 0;
    g_host->get_text_bounds(g_pet_msg, 0, &mw, &mh);
    g_host->draw_text(cx - mw / 2, bubble_y, g_pet_msg, 0, STICKY_COLOR_BLACK);

    // Status Meters Box
    int met_y = box_y + box_h + 15;
    int met_w = sw - 40;
    int met_h = 160;
    g_host->draw_rect(20, met_y, met_w, met_h, STICKY_COLOR_BLACK);

    // Level info
    char lvl_buf[48];
    char l_num[8], xp_num[8];
    mini_itoa(g_pet.level, l_num);
    mini_itoa(g_pet.xp, xp_num);
    mini_strcpy(lvl_buf, "PET LVL ");
    mini_strcat(lvl_buf, l_num);
    mini_strcat(lvl_buf, "  •  XP: ");
    mini_strcat(lvl_buf, xp_num);
    mini_strcat(lvl_buf, "/100");
    g_host->draw_text(35, met_y + 15, lvl_buf, 0, STICKY_COLOR_BLACK);
    g_host->draw_line(35, met_y + 38, sw - 35, met_y + 38, STICKY_COLOR_DARKGRAY);

    int meter_w = 260;
    draw_meter(35, met_y + 50, meter_w, 14, "HUNGER", g_pet.hunger);
    draw_meter(35, met_y + 90, meter_w, 14, "HAPPINESS", g_pet.happiness);
    draw_meter(35, met_y + 130, meter_w, 14, "ENERGY", g_pet.energy);

    // Action Buttons (2x2 grid)
    int btn_w = 190, btn_h = 48;
    int btn_y1 = met_y + met_h + 15;
    int btn_y2 = btn_y1 + btn_h + 12;
    int b1_x = cx - btn_w - 8;
    int b2_x = cx + 8;

    // FEED
    g_host->fill_rect(b1_x, btn_y1, btn_w, btn_h, STICKY_COLOR_BLACK);
    int16_t w1 = 0, h1 = 0;
    g_host->get_text_bounds("FEED [FISH]", 0, &w1, &h1);
    g_host->draw_text(b1_x + (btn_w - w1) / 2, btn_y1 + (btn_h - h1) / 2, "FEED [FISH]", 0, STICKY_COLOR_WHITE);

    // PLAY
    g_host->draw_rect(b2_x, btn_y1, btn_w, btn_h, STICKY_COLOR_BLACK);
    int16_t w2 = 0, h2 = 0;
    g_host->get_text_bounds("PLAY [BALL]", 0, &w2, &h2);
    g_host->draw_text(b2_x + (btn_w - w2) / 2, btn_y1 + (btn_h - h2) / 2, "PLAY [BALL]", 0, STICKY_COLOR_BLACK);

    // SLEEP
    g_host->draw_rect(b1_x, btn_y2, btn_w, btn_h, STICKY_COLOR_BLACK);
    int16_t w3 = 0, h3 = 0;
    g_host->get_text_bounds("SLEEP [REST]", 0, &w3, &h3);
    g_host->draw_text(b1_x + (btn_w - w3) / 2, btn_y2 + (btn_h - h3) / 2, "SLEEP [REST]", 0, STICKY_COLOR_BLACK);

    // CLEAN
    g_host->draw_rect(b2_x, btn_y2, btn_w, btn_h, STICKY_COLOR_BLACK);
    int16_t w4 = 0, h4 = 0;
    g_host->get_text_bounds("CLEAN [BATH]", 0, &w4, &h4);
    g_host->draw_text(b2_x + (btn_w - w4) / 2, btn_y2 + (btn_h - h4) / 2, "CLEAN [BATH]", 0, STICKY_COLOR_BLACK);
}

static void add_xp(int amount) {
    g_pet.xp += amount;
    if (g_pet.xp >= 100) {
        g_pet.xp -= 100;
        g_pet.level++;
        g_pet_msg = "LEVEL UP! Pet is stronger!";
        if (g_host) g_host->play_haptic(1);
    }
}

static void app_touch(int event_type, int16_t x, int16_t y) {
    if (!g_host || event_type != STICKY_TOUCH_CLICK) return;

    int sw = g_host->get_screen_width();
    int cx = sw / 2;

    int box_h = 240;
    int box_y = 65;
    int met_y = box_y + box_h + 15;
    int met_h = 160;

    int btn_w = 190, btn_h = 48;
    int btn_y1 = met_y + met_h + 15;
    int btn_y2 = btn_y1 + btn_h + 12;
    int b1_x = cx - btn_w - 8;
    int b2_x = cx + 8;

    // FEED
    if (x >= b1_x && x < b1_x + btn_w && y >= btn_y1 && y < btn_y1 + btn_h) {
        g_host->play_haptic(0);
        g_pet.hunger = (g_pet.hunger + 25 > 100) ? 100 : g_pet.hunger + 25;
        g_pet_msg = "Yum! That was delicious!";
        add_xp(15);
        save_pet();
        g_host->request_update(true);
        return;
    }

    // PLAY
    if (x >= b2_x && x < b2_x + btn_w && y >= btn_y1 && y < btn_y1 + btn_h) {
        g_host->play_haptic(0);
        g_pet.happiness = (g_pet.happiness + 20 > 100) ? 100 : g_pet.happiness + 20;
        g_pet.energy = (g_pet.energy - 15 < 0) ? 0 : g_pet.energy - 15;
        g_pet_msg = "Yay! Playing is so fun!";
        add_xp(20);
        save_pet();
        g_host->request_update(true);
        return;
    }

    // SLEEP
    if (x >= b1_x && x < b1_x + btn_w && y >= btn_y2 && y < btn_y2 + btn_h) {
        g_host->play_haptic(0);
        g_pet.energy = (g_pet.energy + 35 > 100) ? 100 : g_pet.energy + 35;
        g_pet_msg = "Zzz... Fully rested & energized!";
        add_xp(10);
        save_pet();
        g_host->request_update(true);
        return;
    }

    // CLEAN
    if (x >= b2_x && x < b2_x + btn_w && y >= btn_y2 && y < btn_y2 + btn_h) {
        g_host->play_haptic(0);
        g_pet.happiness = (g_pet.happiness + 15 > 100) ? 100 : g_pet.happiness + 15;
        g_pet_msg = "All clean and sparkling!";
        add_xp(15);
        save_pet();
        g_host->request_update(true);
        return;
    }
}

static void app_exit(void) {
    save_pet();
}

static void app_destroy(void) {}

static const StickyAppPlugin g_plugin = {
    .api_version = STICKY_APP_API_VERSION,
    .app_name = "Pet",
    .app_version = "1.0.0",
    .app_author = "Sticky",
    .icon_name = "pet",
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
