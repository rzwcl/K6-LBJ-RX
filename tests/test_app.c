#include "lbj_app.h"
#include "lbj_pocsag.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

extern volatile uint16_t gFlashLightBlinkCounter;
extern bool gUpdateDisplay;


static unsigned popcount32(uint32_t v)
{
    unsigned n = 0u;
    while (v) { v &= v - 1u; ++n; }
    return n;
}

static uint32_t test_syndrome(uint32_t cw)
{
    uint32_t r = cw >> 1;
    int i;
    for (i = 30; i >= 10; --i)
        if (r & (1u << (unsigned)i)) r ^= 0x769u << (unsigned)(i - 10);
    return r;
}

static uint32_t encode_data(uint32_t data)
{
    uint32_t cw = data << 11;
    cw |= test_syndrome(cw) << 1;
    if ((popcount32(cw) & 1u) != 0u) cw ^= 1u;
    return cw;
}

static uint32_t numeric_word(const char *text, unsigned offset)
{
    uint32_t data = 1u << 20;
    unsigned i, bit;
    for (i = 0u; i < 5u; ++i) {
        const char c = text[offset + i];
        const unsigned value = (c >= '0' && c <= '9') ? (unsigned)(c - '0') : 12u;
        for (bit = 0u; bit < 4u; ++bit)
            if (value & (1u << bit))
                data |= 1u << (19u - (i * 4u + bit));
    }
    return encode_data(data);
}

static void queue_rf_codeword(uint32_t cw, bool inverted, uint32_t *now_ms)
{
    const uint16_t first = (uint16_t)(((cw >> 24) & 0xFFu) | (((cw >> 16) & 0xFFu) << 8));
    const uint16_t second = (uint16_t)(((cw >> 8) & 0xFFu) | ((cw & 0xFFu) << 8));
    LBJ_AppQueueRFWord(inverted ? (uint16_t)~first : first, inverted, (*now_ms)++);
    LBJ_AppProcess();
    LBJ_AppQueueRFWord(inverted ? (uint16_t)~second : second, inverted, (*now_ms)++);
    LBJ_AppProcess();
}

static void test_rf_byte_path(bool inverted)
{
    static const char text[] = "1234000 206 81 2018";
    const uint32_t address = encode_data((((1234000u >> 3) & 0x3FFFFu) << 2));
    uint32_t now_ms = 1000u;
    unsigned offset;

    LBJ_AppBeginRFFrame(inverted, now_ms++); /* Inject the hardware-stripped sync word. */
    LBJ_AppProcess();
    queue_rf_codeword(address, inverted, &now_ms);
    for (offset = 0u; offset < strlen(text); offset += 5u)
        queue_rf_codeword(numeric_word(text, offset), inverted, &now_ms);
    queue_rf_codeword(LBJ_POCSAG_IDLE, inverted, &now_ms);

    assert(LBJ_AppGetEngine()->history_count > 0u);
    assert(LBJ_AppGetEngine()->decoded_1234000 > 0u);
    assert(strcmp(LBJ_AppGetEngine()->history[0].train_no, "206") == 0);
    assert(strcmp(LBJ_AppGetEngine()->history[0].speed, "81") == 0);
    assert(strcmp(LBJ_AppGetEngine()->history[0].km, "201.8") == 0);
}

static void test_boot_splash_timeout_across_tick_wrap(void)
{
    gFlashLightBlinkCounter = 65530u;
    gUpdateDisplay = false;

    LBJ_AppInit();
    assert(LBJ_AppBootSplashActive()); /* First render arms the timer. */

    gFlashLightBlinkCounter = (uint16_t)(gFlashLightBlinkCounter + 149u);
    LBJ_AppProcess();
    assert(LBJ_AppBootSplashActive());

    gFlashLightBlinkCounter = (uint16_t)(gFlashLightBlinkCounter + 1u);
    LBJ_AppProcess();
    assert(!LBJ_AppBootSplashActive());
    assert(gUpdateDisplay); /* Expiry schedules the main page redraw. */
}

int main(void)
{
    test_boot_splash_timeout_across_tick_wrap();
    LBJ_AppInit();
    test_rf_byte_path(false);
    LBJ_AppInit();
    test_rf_byte_path(true);
    puts("app tests: PASS");
    return 0;
}
