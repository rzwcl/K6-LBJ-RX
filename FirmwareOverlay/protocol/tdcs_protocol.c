#include "tdcs_protocol.h"

#include <string.h>

#define TDCS_BCH_GENERATOR 0x05B9u
#define TDCS_BCH_DATA_SHIFT 10u
#define TDCS_CRC_INPUT_SIZE 43u
#define TDCS_EXPECTED_FRAME_LENGTH 0x2Bu
#define TDCS_EXPECTED_PAYLOAD_LENGTH 0x20u
#define TDCS_MILEAGE_TEST_1 9999888u
#define TDCS_MILEAGE_TEST_2 8888888u
#define TDCS_MILEAGE_UNAVAILABLE 9999999u
#define TDCS_MILEAGE_UNKNOWN 0x00FFFFFFu

static uint16_t tdcs_bch_remainder(uint32_t value)
{
    int bit;

    for (bit = 25; bit >= (int)TDCS_BCH_DATA_SHIFT; --bit) {
        if ((value & (1u << (unsigned int)bit)) != 0u) {
            value ^= TDCS_BCH_GENERATOR <<
                     ((unsigned int)bit - TDCS_BCH_DATA_SHIFT);
        }
    }

    return (uint16_t)(value & 0x03FFu);
}

static unsigned int tdcs_popcount26(uint32_t value)
{
    unsigned int count = 0u;

    while (value != 0u) {
        value &= value - 1u;
        ++count;
    }

    return count;
}

/*
 * Find a unique one- or two-bit error pattern for a BCH syndrome.
 * If the syndrome is ambiguous, reject it instead of guessing.
 */
static bool tdcs_find_error_mask(uint16_t syndrome, uint32_t *error_mask)
{
    uint32_t found_mask = 0u;
    unsigned int matches = 0u;
    unsigned int i;
    unsigned int j;

    if (syndrome == 0u || error_mask == NULL) {
        return false;
    }

    for (i = 0u; i < TDCS_CODEWORD_BITS; ++i) {
        const uint32_t mask = 1u << i;

        if (tdcs_bch_remainder(mask) == syndrome) {
            found_mask = mask;
            ++matches;
        }
    }

    for (i = 0u; i < TDCS_CODEWORD_BITS; ++i) {
        for (j = i + 1u; j < TDCS_CODEWORD_BITS; ++j) {
            const uint32_t mask = (1u << i) | (1u << j);

            if (tdcs_bch_remainder(mask) == syndrome) {
                found_mask = mask;
                ++matches;
            }
        }
    }

    if (matches != 1u) {
        return false;
    }

    *error_mask = found_mask;
    return true;
}

static uint8_t tdcs_get_packed_bit(
    const uint8_t *data,
    size_t bit_position,
    bool lsb_first)
{
    const unsigned int bit_in_byte = (unsigned int)(bit_position & 7u);
    const unsigned int shift = lsb_first ? bit_in_byte : (7u - bit_in_byte);

    return (uint8_t)((data[bit_position >> 3] >> shift) & 1u);
}

static uint32_t tdcs_read_le24(const uint8_t *p)
{
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16);
}

static uint16_t tdcs_read_le16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

uint16_t TDCS_CRC16XModem(const uint8_t *data, size_t length)
{
    uint16_t crc = 0u;
    size_t i;

    if (data == NULL && length != 0u) {
        return 0u;
    }

    for (i = 0u; i < length; ++i) {
        unsigned int bit;

        crc ^= (uint16_t)((uint16_t)data[i] << 8);
        for (bit = 0u; bit < 8u; ++bit) {
            if ((crc & 0x8000u) != 0u) {
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            } else {
                crc = (uint16_t)(crc << 1);
            }
        }
    }

    return crc;
}

