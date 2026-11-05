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
#define K6_APP_EEPROM_BASE 0x0002A400u
#define K6_APP_SLOT_SIZE 0x00002000u
#define K6_APP_SLOT_COUNT 8u
#define K6_APP_HEADER_SIZE 64u

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
 * Receive profile: the resident firmware validates the requested frequency
 * and applies only receive-side BK4819 settings. The APP ABI deliberately
 * exposes no arbitrary BK4819 register write and no transmit function.
 */
typedef struct {
    uint32_t frequency_hz;
    uint32_t sync_word;
    uint8_t bitrate;
    uint8_t deviation;
    uint8_t filter;
    uint8_t flags;
} K6_AppRxProfile;

/*
 * Resident services. The application runs from the fixed SRAM overlay and
 * must use this table instead of directly linking to firmware symbols.
 *
 * asset_read() accesses the APP slot tail in EEPROM; assets do not consume
 * overlay SRAM. key_get_event() returns one debounced event per key press.
 */
typedef struct K6_AppAPI {
    uint8_t abi_major;
    uint8_t api_level;
    uint16_t struct_size;

    void (*display_clear)(void);
    void (*print_line)(const char *text, uint8_t line);
    void (*display_refresh)(void);
    void (*backlight_on)(void);

    uint8_t (*key_get_event)(void);
    uint32_t (*millis)(void);
    void (*delay_ms)(uint32_t ms);

    bool (*rx_configure)(const K6_AppRxProfile *profile);
    uint8_t (*rx_fifo_read)(uint16_t *words, uint8_t capacity);
    uint16_t (*rx_read_rssi)(void);
    void (*rx_restore)(void);

    uint32_t asset_size;
    bool (*asset_read)(uint32_t offset, void *destination, uint16_t length);
} K6_AppAPI;

typedef void (*K6_AppEntry)(const K6_AppAPI *api);

#endif
