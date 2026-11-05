#include "tdcs_radio.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define TEST_ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    uint16_t regs[128];
    uint8_t fifo[TDCS_RADIO_PACKET_BYTES];
    size_t fifo_offset;
    unsigned int rx_on_calls;
    unsigned int writes;
} MockRadio;

static MockRadio gMock;

static uint16_t mock_read_register(uint8_t reg)
{
    if (reg == 0x5Fu) {
        uint16_t word = 0u;
        if (gMock.fifo_offset + 1u < sizeof(gMock.fifo)) {
            word = (uint16_t)gMock.fifo[gMock.fifo_offset] |
                   ((uint16_t)gMock.fifo[gMock.fifo_offset + 1u] << 8);
        }
        gMock.fifo_offset += 2u;
        return word;
    }
    return gMock.regs[reg];
}

static void mock_write_register(uint8_t reg, uint16_t value)
{
    gMock.regs[reg] = value;
    ++gMock.writes;
}

static void mock_rx_turn_on(void)
{
    ++gMock.rx_on_calls;
}

static const TDCS_RadioOps kMockOps = {
    mock_read_register,
    mock_write_register,
    mock_rx_turn_on
};

static void make_packet(uint8_t packet[TDCS_RADIO_PACKET_BYTES], uint8_t seed)
{
    size_t i;
    for (i = 0u; i < TDCS_RADIO_PACKET_BYTES; ++i) {
        packet[i] = (uint8_t)(seed + (uint8_t)(i * 13u));
    }
}

static void feed_packet(
    const uint8_t packet[TDCS_RADIO_PACKET_BYTES],
    bool inverted)
{
    size_t i;

    memcpy(gMock.fifo, packet, TDCS_RADIO_PACKET_BYTES);
    if (inverted) {
        for (i = 0u; i < TDCS_RADIO_PACKET_BYTES; ++i) {
            gMock.fifo[i] ^= 0xFFu;
        }
        gMock.regs[0x0Bu] = (1u << 7);
    } else {
        gMock.regs[0x0Bu] = 0u;
    }
    gMock.fifo_offset = 0u;

    TDCS_RadioHandleInterrupt(TDCS_RADIO_IRQ_FSK_SYNC);
    for (i = 0u; i < TDCS_RADIO_PACKET_BYTES / 2u; ++i) {
        TDCS_RadioHandleInterrupt(TDCS_RADIO_IRQ_FIFO_ALMOST_FULL);
    }
    TDCS_RadioHandleInterrupt(TDCS_RADIO_IRQ_RX_FINISHED);
}

static void test_initialization_programs_ffsk_registers(void)
{
    memset(&gMock, 0, sizeof(gMock));
    gMock.regs[0x5Eu] = 0x0201u;

    assert(TDCS_RadioInit(&kMockOps));
    assert(gMock.rx_on_calls == 1u);
    assert(gMock.regs[0x58u] == 0x1C03u);
    assert(gMock.regs[0x72u] == 0x3065u);
    assert(gMock.regs[0x70u] == 0x00E0u);
    assert(gMock.regs[0x5Au] == 0x0DD4u);
    assert(gMock.regs[0x5Bu] == 0x259Fu);
    assert(gMock.regs[0x5Cu] == 0x5625u);
    assert(gMock.regs[0x5Du] == 0x4C00u);
    assert(gMock.regs[0x5Eu] == 0x0201u);
    assert(gMock.regs[0x59u] == 0x1068u);
    assert(gMock.regs[0x3Fu] ==
           (TDCS_RADIO_IRQ_FSK_SYNC |
            TDCS_RADIO_IRQ_FIFO_ALMOST_FULL |
            TDCS_RADIO_IRQ_RX_FINISHED));
}

static void test_capture_full_packet(void)
{
    uint8_t source[TDCS_RADIO_PACKET_BYTES];
    uint8_t output[TDCS_RADIO_PACKET_BYTES];
    bool inverted = true;
    TDCS_RadioStats stats;

    make_packet(source, 0x17u);
    feed_packet(source, false);

    assert(TDCS_RadioPopFrame(output, &inverted));
    assert(!inverted);
    assert(memcmp(output, source, sizeof(source)) == 0);
    assert(!TDCS_RadioPopFrame(output, &inverted));

    TDCS_RadioGetStats(&stats);
    assert(stats.received_frames == 1u);
    assert(stats.dropped_frames == 0u);
    assert(stats.incomplete_frames == 0u);
    assert(stats.fifo_overflows == 0u);
}

static void test_inverted_packet(void)
{
    uint8_t source[TDCS_RADIO_PACKET_BYTES];
    uint8_t output[TDCS_RADIO_PACKET_BYTES];
    bool inverted = false;

    make_packet(source, 0xA3u);
    feed_packet(source, true);

    assert(TDCS_RadioPopFrame(output, &inverted));
    assert(inverted);
    assert(memcmp(output, source, sizeof(source)) == 0);
}

static void test_queue_overflow_drops_newest(void)
{
    uint8_t source[TDCS_RADIO_PACKET_BYTES];
    uint8_t output[TDCS_RADIO_PACKET_BYTES];
    bool inverted;
    TDCS_RadioStats stats;

    make_packet(source, 0x50u);
    feed_packet(source, false);
    feed_packet(source, false);
    feed_packet(source, false);

    TDCS_RadioGetStats(&stats);
    assert(stats.received_frames == 2u);
    assert(stats.dropped_frames == 1u);

    assert(TDCS_RadioPopFrame(output, &inverted));
    assert(memcmp(output, source, sizeof(source)) == 0);
    assert(TDCS_RadioPopFrame(output, &inverted));
    assert(memcmp(output, source, sizeof(source)) == 0);
    assert(!TDCS_RadioPopFrame(output, &inverted));
}

static void test_invalid_callbacks_rejected(void)
{
    TDCS_RadioOps invalid = {0};
    assert(!TDCS_RadioInit(&invalid));
}

int main(void)
{
    static const struct {
        const char *name;
        void (*run)(void);
    } tests[] = {
        {"initialization_programs_ffsk_registers", test_initialization_programs_ffsk_registers},
        {"capture_full_packet", test_capture_full_packet},
        {"inverted_packet", test_inverted_packet},
        {"queue_overflow_drops_newest", test_queue_overflow_drops_newest},
        {"invalid_callbacks_rejected", test_invalid_callbacks_rejected}
    };
    size_t i;

    for (i = 0u; i < TEST_ARRAY_COUNT(tests); ++i) {
        tests[i].run();
        printf("PASS %s\n", tests[i].name);
    }

    printf("PASS %u TDCS radio-adapter tests\n",
           (unsigned int)TEST_ARRAY_COUNT(tests));
    return 0;
}
