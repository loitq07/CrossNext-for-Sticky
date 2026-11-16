#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "sticky_app_api.h"
#include "mini_format.h"

static const StickyHostApi* g_host = NULL;

typedef struct {
    const char* question_l1;
    const char* question_l2;
    const char* answer_l1;
    const char* answer_l2;
    const char* answer_l3;
} Flashcard;

static const Flashcard kDeck[] = {
    {
        "What is an E-Paper / E-Ink",
        "display's primary benefit?",
        "High readability in sunlight,",
        "zero backlight eye strain, and",
        "zero power while static."
    },
    {
        "What is the difference between",
        "RAM and Flash memory?",
        "RAM is volatile (lost on reboot).",
        "Flash is non-volatile (persists",
        "saved files and firmware code)."
    },
    {
        "What is the purpose of FreeRTOS",
        "in embedded microcontrollers?",
        "Provides real-time multitasking,",
        "task scheduling, semaphores,",
        "and queues for hardware timing."
    },
    {
        "What is the function of",
        "an ELF dynamic loader?",
        "Resolves symbol relocations and",
        "links runtime host API to",
        "execute standalone modular apps."
    },
    {
        "What is the HTTP 200 vs 404",
        "status code meaning?",
        "200 OK: Request succeeded.",
        "404 Not Found: Server cannot",
        "find the requested resource."
    }
};

static const int kDeckCount = sizeof(kDeck) / sizeof(kDeck[0]);
static int g_card_idx = 0;
static bool g_is_flipped = false;
static int g_correct_count = 0;
static int g_reviewed_count = 0;

static void app_init(const StickyHostApi* host) {
    g_host = host;
}

static void app_enter(void) {
    if (g_host) {
        g_host->request_update(true);
    }
}

static void app_loop(void) {}

static void app_render(void) {
    if (!g_host) return;

    g_host->clear(STICKY_COLOR_WHITE);
    g_host->draw_header("STUDY FLASHCARDS");

    int sw = g_host->get_screen_width();
    int cx = sw / 2;

    const Flashcard* c = &kDeck[g_card_idx];

    // Card Box
    int box_w = sw - 40;
    int box_h = 300;
    int box_x = 20;
    int box_y = 80;

    g_host->draw_rect(box_x, box_y, box_w, box_h, STICKY_COLOR_BLACK);
    g_host->draw_rect(box_x + 1, box_y + 1, box_w - 2, box_h - 2, STICKY_COLOR_BLACK);
    g_host->draw_rect(box_x + 2, box_y + 2, box_w - 4, box_h - 4, STICKY_COLOR_BLACK);

    // Front / Back Pill
    int pw = 130, ph = 32;
    int px = box_x + (box_w - pw) / 2;
    int py = box_y + 18;
    g_host->fill_rect(px, py, pw, ph, STICKY_COLOR_BLACK);
    const char* pill_text = g_is_flipped ? "ANSWER (BACK)" : "QUESTION (FRONT)";
    int16_t pw_t = 0, ph_t = 0;
    g_host->get_text_bounds(pill_text, 0, &pw_t, &ph_t);
    g_host->draw_text(px + (pw - pw_t) / 2, py + (ph - ph_t) / 2, pill_text, 0, STICKY_COLOR_WHITE);

    // Card Content
    if (!g_is_flipped) {
        // Question Lines
        int text_y = box_y + 100;
        g_host->draw_text(box_x + 30, text_y, c->question_l1, 0, STICKY_COLOR_BLACK);
        if (c->question_l2 && c->question_l2[0]) {
            g_host->draw_text(box_x + 30, text_y + 40, c->question_l2, 0, STICKY_COLOR_BLACK);
        }

        // Tap to flip hint
        const char* hint = "[ TAP CARD TO FLIP ]";
        int16_t hw = 0, hh = 0;
        g_host->get_text_bounds(hint, 0, &hw, &hh);
        g_host->draw_text(cx - hw / 2, box_y + box_h - 40, hint, 0, STICKY_COLOR_DARKGRAY);

    } else {
        // Answer Lines
        int text_y = box_y + 85;
        g_host->draw_text(box_x + 30, text_y, c->answer_l1, 0, STICKY_COLOR_BLACK);
        if (c->answer_l2 && c->answer_l2[0]) {
            g_host->draw_text(box_x + 30, text_y + 38, c->answer_l2, 0, STICKY_COLOR_BLACK);
        }
        if (c->answer_l3 && c->answer_l3[0]) {
            g_host->draw_text(box_x + 30, text_y + 76, c->answer_l3, 0, STICKY_COLOR_BLACK);
        }

        // Tap to flip back hint
        const char* hint = "[ TAP TO FLIP FRONT ]";
        int16_t hw = 0, hh = 0;
        g_host->get_text_bounds(hint, 0, &hw, &hh);
        g_host->draw_text(cx - hw / 2, box_y + box_h - 40, hint, 0, STICKY_COLOR_DARKGRAY);
    }

    // Card indicator & Score
    char stat_buf[64];
    char c_cur[8], c_tot[8], m_cnt[8];
    mini_itoa(g_card_idx + 1, c_cur);
    mini_itoa(kDeckCount, c_tot);
    mini_itoa(g_correct_count, m_cnt);
    mini_strcpy(stat_buf, "CARD ");
    mini_strcat(stat_buf, c_cur);
    mini_strcat(stat_buf, " / ");
    mini_strcat(stat_buf, c_tot);
    mini_strcat(stat_buf, "  •  MASTERED: ");
    mini_strcat(stat_buf, m_cnt);
    int16_t sw_t = 0, sh_t = 0;
    g_host->get_text_bounds(stat_buf, 0, &sw_t, &sh_t);
    g_host->draw_text(cx - sw_t / 2, box_y + box_h + 20, stat_buf, 0, STICKY_COLOR_BLACK);

    // Answer evaluation buttons (when flipped)
    int btn_y = box_y + box_h + 55;
    int btn_w = 170, btn_h = 50;

    if (g_is_flipped) {
        // AGAIN [X]
        int b1_x = cx - btn_w - 10;
        g_host->draw_rect(b1_x, btn_y, btn_w, btn_h, STICKY_COLOR_BLACK);
        int16_t bw1 = 0, bh1 = 0;
        g_host->get_text_bounds("AGAIN [X]", 0, &bw1, &bh1);
        g_host->draw_text(b1_x + (btn_w - bw1) / 2, btn_y + (btn_h - bh1) / 2, "AGAIN [X]", 0, STICKY_COLOR_BLACK);

        // GOT IT [✓]
        int b2_x = cx + 10;
        g_host->fill_rect(b2_x, btn_y, btn_w, btn_h, STICKY_COLOR_BLACK);
        int16_t bw2 = 0, bh2 = 0;
        g_host->get_text_bounds("GOT IT [✓]", 0, &bw2, &bh2);
        g_host->draw_text(b2_x + (btn_w - bw2) / 2, btn_y + (btn_h - bh2) / 2, "GOT IT [✓]", 0, STICKY_COLOR_WHITE);
    } else {
        // NEXT CARD
        int b_x = cx - btn_w / 2;
        g_host->draw_rect(b_x, btn_y, btn_w, btn_h, STICKY_COLOR_BLACK);
        int16_t bw = 0, bh = 0;
        g_host->get_text_bounds("NEXT CARD >", 0, &bw, &bh);
        g_host->draw_text(b_x + (btn_w - bw) / 2, btn_y + (btn_h - bh) / 2, "NEXT CARD >", 0, STICKY_COLOR_BLACK);
    }

    // Reset deck button
    int r_w = 160, r_h = 38;
    int rx = cx - r_w / 2;
    int ry = btn_y + btn_h + 20;
    g_host->draw_rect(rx, ry, r_w, r_h, STICKY_COLOR_BLACK);
    int16_t rw = 0, rh = 0;
    g_host->get_text_bounds("RESET DECK", 0, &rw, &rh);
    g_host->draw_text(rx + (r_w - rw) / 2, ry + (r_h - rh) / 2, "RESET DECK", 0, STICKY_COLOR_DARKGRAY);
}

