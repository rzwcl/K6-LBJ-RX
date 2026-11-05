#include "tdcs_k6_adapter.h"

#include "app/tdcs_app.h"
#include "driver/bk4819.h"
#include "misc.h"

static uint16_t gLastTick;
static uint32_t gElapsedMs;
static uint8_t gStarted;

static uint16_t read_register(uint8_t reg)
{
    return BK4819_ReadRegister(reg);
}

static void write_register(uint8_t reg, uint16_t value)
{
    BK4819_WriteRegister(reg, value);
}

static void rx_turn_on(void)
{
    BK4819_RX_TurnOn();
}

void TDCS_K6_Start(void)
{
    static const TDCS_RadioOps ops = {
        read_register,
        write_register,
        rx_turn_on
    };

    gLastTick = gFlashLightBlinkCounter;
    gElapsedMs = 0u;
    gStarted = (uint8_t)TDCS_AppInit(&ops);
}

void TDCS_K6_Reapply(void)
{
    if (gStarted != 0u) {
        (void)TDCS_RadioReconfigure();
    }
}

void TDCS_K6_HandleInterrupts(uint16_t interrupt_bits)
{
    TDCS_RadioHandleInterrupt(interrupt_bits);
}

void TDCS_K6_Process(void)
{
    const uint16_t now = gFlashLightBlinkCounter;

    gElapsedMs += (uint16_t)(now - gLastTick) * 10u;
    gLastTick = now;

    TDCS_AppProcess(gElapsedMs);
}
