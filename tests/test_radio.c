#include "app/lbj_radio.h"
#include "driver/bk4819.h"
#include "misc.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static uint16_t regs[128];
static uint16_t fifo_words[8];
static unsigned fifo_index;
static bool frame_started;
static bool frame_inverted;
static unsigned queued_words;
static uint16_t queued[8];
static bool queued_inverted[8];
static uint32_t frame_time;
static uint32_t word_time[8];
volatile uint16_t gFlashLightBlinkCounter;

uint16_t BK4819_ReadRegister(BK4819_REGISTER_t reg)
{
    if (reg == BK4819_REG_5F)
        return fifo_words[fifo_index++];
    return regs[(unsigned)reg];
}

void BK4819_WriteRegister(BK4819_REGISTER_t reg, uint16_t value)
{
    regs[(unsigned)reg] = value;
}

void LBJ_AppBeginRFFrame(bool inverted, uint32_t now_ms)
{
    frame_started = true;
    frame_inverted = inverted;
    frame_time = now_ms;
}

void LBJ_AppQueueRFWord(uint16_t word, bool inverted, uint32_t now_ms)
{
    assert(queued_words < 8u);
    queued[queued_words] = word;
    queued_inverted[queued_words] = inverted;
    word_time[queued_words] = now_ms;
    ++queued_words;
}

static void test_rf_configuration(void)
{
    regs[BK4819_REG_3F] = 0x000Cu; /* Preserve the base squelch interrupt bits. */
    gFlashLightBlinkCounter = 123u;
    LBJ_RFStart();

    assert(regs[BK4819_REG_70] == 0x00E0u);
    assert(regs[BK4819_REG_72] == 0x3065u);
    assert(regs[BK4819_REG_58] == 0x00C1u);
    assert(regs[BK4819_REG_5A] == 0x7CD2u);
    assert(regs[BK4819_REG_5B] == 0x15D8u);
    assert(regs[BK4819_REG_5C] == 0x5625u);
    assert(regs[BK4819_REG_5D] == 0xFFE0u);
    assert(regs[BK4819_REG_5E] == 0x0204u);
    assert(regs[BK4819_REG_59] == 0x10F8u);
    assert((regs[BK4819_REG_3F] & 0x3002u) == 0x3002u);
    assert((regs[BK4819_REG_3F] & 0x000Cu) == 0x000Cu);
}

static void test_fifo_dispatch_and_rearm(void)
{
    unsigned i;
    fifo_words[0] = 0x3412u;
    fifo_words[1] = 0x7856u;
    fifo_words[2] = 0xBC9Au;
    fifo_words[3] = 0xF0DEu;
    fifo_index = 0u;
    queued_words = 0u;
    frame_started = false;
    regs[BK4819_REG_0B] = 0x0080u; /* Negative sync found. */
    regs[BK4819_REG_5E] = 0x0204u;

    LBJ_RFHandleInterrupts((uint16_t)(BK4819_REG_02_FSK_RX_SYNC |
                                     BK4819_REG_02_FSK_FIFO_ALMOST_FULL));
    assert(frame_started);
    assert(frame_inverted);
    assert(frame_time == 1230u);
    assert(queued_words == 4u);
    for (i = 0u; i < 4u; ++i) {
        assert(queued[i] == fifo_words[i]);
        assert(queued_inverted[i]);
        assert(word_time[i] == 1230u);
    }

    LBJ_RFHandleInterrupts(BK4819_REG_02_FSK_RX_FINISHED);
    assert(regs[BK4819_REG_59] == 0x10F8u);
}

int main(void)
{
    test_rf_configuration();
    test_fifo_dispatch_and_rearm();
    puts("radio adapter tests: PASS");
    return 0;
}
