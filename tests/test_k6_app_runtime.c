#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "k6_app_runtime.h"

#define EEPROM_BYTES 0x40000u
#define TEST_CODE_SIZE 300u
#define TEST_BSS_SIZE 64u
#define TEST_ASSET_SIZE 130u

static uint8_t g_eeprom[EEPROM_BYTES];
static uint8_t g_code[TEST_CODE_SIZE];
static uint8_t g_assets[TEST_ASSET_SIZE];

static bool mock_read(uint32_t address, void *destination, uint8_t length)
{
    if ((uint64_t)address + length > EEPROM_BYTES)
        return false;
    memcpy(destination, &g_eeprom[address], length);
    return true;
}

#define TEST_PACKAGE_SIZE (K6_APP_HEADER_SIZE + TEST_CODE_SIZE + TEST_ASSET_SIZE)

static uint32_t g_write_count;
static uint32_t g_fail_write_at;

static bool mock_write(uint32_t address, const void *source, uint8_t length)
{
    if ((uint64_t)address + length > EEPROM_BYTES ||
        length > 128u ||
        ((address & 0x7Fu) + length) > 128u)
        return false;

    ++g_write_count;
    if (g_fail_write_at != 0u && g_write_count == g_fail_write_at)
        return false;

    memcpy(&g_eeprom[address], source, length);
    return true;
}

static void make_package(uint8_t package[TEST_PACKAGE_SIZE])
{
    K6_AppHeader header;

    memset(&header, 0, sizeof(header));
    memcpy(header.magic, "K6A1", 4u);
    header.header_version = K6_APP_HEADER_VERSION;
    header.abi_major = K6_APP_ABI_MAJOR;
    header.api_min = K6_APP_API_LEVEL;
    header.image_size = TEST_CODE_SIZE;
    header.image_crc32 = K6APP_CRC32(g_code, sizeof(g_code));
    header.entry_offset = 0u;
    header.flags = K6_APP_FLAG_COMMITTED;
    memcpy(header.name, "INSTALL", sizeof("INSTALL"));
    memcpy(header.version, "1.0", sizeof("1.0"));
    header.link_vma = K6_APP_LINK_VMA;
    header.bss_size = TEST_BSS_SIZE;
    header.required_caps = 0u;
    header.asset_size = TEST_ASSET_SIZE;
    header.asset_crc32 = K6APP_CRC32(g_assets, sizeof(g_assets));

    memcpy(package, &header, sizeof(header));
    memcpy(package + K6_APP_HEADER_SIZE, g_code, sizeof(g_code));
    memcpy(package + K6_APP_HEADER_SIZE + sizeof(g_code),
           g_assets, sizeof(g_assets));
}

static void make_image(uint8_t slot, uint16_t flags)
{
    K6_AppHeader header;
    const uint32_t base = K6_APP_EEPROM_BASE +
                          ((uint32_t)slot * K6_APP_SLOT_SIZE);

    memset(&header, 0, sizeof(header));
    memcpy(header.magic, "K6A1", 4u);
    header.header_version = K6_APP_HEADER_VERSION;
    header.abi_major = K6_APP_ABI_MAJOR;
    header.api_min = K6_APP_API_LEVEL;
    header.image_size = TEST_CODE_SIZE;
    header.image_crc32 = K6APP_CRC32(g_code, sizeof(g_code));
    header.entry_offset = 0u;
    header.flags = flags;
    memcpy(header.name, "LBJRX-K6", sizeof("LBJRX-K6"));
    memcpy(header.version, "0.1", sizeof("0.1"));
    header.link_vma = K6_APP_LINK_VMA;
    header.bss_size = TEST_BSS_SIZE;
    header.required_caps = 0u;
    header.asset_size = TEST_ASSET_SIZE;
    header.asset_crc32 = K6APP_CRC32(g_assets, sizeof(g_assets));

    memcpy(&g_eeprom[base], &header, sizeof(header));
    memcpy(&g_eeprom[base + K6_APP_HEADER_SIZE],
           g_code, sizeof(g_code));
    memcpy(&g_eeprom[base + K6_APP_HEADER_SIZE + sizeof(g_code)],
           g_assets, sizeof(g_assets));
}

static void stub_display_clear(void) {}
static void stub_print_line(const char *text, uint8_t line)
{
    (void)text;
    (void)line;
}
static void stub_display_refresh(void) {}
static void stub_backlight_on(void) {}
static uint8_t stub_key_get_event(void) { return K6_APP_KEY_INVALID; }
static uint32_t stub_millis(void) { return 0u; }
static void stub_delay_ms(uint32_t ms) { (void)ms; }

