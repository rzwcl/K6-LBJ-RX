#include "tdcs_app.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define TEST_PACKET_BYTES TDCS_RADIO_PACKET_BYTES
#define TEST_FRAME_BITS (23u * 26u)

typedef struct {
    uint16_t regs[128];
    uint8_t fifo[TEST_PACKET_BYTES];
    size_t fifo_offset;
} MockRadio;

static MockRadio gMock;

static void put_le16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void put_le24(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
}

static void make_payload(uint8_t payload[TDCS_DATA_SIZE], uint32_t train_number)
{
    memset(payload, 0, TDCS_DATA_SIZE);
    payload[0] = 3u;
    payload[1] = 3u;
    payload[2] = 1u;
    memcpy(payload + 3, "T   ", 4u);
    put_le24(payload + 7, train_number);
    put_le16(payload + 10, 242u);
    put_le16(payload + 12, 521u);
    put_le24(payload + 14, 1161011u);
    put_le24(payload + 17, 124u);
    put_le16(payload + 20, 800u);
    put_le16(payload + 22, 410u);
    payload[24] = 18u;
    put_le16(payload + 25, 130u);
    payload[27] = 4u;
    payload[28] = 5u;
    put_le24(payload + 29, 123456u);
}

static uint16_t test_remainder(uint32_t value)
{
    int bit;

    for (bit = 25; bit >= 10; --bit) {
        if ((value & (1u << (unsigned int)bit)) != 0u) {
            value ^= 0x05B9u << ((unsigned int)bit - 10u);
        }
    }
    return (uint16_t)(value & 0x03FFu);
}

static void set_packed_bit(
    uint8_t *bytes,
    size_t bit_position,
    bool lsb_first,
    uint8_t bit)
{
    const unsigned int bit_in_byte = (unsigned int)(bit_position & 7u);
    const unsigned int shift = lsb_first ? bit_in_byte : (7u - bit_in_byte);
    const uint8_t mask = (uint8_t)(1u << shift);

    if (bit != 0u) {
        bytes[bit_position >> 3] |= mask;
    } else {
        bytes[bit_position >> 3] &= (uint8_t)~mask;
    }
}

static void encode_packet(
    const uint8_t payload[TDCS_DATA_SIZE],
    uint8_t packet[TEST_PACKET_BYTES],
    bool lsb_first)
{
    uint8_t info[TDCS_INFO_SIZE];
    size_t word_index;

    memset(packet, 0, TEST_PACKET_BYTES);
    memset(info, 0, sizeof(info));
    info[0] = 0x0Cu;
    info[1] = 0x2Bu;
    info[2] = 0x00u;
    info[3] = 0x3Fu;
    info[4] = 0x1Fu;
    info[5] = 0x00u;
    info[6] = 0x00u;
    info[7] = 0x1Fu;
    info[8] = 0x8Cu;
    info[9] = 0x30u;
    info[10] = 0x20u;
    memcpy(&info[11], payload, TDCS_DATA_SIZE);
    {
        const uint16_t crc = TDCS_CRC16XModem(info, 43u);
        info[43] = (uint8_t)(crc >> 8);
        info[44] = (uint8_t)crc;
    }
    info[45] = 0xA5u; /* outside the 45-byte application frame */

    for (word_index = 0u; word_index < 23u; ++word_index) {
        const uint16_t data =
            (uint16_t)(((uint16_t)info[word_index * 2u] << 8) |
                       info[word_index * 2u + 1u]);
        const uint32_t shifted = (uint32_t)data << 10;
        const uint32_t codeword = shifted | test_remainder(shifted);
        unsigned int bit;

        for (bit = 0u; bit < 26u; ++bit) {
            const uint8_t value =
                (uint8_t)((codeword >> (25u - bit)) & 1u);
            set_packed_bit(packet, word_index * 26u + bit, lsb_first, value);
        }
    }
    (void)TEST_FRAME_BITS;
}

static uint16_t mock_read_register(uint8_t reg)
{
    if (reg == 0x5Fu) {
        uint16_t value = 0u;
        if (gMock.fifo_offset + 1u < sizeof(gMock.fifo)) {
            value = (uint16_t)gMock.fifo[gMock.fifo_offset] |
                    ((uint16_t)gMock.fifo[gMock.fifo_offset + 1u] << 8);
        }
        gMock.fifo_offset += 2u;
        return value;
    }
    return gMock.regs[reg];
}

static void mock_write_register(uint8_t reg, uint16_t value)
{
    gMock.regs[reg] = value;
}

static void feed_packet(
    const uint8_t packet[TEST_PACKET_BYTES],
    bool inverted)
{
    size_t i;

    memcpy(gMock.fifo, packet, TEST_PACKET_BYTES);
    if (inverted) {
        for (i = 0u; i < TEST_PACKET_BYTES; ++i) {
            gMock.fifo[i] ^= 0xFFu;
        }
        gMock.regs[0x0Bu] = (1u << 7);
    } else {
        gMock.regs[0x0Bu] = 0u;
    }
    gMock.fifo_offset = 0u;

    TDCS_RadioHandleInterrupt(TDCS_RADIO_IRQ_FSK_SYNC);
    for (i = 0u; i < TEST_PACKET_BYTES / 2u; ++i) {
        TDCS_RadioHandleInterrupt(TDCS_RADIO_IRQ_FIFO_ALMOST_FULL);
    }
    TDCS_RadioHandleInterrupt(TDCS_RADIO_IRQ_RX_FINISHED);
}

