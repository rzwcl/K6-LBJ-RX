#include "k6_app_api.h"

/* Initialized data and BSS deliberately exercise both loader paths. */
static volatile uint32_t s_app_cookie = 0x4B364150u;
static uint32_t s_key_count;

__attribute__((section(".text.k6_app_entry"), used))
void k6_app_main(const K6_AppAPI *api)
{
    uint32_t started;
    char asset_text[13];

    if (api == 0 ||
        api->abi_major != K6_APP_ABI_MAJOR ||
        api->api_level < K6_APP_API_LEVEL ||
        api->struct_size < sizeof(K6_AppAPI)) {
        return;
    }

    asset_text[12] = '\0';

    api->backlight_on();
    api->display_clear();
    api->print_line("K6 APP PROBE", 1u);
    api->print_line("SRAM OVERLAY OK", 3u);
    if (api->asset_read != 0 && api->asset_read(0u, asset_text, 12u)) {
        asset_text[12] = '\0';
        api->print_line(asset_text, 4u);
    }
    api->print_line("EXIT TO RETURN", 5u);
    api->display_refresh();

    started = api->millis();
    s_key_count = 0u;

    while ((uint32_t)(api->millis() - started) < 15000u) {
        const uint8_t key = api->key_get_event();

        if (key == K6_APP_KEY_EXIT)
            break;

        if (key != K6_APP_KEY_INVALID)
            ++s_key_count;

        /* A loaded data symbol must retain its initialized value. */
        if (s_app_cookie != 0x4B364150u)
            break;

        api->delay_ms(10u);
    }

    api->display_clear();
    api->print_line("APP RETURN", 2u);
    api->display_refresh();
    api->delay_ms(250u);
}
