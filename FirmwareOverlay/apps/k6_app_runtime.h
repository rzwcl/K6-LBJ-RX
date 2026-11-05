#ifndef K6_APP_RUNTIME_H
#define K6_APP_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>
#include "k6_app_format.h"

typedef bool (*K6_AppEEPROMRead)(uint32_t address,
                                 void *destination,
                                 uint8_t length);
typedef bool (*K6_AppEEPROMWrite)(uint32_t address,
                                  const void *source,
                                  uint8_t length);

typedef struct {
    K6_AppEEPROMRead eeprom_read;

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
} K6_AppHostServices;

typedef struct {
    K6_AppHeader header;
    uint32_t slot_address;
    uint8_t slot_index;
} K6_AppSlotInfo;

/*
 * Bind one active APP to the resident host service table. Only one APP may
 * execute at a time. The caller must call K6APP_ClearAPI() after it returns.
 */
bool K6APP_BuildAPI(const K6_AppHostServices *host,
                    const K6_AppSlotInfo *slot,
                    K6_AppAPI *api);
void K6APP_ClearAPI(void);

/*
 * Install a complete .k6app package into an erased slot.
 * Writes are split at 128-byte EEPROM page boundaries. The slot header is
 * first written uncommitted and the committed header is written last.
 * This is host-tested only; the hardware adapter must wait for EEPROM writes
 * to finish and report NACK/errors correctly.
 */
bool K6APP_Install(K6_AppEEPROMRead read_eeprom,
                   K6_AppEEPROMWrite write_eeprom,
                   uint8_t slot_index,
                   const void *package,
                   uint32_t package_size);

/* Read and verify the code/asset CRCs stored in a slot. */
bool K6APP_VerifySlot(K6_AppEEPROMRead read_eeprom,
                      uint8_t slot_index,
                      K6_AppSlotInfo *info);

/* Header/range check only; code and asset CRCs are checked during Load(). */
bool K6APP_ReadSlotInfo(K6_AppEEPROMRead read_eeprom,
                        uint8_t slot_index,
                        K6_AppSlotInfo *info);

/* Verify cached slot identity against the current EEPROM header. */
bool K6APP_RefreshSlotInfo(K6_AppEEPROMRead read_eeprom,
                           const K6_AppSlotInfo *cached,
                           K6_AppSlotInfo *fresh);

/* Verify code + assets, load image to overlay_ram, and clear its BSS. */
bool K6APP_Load(K6_AppEEPROMRead read_eeprom,
                const K6_AppSlotInfo *info,
                void *overlay_ram,
                uint32_t overlay_capacity);

/* Read a bounded resource range from the APP's EEPROM tail. */
bool K6APP_ReadAsset(K6_AppEEPROMRead read_eeprom,
                     const K6_AppSlotInfo *info,
                     uint32_t offset,
                     void *destination,
                     uint16_t length);

/* Return the Thumb entry address inside a successfully loaded overlay image. */
K6_AppEntry K6APP_GetEntry(const K6_AppSlotInfo *info, void *overlay_ram);

uint32_t K6APP_CRC32(const void *data, uint32_t length);

#endif
