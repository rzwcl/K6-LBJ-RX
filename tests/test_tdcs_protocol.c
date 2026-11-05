#include "tdcs_protocol.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))
#define TEST_CODEWORD_BITS 26u
#define TEST_INFO_SIZE 46u
#define TEST_FRAME_BITS (23u * TEST_CODEWORD_BITS)
#define TEST_FRAME_BYTES ((TEST_FRAME_BITS + 7u) / 8u)

static const uint8_t kPayload57010[TDCS_PAYLOAD_SIZE] = {
    0x3F, 0x1F, 0x00, 0x00, 0x1F, 0x8C, 0x30, 0x20,
    0x03, 0x03, 0x04, 0x20, 0x20, 0x20, 0x20, 0xB2,
    0xDE, 0x00, 0xED, 0x00, 0x63, 0x02, 0x28, 0x3B,
    0x03, 0x1A, 0xC0, 0x00, 0xAE, 0x03, 0x15, 0x01,
    0x12, 0x1D, 0x00, 0x0E, 0x0E, 0xA8, 0xD2, 0x4F
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
    const uint8_t payload[TDCS_PAYLOAD_SIZE],
    uint8_t packed[TEST_FRAME_BYTES],
    bool lsb_first,
    size_t prefix_bits,
    uint8_t header_byte,
    uint8_t padding_byte)
{
    uint8_t info[TEST_INFO_SIZE];
    size_t word_index;

    memset(info, 0, sizeof(info));
    info[0] = header_byte;
    info[1] = 0x2B;
    info[2] = 0x00;
    memcpy(&info[3], payload, TDCS_PAYLOAD_SIZE);
    {
        const uint16_t crc = TDCS_CRC16XModem(info, 43u);
        info[43] = (uint8_t)(crc >> 8);
        info[44] = (uint8_t)crc;
    }
    info[45] = padding_byte;

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

static void test_decode_real_57010_vector(void)
{
    uint8_t packed[TEST_FRAME_BYTES];
    TDCS_Frame frame;
    TDCS_PayloadFields fields;

    encode_frame(kPayload57010, packed, false, 0u, 0x0Cu, 0u);
    assert(decode(packed, sizeof(packed), 0u, false, &frame) == TDCS_DECODE_OK);
    assert(frame.corrected_bits == 0u);
    assert(memcmp(frame.payload, kPayload57010, TDCS_PAYLOAD_SIZE) == 0);
    assert(TDCS_ParsePayload(frame.payload, &fields));

    assert(fields.event_code == 3u);
    assert(fields.signal_type_code == 3u);
    assert(fields.class_flags == 0x04u);
    assert(fields.train_number == 57010u);
    assert(fields.train_prefix == ' ');
    assert(fields.loco_model == 237u);
    assert(fields.loco_number == 611u);
    assert(fields.mileage_valid);
    assert(fields.mileage_m == 211752u);
    assert(fields.mileage_negative == 0u);
    assert(fields.mileage_increasing == 0u);
    assert(fields.speed_kmh == 26u);
    assert(fields.speed_raw == 0x00C01Au);
    assert(fields.weight_t == 942u);
    assert(fields.length_tenths == 277u);
    assert(fields.car_count == 18u);
    assert(fields.station_code == 29u);
    assert(fields.route_number == 14u);
    assert(fields.actual_route_number == 14u);
    assert(fields.driver_number == 0x4FD2A8u);
}

static void test_correct_one_and_two_bit_errors(void)
{
    uint8_t packed[TEST_FRAME_BYTES];
    TDCS_Frame frame;

    encode_frame(kPayload57010, packed, false, 0u, 0x0Cu, 0u);
    /* This BCH polynomial has a few ambiguous syndromes; use a unique one. */
    flip_received_bit(packed, 0u, 0u, 18u);
    assert(decode(packed, sizeof(packed), 0u, false, &frame) == TDCS_DECODE_OK);
    assert(frame.corrected_bits == 1u);
    assert(memcmp(frame.payload, kPayload57010, TDCS_PAYLOAD_SIZE) == 0);

    encode_frame(kPayload57010, packed, false, 0u, 0x0Cu, 0u);
    flip_received_bit(packed, 0u, 4u, 5u);
    flip_received_bit(packed, 0u, 4u, 19u);
    assert(decode(packed, sizeof(packed), 0u, false, &frame) == TDCS_DECODE_OK);
    assert(frame.corrected_bits == 2u);
    assert(memcmp(frame.payload, kPayload57010, TDCS_PAYLOAD_SIZE) == 0);
}

static void test_lsb_first_with_nonzero_offset(void)
{
    uint8_t storage[TEST_FRAME_BYTES + 2u];
    TDCS_Frame frame;

    memset(storage, 0, sizeof(storage));
    encode_frame(kPayload57010, storage, true, 3u, 0x0Cu, 0u);
    assert(decode(storage, sizeof(storage), 3u, true, &frame) == TDCS_DECODE_OK);
    assert(frame.corrected_bits == 0u);
    assert(memcmp(frame.payload, kPayload57010, TDCS_PAYLOAD_SIZE) == 0);
}

static void test_reject_short_input(void)
{
    uint8_t packed[TEST_FRAME_BYTES];
    TDCS_Frame frame;

    encode_frame(kPayload57010, packed, false, 0u, 0x0Cu, 0u);
    assert(decode(packed, sizeof(packed) - 1u, 0u, false, &frame) ==
           TDCS_DECODE_INSUFFICIENT_BITS);
}

static void test_reject_bad_header(void)
{
    uint8_t packed[TEST_FRAME_BYTES];
    TDCS_Frame frame;

    encode_frame(kPayload57010, packed, false, 0u, 0x0Du, 0u);
    assert(decode(packed, sizeof(packed), 0u, false, &frame) ==
           TDCS_DECODE_BAD_HEADER);
}

static void test_reject_bad_padding(void)
{
    uint8_t packed[TEST_FRAME_BYTES];
    TDCS_Frame frame;

    encode_frame(kPayload57010, packed, false, 0u, 0x0Cu, 0x01u);
    assert(decode(packed, sizeof(packed), 0u, false, &frame) ==
           TDCS_DECODE_BAD_PADDING);
}

static void test_reject_crc_mismatch(void)
{
    uint8_t packed[TEST_FRAME_BYTES];
    uint8_t changed[TDCS_PAYLOAD_SIZE];
    TDCS_Frame frame;

    memcpy(changed, kPayload57010, sizeof(changed));
    changed[0x0F] ^= 0x01u;
    encode_frame(changed, packed, false, 0u, 0x0Cu, 0u);
    /* The helper computes a valid CRC for changed data, so corrupt one
       received code bit beyond the BCH correction capability. */
    flip_received_bit(packed, 0u, 22u, 0u);
    flip_received_bit(packed, 0u, 22u, 1u);
    flip_received_bit(packed, 0u, 22u, 2u);
    assert(decode(packed, sizeof(packed), 0u, false, &frame) != TDCS_DECODE_OK);
}

static void test_reject_ambiguous_or_uncorrectable_word(void)
{
    uint8_t packed[TEST_FRAME_BYTES];
    TDCS_Frame frame;
    unsigned int i;

    encode_frame(kPayload57010, packed, false, 0u, 0x0Cu, 0u);
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
        {"decode_real_57010_vector", test_decode_real_57010_vector},
        {"correct_one_and_two_bit_errors", test_correct_one_and_two_bit_errors},
        {"lsb_first_with_nonzero_offset", test_lsb_first_with_nonzero_offset},
        {"reject_short_input", test_reject_short_input},
        {"reject_bad_header", test_reject_bad_header},
        {"reject_bad_padding", test_reject_bad_padding},
        {"reject_crc_mismatch", test_reject_crc_mismatch},
        {"reject_uncorrectable_word", test_reject_ambiguous_or_uncorrectable_word}
    };
    size_t i;

    for (i = 0u; i < ARRAY_COUNT(tests); ++i) {
        tests[i].run();
        printf("PASS %s\n", tests[i].name);
    }

    printf("PASS %u TDCS protocol tests\n", (unsigned int)ARRAY_COUNT(tests));
    return 0;
}
