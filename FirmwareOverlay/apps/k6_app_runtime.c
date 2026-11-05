#include "k6_app_runtime.h"

#include <stddef.h>
#include <string.h>

#define K6_APP_IO_CHUNK 64u

static bool K6APP_HeaderBasicValid(const K6_AppHeader *h)
{
    const uint64_t ram_total = (uint64_t)h->image_size + (uint64_t)h->bss_size;
    const uint64_t file_total = (uint64_t)K6_APP_HEADER_SIZE +
                               (uint64_t)h->image_size +
                               (uint64_t)h->asset_size;

    if (h->magic[0] != K6_APP_MAGIC_0 ||
        h->magic[1] != K6_APP_MAGIC_1 ||
        h->magic[2] != K6_APP_MAGIC_2 ||
        h->magic[3] != K6_APP_MAGIC_3)
        return false;

    if (h->header_version != K6_APP_HEADER_VERSION ||
        h->abi_major != K6_APP_ABI_MAJOR ||
        h->api_min > K6_APP_API_LEVEL ||
        (h->flags & K6_APP_FLAG_COMMITTED) == 0u)
        return false;

    if (h->image_size == 0u ||
        h->link_vma != K6_APP_LINK_VMA ||
        ram_total > K6_APP_RAM_SIZE ||
        file_total > K6_APP_SLOT_SIZE)
        return false;

    if (h->entry_offset >= h->image_size ||
        (h->entry_offset & 1u) != 0u)
        return false;

    if (h->name[0] == '\0' ||
        memchr(h->name, '\0', sizeof(h->name)) == NULL ||
        memchr(h->version, '\0', sizeof(h->version)) == NULL)
        return false;

    if (h->asset_size == 0u && h->asset_crc32 != 0u)
        return false;

    return true;
}

static uint32_t K6APP_CRC32Update(uint32_t crc,
                                  const uint8_t *data,
                                  uint32_t length)
{
    for (uint32_t i = 0u; i < length; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0u; bit < 8u; ++bit)
            crc = (crc & 1u) ? ((crc >> 1) ^ 0xEDB88320u) : (crc >> 1);
    }
    return crc;
}

uint32_t K6APP_CRC32(const void *data, uint32_t length)
{
    return K6APP_CRC32Update(0xFFFFFFFFu, (const uint8_t *)data, length) ^
           0xFFFFFFFFu;
}

bool K6APP_ReadSlotInfo(K6_AppEEPROMRead read_eeprom,
                        uint8_t slot_index,
                        K6_AppSlotInfo *info)
{
    uint8_t raw[K6_APP_HEADER_SIZE];
    uint32_t slot_address;

    if (read_eeprom == NULL || info == NULL ||
        slot_index >= K6_APP_SLOT_COUNT)
        return false;

    slot_address = K6_APP_EEPROM_BASE +
                   ((uint32_t)slot_index * K6_APP_SLOT_SIZE);

    if (!read_eeprom(slot_address, raw, (uint8_t)sizeof(raw)))
        return false;

    memcpy(&info->header, raw, sizeof(info->header));
    if (!K6APP_HeaderBasicValid(&info->header))
        return false;

    info->slot_address = slot_address;
    info->slot_index = slot_index;
    return true;
}

bool K6APP_RefreshSlotInfo(K6_AppEEPROMRead read_eeprom,
                           const K6_AppSlotInfo *cached,
                           K6_AppSlotInfo *fresh)
{
    K6_AppSlotInfo current;

    if (cached == NULL || fresh == NULL ||
        !K6APP_ReadSlotInfo(read_eeprom, cached->slot_index, &current))
        return false;

    if (current.slot_address != cached->slot_address ||
        memcmp(&current.header, &cached->header,
               sizeof(current.header)) != 0)
        return false;

    *fresh = current;
    return true;
}

