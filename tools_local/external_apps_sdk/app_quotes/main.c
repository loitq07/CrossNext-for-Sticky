#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "sticky_app_api.h"
#include "mini_format.h"

static const StickyHostApi* g_host = NULL;

typedef struct {
    const char* text_l1;
    const char* text_l2;
    const char* text_l3;
    const char* author;
} Quote;

static const Quote kQuotes[] = {
    {
        "\"You have power over your mind - not",
        "outside events. Realize this, and",
        "you will find strength.\"",
        "— Marcus Aurelius"
    },
    {
        "\"The only way to do great work",
        "is to love what you do. If you haven't",
        "found it yet, keep looking.\"",
        "— Steve Jobs"
    },
    {
        "\"It is not that we have a short time",
        "to live, but that we waste a lot",
        "of it.\"",
        "— Seneca"
    },
    {
        "\"Simplicity is the ultimate",
        "sophistication.\"",
        "",
        "— Leonardo da Vinci"
    },
    {
        "\"In the middle of difficulty",
        "lies opportunity.\"",
        "",
        "— Albert Einstein"
    },
    {
        "\"A journey of a thousand miles",
        "must begin with a single step.\"",
        "",
        "— Lao Tzu"
    },
    {
        "\"We suffer more often in imagination",
        "than in reality.\"",
        "",
        "— Seneca"
    },
    {
        "\"Stay hungry, stay foolish.\"",
        "",
        "",
        "— Whole Earth Catalog"
    }
};

static const int kQuoteCount = sizeof(kQuotes) / sizeof(kQuotes[0]);
static int g_current_quote = 0;
static bool g_favorites[sizeof(kQuotes) / sizeof(kQuotes[0])] = {false};

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
    g_host->draw_header("DAILY QUOTES");

    int sw = g_host->get_screen_width();
    int cx = sw / 2;

    // Quote Box
    int box_w = sw - 40;
    int box_h = 280;
    int box_x = 20;
    int box_y = 90;

    g_host->draw_rect(box_x, box_y, box_w, box_h, STICKY_COLOR_BLACK);
    g_host->draw_rect(box_x + 1, box_y + 1, box_w - 2, box_h - 2, STICKY_COLOR_BLACK);
    g_host->draw_rect(box_x + 2, box_y + 2, box_w - 4, box_h - 4, STICKY_COLOR_BLACK);

    // Star icon if favorited
    if (g_favorites[g_current_quote]) {
        g_host->draw_text(box_x + box_w - 45, box_y + 18, "[*]", 0, STICKY_COLOR_BLACK);
    }

    // Quote lines
    const Quote* q = &kQuotes[g_current_quote];
    int line_y = box_y + 40;
    int line_gap = 36;

    if (q->text_l1 && q->text_l1[0]) {
        g_host->draw_text(box_x + 25, line_y, q->text_l1, 0, STICKY_COLOR_BLACK);
        line_y += line_gap;
    }
    if (q->text_l2 && q->text_l2[0]) {
        g_host->draw_text(box_x + 25, line_y, q->text_l2, 0, STICKY_COLOR_BLACK);
        line_y += line_gap;
    }
    if (q->text_l3 && q->text_l3[0]) {
        g_host->draw_text(box_x + 25, line_y, q->text_l3, 0, STICKY_COLOR_BLACK);
        line_y += line_gap;
    }

    // Author
    int16_t aw = 0, ah = 0;
    g_host->get_text_bounds(q->author, 0, &aw, &ah);
    g_host->draw_text(box_x + box_w - 35 - aw, box_y + box_h - 45, q->author, 0, STICKY_COLOR_DARKGRAY);

    // Indicator
    char ind_buf[32];
    char q_cur[8], q_tot[8];
    mini_itoa(g_current_quote + 1, q_cur);
    mini_itoa(kQuoteCount, q_tot);
    mini_strcpy(ind_buf, "QUOTE ");
    mini_strcat(ind_buf, q_cur);
    mini_strcat(ind_buf, " OF ");
    mini_strcat(ind_buf, q_tot);
    int16_t iw = 0, ih = 0;
    g_host->get_text_bounds(ind_buf, 0, &iw, &ih);
    g_host->draw_text(cx - iw / 2, box_y + box_h + 30, ind_buf, 0, STICKY_COLOR_BLACK);

    // Controls
    int btn_w = 120, btn_h = 48;
    int btn_y = box_y + box_h + 75;

    // PREV [<]
    int b1_x = cx - 190;
    g_host->draw_rect(b1_x, btn_y, btn_w, btn_h, STICKY_COLOR_BLACK);
    int16_t w1 = 0, h1 = 0;
    g_host->get_text_bounds("< PREV", 0, &w1, &h1);
    g_host->draw_text(b1_x + (btn_w - w1) / 2, btn_y + (btn_h - h1) / 2, "< PREV", 0, STICKY_COLOR_BLACK);

    // RANDOM
    int b2_x = cx - btn_w / 2;
    g_host->fill_rect(b2_x, btn_y, btn_w, btn_h, STICKY_COLOR_BLACK);
    int16_t w2 = 0, h2 = 0;
    g_host->get_text_bounds("RANDOM", 0, &w2, &h2);
    g_host->draw_text(b2_x + (btn_w - w2) / 2, btn_y + (btn_h - h2) / 2, "RANDOM", 0, STICKY_COLOR_WHITE);

    // NEXT [>]
    int b3_x = cx + 70;
    g_host->draw_rect(b3_x, btn_y, btn_w, btn_h, STICKY_COLOR_BLACK);
    int16_t w3 = 0, h3 = 0;
    g_host->get_text_bounds("NEXT >", 0, &w3, &h3);
    g_host->draw_text(b3_x + (btn_w - w3) / 2, btn_y + (btn_h - h3) / 2, "NEXT >", 0, STICKY_COLOR_BLACK);

    // FAVORITE TOGGLE
    int fav_w = 200, fav_h = 42;
    int fav_x = cx - fav_w / 2;
    int fav_y = btn_y + btn_h + 20;
    g_host->draw_rect(fav_x, fav_y, fav_w, fav_h, STICKY_COLOR_BLACK);
    const char* f_label = g_favorites[g_current_quote] ? "REMOVE FAVORITE" : "MARK FAVORITE [★]";
    int16_t fw = 0, fh = 0;
    g_host->get_text_bounds(f_label, 0, &fw, &fh);
    g_host->draw_text(fav_x + (fav_w - fw) / 2, fav_y + (fav_h - fh) / 2, f_label, 0, STICKY_COLOR_BLACK);
}

