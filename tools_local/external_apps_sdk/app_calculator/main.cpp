#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "sticky_app_api.h"

#define MAX_DIGITS 16

typedef struct {
    char display[MAX_DIGITS + 1];
    char pending_op;
    double operand_a;
    bool reset_on_next;
} CalcState;

static const StickyHostApi* g_host = NULL;
static CalcState g_calc;

static void reset_calc(void) {
    strcpy(g_calc.display, "0");
    g_calc.pending_op = 0;
    g_calc.operand_a = 0;
    g_calc.reset_on_next = false;
}

static double simple_atof(const char* s) {
    double res = 0.0;
    double factor = 1.0;
    bool negative = false;
    if (*s == '-') { negative = true; s++; }
    while (*s >= '0' && *s <= '9') {
        res = res * 10.0 + (*s - '0');
        s++;
    }
    if (*s == '.') {
        s++;
        while (*s >= '0' && *s <= '9') {
            factor *= 0.1;
            res += (*s - '0') * factor;
            s++;
        }
    }
    return negative ? -res : res;
}

// Minimal float to string without pulling vfiprintf
static void double_to_str(double val, char* out, int max_len) {
    if (val < 0) {
        *out++ = '-';
        val = -val;
        max_len--;
    }
    int64_t int_part = (int64_t)val;
    double frac = val - (double)int_part;

    // Integer part
    char temp[32];
    int i = 0;
    if (int_part == 0) {
        temp[i++] = '0';
    } else {
        while (int_part > 0 && i < 20) {
            temp[i++] = '0' + (int_part % 10);
            int_part /= 10;
        }
    }
    int j = 0;
    while (i > 0 && j < max_len - 1) {
        out[j++] = temp[--i];
    }

    // Fraction part (up to 4 digits)
    if (frac > 0.00001 && j < max_len - 6) {
        out[j++] = '.';
        for (int f = 0; f < 4; ++f) {
            frac *= 10.0;
            int digit = (int)frac;
            out[j++] = '0' + digit;
            frac -= digit;
        }
        // Strip trailing zeros
        while (j > 0 && out[j - 1] == '0') j--;
        if (j > 0 && out[j - 1] == '.') j--;
    }
    out[j] = '\0';
}

static void on_key(const char* key) {
    if (strcmp(key, "C") == 0) {
        reset_calc();
        return;
    }
    if (strcmp(key, "DEL") == 0) {
        int len = strlen(g_calc.display);
        if (len > 1) {
            g_calc.display[len - 1] = '\0';
        } else {
            strcpy(g_calc.display, "0");
        }
        return;
    }
    if (strcmp(key, "+/-") == 0) {
        if (strcmp(g_calc.display, "0") != 0) {
            if (g_calc.display[0] == '-') {
                memmove(g_calc.display, g_calc.display + 1, strlen(g_calc.display));
            } else {
                memmove(g_calc.display + 1, g_calc.display, strlen(g_calc.display) + 1);
                g_calc.display[0] = '-';
            }
        }
        return;
    }
    if (strcmp(key, "%") == 0) {
        double v = simple_atof(g_calc.display) / 100.0;
        double_to_str(v, g_calc.display, MAX_DIGITS);
        g_calc.reset_on_next = true;
        return;
    }
    if (strcmp(key, "+") == 0 || strcmp(key, "-") == 0 ||
        strcmp(key, "*") == 0 || strcmp(key, "/") == 0) {
        g_calc.operand_a = simple_atof(g_calc.display);
        g_calc.pending_op = key[0];
        g_calc.reset_on_next = true;
        return;
    }
    if (strcmp(key, "=") == 0) {
        if (g_calc.pending_op != 0) {
            double op_b = simple_atof(g_calc.display);
            double res = 0;
            switch (g_calc.pending_op) {
                case '+': res = g_calc.operand_a + op_b; break;
                case '-': res = g_calc.operand_a - op_b; break;
                case '*': res = g_calc.operand_a * op_b; break;
                case '/': res = (op_b != 0) ? (g_calc.operand_a / op_b) : 0; break;
            }
            double_to_str(res, g_calc.display, MAX_DIGITS);
            g_calc.pending_op = 0;
            g_calc.reset_on_next = true;
        }
        return;
    }

    // Digit or decimal point
    if (g_calc.reset_on_next) {
        strcpy(g_calc.display, "0");
        g_calc.reset_on_next = false;
    }

    if (strcmp(key, ".") == 0) {
        if (!strchr(g_calc.display, '.')) {
            strcat(g_calc.display, ".");
        }
        return;
    }

    // Numeric digit
    if (strcmp(g_calc.display, "0") == 0) {
        strcpy(g_calc.display, key);
    } else {
        if (strlen(g_calc.display) < MAX_DIGITS) {
            strcat(g_calc.display, key);
        }
    }
}

// ----------------- App Lifecycle -----------------

static void app_init(const StickyHostApi* host) {
    g_host = host;
}

