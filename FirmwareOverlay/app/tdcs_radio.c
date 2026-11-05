#include "tdcs_radio.h"

#include <string.h>

#define BK_REG_02 0x02u
#define BK_REG_0B 0x0Bu
#define BK_REG_3F 0x3Fu
#define BK_REG_58 0x58u
#define BK_REG_59 0x59u
#define BK_REG_5A 0x5Au
#define BK_REG_5B 0x5Bu
#define BK_REG_5C 0x5Cu
#define BK_REG_5D 0x5Du
#define BK_REG_5E 0x5Eu
#define BK_REG_5F 0x5Fu
#define BK_REG_70 0x70u
#define BK_REG_72 0x72u

#define TDCS_FFSK_MODE_1200_1800_RX 0x1C03u
#define TDCS_FFSK_SYNC_WORD_0_1    0x0DD4u
#define TDCS_FFSK_SYNC_WORD_2_3    0x259Fu
#define TDCS_FFSK_PREAMBLE_7_SYNC4  0x0068u
#define TDCS_FFSK_RX_ENABLE         0x1068u
#define TDCS_FFSK_CLEAR_FIFO        0xC000u
#define TDCS_FFSK_PACKET_BYTES      TDCS_RADIO_PACKET_BYTES
#define TDCS_FFSK_PACKET_LENGTH_REG ((TDCS_FFSK_PACKET_BYTES) << 8)
#define TDCS_FFSK_FIFO_THRESHOLD    ((64u << 3) | 1u)
#define TDCS_FFSK_IRQ_MASK          (TDCS_RADIO_IRQ_FSK_SYNC | \
                                     TDCS_RADIO_IRQ_FIFO_ALMOST_FULL | \
                                     TDCS_RADIO_IRQ_RX_FINISHED)

typedef struct {
    uint8_t packet[TDCS_RADIO_PACKET_BYTES];
    uint8_t inverted;
} TDCS_QueuedFrame;

static TDCS_RadioOps gOps;
static TDCS_RadioStats gStats;
static uint8_t gRxPacket[TDCS_RADIO_PACKET_BYTES];
static uint16_t gRxBytes;
static uint8_t gRxActive;
static uint8_t gRxInverted;
static TDCS_QueuedFrame gQueue[TDCS_RADIO_QUEUE_DEPTH];
static volatile uint8_t gQueueHead;
static volatile uint8_t gQueueTail;
static volatile uint8_t gQueueCount;

static bool tdcs_radio_ready(void)
{
    return gOps.read_register != NULL &&
           gOps.write_register != NULL;
}

static void tdcs_radio_write(uint8_t reg, uint16_t value)
{
    gOps.write_register(reg, value);
}

static uint16_t tdcs_radio_read(uint8_t reg)
{
    return gOps.read_register(reg);
}

static void tdcs_radio_rearm(void)
{
    tdcs_radio_write(BK_REG_59,
                     TDCS_FFSK_CLEAR_FIFO | TDCS_FFSK_PREAMBLE_7_SYNC4);
    tdcs_radio_write(BK_REG_59, TDCS_FFSK_RX_ENABLE);
    tdcs_radio_write(BK_REG_02, 0u);
}

static void tdcs_radio_append_word(void)
{
    uint16_t word;
    uint8_t low;
    uint8_t high;

    if (gRxBytes + 2u > TDCS_RADIO_PACKET_BYTES) {
        ++gStats.fifo_overflows;
        gRxActive = 0u;
        return;
    }

    word = tdcs_radio_read(BK_REG_5F);
    low = (uint8_t)word;
    high = (uint8_t)(word >> 8);

    /* LOSEHU's existing FSK FIFO consumers read the low byte first. */
    if (gRxInverted != 0u) {
        low ^= 0xFFu;
        high ^= 0xFFu;
    }

    gRxPacket[gRxBytes++] = low;
    gRxPacket[gRxBytes++] = high;
}

static void tdcs_radio_queue_completed(void)
{
    uint8_t head;

    if (gQueueCount >= TDCS_RADIO_QUEUE_DEPTH) {
        ++gStats.dropped_frames;
        return;
    }

    head = gQueueHead;
    memcpy(gQueue[head].packet, gRxPacket, TDCS_RADIO_PACKET_BYTES);
    gQueue[head].inverted = gRxInverted;
    gQueueHead = (uint8_t)((head + 1u) % TDCS_RADIO_QUEUE_DEPTH);
    ++gQueueCount;
    ++gStats.received_frames;
}