static int test_build_api_asset_binding(void)
{
    K6_AppSlotInfo info;
    K6_AppHostServices host;
    K6_AppAPI api;
    uint8_t resource[12];

    if (!K6APP_ReadSlotInfo(mock_read, 0u, &info))
        return 1;

    memset(&host, 0, sizeof(host));
    host.eeprom_read = mock_read;
    host.display_clear = stub_display_clear;
    host.print_line = stub_print_line;
    host.display_refresh = stub_display_refresh;
    host.backlight_on = stub_backlight_on;
    host.key_get_event = stub_key_get_event;
    host.millis = stub_millis;
    host.delay_ms = stub_delay_ms;

    if (!K6APP_BuildAPI(&host, &info, &api))
        return 2;
    if (api.abi_major != K6_APP_ABI_MAJOR ||
        api.api_level != K6_APP_API_LEVEL ||
        api.struct_size != sizeof(api) ||
        api.asset_size != TEST_ASSET_SIZE)
        return 3;

    memset(resource, 0, sizeof(resource));
    if (!api.asset_read(0u, resource, sizeof(resource)))
        return 4;
    if (memcmp(resource, g_assets, sizeof(resource)) != 0)
        return 5;

    K6APP_ClearAPI();
    if (api.asset_read(0u, resource, sizeof(resource)))
        return 6;
    return 0;
}

static int test_load_and_read_asset(void)
{
    K6_AppSlotInfo info;
    uint8_t ram[K6_APP_RAM_SIZE];
    uint8_t resource[70];

    if (!K6APP_ReadSlotInfo(mock_read, 0u, &info))
        return 1;
    memset(ram, 0xCC, sizeof(ram));

    if (!K6APP_Load(mock_read, &info, ram, sizeof(ram)))
        return 2;
    if (memcmp(ram, g_code, sizeof(g_code)) != 0)
        return 3;
    for (uint32_t i = 0u; i < TEST_BSS_SIZE; ++i)
        if (ram[TEST_CODE_SIZE + i] != 0u)
            return 4;

    memset(resource, 0, sizeof(resource));
    if (!K6APP_ReadAsset(mock_read, &info, 57u,
                         resource, sizeof(resource)))
        return 5;
    if (memcmp(resource, g_assets + 57u, sizeof(resource)) != 0)
        return 6;

    if (K6APP_ReadAsset(mock_read, &info, TEST_ASSET_SIZE - 2u,
                        resource, 3u))
        return 7;

    return 0;
}

static int test_install_page_safe(void)
{
    uint8_t package[TEST_PACKAGE_SIZE];
    K6_AppSlotInfo info;

    make_package(package);
    g_write_count = 0u;
    g_fail_write_at = 0u;

    if (!K6APP_Install(mock_read, mock_write, 7u,
                       package, sizeof(package)))
        return 1;
    if (!K6APP_VerifySlot(mock_read, 7u, &info))
        return 2;
    if (info.slot_index != 7u ||
        info.header.image_size != TEST_CODE_SIZE ||
        info.header.asset_size != TEST_ASSET_SIZE)
        return 3;
    if (memcmp(&g_eeprom[info.slot_address + K6_APP_HEADER_SIZE],
               g_code, sizeof(g_code)) != 0)
        return 4;
    return 0;
}

static int test_install_power_loss_stays_uncommitted(void)
{
    uint8_t package[TEST_PACKAGE_SIZE];
    uint32_t base = K6_APP_EEPROM_BASE + 6u * K6_APP_SLOT_SIZE;
    K6_AppSlotInfo info;
    K6_AppHeader header;

    make_package(package);
    g_write_count = 0u;
    g_fail_write_at = 3u; /* header invalidation, then body page, then fail */

    if (K6APP_Install(mock_read, mock_write, 6u,
                      package, sizeof(package)))
        return 1;

    memcpy(&header, &g_eeprom[base], sizeof(header));
    if ((header.flags & K6_APP_FLAG_COMMITTED) != 0u)
        return 2;
    if (K6APP_ReadSlotInfo(mock_read, 6u, &info))
        return 3;

    g_fail_write_at = 0u;
    return 0;
}

static int test_install_refuses_occupied_slot(void)
{
    uint8_t package[TEST_PACKAGE_SIZE];
    uint32_t base = K6_APP_EEPROM_BASE + 5u * K6_APP_SLOT_SIZE;
    uint8_t original_header[K6_APP_HEADER_SIZE];

    make_image(5u, K6_APP_FLAG_COMMITTED);
    memcpy(original_header, &g_eeprom[base], sizeof(original_header));
    make_package(package);
    g_write_count = 0u;
    g_fail_write_at = 0u;

    if (K6APP_Install(mock_read, mock_write, 5u,
                      package, sizeof(package)))
        return 1;
    if (memcmp(original_header, &g_eeprom[base],
               sizeof(original_header)) != 0)
        return 2;
    if (g_write_count != 0u)
        return 3;
    return 0;
}

