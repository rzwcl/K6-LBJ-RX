#ifndef K6_APP_RUNTIME_H
#define K6_APP_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>
#include "k6_app_format.h"

typedef bool (*K6_AppEEPROMRead)(uint32_t address,
                                 void *destination,
                                 uint8_t length);

typedef struct {
    K6_AppHeader header;
    uint32_t slot_address;
    uint8_t slot_index;
} K6_AppSlotInfo;

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
