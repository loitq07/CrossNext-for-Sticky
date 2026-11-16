#pragma once
#include <stdint.h>
#include <stdbool.h>

static inline void mini_itoa(int val, char* buf) {
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    bool neg = false;
    if (val < 0) {
        neg = true;
        val = -val;
    }
    char tmp[16];
    int i = 0;
    while (val > 0) {
        tmp[i++] = '0' + (val % 10);
        val /= 10;
    }
    int j = 0;
    if (neg) buf[j++] = '-';
    while (i > 0) {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
}

static inline void mini_format_2d(int val, char* buf) {
    if (val < 0) val = 0;
    buf[0] = '0' + ((val / 10) % 10);
    buf[1] = '0' + (val % 10);
    buf[2] = '\0';
}

static inline void mini_format_4d(int val, char* buf) {
    if (val < 0) val = 0;
    buf[0] = '0' + ((val / 1000) % 10);
    buf[1] = '0' + ((val / 100) % 10);
    buf[2] = '0' + ((val / 10) % 10);
    buf[3] = '0' + (val % 10);
    buf[4] = '\0';
}

static inline void mini_strcat(char* dest, const char* src) {
    while (*dest) dest++;
    while (*src) *dest++ = *src++;
    *dest = '\0';
}

static inline void mini_strcpy(char* dest, const char* src) {
    while (*src) *dest++ = *src++;
    *dest = '\0';
}
