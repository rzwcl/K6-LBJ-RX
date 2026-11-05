#ifndef K6_APP_FORMAT_H
#define K6_APP_FORMAT_H

#include <stdint.h>
#include "k6_app_api.h"

#define K6_APP_MAGIC_0 'K'
#define K6_APP_MAGIC_1 '6'
#define K6_APP_MAGIC_2 'A'
#define K6_APP_MAGIC_3 '1'
#define K6_APP_HEADER_VERSION 1u
#define K6_APP_FLAG_COMMITTED 0x0001u

/*
 * K6A1 header, exactly 64 bytes, little-endian:
 * 00 magic[4], 04 header_version, 06 abi_major, 07 api_min,
 * 08 image_size, 0C image_crc32, 10 entry_offset, 12 flags,
 * 14 name[12], 20 version[12], 2C link_vma, 30 bss_size,
 * 34 required_caps, 38 asset_size, 3C asset_crc32.
 */
typedef struct __attribute__((packed)) {
    char magic[4];
    uint16_t header_version;
    uint8_t abi_major;
    uint8_t api_min;
    uint32_t image_size;
    uint32_t image_crc32;
    uint16_t entry_offset;
    uint16_t flags;
    char name[12];
    char version[12];
    uint32_t link_vma;
    uint32_t bss_size;
    uint32_t required_caps;
    uint32_t asset_size;
    uint32_t asset_crc32;
} K6_AppHeader;

_Static_assert(sizeof(K6_AppHeader) == K6_APP_HEADER_SIZE,
               "K6A1 header must remain exactly 64 bytes");
_Static_assert(__builtin_offsetof(K6_AppHeader, image_size) == 8u,
               "K6A1 image_size offset changed");
_Static_assert(__builtin_offsetof(K6_AppHeader, name) == 20u,
               "K6A1 name offset changed");
_Static_assert(__builtin_offsetof(K6_AppHeader, link_vma) == 44u,
               "K6A1 link_vma offset changed");
_Static_assert(__builtin_offsetof(K6_AppHeader, asset_size) == 56u,
               "K6A1 asset_size offset changed");

#endif
