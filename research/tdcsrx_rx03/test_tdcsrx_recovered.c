#include "tdcsrx_recovered_protocol.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

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

static void make_frame(uint8_t frame[TDCSRX_FRAME_SIZE], int ff_header)
{
    uint8_t *payload;
    uint16_t crc;
    memset(frame, 0, TDCSRX_FRAME_SIZE);
    frame[0] = 0x0c;
    frame[1] = 0x2b;
    if (ff_header) {
        frame[7] = 0xff;
        frame[8] = 0xff;
        frame[9] = 0xff;
    } else {
        frame[7] = 0x1f;
        frame[8] = 0x8c;
        frame[9] = 0x30;
    }
    frame[10] = TDCSRX_PAYLOAD_SIZE;
    payload = frame + 11;

    payload[0] = 0xf2;
    payload[1] = 0xfd;
    payload[2] = 0xd5;
    memcpy(payload + 3, "T123", 4);
    put_le24(payload + 7, 7654321u);
    put_le16(payload + 10, 2390u);
    put_le16(payload + 12, 1234u);
    put_le24(payload + 14, 0x00c12345u);
    put_le24(payload + 17, 0x00a5fc7cu);
    put_le16(payload + 20, 800u);
    put_le16(payload + 22, 412u);
    payload[24] = 17u;
    put_le16(payload + 25, 321u);
    payload[27] = 4u;
    payload[28] = 5u;
    put_le24(payload + 29, 654321u);

    crc = tdcsrx_crc16_xmodem(frame, 43u);
    frame[43] = (uint8_t)(crc >> 8);
    frame[44] = (uint8_t)crc;
}

static void test_crc(void)
{
    static const uint8_t check[] = "123456789";
    assert(tdcsrx_crc16_xmodem(check, 9u) == 0x31c3u);
    assert(tdcsrx_crc16_xmodem(NULL, 0u) == 0u);
    assert(tdcsrx_crc16_xmodem(NULL, 1u) == 0u);
}

static void test_parse_variant_1f(void)
{
    uint8_t frame[TDCSRX_FRAME_SIZE];
    tdcsrx_message_t message;
    make_frame(frame, 0);

    assert(tdcsrx_parse_frame(frame, sizeof(frame), &message));
    assert(message.signal_machine == 2u);
    assert(message.signal_kind == 5u);
    assert(message.locomotive_role == 1u);
    assert(memcmp(message.train_prefix, "T123", 4u) == 0);
    assert(message.train_number == 7654321u);
    assert(message.locomotive_type == 2390u);
    assert(message.locomotive_serial == 1234u);
    assert(message.mileage_raw24 == 0x00c12345u);
    assert(message.mileage_value22 == 0x0012345u);
    assert(message.mileage_negative);
    assert(message.mileage_direction);
    assert(message.mileage_present);
    assert(message.speed_kmh == 124u);
    assert(message.weight == 800u);
    assert(message.calculated_length == 412u);
    assert(message.car_count == 17u);
    assert(message.station_number == 321u);
    assert(message.track_number == 4u);
    assert(message.actual_track == 5u);
    assert(message.driver_number == 654321u);
    assert(memcmp(message.raw_frame, frame, sizeof(frame)) == 0);
    assert(memcmp(message.raw_payload, frame + 11, TDCSRX_PAYLOAD_SIZE) == 0);
    assert(message.received_crc == message.calculated_crc);
}

static void test_parse_variant_ff(void)
{
    uint8_t frame[TDCSRX_FRAME_SIZE];
    tdcsrx_message_t message;
    make_frame(frame, 1);
    assert(tdcsrx_parse_frame(frame, sizeof(frame), &message));
}

static void test_rejections(void)
{
    uint8_t frame[TDCSRX_FRAME_SIZE];
    tdcsrx_message_t message;
    make_frame(frame, 0);
    frame[44] ^= 1u;
    assert(!tdcsrx_parse_frame(frame, sizeof(frame), &message));

    make_frame(frame, 0);
    frame[7] = 0;
    assert(!tdcsrx_parse_frame(frame, sizeof(frame), &message));

    make_frame(frame, 0);
    frame[10] = 40u;
    assert(!tdcsrx_parse_frame(frame, sizeof(frame), &message));

    make_frame(frame, 0);
    assert(!tdcsrx_parse_frame(frame, sizeof(frame) - 1u, &message));
    assert(!tdcsrx_parse_frame(NULL, sizeof(frame), &message));
    assert(!tdcsrx_parse_frame(frame, sizeof(frame), NULL));
}

static void test_bch_syndrome_columns(void)
{
    uint16_t original[26];
    uint16_t reference[26];
    unsigned int unique_original = 0u;
    unsigned int unique_reference = 0u;
    unsigned int i;

    for (i = 0u; i < 26u; ++i) {
        unsigned int j;
        original[i] = tdcsrx_original_syndrome(1u << i);
        reference[i] = tdcsrx_reference_syndrome_5b9(1u << i);

        for (j = 0u; j < i; ++j) {
            if (original[j] == original[i]) break;
        }
        if (j == i) ++unique_original;

        for (j = 0u; j < i; ++j) {
            if (reference[j] == reference[i]) break;
        }
        if (j == i) ++unique_reference;
    }

    assert(unique_original == 16u);
    assert(unique_reference == 26u);

    {
        int corrected_bit = 0;
        assert(tdcsrx_original_bch_decode(0u, &corrected_bit) == 0u);
        assert(corrected_bit == -1);
    }

    printf("BCH syndrome columns: target literal 0xB905 = %u/26 unique; "
           "reference 0x05B9 = %u/26 unique\n",
           unique_original, unique_reference);
}

int main(void)
{
    test_crc();
    test_parse_variant_1f();
    test_parse_variant_ff();
    test_rejections();
    test_bch_syndrome_columns();
    puts("CRC-16/XMODEM: PASS");
    puts("45-byte target-frame parser, both header variants, all fields: PASS");
    puts("CRC/header/length/null rejection: PASS");
    return 0;
}