static bool init_app(void)
{
    TDCS_RadioOps ops = {
        mock_read_register,
        mock_write_register,
        NULL
    };

    memset(&gMock, 0, sizeof(gMock));
    return TDCS_AppInit(&ops);
}

static void test_process_synthetic_tdcs_frame(void)
{
    uint8_t payload[TDCS_DATA_SIZE];
    uint8_t packet[TEST_PACKET_BYTES];
    const TDCS_TrainRecord *record;
    TDCS_AppStats stats;

    assert(init_app());
    make_payload(payload, 112u);
    encode_packet(payload, packet, false);
    feed_packet(packet, false);
    TDCS_AppProcess(1000u);

    assert(TDCS_AppGetHistoryCount() == 1u);
    assert(TDCS_AppGetSelectedIndex() == 0u);
    record = TDCS_AppGetSelected();
    assert(record != NULL);
    assert(record->valid == 1u);
    assert(record->received_ms == 1000u);
    assert(record->fields.train_number == 112u);
    assert(record->fields.loco_model == 242u);
    assert(record->fields.loco_number == 521u);
    assert(record->fields.mileage_m == 1161011u);
    assert(record->fields.speed_kmh == 124u);
    assert(record->fields.weight_t == 800u);
    assert(record->fields.length_tenths == 410u);
    assert(record->fields.station_code == 130u);
    assert(record->fields.driver_number == 123456u);
    assert(record->corrected_bits == 0u);
    assert(record->inverted == 0u);
    assert(record->bit_order_lsb == 0u);
    assert(memcmp(record->payload, payload, TDCS_DATA_SIZE) == 0);

    TDCS_AppGetStats(&stats);
    assert(stats.accepted_frames == 1u);
    assert(stats.rejected_frames == 0u);
    assert(stats.radio.received_frames == 1u);
}

static void test_repeat_updates_existing_record(void)
{
    uint8_t payload[TDCS_DATA_SIZE];
    uint8_t packet[TEST_PACKET_BYTES];
    const TDCS_TrainRecord *record;
    TDCS_AppStats stats;

    assert(init_app());
    make_payload(payload, 112u);
    encode_packet(payload, packet, false);
    feed_packet(packet, false);
    TDCS_AppProcess(1000u);

    feed_packet(packet, false);
    TDCS_AppProcess(2000u);

    assert(TDCS_AppGetHistoryCount() == 1u);
    record = TDCS_AppGetSelected();
    assert(record != NULL);
    assert(record->received_ms == 2000u);

    TDCS_AppGetStats(&stats);
    assert(stats.accepted_frames == 2u);
}

static void test_history_order_and_selection(void)
{
    uint8_t payload_a[TDCS_DATA_SIZE];
    uint8_t payload_b[TDCS_DATA_SIZE];
    uint8_t packet_a[TEST_PACKET_BYTES];
    uint8_t packet_b[TEST_PACKET_BYTES];
    const TDCS_TrainRecord *record;

    assert(init_app());
    make_payload(payload_a, 112u);
    make_payload(payload_b, 113u);
    encode_packet(payload_a, packet_a, false);
    encode_packet(payload_b, packet_b, true);

    feed_packet(packet_a, false);
    TDCS_AppProcess(1000u);
    feed_packet(packet_b, true);
    TDCS_AppProcess(1500u);

    assert(TDCS_AppGetHistoryCount() == 2u);
    record = TDCS_AppGetRecord(0u);
    assert(record != NULL);
    assert(record->fields.train_number == 113u);
    assert(record->inverted == 1u);
    assert(record->bit_order_lsb == 1u);

    record = TDCS_AppGetRecord(1u);
    assert(record != NULL);
    assert(record->fields.train_number == 112u);

    TDCS_AppMoveSelection(1);
    assert(TDCS_AppGetSelected()->fields.train_number == 112u);
    TDCS_AppMoveSelection(-1);
    assert(TDCS_AppGetSelected()->fields.train_number == 113u);

    TDCS_AppToggleDetail();
    assert(TDCS_AppGetDetailPage() == 1u);
}

static void test_reject_corrupted_frame(void)
{
    uint8_t payload[TDCS_DATA_SIZE];
    uint8_t packet[TEST_PACKET_BYTES];
    TDCS_AppStats stats;

    assert(init_app());
    make_payload(payload, 112u);
    encode_packet(payload, packet, false);
    packet[0] ^= 0x07u;

    feed_packet(packet, false);
    TDCS_AppProcess(1000u);

    assert(TDCS_AppGetHistoryCount() == 0u);
    TDCS_AppGetStats(&stats);
    assert(stats.accepted_frames == 0u);
    assert(stats.rejected_frames == 1u);
}

int main(void)
{
    static const struct {
        const char *name;
        void (*run)(void);
    } tests[] = {
        {"process_synthetic_tdcs_frame", test_process_synthetic_tdcs_frame},
        {"repeat_updates_existing_record", test_repeat_updates_existing_record},
        {"history_order_and_selection", test_history_order_and_selection},
        {"reject_corrupted_frame", test_reject_corrupted_frame}
    };
    size_t i;

    for (i = 0u; i < sizeof(tests) / sizeof(tests[0]); ++i) {
        tests[i].run();
        printf("PASS %s\n", tests[i].name);
    }

    printf("PASS %u TDCS application tests\n",
           (unsigned int)(sizeof(tests) / sizeof(tests[0])));
    return 0;
}