TDCS_DecodeResult TDCS_DecodePackedFrame(
    const uint8_t *packed_bits,
    size_t packed_size,
    size_t bit_offset,
    bool lsb_first,
    TDCS_Frame *result)
{
    uint8_t info[TDCS_INFO_SIZE];
    size_t available_bits;
    size_t codeword_index;
    unsigned int corrected_total = 0u;
    bool header_variant_ff;
    bool header_variant_1f;

    if (packed_bits == NULL || result == NULL) {
        return TDCS_DECODE_BAD_ARGUMENT;
    }

    if (packed_size > ((size_t)-1) / 8u) {
        return TDCS_DECODE_BAD_ARGUMENT;
    }

    available_bits = packed_size * 8u;
    if (bit_offset > available_bits ||
        available_bits - bit_offset < (size_t)TDCS_FRAME_CODED_BITS) {
        return TDCS_DECODE_INSUFFICIENT_BITS;
    }

    memset(info, 0, sizeof(info));

    for (codeword_index = 0u;
         codeword_index < TDCS_CODEWORD_COUNT;
         ++codeword_index) {
        uint32_t word = 0u;
        uint16_t syndrome;
        uint32_t error_mask = 0u;
        unsigned int bit;
        uint16_t data_word;
        const size_t word_start =
            bit_offset + codeword_index * TDCS_CODEWORD_BITS;

        for (bit = 0u; bit < TDCS_CODEWORD_BITS; ++bit) {
            word = (word << 1) |
                   tdcs_get_packed_bit(
                       packed_bits, word_start + bit, lsb_first);
        }

        syndrome = tdcs_bch_remainder(word);
        if (syndrome != 0u) {
            if (!tdcs_find_error_mask(syndrome, &error_mask)) {
                return TDCS_DECODE_BCH_UNCORRECTABLE;
            }

            word ^= error_mask;
            if (tdcs_bch_remainder(word) != 0u) {
                return TDCS_DECODE_BCH_UNCORRECTABLE;
            }
            corrected_total += tdcs_popcount26(error_mask);
        }

        data_word = (uint16_t)((word >> TDCS_BCH_DATA_SHIFT) & 0xFFFFu);
        info[codeword_index * 2u] = (uint8_t)(data_word >> 8);
        info[codeword_index * 2u + 1u] = (uint8_t)data_word;
    }

    /*
     * The executable's 45-byte parser checks the frame length, message type,
     * two permitted address/control triplets, and the 32-byte data length.
     * info[45] is the extra byte yielded by the 23rd BCH word, not part of
     * the application frame; do not validate it or include it in the CRC.
     */
    header_variant_ff =
        info[7] == 0xFFu && info[8] == 0xFFu && info[9] == 0xFFu;
    header_variant_1f =
        info[7] == 0x1Fu && info[8] == 0x8Cu && info[9] == 0x30u;

    if (info[0] != 0x0Cu ||
        info[1] != TDCS_EXPECTED_FRAME_LENGTH ||
        info[2] != 0x00u ||
        (!header_variant_ff && !header_variant_1f) ||
        info[10] != TDCS_EXPECTED_PAYLOAD_LENGTH) {
        return TDCS_DECODE_BAD_HEADER;
    }

    {
        const uint16_t expected_crc =
            (uint16_t)(((uint16_t)info[43] << 8) | info[44]);
        const uint16_t actual_crc =
            TDCS_CRC16XModem(info, TDCS_CRC_INPUT_SIZE);

        if (actual_crc != expected_crc) {
            return TDCS_DECODE_BAD_CRC;
        }

        memcpy(result->frame_body, &info[3], TDCS_FRAME_BODY_SIZE);
        result->crc16 = expected_crc;
        result->corrected_bits = (uint8_t)corrected_total;
    }

    return TDCS_DECODE_OK;
}

bool TDCS_ParsePayload(
    const uint8_t payload[TDCS_DATA_SIZE],
    TDCS_PayloadFields *fields)
{
    uint32_t mileage;
    uint32_t speed;
    uint32_t driver;

    if (payload == NULL || fields == NULL) {
        return false;
    }

    memset(fields, 0, sizeof(*fields));

    fields->event_code = (uint8_t)(payload[0x00] & 0x0Fu);
    fields->signal_type_code = (uint8_t)(payload[0x01] & 0x07u);
    fields->class_flags = payload[0x02];

    memcpy(fields->train_type_id, &payload[0x03], sizeof(fields->train_type_id));
    if (payload[0x06] >= 0x20u && payload[0x06] <= 0x7Eu) {
        fields->train_prefix = (char)payload[0x06];
    } else {
        fields->train_prefix = '\0';
    }

    fields->train_number = tdcs_read_le24(&payload[0x07]);
    fields->loco_model = payload[0x0A];
    fields->loco_model_ext = payload[0x0B];
    fields->loco_number = tdcs_read_le16(&payload[0x0C]);

    mileage = tdcs_read_le24(&payload[0x0E]);
    fields->mileage_raw = mileage;
    fields->mileage_negative = (uint8_t)((mileage >> 23) & 1u);
    fields->mileage_increasing = (uint8_t)((mileage >> 22) & 1u);
    fields->mileage_m = mileage & 0x003FFFFFu;
    fields->mileage_test_frame =
        (uint8_t)(mileage == TDCS_MILEAGE_TEST_1 ||
                  mileage == TDCS_MILEAGE_TEST_2);
    fields->mileage_valid =
        !fields->mileage_test_frame &&
        mileage != TDCS_MILEAGE_UNAVAILABLE &&
        mileage != TDCS_MILEAGE_UNKNOWN;

    speed = tdcs_read_le24(&payload[0x11]);
    fields->speed_raw = speed;
    fields->speed_kmh = (uint16_t)(speed & 0x03FFu);

    fields->weight_t = tdcs_read_le16(&payload[0x14]);
    fields->length_tenths = tdcs_read_le16(&payload[0x16]);
    fields->car_count = payload[0x18];
    fields->station_code = tdcs_read_le16(&payload[0x19]);
    fields->route_number = payload[0x1B];
    fields->actual_route_number = payload[0x1C];

    driver = tdcs_read_le24(&payload[0x1D]);
    fields->driver_number = driver;

    return true;
}