static bool tdcs_radio_configure_modem(void)
{
    uint16_t interrupt_mask;

    if (!tdcs_radio_ready()) {
        return false;
    }

    /* Retain all existing CSS/squelch/other firmware interrupt enables. */
    interrupt_mask = tdcs_radio_read(BK_REG_3F);
    interrupt_mask &= (uint16_t)~TDCS_FFSK_IRQ_MASK;
    tdcs_radio_write(BK_REG_3F, interrupt_mask);
    tdcs_radio_write(BK_REG_02, 0u);

    /* BK4819 native FFSK 1200/1800 Hz receive modem. */
    tdcs_radio_write(BK_REG_58, TDCS_FFSK_MODE_1200_1800_RX);
    tdcs_radio_write(BK_REG_72, 0x3065u); /* 1200 Hz, 26 MHz crystal */
    tdcs_radio_write(BK_REG_70, 0x00E0u); /* TONE-2 enabled, gain 96 */
    tdcs_radio_write(BK_REG_5A, TDCS_FFSK_SYNC_WORD_0_1);
    tdcs_radio_write(BK_REG_5B, TDCS_FFSK_SYNC_WORD_2_3);
    tdcs_radio_write(BK_REG_5C, 0x5625u); /* software validates frame CRC */
    tdcs_radio_write(BK_REG_5D, TDCS_FFSK_PACKET_LENGTH_REG);
    tdcs_radio_write(BK_REG_5E, TDCS_FFSK_FIFO_THRESHOLD);

    tdcs_radio_write(BK_REG_59,
                     TDCS_FFSK_CLEAR_FIFO | TDCS_FFSK_PREAMBLE_7_SYNC4);
    tdcs_radio_write(BK_REG_59, TDCS_FFSK_RX_ENABLE);
    tdcs_radio_write(BK_REG_02, 0u);
    tdcs_radio_write(BK_REG_3F,
                     (uint16_t)(interrupt_mask | TDCS_FFSK_IRQ_MASK));

    return true;
}

bool TDCS_RadioInit(const TDCS_RadioOps *ops)
{
    if (ops == NULL ||
        ops->read_register == NULL ||
        ops->write_register == NULL) {
        return false;
    }

    gOps = *ops;
    memset(&gStats, 0, sizeof(gStats));
    memset(gRxPacket, 0, sizeof(gRxPacket));
    memset(gQueue, 0, sizeof(gQueue));
    gRxBytes = 0u;
    gRxActive = 0u;
    gRxInverted = 0u;
    gQueueHead = 0u;
    gQueueTail = 0u;
    gQueueCount = 0u;

    if (gOps.rx_turn_on != NULL) {
        gOps.rx_turn_on();
    }
    return tdcs_radio_configure_modem();
}

bool TDCS_RadioReconfigure(void)
{
    return tdcs_radio_configure_modem();
}

void TDCS_RadioHandleInterrupt(uint16_t interrupt_bits)
{
    if (!tdcs_radio_ready()) {
        return;
    }

    if ((interrupt_bits & TDCS_RADIO_IRQ_FSK_SYNC) != 0u) {
        if (gRxActive != 0u) {
            ++gStats.incomplete_frames;
        }
        gRxBytes = 0u;
        gRxInverted =
            (uint8_t)((tdcs_radio_read(BK_REG_0B) & (1u << 7)) != 0u);
        gRxActive = 1u;
    }

    if ((interrupt_bits & TDCS_RADIO_IRQ_FIFO_ALMOST_FULL) != 0u &&
        gRxActive != 0u) {
        uint16_t words = (uint16_t)(tdcs_radio_read(BK_REG_5E) & 7u);

        /* The modem threshold is programmed to one 16-bit FIFO word. */
        if (words == 0u) {
            words = 1u;
        }

        while (words-- != 0u && gRxActive != 0u) {
            tdcs_radio_append_word();
        }
    }

    if ((interrupt_bits & TDCS_RADIO_IRQ_RX_FINISHED) != 0u) {
        if (gRxActive != 0u) {
            while (gRxBytes < TDCS_RADIO_PACKET_BYTES) {
                tdcs_radio_append_word();
                if (gRxActive == 0u) {
                    break;
                }
            }

            if (gRxActive != 0u &&
                gRxBytes == TDCS_RADIO_PACKET_BYTES) {
                tdcs_radio_queue_completed();
            } else {
                ++gStats.incomplete_frames;
            }
        }

        gRxActive = 0u;
        gRxBytes = 0u;
        tdcs_radio_rearm();
    }
}

bool TDCS_RadioPopFrame(
    uint8_t packet[TDCS_RADIO_PACKET_BYTES],
    bool *inverted)
{
    uint8_t tail;

    if (packet == NULL || inverted == NULL || gQueueCount == 0u) {
        return false;
    }

    tail = gQueueTail;
    memcpy(packet, gQueue[tail].packet, TDCS_RADIO_PACKET_BYTES);
    *inverted = (gQueue[tail].inverted != 0u);
    gQueueTail = (uint8_t)((tail + 1u) % TDCS_RADIO_QUEUE_DEPTH);
    --gQueueCount;
    return true;
}

void TDCS_RadioGetStats(TDCS_RadioStats *stats)
{
    if (stats != NULL) {
        *stats = gStats;
    }
}
