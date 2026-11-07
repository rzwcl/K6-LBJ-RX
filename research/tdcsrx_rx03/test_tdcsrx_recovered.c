#include "tdcsrx_recovered_stream.h"

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

static void refresh_frame_crc(uint8_t frame[TDCSRX_FRAME_SIZE])
{
    uint16_t crc = tdcsrx_crc16_xmodem(frame, 43u);
    frame[43] = (uint8_t)(crc >> 8);
    frame[44] = (uint8_t)crc;
}

static void test_original_40_byte_app_record_abi(void)
{
    uint8_t frame[TDCSRX_FRAME_SIZE];
    tdcsrx_app_record_t record;
    uint16_t speed;

    make_frame(frame, 0);
    memset(&record, 0xa5, sizeof(record));
    assert(tdcsrx_build_app_record(frame, sizeof(frame), &record));

    assert(memcmp(record.train_prefix, frame + 0x0e, 4u) == 0);
    assert(memcmp(record.train_number_le, frame + 0x12, 3u) == 0);
    assert(record.train_number_le[3] == 0u);

    assert(record.mileage_magnitude_le[0] == frame[0x19]);
    assert(record.mileage_magnitude_le[1] == frame[0x1a]);
    assert(record.mileage_magnitude_le[2] == (uint8_t)(frame[0x1b] & 0x3fu));
    assert(record.mileage_magnitude_le[3] == 0u);
    assert(memcmp(record.locomotive_serial_le, frame + 0x17, 2u) == 0);
    assert(memcmp(record.total_weight_le, frame + 0x1f, 2u) == 0);
    assert(memcmp(record.calculated_length_le, frame + 0x21, 2u) == 0);
    assert(record.auxiliary_12_13[0] == frame[0x28]);
    assert(record.auxiliary_12_13[1] == frame[0x29]);

    speed = (uint16_t)((uint16_t)frame[0x1c] |
                       ((uint16_t)frame[0x1d] << 8));
    speed &= 0x03ffu;
    assert(record.speed_le[0] == (uint8_t)speed);
    assert(record.speed_le[1] == (uint8_t)(speed >> 8));

    assert(memcmp(record.locomotive_type_le, frame + 0x15, 2u) == 0);
    assert(record.locomotive_role == (uint8_t)(frame[0x0d] & 0x03u));
    assert(record.mileage_present == 1u);
    assert(record.mileage_negative == (uint8_t)(frame[0x1b] >> 7));
    assert(record.mileage_direction == (uint8_t)((frame[0x1b] >> 6) & 1u));
    assert(record.car_count == frame[0x23]);
    assert(memcmp(record.station_number_le, frame + 0x24, 2u) == 0);
    assert(record.frame_26_copy == frame[0x26]);
    assert(record.frame_27_copy == frame[0x27]);
    assert(record.frame_2a_copy == frame[0x2a]);
    assert(record.signal_machine == (uint8_t)(frame[0x0b] & 0x0fu));
    assert(record.signal_kind == (uint8_t)(frame[0x0c] & 0x07u));
    assert(record.header_variant == 0u);
    assert(record.fixed_index_one == 1u);
    assert(record.reserved_26_27[0] == 0u);
    assert(record.reserved_26_27[1] == 0u);

    /* Prove the parser's two dynamic-looking copies use literal index one. */
    frame[0x27] = 0xa6u;
    frame[0x29] = 0x5cu;
    refresh_frame_crc(frame);
    assert(tdcsrx_build_app_record(frame, sizeof(frame), &record));
    assert(record.frame_27_copy == 0xa6u);
    assert(record.auxiliary_12_13[1] == 0x5cu);
    assert(record.fixed_index_one == 1u);

    make_frame(frame, 1);
    assert(tdcsrx_build_app_record(frame, sizeof(frame), &record));
    assert(record.header_variant == 1u);

    /* All-FF mileage is absent; the payload magnitude still keeps 22 bits. */
    make_frame(frame, 0);
    frame[0x19] = 0xffu;
    frame[0x1a] = 0xffu;
    frame[0x1b] = 0xffu;
    refresh_frame_crc(frame);
    assert(tdcsrx_build_app_record(frame, sizeof(frame), &record));
    assert(record.mileage_present == 0u);
    assert(record.mileage_magnitude_le[0] == 0xffu);
    assert(record.mileage_magnitude_le[1] == 0xffu);
    assert(record.mileage_magnitude_le[2] == 0x3fu);

    frame[44] ^= 1u;
    memset(&record, 0xa5, sizeof(record));
    assert(!tdcsrx_build_app_record(frame, sizeof(frame), &record));
    assert(((const uint8_t *)&record)[0] == 0xa5u);
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

static uint32_t make_bch_codeword(uint16_t data)
{
    uint32_t shifted = (uint32_t)data << TDCSRX_CODEWORD_PARITY;
    return shifted | tdcsrx_original_syndrome(shifted);
}

static void test_bch_syndrome_and_single_bit_correction(void)
{
    static const uint16_t data_words[] = {
        0x0000u, 0x0001u, 0x1234u, 0x5aa5u, 0xa55au, 0xffffu
    };
    uint16_t syndromes[TDCSRX_CODEWORD_BITS];
    unsigned int i;
    unsigned int unique = 0u;

    for (i = 0u; i < TDCSRX_CODEWORD_BITS; ++i) {
        unsigned int j;
        syndromes[i] = tdcsrx_original_syndrome(1u << i);
        assert(syndromes[i] == tdcsrx_reference_syndrome_5b9(1u << i));
        for (j = 0u; j < i; ++j) {
            if (syndromes[j] == syndromes[i]) break;
        }
        if (j == i) ++unique;
    }
    assert(unique == 26u);

    for (i = 0u; i < sizeof(data_words) / sizeof(data_words[0]); ++i) {
        uint32_t codeword = make_bch_codeword(data_words[i]);
        int corrected_bit = 99;
        unsigned int bit;

        assert(tdcsrx_original_syndrome(codeword) == 0u);
        assert(tdcsrx_original_bch_decode(codeword, &corrected_bit) == data_words[i]);
        assert(corrected_bit == -1);

        for (bit = 0u; bit < TDCSRX_CODEWORD_BITS; ++bit) {
            uint32_t damaged = codeword ^ (1u << bit);
            corrected_bit = -1;
            assert(tdcsrx_original_bch_decode(damaged, &corrected_bit) == data_words[i]);
            assert(corrected_bit == (int)bit);
        }
    }

    {
        uint32_t value = 0x013579bdu;
        unsigned int n;
        for (n = 0u; n < 128u; ++n) {
            value = value * 1664525u + 1013904223u;
            value &= 0x03ffffffu;
            assert(tdcsrx_original_syndrome(value) ==
                   tdcsrx_reference_syndrome_5b9(value));
        }
    }

    printf("Original TDCSRX BCH polynomial 0x05B9: %u/26 unique syndromes; "
           "all 26 single-bit positions corrected: PASS\n", unique);
}


static uint32_t make_bch_codeword(uint16_t data);

static void feed_sync_word(tdcsrx_stream_state_t *state, uint32_t word,
                           unsigned int corrupt_mask,
                           tdcsrx_stream_event_t expected_event)
{
    unsigned int bit;
    tdcsrx_stream_event_t event = TDCSRX_STREAM_NO_EVENT;
    for (bit = 0u; bit < 32u; ++bit) {
        uint8_t value = (uint8_t)((word >> (31u - bit)) & 1u);
        if ((corrupt_mask & (1u << bit)) != 0u)
            value ^= 1u;
        event = tdcsrx_stream_feed_bit(state, value, NULL, NULL);
        if (bit != 31u)
            assert(event == TDCSRX_STREAM_NO_EVENT);
    }
    assert(event == expected_event);
}

static void feed_codeword(tdcsrx_stream_state_t *state, uint32_t codeword,
                          uint16_t expected_data, int expected_corrected_bit)
{
    unsigned int bit;
    tdcsrx_stream_event_t event = TDCSRX_STREAM_NO_EVENT;
    uint16_t decoded = 0u;
    int corrected = -99;

    for (bit = 0u; bit < TDCSRX_CODEWORD_BITS; ++bit) {
        uint8_t value = (uint8_t)((codeword >> (25u - bit)) & 1u);
        if (state->inverted)
            value ^= 1u;
        event = tdcsrx_stream_feed_bit(state, value, &decoded, &corrected);
        if (bit + 1u < TDCSRX_CODEWORD_BITS)
            assert(event == TDCSRX_STREAM_NO_EVENT);
    }

    assert(event == TDCSRX_STREAM_CODEWORD_READY);
    assert(decoded == expected_data);
    assert(corrected == expected_corrected_bit);
}

static void test_stream_sync_and_codeword_normal(void)
{
    tdcsrx_stream_state_t state;
    uint32_t codeword = make_bch_codeword(0x5aa5u);
    tdcsrx_stream_init(&state);
    feed_sync_word(&state, TDCSRX_SYNC_WORD, 0u,
                   TDCSRX_STREAM_SYNC_ACQUIRED);
    assert(state.synchronized);
    assert(!state.inverted);
    feed_codeword(&state, codeword ^ (1u << 7), 0x5aa5u, 7);
}

static void test_stream_sync_and_codeword_inverted(void)
{
    tdcsrx_stream_state_t state;
    uint32_t codeword = make_bch_codeword(0xa55au);
    tdcsrx_stream_init(&state);
    feed_sync_word(&state, ~TDCSRX_SYNC_WORD, 0u,
                   TDCSRX_STREAM_SYNC_ACQUIRED);
    assert(state.synchronized);
    assert(state.inverted);
    feed_codeword(&state, codeword, 0xa55au, -1);
}

static void test_stream_sync_error_tolerance(void)
{
    tdcsrx_stream_state_t state;
    tdcsrx_stream_init(&state);
    feed_sync_word(&state, TDCSRX_SYNC_WORD, (1u << 3) | (1u << 19),
                   TDCSRX_STREAM_SYNC_ACQUIRED);
    assert(state.synchronized);
    assert(!state.inverted);
}


static void test_stream_sync_31bit_window_variants(void)
{
    tdcsrx_stream_state_t state;

    /* The most-significant rolling bit is deliberately different. */
    tdcsrx_stream_init(&state);
    feed_sync_word(&state, 0x8dd4259fu, 0u,
                   TDCSRX_STREAM_SYNC_ACQUIRED);
    assert(state.synchronized);
    assert(!state.inverted);

    tdcsrx_stream_init(&state);
    feed_sync_word(&state, 0x722bda60u, 0u,
                   TDCSRX_STREAM_SYNC_ACQUIRED);
    assert(state.synchronized);
    assert(state.inverted);
}


static void test_stream_rejects_three_sync_errors(void)
{
    tdcsrx_stream_state_t state;
    unsigned int bit;
    unsigned int acquisitions = 0u;
    tdcsrx_stream_init(&state);

    for (bit = 0u; bit < 32u; ++bit) {
        uint8_t value = (uint8_t)((TDCSRX_SYNC_WORD >> (31u - bit)) & 1u);
        if (bit == 1u || bit == 8u || bit == 23u)
            value ^= 1u;
        if (tdcsrx_stream_feed_bit(&state, value, NULL, NULL) ==
            TDCSRX_STREAM_SYNC_ACQUIRED)
            ++acquisitions;
    }
    assert(acquisitions == 0u);
    assert(!state.synchronized);
}


int main(void)
{
    test_crc();
    test_stream_sync_and_codeword_normal();
    test_stream_sync_and_codeword_inverted();
    test_stream_sync_error_tolerance();
    test_stream_rejects_three_sync_errors();
    test_stream_sync_31bit_window_variants();
    puts("Sync acquisition normal/inverted, 31/32-bit windows, 2-bit tolerance and 3-bit rejection: PASS");
    puts("MSB-first 26-bit collection and BCH decode: PASS");
    test_parse_variant_1f();
    test_original_40_byte_app_record_abi();
    test_parse_variant_ff();
    test_rejections();
    test_bch_syndrome_and_single_bit_correction();
    puts("CRC-16/XMODEM: PASS");
    puts("45-byte target-frame parser, both header variants, all fields: PASS");
    puts("Original 40-byte app-record ABI, dynamic copies and safe tail: PASS");
    puts("CRC/header/length/null rejection: PASS");
    return 0;
}
