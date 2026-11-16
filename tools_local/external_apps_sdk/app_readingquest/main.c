#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "sticky_app_api.h"
#include "mini_format.h"

static const StickyHostApi* g_host = NULL;

typedef struct {
    int level;
    int xp;
    int gold;
    int pages_read_today;
    int daily_goal_pages;
    int quests_completed;
} QuestHero;

static QuestHero g_hero = {
    .level = 4,
    .xp = 75,
    .gold = 120,
    .pages_read_today = 15,
    .daily_goal_pages = 25,
    .quests_completed = 8
};

static void save_quest(void) {
    if (g_host) {
        g_host->write_file("/apps/quest.dat", &g_hero, sizeof(g_hero));
    }
}

static void load_quest(void) {
    if (g_host && g_host->file_exists("/apps/quest.dat")) {
        uint32_t len = 0;
        g_host->read_file("/apps/quest.dat", &g_hero, sizeof(g_hero), &len);
    }
}

static void app_init(const StickyHostApi* host) {
    g_host = host;
    load_quest();
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
    g_host->draw_header("READING QUEST RPG");

    int sw = g_host->get_screen_width();
    int cx = sw / 2;

    // Hero Stats Banner Box
    int box_w = sw - 40;
    int box_h = 105;
    int box_x = 20;
    int box_y = 65;

    g_host->draw_rect(box_x, box_y, box_w, box_h, STICKY_COLOR_BLACK);
    g_host->draw_rect(box_x + 1, box_y + 1, box_w - 2, box_h - 2, STICKY_COLOR_BLACK);

    char lvl_str[48];
    char l_num[8];
    mini_itoa(g_hero.level, l_num);
    mini_strcpy(lvl_str, "KNIGHT SCHOLAR  •  LEVEL ");
    mini_strcat(lvl_str, l_num);
    g_host->draw_text(35, box_y + 15, lvl_str, 0, STICKY_COLOR_BLACK);

    char stats_str[64];
    char xp_buf[8], g_buf[8], q_buf[8];
    mini_itoa(g_hero.xp, xp_buf);
    mini_itoa(g_hero.gold, g_buf);
    mini_itoa(g_hero.quests_completed, q_buf);
    mini_strcpy(stats_str, "XP: ");
    mini_strcat(stats_str, xp_buf);
    mini_strcat(stats_str, "/100  GOLD: ");
    mini_strcat(stats_str, g_buf);
    mini_strcat(stats_str, "G  QUESTS: ");
    mini_strcat(stats_str, q_buf);
    g_host->draw_text(35, box_y + 44, stats_str, 0, STICKY_COLOR_DARKGRAY);

    // XP Bar
    int xp_w = box_w - 30;
    int bar_y = box_y + 75;
    g_host->draw_rect(35, bar_y, xp_w, 14, STICKY_COLOR_BLACK);
    int fill = g_hero.xp * xp_w / 100;
    if (fill > xp_w) fill = xp_w;
    if (fill > 0) {
        g_host->fill_rect(35, bar_y, fill, 14, STICKY_COLOR_BLACK);
    }

    // Daily Reading Goal Section
    int goal_y = box_y + box_h + 15;
    int goal_h = 100;
    g_host->draw_rect(20, goal_y, box_w, goal_h, STICKY_COLOR_BLACK);

    char goal_title[48];
    char p_buf[8], gp_buf[8];
    mini_itoa(g_hero.pages_read_today, p_buf);
    mini_itoa(g_hero.daily_goal_pages, gp_buf);
    mini_strcpy(goal_title, "DAILY GOAL: ");
    mini_strcat(goal_title, p_buf);
    mini_strcat(goal_title, " / ");
    mini_strcat(goal_title, gp_buf);
    mini_strcat(goal_title, " PAGES READ");
    g_host->draw_text(35, goal_y + 16, goal_title, 0, STICKY_COLOR_BLACK);

    int g_fill = (g_hero.daily_goal_pages > 0) ? (g_hero.pages_read_today * xp_w / g_hero.daily_goal_pages) : 0;
    if (g_fill > xp_w) g_fill = xp_w;
    int g_bar_y = goal_y + 48;
    g_host->draw_rect(35, g_bar_y, xp_w, 16, STICKY_COLOR_BLACK);
    if (g_fill > 0) {
        g_host->fill_rect(35, g_bar_y, g_fill, 16, STICKY_COLOR_BLACK);
    }

    const char* sub_hint = (g_hero.pages_read_today >= g_hero.daily_goal_pages)
                               ? "Goal Achieved! Bonus +50 XP awarded!"
                               : "Keep reading to earn XP and level up!";
    g_host->draw_text(35, goal_y + 72, sub_hint, 0, STICKY_COLOR_DARKGRAY);

    // Active Quests Card
    int q_y = goal_y + goal_h + 15;
    int q_h = 160;
    g_host->draw_rect(20, q_y, box_w, q_h, STICKY_COLOR_BLACK);

    g_host->draw_text(35, q_y + 15, "ACTIVE READING QUESTS", 0, STICKY_COLOR_BLACK);
    g_host->draw_line(35, q_y + 36, sw - 35, q_y + 36, STICKY_COLOR_BLACK);

    g_host->draw_text(35, q_y + 48, "[1] Read 1 complete book chapter (+40 XP)", 0, STICKY_COLOR_BLACK);
    g_host->draw_text(35, q_y + 80, "[2] Focused 25m reading session (+30 XP)", 0, STICKY_COLOR_BLACK);
    g_host->draw_text(35, q_y + 112, "[3] Save 3 insightful quotes (+25 XP)", 0, STICKY_COLOR_BLACK);

    // Action Log Buttons
    int btn_w = 200, btn_h = 50;
    int btn_y = q_y + q_h + 18;

    int b1_x = cx - btn_w - 10;
    int b2_x = cx + 10;

    // +5 PAGES
    g_host->fill_rect(b1_x, btn_y, btn_w, btn_h, STICKY_COLOR_BLACK);
    int16_t w1 = 0, h1 = 0;
    g_host->get_text_bounds("+5 PAGES READ", 0, &w1, &h1);
    g_host->draw_text(b1_x + (btn_w - w1) / 2, btn_y + (btn_h - h1) / 2, "+5 PAGES READ", 0, STICKY_COLOR_WHITE);

    // +10 PAGES
    g_host->draw_rect(b2_x, btn_y, btn_w, btn_h, STICKY_COLOR_BLACK);
    g_host->draw_rect(b2_x + 1, btn_y + 1, btn_w - 2, btn_h - 2, STICKY_COLOR_BLACK);
    int16_t w2 = 0, h2 = 0;
    g_host->get_text_bounds("+10 PAGES READ", 0, &w2, &h2);
    g_host->draw_text(b2_x + (btn_w - w2) / 2, btn_y + (btn_h - h2) / 2, "+10 PAGES READ", 0, STICKY_COLOR_BLACK);
}

