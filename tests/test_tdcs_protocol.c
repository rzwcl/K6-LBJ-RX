#include "tdcs_protocol.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define TEST_CODEWORD_BITS 26u
#define TEST_FRAME_BITS (23u * TEST_CODEWORD_BITS)
#define TEST_FRAME_BYTES ((TEST_FRAME_BITS + 7u) / 8u)

static const uint8_t kDemoPayload[TDCS_DATA_SIZE] = {
    0x03, 0x03, 0x01, 'T', ' ', ' ', ' ',
    0x70, 0x00, 0x00,
    0xF2, 0x00,
    0x09, 0x02,
    0x33, 0xB7, 0x11,
    0x7C, 0x00, 0x00,
    0x20, 0x03,
    0x9A, 0x01,
    0x12,
    0x82, 0x00,
    0x04, 0x05,
    0x40, 0xE2, 0x01
};

static uint16_t test_remainder(uint32_t value)
{
    const uint32_t generator = 0x05B9u;
    int bit;

    for (bit = 25; bit >= 10; --bit) {
        if ((value & (1u << (unsigned int)bit)) != 0u) {
            value ^= generator << ((unsigned int)bit - 10u);
        }
    }

    return (uint16_t)(value & 0x03FFu);
}

static void set_bit(
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

static void encode_frame(
    const uint8_t payload[TDCS_DATA_SIZE],
    uint8_t packed[TEST_FRAME_BYTES],
    bool lsb_first,
    size_t prefix_bits,
    uint8_t first_header_byte,
    uint8_t address_variant,
    uint8_t payload_length,
    uint8_t trailing_byte)
{
    uint8_t info[TDCS_INFO_SIZE];
    size_t word_index;

    memset(info, 0, sizeof(info));
    info[0] = first_header_byte;
    info[1] = 0x2Bu;
    info[2] = 0x00u;
    info[3] = 0x3Fu;
    info[4] = 0x1Fu;
    info[5] = 0x00u;
    info[6] = 0x00u;

    if (address_variant == 0u) {
        info[7] = 0x1Fu;
        info[8] = 0x8Cu;
        info[9] = 0x30u;
    } else if (address_variant == 1u) {
        info[7] = 0xFFu;
        info[8] = 0xFFu;
        info[9] = 0xFFu;
    }

    info[10] = payload_length;
    memcpy(&info[11], payload, TDCS_DATA_SIZE);

    {
        const uint16_t crc = TDCS_CRC16XModem(info, 43u);
        info[43] = (uint8_t)(crc >> 8);
        info[44] = (uint8_t)crc;
    }
    info[45] = trailing_byte;

    memset(packed, 0, TEST_FRAME_BYTES);
    for (word_index = 0u; word_index < 23u; ++word_index) {
        const uint16_t data_word =
            (uint16_t)(((uint16_t)info[word_index * 2u] << 8) |
                       info[word_index * 2u + 1u]);
        const uint32_t shifted = (uint32_t)data_word << 10;
        const uint32_t codeword = shifted | test_remainder(shifted);
        unsigned int bit;

        for (bit = 0u; bit < TEST_CODEWORD_BITS; ++bit) {
            const unsigned int source_shift = 25u - bit;
            const uint8_t value =
                (uint8_t)((codeword >> source_shift) & 1u);
            set_bit(packed, prefix_bits + word_index * TEST_CODEWORD_BITS + bit,
                    lsb_first, value);
        }
    }
}

static void flip_received_bit(
    uint8_t *packed,
    size_t prefix_bits,
    size_t codeword_index,
    size_t bit_index)
{
    const size_t bit_position =
        prefix_bits + codeword_index * TEST_CODEWORD_BITS + bit_index;
    const unsigned int shift = 7u - (unsigned int)(bit_position & 7u);

    packed[bit_position >> 3] ^= (uint8_t)(1u << shift);
}

static TDCS_DecodeResult decode(
    const uint8_t *packed,
    size_t size,
    size_t bit_offset,
    bool lsb_first,
    TDCS_Frame *frame)
{
    return TDCS_DecodePackedFrame(
        packed, size, bit_offset, lsb_first, frame);
}

static void test_crc_known_vector(void)
{
    static const uint8_t sample[] = "123456789";
    assert(TDCS_CRC16XModem(sample, sizeof(sample) - 1u) == 0x31C3u);
}

static void test_decode_32_byte_payload(void)
{
    uint8_t packed[TEST_FRAME_BYTES];
    TDCS_Frame frame;
    TDCS_PayloadFields fields;

    encode_frame(kDemoPayload, packed, false, 0u, 0x0Cu, 0u, 0x20u, 0u);
    assert(decode(packed, sizeof(packed), 0u, false, &frame) == TDCS_DECODE_OK);
    assert(frame.corrected_bits == 0u);
    assert(TDCS_ParsePayload(frame.frame_body + 8u, &fields));

    assert(fields.event_code == 3u);
    assert(fields.signal_type_code == 3u);
    assert(fields.class_flags == 1u);
    assert(memcmp(fields.train_type_id, "T   ", 4u) == 0);
    assert(fields.train_number == 112u);
    assert(fields.train_prefix == ' ');
    assert(fields.loco_model == 242u);
    assert(fields.loco_model_ext == 0u);
    assert(fields.loco_number == 521u);
    assert(fields.mileage_raw == 1161011u);
    assert(fields.mileage_valid);
    assert(fields.mileage_test_frame == 0u);
    assert(fields.mileage_m == 1161011u);
    assert(fields.mileage_negative == 0u);
    assert(fields.mileage_increasing == 0u);
    assert(fields.speed_kmh == 124u);
    assert(fields.speed_raw == 124u);
    assert(fields.weight_t == 800u);
    assert(fields.length_tenths == 410u);
    assert(fields.car_count == 18u);
    assert(fields.station_code == 130u);
    assert(fields.route_number == 4u);
    assert(fields.actual_route_number == 5u);
    assert(fields.driver_number == 123456u);
}

static void test_both_original_address_variants(void)
{
    uint8_t packed[TEST_FRAME_BYTES];
    TDCS_Frame frame;

    encode_frame(kDemoPayload, packed, false, 0u, 0x0Cu, 0u, 0x20u, 0u);
    assert(decode(packed, sizeof(packed), 0u, false, &frame) == TDCS_DECODE_OK);

    encode_frame(kDemoPayload, packed, false, 0u, 0x0Cu, 1u, 0x20u, 0u);
    assert(decode(packed, sizeof(packed), 0u, false, &frame) == TDCS_DECODE_OK);
}

static void test_trailing_byte_is_outside_application_frame(void)
{
    uint8_t packed[TEST_FRAME_BYTES];
    TDCS_Frame frame;

    encode_frame(kDemoPayload, packed, false, 0u, 0x0Cu, 0u, 0x20u, 0xA5u);
    assert(decode(packed, sizeof(packed), 0u, false, &frame) == TDCS_DECODE_OK);
}

static void test_correct_one_and_two_bit_errors(void)
{
    uint8_t packed[TEST_FRAME_BYTES];
    TDCS_Frame frame;

    encode_frame(kDemoPayload, packed, false, 0u, 0x0Cu, 0u, 0x20u, 0u);
    flip_received_bit(packed, 0u, 0u, 18u);
    assert(decode(packed, sizeof(packed), 0u, false, &frame) == TDCS_DECODE_OK);
    assert(frame.corrected_bits == 1u);

    encode_frame(kDemoPayload, packed, false, 0u, 0x0Cu, 0u, 0x20u, 0u);
    flip_received_bit(packed, 0u, 4u, 5u);
    flip_received_bit(packed, 0u, 4u, 19u);
    assert(decode(packed, sizeof(packed), 0u, false, &frame) == TDCS_DECODE_OK);
    assert(frame.corrected_bits == 2u);
}

static void test_lsb_first_with_nonzero_offset(void)
{
    uint8_t storage[TEST_FRAME_BYTES + 2u];
    TDCS_Frame frame;

    memset(storage, 0, sizeof(storage));
    encode_frame(kDemoPayload, storage, true, 3u, 0x0Cu, 0u, 0x20u, 0u);
    assert(decode(storage, sizeof(storage), 3u, true, &frame) == TDCS_DECODE_OK);
    assert(frame.corrected_bits == 0u);
}

static void test_reject_short_input(void)
{
    uint8_t packed[TEST_FRAME_BYTES];
    TDCS_Frame frame;

    encode_frame(kDemoPayload, packed, false, 0u, 0x0Cu, 0u, 0x20u, 0u);
    assert(decode(packed, sizeof(packed) - 1u, 0u, false, &frame) ==
           TDCS_DECODE_INSUFFICIENT_BITS);
}

static void test_reject_bad_header(void)
{
    uint8_t packed[TEST_FRAME_BYTES];
    TDCS_Frame frame;

    encode_frame(kDemoPayload, packed, false, 0u, 0x0Du, 0u, 0x20u, 0u);
    assert(decode(packed, sizeof(packed), 0u, false, &frame) ==
           TDCS_DECODE_BAD_HEADER);

    encode_frame(kDemoPayload, packed, false, 0u, 0x0Cu, 2u, 0x20u, 0u);
    assert(decode(packed, sizeof(packed), 0u, false, &frame) ==
           TDCS_DECODE_BAD_HEADER);

    encode_frame(kDemoPayload, packed, false, 0u, 0x0Cu, 0u, 0x28u, 0u);
    assert(decode(packed, sizeof(packed), 0u, false, &frame) ==
           TDCS_DECODE_BAD_HEADER);
}

static void test_reject_crc_mismatch(void)
{
    uint8_t packed[TEST_FRAME_BYTES];
    TDCS_Frame frame;

    encode_frame(kDemoPayload, packed, false, 0u, 0x0Cu, 0u, 0x20u, 0u);
    /* Three bit flips in one BCH word exceed the correction search. */
    flip_received_bit(packed, 0u, 22u, 0u);
    flip_received_bit(packed, 0u, 22u, 1u);
    flip_received_bit(packed, 0u, 22u, 2u);
    assert(decode(packed, sizeof(packed), 0u, false, &frame) != TDCS_DECODE_OK);
}

static void put_le24(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
}

static void test_mileage_special_values(void)
{
    static const uint32_t values[] = {
        9999888u, 8888888u, 9999999u, 0x00FFFFFFu, 0x00C0007Bu
    };
    size_t i;

    for (i = 0u; i < sizeof(values) / sizeof(values[0]); ++i) {
        uint8_t payload[TDCS_DATA_SIZE];
        TDCS_PayloadFields fields;

        memcpy(payload, kDemoPayload, sizeof(payload));
        put_le24(&payload[0x0E], values[i]);
        assert(TDCS_ParsePayload(payload, &fields));

        if (values[i] == 0x00C0007Bu) {
            assert(fields.mileage_valid);
            assert(fields.mileage_m == 123u);
            assert(fields.mileage_negative == 1u);
            assert(fields.mileage_increasing == 1u);
        } else {
            assert(!fields.mileage_valid);
            assert(fields.mileage_test_frame ==
                   (values[i] == 9999888u || values[i] == 8888888u));
        }
    }
}

static void test_reject_uncorrectable_word(void)
{
    uint8_t packed[TEST_FRAME_BYTES];
    TDCS_Frame frame;
    unsigned int i;

    encode_frame(kDemoPayload, packed, false, 0u, 0x0Cu, 0u, 0x20u, 0u);
    for (i = 0u; i < 8u; ++i) {
        flip_received_bit(packed, 0u, 8u, i);
    }
    assert(decode(packed, sizeof(packed), 0u, false, &frame) != TDCS_DECODE_OK);
}

int main(void)
{
    static const struct {
        const char *name;
        void (*run)(void);
    } tests[] = {
        {"crc_known_vector", test_crc_known_vector},
        {"decode_32_byte_payload", test_decode_32_byte_payload},
        {"both_original_address_variants", test_both_original_address_variants},
        {"trailing_byte_is_outside_application_frame", test_trailing_byte_is_outside_application_frame},
        {"correct_one_and_two_bit_errors", test_correct_one_and_two_bit_errors},
        {"lsb_first_with_nonzero_offset", test_lsb_first_with_nonzero_offset},
        {"reject_short_input", test_reject_short_input},
        {"reject_bad_header", test_reject_bad_header},
        {"reject_crc_mismatch", test_reject_crc_mismatch},
        {"mileage_special_values", test_mileage_special_values},
        {"reject_uncorrectable_word", test_reject_uncorrectable_word}
    };
    size_t i;

    for (i = 0u; i < sizeof(tests) / sizeof(tests[0]); ++i) {
        tests[i].run();
        printf("PASS %s\n", tests[i].name);
    }

    printf("PASS %u TDCS protocol tests\n",
           (unsigned int)(sizeof(tests) / sizeof(tests[0])));
    return 0;
}
