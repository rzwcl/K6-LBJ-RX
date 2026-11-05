#ifndef K6_APP_API_H
#define K6_APP_API_H

#include <stdbool.h>
#include <stdint.h>

/*
 * K6-native APP ABI.
 *
 * This is intentionally separate from the upstream F4HWN FAP1 ABI:
 * that ABI is linked for PY32F071/PY25Q16 and is not executable on DP32G030.
 *
 * ABI rules:
 * - append new service pointers only; never reorder existing members;
 * - bump K6_APP_API_LEVEL when adding append-only services;
 * - bump K6_APP_ABI_MAJOR for incompatible layout changes.
 */
#define K6_APP_ABI_MAJOR 1u
#define K6_APP_API_LEVEL 1u

#define K6_APP_LINK_VMA 0x20001800u
#define K6_APP_RAM_SIZE 0x00001800u

typedef enum {
    K6_APP_KEY_0 = 0,
    K6_APP_KEY_1 = 1,
    K6_APP_KEY_2 = 2,
    K6_APP_KEY_3 = 3,
    K6_APP_KEY_4 = 4,
    K6_APP_KEY_5 = 5,
    K6_APP_KEY_6 = 6,
    K6_APP_KEY_7 = 7,
    K6_APP_KEY_8 = 8,
    K6_APP_KEY_9 = 9,
    K6_APP_KEY_MENU = 10,
    K6_APP_KEY_UP = 11,
    K6_APP_KEY_DOWN = 12,
    K6_APP_KEY_EXIT = 13,
    K6_APP_KEY_STAR = 14,
    K6_APP_KEY_F = 15,
    K6_APP_KEY_PTT = 16,
    K6_APP_KEY_SIDE2 = 17,
    K6_APP_KEY_SIDE1 = 18,
    K6_APP_KEY_INVALID = 19
} K6_AppKey;

/*
 * Resident services. The application runs from the fixed SRAM overlay and
 * must use this table instead of directly linking to firmware symbols.
 *
 * radio_read/radio_write expose BK4819 registers. The host must restore the
 * current VFO/radio configuration when the app returns.
 */
typedef struct K6_AppAPI {
    uint8_t abi_major;
    uint8_t api_level;
    uint16_t struct_size;

    void (*display_clear)(void);
    void (*print_line)(const char *text, uint8_t line);
    void (*display_refresh)(void);
    void (*backlight_on)(void);

    uint8_t (*get_key)(void);
    uint32_t (*millis)(void);
    void (*delay_ms)(uint32_t ms);

    uint16_t (*radio_read)(uint8_t reg);
    void (*radio_write)(uint8_t reg, uint16_t value);
    void (*radio_restore)(void);
} K6_AppAPI;

typedef void (*K6_AppEntry)(const K6_AppAPI *api);

#endif