static void log_pages(int count) {
    g_hero.pages_read_today += count;
    g_hero.xp += count * 5;
    g_hero.gold += count * 2;

    if (g_hero.xp >= 100) {
        g_hero.xp -= 100;
        g_hero.level++;
        if (g_host) g_host->play_haptic(1);
    }
    if (g_hero.pages_read_today >= g_hero.daily_goal_pages) {
        g_hero.quests_completed++;
    }
    save_quest();
}

static void app_touch(int event_type, int16_t x, int16_t y) {
    if (!g_host || event_type != STICKY_TOUCH_CLICK) return;

    int sw = g_host->get_screen_width();
    int cx = sw / 2;

    int box_h = 105;
    int box_y = 65;
    int goal_y = box_y + box_h + 15;
    int goal_h = 100;
    int q_y = goal_y + goal_h + 15;
    int q_h = 160;

    int btn_w = 200, btn_h = 50;
    int btn_y = q_y + q_h + 18;

    int b1_x = cx - btn_w - 10;
    int b2_x = cx + 10;

    // +5 PAGES
    if (x >= b1_x && x < b1_x + btn_w && y >= btn_y && y < btn_y + btn_h) {
        g_host->play_haptic(0);
        log_pages(5);
        g_host->request_update(true);
        return;
    }

    // +10 PAGES
    if (x >= b2_x && x < b2_x + btn_w && y >= btn_y && y < btn_y + btn_h) {
        g_host->play_haptic(0);
        log_pages(10);
        g_host->request_update(true);
        return;
    }
}

static void app_exit(void) {
    save_quest();
}

static void app_destroy(void) {}

static const StickyAppPlugin g_plugin = {
    .api_version = STICKY_APP_API_VERSION,
    .app_name = "ReadingQuest",
    .app_version = "1.0.0",
    .app_author = "Sticky",
    .icon_name = "readingquest",
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