static void app_touch(int event_type, int16_t x, int16_t y) {
    if (!g_host || event_type != STICKY_TOUCH_CLICK) return;

    int sw = g_host->get_screen_width();
    int cx = sw / 2;
    int box_w = sw - 40;
    int box_h = 300;
    int box_x = 20;
    int box_y = 80;

    // Flip Card on card body tap
    if (x >= box_x && x < box_x + box_w && y >= box_y && y < box_y + box_h) {
        g_host->play_haptic(0);
        g_is_flipped = !g_is_flipped;
        g_host->request_update(true);
        return;
    }

    int btn_y = box_y + box_h + 55;
    int btn_w = 170, btn_h = 50;

    if (g_is_flipped) {
        int b1_x = cx - btn_w - 10;
        int b2_x = cx + 10;

        // AGAIN
        if (x >= b1_x && x < b1_x + btn_w && y >= btn_y && y < btn_y + btn_h) {
            g_host->play_haptic(0);
            g_is_flipped = false;
            g_card_idx = (g_card_idx + 1) % kDeckCount;
            g_host->request_update(true);
            return;
        }

        // GOT IT
        if (x >= b2_x && x < b2_x + btn_w && y >= btn_y && y < btn_y + btn_h) {
            g_host->play_haptic(0);
            g_correct_count++;
            g_is_flipped = false;
            g_card_idx = (g_card_idx + 1) % kDeckCount;
            g_host->request_update(true);
            return;
        }
    } else {
        // NEXT CARD
        int b_x = cx - btn_w / 2;
        if (x >= b_x && x < b_x + btn_w && y >= btn_y && y < btn_y + btn_h) {
            g_host->play_haptic(0);
            g_card_idx = (g_card_idx + 1) % kDeckCount;
            g_host->request_update(true);
            return;
        }
    }

    // RESET DECK
    int r_w = 160, r_h = 38;
    int rx = cx - r_w / 2;
    int ry = btn_y + btn_h + 20;
    if (x >= rx && x < rx + r_w && y >= ry && y < ry + r_h) {
        g_host->play_haptic(0);
        g_card_idx = 0;
        g_is_flipped = false;
        g_correct_count = 0;
        g_reviewed_count = 0;
        g_host->request_update(true);
        return;
    }
}

static void app_exit(void) {}
static void app_destroy(void) {}

static const StickyAppPlugin g_plugin = {
    .api_version = STICKY_APP_API_VERSION,
    .app_name = "Flashcards",
    .app_version = "1.0.0",
    .app_author = "Sticky",
    .icon_name = "flashcards",
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