static void app_touch(int event_type, int16_t x, int16_t y) {
    if (!g_host || event_type != STICKY_TOUCH_CLICK) return;

    int sw = g_host->get_screen_width();
    int cx = sw / 2;
    int box_h = 280;
    int box_y = 90;

    int btn_w = 120, btn_h = 48;
    int btn_y = box_y + box_h + 75;
    int b1_x = cx - 190;
    int b2_x = cx - btn_w / 2;
    int b3_x = cx + 70;

    // PREV
    if (x >= b1_x && x < b1_x + btn_w && y >= btn_y && y < btn_y + btn_h) {
        g_host->play_haptic(0);
        g_current_quote = (g_current_quote - 1 + kQuoteCount) % kQuoteCount;
        g_host->request_update(true);
        return;
    }

    // RANDOM
    if (x >= b2_x && x < b2_x + btn_w && y >= btn_y && y < btn_y + btn_h) {
        g_host->play_haptic(0);
        g_current_quote = (g_current_quote + 3) % kQuoteCount;
        g_host->request_update(true);
        return;
    }

    // NEXT
    if (x >= b3_x && x < b3_x + btn_w && y >= btn_y && y < btn_y + btn_h) {
        g_host->play_haptic(0);
        g_current_quote = (g_current_quote + 1) % kQuoteCount;
        g_host->request_update(true);
        return;
    }

    // FAVORITE
    int fav_w = 200, fav_h = 42;
    int fav_x = cx - fav_w / 2;
    int fav_y = btn_y + btn_h + 20;
    if (x >= fav_x && x < fav_x + fav_w && y >= fav_y && y < fav_y + fav_h) {
        g_host->play_haptic(0);
        g_favorites[g_current_quote] = !g_favorites[g_current_quote];
        g_host->request_update(true);
        return;
    }
}

static void app_exit(void) {}
static void app_destroy(void) {}

static const StickyAppPlugin g_plugin = {
    .api_version = STICKY_APP_API_VERSION,
    .app_name = "Quotes",
    .app_version = "1.0.0",
    .app_author = "Sticky",
    .icon_name = "quotes",
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
