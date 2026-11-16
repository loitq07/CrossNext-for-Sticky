#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "sticky_app_api.h"
#include "mini_format.h"

static const StickyHostApi* g_host = NULL;

typedef struct {
    const char* word;
    const char* phonetic;
    const char* part_of_speech;
    const char* definition;
    const char* example_l1;
    const char* example_l2;
} WordCard;

static const WordCard kWords[] = {
    {
        "SERENDIPITY",
        "/ˌser.ənˈdɪp.ə.ti/",
        "[noun]",
        "Finding valuable things not sought for.",
        "Example: A fortunate discovery made",
        "by pure chance and happy accident."
    },
    {
        "RESILIENCE",
        "/rɪˈzɪl.jəns/",
        "[noun]",
        "The capacity to recover quickly from difficulties.",
        "Example: Showing great resilience through",
        "challenging times and emerging stronger."
    },
    {
        "EPHEMERAL",
        "/əˈfem.ər.əl/",
        "[adjective]",
        "Lasting for a very short period of time.",
        "Example: The ephemeral beauty of cherry",
        "blossoms blooming in spring."
    },
    {
        "ELOQUENT",
        "/ˈel.ə.kwənt/",
        "[adjective]",
        "Fluent or persuasive in speaking or writing.",
        "Example: An eloquent speech that moved",
        "the entire audience to tears."
    },
    {
        "PERSEVERANCE",
        "/ˌpɜː.sɪˈvɪə.rəns/",
        "[noun]",
        "Continued effort despite difficulties or obstacles.",
        "Example: His perseverance led him to ultimate",
        "mastery of the art."
    },
    {
        "PRAGMATIC",
        "/præɡˈmæt.ɪk/",
        "[adjective]",
        "Dealing with things sensibly and realistically.",
        "Example: A pragmatic approach to solving",
        "complex software architecture challenges."
    }
};

static const int kWordCount = sizeof(kWords) / sizeof(kWords[0]);
static int g_current_word = 0;
static bool g_learned[sizeof(kWords) / sizeof(kWords[0])] = {false};

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
    g_host->draw_header("WORDS VOCABULARY");

    int sw = g_host->get_screen_width();
    int cx = sw / 2;

    const WordCard* w = &kWords[g_current_word];

    // Main Word Box
    int box_w = sw - 40;
    int box_h = 320;
    int box_x = 20;
    int box_y = 80;

    g_host->draw_rect(box_x, box_y, box_w, box_h, STICKY_COLOR_BLACK);
    g_host->draw_rect(box_x + 1, box_y + 1, box_w - 2, box_h - 2, STICKY_COLOR_BLACK);

    // Learned badge
    if (g_learned[g_current_word]) {
        int bw = 90, bh = 28;
        g_host->fill_rect(box_x + box_w - bw - 15, box_y + 15, bw, bh, STICKY_COLOR_BLACK);
        int16_t lw = 0, lh = 0;
        g_host->get_text_bounds("LEARNED", 0, &lw, &lh);
        g_host->draw_text(box_x + box_w - bw - 15 + (bw - lw) / 2, box_y + 15 + (bh - lh) / 2, "LEARNED", 0,
                          STICKY_COLOR_WHITE);
    }

    // Word Title
    g_host->draw_text(box_x + 25, box_y + 25, w->word, 0, STICKY_COLOR_BLACK);

    // Phonetic & POS
    char sub_buf[64];
    mini_strcpy(sub_buf, w->phonetic);
    mini_strcat(sub_buf, "  ");
    mini_strcat(sub_buf, w->part_of_speech);
    g_host->draw_text(box_x + 25, box_y + 65, sub_buf, 0, STICKY_COLOR_DARKGRAY);

    g_host->draw_line(box_x + 20, box_y + 105, box_x + box_w - 20, box_y + 105, STICKY_COLOR_BLACK);

    // Definition
    g_host->draw_text(box_x + 25, box_y + 125, "DEFINITION:", 0, STICKY_COLOR_BLACK);
    g_host->draw_text(box_x + 25, box_y + 155, w->definition, 0, STICKY_COLOR_BLACK);

    // Example
    g_host->draw_text(box_x + 25, box_y + 205, "EXAMPLE:", 0, STICKY_COLOR_DARKGRAY);
    g_host->draw_text(box_x + 25, box_y + 235, w->example_l1, 0, STICKY_COLOR_BLACK);
    if (w->example_l2 && w->example_l2[0]) {
        g_host->draw_text(box_x + 25, box_y + 265, w->example_l2, 0, STICKY_COLOR_BLACK);
    }

    // Indicator
    char ind_buf[32];
    char w_cur[8], w_tot[8];
    mini_itoa(g_current_word + 1, w_cur);
    mini_itoa(kWordCount, w_tot);
    mini_strcpy(ind_buf, "WORD ");
    mini_strcat(ind_buf, w_cur);
    mini_strcat(ind_buf, " OF ");
    mini_strcat(ind_buf, w_tot);
    int16_t iw = 0, ih = 0;
    g_host->get_text_bounds(ind_buf, 0, &iw, &ih);
    g_host->draw_text(cx - iw / 2, box_y + box_h + 25, ind_buf, 0, STICKY_COLOR_BLACK);

    // Navigation Controls
    int btn_w = 120, btn_h = 48;
    int btn_y = box_y + box_h + 65;

    // PREV
    int b1_x = cx - 190;
    g_host->draw_rect(b1_x, btn_y, btn_w, btn_h, STICKY_COLOR_BLACK);
    int16_t bw1 = 0, bh1 = 0;
    g_host->get_text_bounds("< PREV", 0, &bw1, &bh1);
    g_host->draw_text(b1_x + (btn_w - bw1) / 2, btn_y + (btn_h - bh1) / 2, "< PREV", 0, STICKY_COLOR_BLACK);

    // RANDOM
    int b2_x = cx - btn_w / 2;
    g_host->fill_rect(b2_x, btn_y, btn_w, btn_h, STICKY_COLOR_BLACK);
    int16_t bw2 = 0, bh2 = 0;
    g_host->get_text_bounds("RANDOM", 0, &bw2, &bh2);
    g_host->draw_text(b2_x + (btn_w - bw2) / 2, btn_y + (btn_h - bh2) / 2, "RANDOM", 0, STICKY_COLOR_WHITE);

    // NEXT
    int b3_x = cx + 70;
    g_host->draw_rect(b3_x, btn_y, btn_w, btn_h, STICKY_COLOR_BLACK);
    int16_t bw3 = 0, bh3 = 0;
    g_host->get_text_bounds("NEXT >", 0, &bw3, &bh3);
    g_host->draw_text(b3_x + (btn_w - bw3) / 2, btn_y + (btn_h - bh3) / 2, "NEXT >", 0, STICKY_COLOR_BLACK);

    // LEARNED TOGGLE
    int lrn_w = 220, lrn_h = 42;
    int lrn_x = cx - lrn_w / 2;
    int lrn_y = btn_y + btn_h + 20;
    g_host->draw_rect(lrn_x, lrn_y, lrn_w, lrn_h, STICKY_COLOR_BLACK);
    const char* l_label = g_learned[g_current_word] ? "RESET STATUS" : "MARK AS MASTERED [✓]";
    int16_t lw = 0, lh = 0;
    g_host->get_text_bounds(l_label, 0, &lw, &lh);
    g_host->draw_text(lrn_x + (lrn_w - lw) / 2, lrn_y + (lrn_h - lh) / 2, l_label, 0, STICKY_COLOR_BLACK);
}