K6_AppEntry K6APP_GetEntry(const K6_AppSlotInfo *info, void *overlay_ram)
{
    uintptr_t address;

    if (info == NULL || overlay_ram == NULL ||
        !K6APP_HeaderBasicValid(&info->header))
        return NULL;

    address = (uintptr_t)overlay_ram + info->header.entry_offset;
    address |= (uintptr_t)1u; /* Cortex-M Thumb-state entry point. */
    return (K6_AppEntry)address;
}

bool K6APP_ReadAsset(K6_AppEEPROMRead read_eeprom,
                     const K6_AppSlotInfo *info,
                     uint32_t offset,
                     void *destination,
                     uint16_t length)
{
    uint8_t *out = (uint8_t *)destination;
    uint32_t asset_address;
    uint32_t remaining = length;

    if (read_eeprom == NULL || info == NULL ||
        (destination == NULL && length != 0u))
        return false;

    if (offset > info->header.asset_size ||
        (uint64_t)offset + length > info->header.asset_size)
        return false;

    asset_address = info->slot_address + K6_APP_HEADER_SIZE +
                    info->header.image_size + offset;

    while (remaining != 0u) {
        const uint8_t chunk = (remaining > K6_APP_IO_CHUNK)
                                  ? K6_APP_IO_CHUNK
                                  : (uint8_t)remaining;
        if (!read_eeprom(asset_address, out, chunk))
            return false;
        asset_address += chunk;
        out += chunk;
        remaining -= chunk;
    }

    return true;
}

bool K6APP_Load(K6_AppEEPROMRead read_eeprom,
                const K6_AppSlotInfo *info,
                void *overlay_ram,
                uint32_t overlay_capacity)
{
    uint8_t chunk_buffer[K6_APP_IO_CHUNK];
    uint8_t *overlay = (uint8_t *)overlay_ram;
    uint32_t image_offset = 0u;
    uint32_t crc = 0xFFFFFFFFu;
    uint32_t asset_offset = 0u;

    if (read_eeprom == NULL || info == NULL || overlay_ram == NULL)
        return false;

    if (!K6APP_HeaderBasicValid(&info->header) ||
        overlay_capacity < ((uint64_t)info->header.image_size +
                            info->header.bss_size))
        return false;

    /* Copy code while calculating its CRC. A bad image is cleared below. */
    while (image_offset < info->header.image_size) {
        uint32_t left = info->header.image_size - image_offset;
        uint8_t chunk = (left > K6_APP_IO_CHUNK)
                            ? K6_APP_IO_CHUNK
                            : (uint8_t)left;
        const uint32_t source = info->slot_address +
                                K6_APP_HEADER_SIZE + image_offset;

        if (!read_eeprom(source, chunk_buffer, chunk))
            goto fail;

        memcpy(overlay + image_offset, chunk_buffer, chunk);
        crc = K6APP_CRC32Update(crc, chunk_buffer, chunk);
        image_offset += chunk;
    }

    crc ^= 0xFFFFFFFFu;
    if (crc != info->header.image_crc32)
        goto fail;

    /* Assets remain in EEPROM and are verified without consuming overlay RAM. */
    crc = 0xFFFFFFFFu;
    while (asset_offset < info->header.asset_size) {
        uint32_t left = info->header.asset_size - asset_offset;
        uint8_t chunk = (left > K6_APP_IO_CHUNK)
                            ? K6_APP_IO_CHUNK
                            : (uint8_t)left;
        const uint32_t source = info->slot_address +
                                K6_APP_HEADER_SIZE +
                                info->header.image_size + asset_offset;

        if (!read_eeprom(source, chunk_buffer, chunk))
            goto fail;

        crc = K6APP_CRC32Update(crc, chunk_buffer, chunk);
        asset_offset += chunk;
    }

    crc ^= 0xFFFFFFFFu;
    if (crc != info->header.asset_crc32)
        goto fail;

    memset(overlay + info->header.image_size, 0, info->header.bss_size);
    return true;

fail:
    memset(overlay, 0,
           (size_t)(info->header.image_size + info->header.bss_size));
    return false;
}
