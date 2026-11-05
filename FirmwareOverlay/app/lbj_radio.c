#include "app/lbj_radio.h"

#include "app/lbj_app.h"
#include "driver/bk4819.h"
#include "misc.h"

/*
 * BK4819 FSK packet engine limits a single RX data field to 1024 words
 * (2048 bytes). POCSAG's first preamble/sync is used to enter the frame;
 * the rest of the burst, including its periodic 0x7CD215D8 batch syncs,
 * is delivered as ordinary data for the MCU-side POCSAG decoder.
 */
#define LBJ_FSK_DATA_BYTES       2048u
#define LBJ_FSK_FIFO_WORDS       4u
#define LBJ_FSK_DATA_LENGTH_FIELD (LBJ_FSK_DATA_BYTES - 1u)
#define LBJ_FSK_SYNC_WORD        0x7CD215D8u
#define LBJ_FSK_REG58            0x00C1u /* direct FSK 1.2K RX, FSK enable */
#define LBJ_FSK_REG59            0x10F8u /* RX enable, 16-byte preamble, 4-byte sync, no scramble */
#define LBJ_FSK_REG5C            0x5625u /* LOSEHU's known CRC-disabled FSK setting */
#define LBJ_FSK_REG5D            (uint16_t)((((LBJ_FSK_DATA_LENGTH_FIELD) & 0xFFu) << 8) | (((LBJ_FSK_DATA_LENGTH_FIELD) >> 8) << 5))
#define LBJ_FSK_REG5E            (uint16_t)(0x0200u | LBJ_FSK_FIFO_WORDS) /* four RX words */

static bool gLBJRadioStarted;
static bool gLBJFrameActive;
static bool gLBJFrameInverted;
static uint16_t gLBJLastTick;
static uint32_t gLBJTimeMs;

static uint32_t current_time_ms(void)
{
    const uint16_t tick = gFlashLightBlinkCounter;
    gLBJTimeMs += (uint16_t)(tick - gLBJLastTick) * 10u;
    gLBJLastTick = tick;
    return gLBJTimeMs;
}

static void rearm_fsk_rx(void)
{
    const uint16_t base = (uint16_t)(LBJ_FSK_REG59 & ~(1u << 12));
    /* Same FIFO-clear/re-enable sequence used by LOSEHU's FSK receiver. */
    BK4819_WriteRegister(BK4819_REG_59, (uint16_t)(0xC000u | base));
    BK4819_WriteRegister(BK4819_REG_59, (uint16_t)(base | (1u << 12)));
}

void LBJ_RFStart(void)
{
    uint16_t interrupt_mask;

    gLBJRadioStarted = false;
    gLBJFrameActive = false;
    gLBJFrameInverted = false;
    gLBJLastTick = gFlashLightBlinkCounter;
    gLBJTimeMs = (uint32_t)gLBJLastTick * 10u;

    /* Preserve the radio's existing squelch/CSS/DTMF interrupt selections. */
    interrupt_mask = BK4819_ReadRegister(BK4819_REG_3F);
    interrupt_mask &= (uint16_t)~(BK4819_REG_3F_FSK_RX_SYNC |
                                  BK4819_REG_3F_FSK_RX_FINISHED |
                                  BK4819_REG_3F_FSK_FIFO_ALMOST_FULL);
    BK4819_WriteRegister(BK4819_REG_3F, interrupt_mask);
    BK4819_WriteRegister(BK4819_REG_02, 0u);

    /* Known LOSEHU direct 1200-baud FSK settings, not FFSK 1200/1800. */
    BK4819_WriteRegister(BK4819_REG_70, 0x00E0u);
    BK4819_WriteRegister(BK4819_REG_72, 0x3065u);
    BK4819_WriteRegister(BK4819_REG_58, LBJ_FSK_REG58);

    /* POCSAG sync word, byte order as specified by REG_5A/REG_5B. */
    BK4819_WriteRegister(BK4819_REG_5A, (uint16_t)(LBJ_FSK_SYNC_WORD >> 16));
    BK4819_WriteRegister(BK4819_REG_5B, (uint16_t)(LBJ_FSK_SYNC_WORD & 0xFFFFu));
    BK4819_WriteRegister(BK4819_REG_5C, LBJ_FSK_REG5C);
    BK4819_WriteRegister(BK4819_REG_5D, LBJ_FSK_REG5D);
    BK4819_WriteRegister(BK4819_REG_5E, LBJ_FSK_REG5E);

    /* Clear both FIFOs, then enable RX. Scrambler and CRC remain disabled. */
    rearm_fsk_rx();

    interrupt_mask |= BK4819_REG_3F_FSK_RX_SYNC |
                      BK4819_REG_3F_FSK_RX_FINISHED |
                      BK4819_REG_3F_FSK_FIFO_ALMOST_FULL;
    BK4819_WriteRegister(BK4819_REG_3F, interrupt_mask);
    gLBJRadioStarted = true;
}

void LBJ_RFHandleInterrupts(uint16_t interrupt_bits)
{
    uint32_t now_ms;

    if (!gLBJRadioStarted)
        return;

    now_ms = current_time_ms();

    /* The chip detects only the first sync, then streams all later batch syncs. */
    if (interrupt_bits & BK4819_REG_02_FSK_RX_SYNC) {
        const uint16_t sync_status = BK4819_ReadRegister(BK4819_REG_0B);
        gLBJFrameInverted = (sync_status & (1u << 7)) != 0u;
        gLBJFrameActive = true;
        LBJ_AppBeginRFFrame(gLBJFrameInverted, now_ms);
    }

    if (interrupt_bits & BK4819_REG_02_FSK_FIFO_ALMOST_FULL) {
        const uint16_t count = (uint16_t)(BK4819_ReadRegister(BK4819_REG_5E) & 7u);
        uint16_t i;

        /* REG_5E[2:0] is the configured FIFO threshold; four words per drain. */
        for (i = 0u; i < count; ++i) {
            const uint16_t word = BK4819_ReadRegister(BK4819_REG_5F);
            if (gLBJFrameActive)
                LBJ_AppQueueRFWord(word, gLBJFrameInverted, now_ms);
        }
    }

    if (interrupt_bits & BK4819_REG_02_FSK_RX_FINISHED) {
        gLBJFrameActive = false;
        rearm_fsk_rx();
    }
}