static void app_touch(int event_type, int16_t x, int16_t y) {
    if (!g_host || event_type != STICKY_TOUCH_CLICK) return;

    int sw = g_host->get_screen_width();
    int cx = sw / 2;
    int box_h = 320;
    int box_y = 80;

    int btn_w = 120, btn_h = 48;
    int btn_y = box_y + box_h + 65;
    int b1_x = cx - 190;
    int b2_x = cx - btn_w / 2;
    int b3_x = cx + 70;

    // PREV
    if (x >= b1_x && x < b1_x + btn_w && y >= btn_y && y < btn_y + btn_h) {
        g_host->play_haptic(0);
        g_current_word = (g_current_word - 1 + kWordCount) % kWordCount;
        g_host->request_update(true);
        return;
    }

    // RANDOM
    if (x >= b2_x && x < b2_x + btn_w && y >= btn_y && y < btn_y + btn_h) {
        g_host->play_haptic(0);
        g_current_word = (g_current_word + 2) % kWordCount;
        g_host->request_update(true);
        return;
    }

    // NEXT
    if (x >= b3_x && x < b3_x + btn_w && y >= btn_y && y < btn_y + btn_h) {
        g_host->play_haptic(0);
        g_current_word = (g_current_word + 1) % kWordCount;
        g_host->request_update(true);
        return;
    }

    // LEARNED TOGGLE
    int lrn_w = 220, lrn_h = 42;
    int lrn_x = cx - lrn_w / 2;
    int lrn_y = btn_y + btn_h + 20;
    if (x >= lrn_x && x < lrn_x + lrn_w && y >= lrn_y && y < lrn_y + lrn_h) {
        g_host->play_haptic(0);
        g_learned[g_current_word] = !g_learned[g_current_word];
        g_host->request_update(true);
        return;
    }
}

static void app_exit(void) {}
static void app_destroy(void) {}

static const StickyAppPlugin g_plugin = {
    .api_version = STICKY_APP_API_VERSION,
    .app_name = "Words",
    .app_version = "1.0.0",
    .app_author = "Sticky",
    .icon_name = "words",
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