static void app_enter(void) {
    reset_calc();
    if (g_host) {
        g_host->request_update(true);
    }
}

static void app_loop(void) {}

static const char* kGrid[5][4] = {
    {"C", "+/-", "%", "/"},
    {"7", "8", "9", "*"},
    {"4", "5", "6", "-"},
    {"1", "2", "3", "+"},
    {"0", ".", "DEL", "="}
};

static void app_render(void) {
    if (!g_host) return;

    g_host->clear(STICKY_COLOR_WHITE);
    g_host->draw_header("CALCULATOR");

    int screen_w = g_host->get_screen_width();

    // Display Box
    int disp_x = 20;
    int disp_y = 70;
    int disp_w = screen_w - 40;
    int disp_h = 100;

    g_host->draw_rect(disp_x, disp_y, disp_w, disp_h, STICKY_COLOR_BLACK);
    g_host->draw_rect(disp_x + 1, disp_y + 1, disp_w - 2, disp_h - 2, STICKY_COLOR_BLACK);

    // Operator indicator
    if (g_calc.pending_op != 0) {
        char op_str[2] = { g_calc.pending_op, '\0' };
        g_host->draw_text(disp_x + 15, disp_y + 15, op_str, 0, STICKY_COLOR_BLACK);
    }

    // Display Value
    int16_t tw = 0, th = 0;
    g_host->get_text_bounds(g_calc.display, 0, &tw, &th);
    int tx = disp_x + disp_w - tw - 20;
    int ty = disp_y + (disp_h - th) / 2 + 5;
    g_host->draw_text(tx, ty, g_calc.display, 0, STICKY_COLOR_BLACK);

    // Keypad Layout (4 columns x 5 rows)
    int start_y = disp_y + disp_h + 25;
    int pad_w = screen_w - 40;
    int gap = 10;
    int key_w = (pad_w - gap * 3) / 4;
    int key_h = 80;

    for (int r = 0; r < 5; ++r) {
        for (int c = 0; c < 4; ++c) {
            int kx = disp_x + c * (key_w + gap);
            int ky = start_y + r * (key_h + gap);
            const char* label = kGrid[r][c];

            bool is_equals = (strcmp(label, "=") == 0);
            if (is_equals) {
                g_host->fill_rect(kx, ky, key_w, key_h, STICKY_COLOR_BLACK);
            } else {
                g_host->fill_rect(kx, ky, key_w, key_h, STICKY_COLOR_WHITE);
                g_host->draw_rect(kx, ky, key_w, key_h, STICKY_COLOR_BLACK);
                g_host->draw_rect(kx + 1, ky + 1, key_w - 2, key_h - 2, STICKY_COLOR_BLACK);
            }

            int16_t lw = 0, lh = 0;
            g_host->get_text_bounds(label, 0, &lw, &lh);
            int lx = kx + (key_w - lw) / 2;
            int ly = ky + (key_h - lh) / 2;
            g_host->draw_text(lx, ly, label, 0, is_equals ? STICKY_COLOR_WHITE : STICKY_COLOR_BLACK);
        }
    }
}

static void app_touch(int event_type, int16_t x, int16_t y) {
    if (!g_host) return;

    int screen_w = g_host->get_screen_width();
    int disp_x = 20;
    int disp_y = 70;
    int disp_h = 100;
    int start_y = disp_y + disp_h + 25;
    int pad_w = screen_w - 40;
    int gap = 10;
    int key_w = (pad_w - gap * 3) / 4;
    int key_h = 80;

    if (event_type == STICKY_TOUCH_CLICK) {
        for (int r = 0; r < 5; ++r) {
            for (int c = 0; c < 4; ++c) {
                int kx = disp_x + c * (key_w + gap);
                int ky = start_y + r * (key_h + gap);
                if (x >= kx && x <= kx + key_w && y >= ky && y <= ky + key_h) {
                    on_key(kGrid[r][c]);
                    g_host->play_haptic(1);
                    g_host->request_update(true);
                    return;
                }
            }
        }
    }
}

static void app_exit(void) {}
static void app_destroy(void) {}

// ----------------- Plugin Definition -----------------

static const StickyAppPlugin s_calculator_plugin = {
    .api_version = STICKY_APP_API_VERSION,
    .app_name = "Calculator",
    .app_version = "1.0.0",
    .app_author = "CrossNext",
    .icon_name = "calculator",
    .on_init = app_init,
    .on_enter = app_enter,
    .on_loop = app_loop,
    .on_render = app_render,
    .on_touch = app_touch,
    .on_exit = app_exit,
    .on_destroy = app_destroy,
};

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((visibility("default")))
const StickyAppPlugin* sticky_app_get_plugin(void) {
    return &s_calculator_plugin;
}

__attribute__((visibility("default")))
const StickyAppPlugin* main_entry(const StickyHostApi* host) {
    app_init(host);
    return &s_calculator_plugin;
}

#ifdef __cplusplus
}
#endif
