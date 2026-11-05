#include "lbj_app.h"
#include "lbj_core.h"
#include "lbj_pocsag.h"
#include "ui/ui.h"
#include "misc.h"

#define LBJ_BIT_QUEUE_SIZE 128u
#define LBJ_BIT_QUEUE_MASK (LBJ_BIT_QUEUE_SIZE - 1u)

typedef struct {
    uint8_t bit;
    uint8_t pad[3];
    uint32_t now_ms;
} LBJ_QueuedBit;

static LBJ_Engine gLBJEngine;
static LBJ_PocsagDecoder gLBJDecoder;
static LBJ_QueuedBit gBitQueue[LBJ_BIT_QUEUE_SIZE];
static volatile uint8_t gBitHead;
static volatile uint8_t gBitTail;
static volatile uint32_t gBitQueueDrops;
static uint8_t gBootSplashActive;
static uint8_t gBootSplashArmed;
static uint16_t gBootSplashRemaining;
static uint16_t gBootSplashLastTick;

void LBJ_AppInit(void)
{
    LBJ_EngineInit(&gLBJEngine);
    LBJ_PocsagInit(&gLBJDecoder, LBJ_EngineOnPage, &gLBJEngine);
    gBitHead = 0u;
    gBitTail = 0u;
    gBitQueueDrops = 0u;
    gBootSplashActive = 1u;
    gBootSplashArmed = 0u;
    gBootSplashRemaining = 150u; /* 1.5 seconds after the LBJ screen first appears. */
    gBootSplashLastTick = 0u;
}

bool LBJ_AppBootSplashActive(void)
{
    if (!gBootSplashActive)
        return false;
    if (!gBootSplashArmed) {
        gBootSplashLastTick = gFlashLightBlinkCounter;
        gBootSplashArmed = 1u;
    }
    return true;
}

static void update_boot_splash(void)
{
    uint16_t now;
    uint16_t elapsed;
    if (!gBootSplashActive || !gBootSplashArmed)
        return;
    now = gFlashLightBlinkCounter;
    elapsed = (uint16_t)(now - gBootSplashLastTick);
    if (elapsed == 0u)
        return;
    gBootSplashLastTick = now;
    if (elapsed >= gBootSplashRemaining) {
        gBootSplashRemaining = 0u;
        gBootSplashActive = 0u;
        gUpdateDisplay = true;
    } else {
        gBootSplashRemaining = (uint16_t)(gBootSplashRemaining - elapsed);
    }
}

void LBJ_AppProcess(void)
{
    update_boot_splash();
    unsigned budget = 64u;
    uint32_t before_000 = gLBJEngine.decoded_1234000;
    uint32_t before_002 = gLBJEngine.decoded_1234002;

    /* The interrupt-side producer only queues bits. BCH and page parsing run here. */
    while (budget-- && gBitTail != gBitHead) {
        LBJ_QueuedBit item = gBitQueue[gBitTail];
        gBitTail = (uint8_t)((gBitTail + 1u) & LBJ_BIT_QUEUE_MASK);
        LBJ_PocsagFeedBit(&gLBJDecoder, item.bit, item.now_ms);
    }

    if (before_000 != gLBJEngine.decoded_1234000 ||
        before_002 != gLBJEngine.decoded_1234002)
        gUpdateDisplay = true;
}

void LBJ_AppBeginRFFrame(bool inverted, uint32_t now_ms)
{
    unsigned bit;
    (void)inverted;

    /* Hardware strips the first 32-bit sync word; put it back for the POCSAG parser. */
    gBitHead = 0u;
    gBitTail = 0u;
    LBJ_PocsagReset(&gLBJDecoder);
    for (bit = 32u; bit > 0u; --bit)
        LBJ_AppQueueBit((uint8_t)((LBJ_POCSAG_SYNC >> (bit - 1u)) & 1u), now_ms);
}

void LBJ_AppQueueRFWord(uint16_t word, bool inverted, uint32_t now_ms)
{
    uint8_t first = (uint8_t)(word & 0xFFu);
    uint8_t second = (uint8_t)(word >> 8);
    int bit;

    /* LOSEHU's FSK FIFO is consumed low byte first; POCSAG bits are MSB first. */
    if (inverted) {
        first ^= 0xFFu;
        second ^= 0xFFu;
    }
    for (bit = 7; bit >= 0; --bit)
        LBJ_AppQueueBit((uint8_t)((first >> (unsigned)bit) & 1u), now_ms);
    for (bit = 7; bit >= 0; --bit)
        LBJ_AppQueueBit((uint8_t)((second >> (unsigned)bit) & 1u), now_ms);
}

void LBJ_AppQueueBit(uint8_t bit, uint32_t now_ms)
{
    uint8_t head = gBitHead;
    uint8_t next = (uint8_t)((head + 1u) & LBJ_BIT_QUEUE_MASK);
    if (next == gBitTail) {
        ++gBitQueueDrops;
        return;
    }
    gBitQueue[head].bit = (uint8_t)(bit & 1u);
    gBitQueue[head].now_ms = now_ms;
    gBitHead = next;
}

void LBJ_ProcessKeys(KEY_Code_t key, bool pressed, bool held)
{
    (void)held;
    if (!pressed)
        return;

    switch (key) {
        case KEY_UP:
            LBJ_EngineMove(&gLBJEngine, -1);
            gUpdateDisplay = true;
            break;
        case KEY_DOWN:
            LBJ_EngineMove(&gLBJEngine, +1);
            gUpdateDisplay = true;
            break;
        case KEY_MENU:
            LBJ_EngineToggleDetail(&gLBJEngine);
            gUpdateDisplay = true;
            break;
        case KEY_EXIT:
            gRequestDisplayScreen = DISPLAY_MAIN;
            break;
        default:
            break;
    }
}

const LBJ_Engine *LBJ_AppGetEngine(void)
{
    return &gLBJEngine;
}