static int test_refresh_and_entry(void)
{
    K6_AppSlotInfo cached;
    K6_AppSlotInfo fresh;
    uint8_t ram[K6_APP_RAM_SIZE];

    make_image(6u, K6_APP_FLAG_COMMITTED);
    if (!K6APP_ReadSlotInfo(mock_read, 6u, &cached))
        return 1;
    if (!K6APP_RefreshSlotInfo(mock_read, &cached, &fresh))
        return 2;
    if (fresh.slot_index != 6u ||
        fresh.slot_address != cached.slot_address)
        return 3;
    if ((uintptr_t)K6APP_GetEntry(&fresh, ram) !=
        (((uintptr_t)ram) | (uintptr_t)1u))
        return 4;

    g_eeprom[cached.slot_address + 20u] ^= 0x01u;
    if (K6APP_RefreshSlotInfo(mock_read, &cached, &fresh))
        return 5;

    return 0;
}

static int test_reject_uncommitted(void)
{
    K6_AppSlotInfo info;
    make_image(1u, 0u);
    return K6APP_ReadSlotInfo(mock_read, 1u, &info) ? 1 : 0;
}

static int test_reject_code_crc(void)
{
    K6_AppSlotInfo info;
    uint8_t ram[K6_APP_RAM_SIZE];

    make_image(2u, K6_APP_FLAG_COMMITTED);
    if (!K6APP_ReadSlotInfo(mock_read, 2u, &info))
        return 1;

    g_eeprom[info.slot_address + K6_APP_HEADER_SIZE + 3u] ^= 0x80u;
    memset(ram, 0xCC, sizeof(ram));
    if (K6APP_Load(mock_read, &info, ram, sizeof(ram)))
        return 2;

    for (uint32_t i = 0u; i < TEST_CODE_SIZE + TEST_BSS_SIZE; ++i)
        if (ram[i] != 0u)
            return 3;
    return 0;
}

static int test_reject_asset_crc(void)
{
    K6_AppSlotInfo info;
    uint8_t ram[K6_APP_RAM_SIZE];

    make_image(3u, K6_APP_FLAG_COMMITTED);
    if (!K6APP_ReadSlotInfo(mock_read, 3u, &info))
        return 1;

    g_eeprom[info.slot_address + K6_APP_HEADER_SIZE +
             TEST_CODE_SIZE + 17u] ^= 0x01u;

    if (K6APP_Load(mock_read, &info, ram, sizeof(ram)))
        return 2;
    return 0;
}

static int test_reject_wrong_vma(void)
{
    K6_AppHeader header;
    const uint32_t base = K6_APP_EEPROM_BASE + 4u * K6_APP_SLOT_SIZE;
    make_image(4u, K6_APP_FLAG_COMMITTED);
    memcpy(&header, &g_eeprom[base], sizeof(header));
    header.link_vma = 0x20001000u;
    memcpy(&g_eeprom[base], &header, sizeof(header));

    K6_AppSlotInfo info;
    return K6APP_ReadSlotInfo(mock_read, 4u, &info) ? 1 : 0;
}

static int test_reject_slot_index(void)
{
    K6_AppSlotInfo info;
    return K6APP_ReadSlotInfo(mock_read, K6_APP_SLOT_COUNT, &info) ? 1 : 0;
}

static int test_reject_ram_overflow(void)
{
    K6_AppHeader header;
    const uint32_t base = K6_APP_EEPROM_BASE + 5u * K6_APP_SLOT_SIZE;
    make_image(5u, K6_APP_FLAG_COMMITTED);
    memcpy(&header, &g_eeprom[base], sizeof(header));
    header.bss_size = K6_APP_RAM_SIZE;
    memcpy(&g_eeprom[base], &header, sizeof(header));

    K6_AppSlotInfo info;
    return K6APP_ReadSlotInfo(mock_read, 5u, &info) ? 1 : 0;
}

int main(void)
{
    int result;
    for (uint32_t i = 0u; i < sizeof(g_code); ++i)
        g_code[i] = (uint8_t)((i * 37u + 11u) & 0xFFu);
    for (uint32_t i = 0u; i < sizeof(g_assets); ++i)
        g_assets[i] = (uint8_t)((i * 13u + 7u) & 0xFFu);

    memset(g_eeprom, 0xFF, sizeof(g_eeprom));
    make_image(0u, K6_APP_FLAG_COMMITTED);

#define RUN_TEST(fn) do { \
    result = fn(); \
    if (result != 0) { \
        fprintf(stderr, "%s failed at check %d\n", #fn, result); \
        return 1; \
    } \
    printf("PASS %s\n", #fn); \
} while (0)

    RUN_TEST(test_load_and_read_asset);
    RUN_TEST(test_build_api_asset_binding);
    RUN_TEST(test_install_page_safe);
    RUN_TEST(test_install_power_loss_stays_uncommitted);
    RUN_TEST(test_install_refuses_occupied_slot);
    RUN_TEST(test_refresh_and_entry);
    RUN_TEST(test_reject_uncommitted);
    RUN_TEST(test_reject_code_crc);
    RUN_TEST(test_reject_asset_crc);
    RUN_TEST(test_reject_wrong_vma);
    RUN_TEST(test_reject_slot_index);
    RUN_TEST(test_reject_ram_overflow);

    puts("All K6 APP runtime tests passed.");
    return 0;
}
