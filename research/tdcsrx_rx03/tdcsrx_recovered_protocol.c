#include "tdcsrx_recovered_protocol.h"

#include <string.h>

static uint16_t read_le16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t read_le24(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
}

uint16_t tdcsrx_crc16_xmodem(const uint8_t *data, size_t length)
{
    uint16_t crc = 0;
    size_t i;
    unsigned bit;

    if (data == NULL && length != 0u)
        return 0;

    for (i = 0; i < length; ++i) {
        crc ^= (uint16_t)((uint16_t)data[i] << 8);
        for (bit = 0; bit < 8u; ++bit) {
            crc = (uint16_t)((crc & 0x8000u) != 0u
                ? (uint16_t)(crc << 1) ^ 0x1021u
                : (uint16_t)(crc << 1));
        }
    }
    return crc;
}

bool tdcsrx_parse_frame(const uint8_t *frame, size_t frame_length,
                        tdcsrx_message_t *out)
{
    const uint8_t *p;
    uint16_t crc_calculated;
    uint16_t crc_received;
    bool header_variant_ff;
    bool header_variant_1f;

    if (frame == NULL || out == NULL || frame_length != TDCSRX_FRAME_SIZE)
        return false;

    /* Direct predicates recovered from the TDCSRX 0.3 frame parser. */
    if (frame[0] != 0x0cu || frame[1] != 0x2bu || frame[10] != 0x20u)
        return false;

    header_variant_ff = frame[7] == 0xffu &&
                        frame[8] == 0xffu &&
                        frame[9] == 0xffu;
    header_variant_1f = frame[7] == 0x1fu &&
                        frame[8] == 0x8cu &&
                        frame[9] == 0x30u;
    if (!header_variant_ff && !header_variant_1f)
        return false;

    crc_calculated = tdcsrx_crc16_xmodem(frame, 43u);
    crc_received = (uint16_t)(((uint16_t)frame[43] << 8) | frame[44]);
    if (crc_calculated != crc_received)
        return false;

    p = frame + 11;
    memset(out, 0, sizeof(*out));

    out->signal_machine = (uint8_t)(p[0] & 0x0fu);
    out->signal_kind = (uint8_t)(p[1] & 0x07u);
    out->locomotive_role = (uint8_t)(p[2] & 0x03u);

    memcpy(out->train_prefix, p + 3, sizeof(out->train_prefix));
    out->train_number = read_le24(p + 7);
    out->locomotive_type = read_le16(p + 10);
    out->locomotive_serial = read_le16(p + 12);

    out->mileage_raw24 = read_le24(p + 14);
    out->mileage_value22 = out->mileage_raw24 & 0x003fffffu;
    out->mileage_negative = (p[16] & 0x80u) != 0u;
    out->mileage_direction = (p[16] & 0x40u) != 0u;
    out->mileage_present = out->mileage_raw24 != 0x00ffffffu;

    out->speed_kmh = (uint16_t)(read_le16(p + 17) & 0x03ffu);
    out->weight = read_le16(p + 20);
    out->calculated_length = read_le16(p + 22);
    out->car_count = p[24];
    out->station_number = read_le16(p + 25);
    out->track_number = p[27];
    out->actual_track = p[28];
    out->driver_number = read_le24(p + 29);

    memcpy(out->raw_payload, p, TDCSRX_PAYLOAD_SIZE);
    memcpy(out->raw_frame, frame, TDCSRX_FRAME_SIZE);
    out->received_crc = crc_received;
    out->calculated_crc = crc_calculated;
    return true;
}

uint16_t tdcsrx_original_syndrome(uint32_t codeword26)
{
    int bit;
    uint32_t value = codeword26 & 0x03ffffffu;

    /*
     * Literal and loop bounds reproduced from offsets 0x1d4..0x1fc:
     * r3=25 down through 10; if bit r3 is set, xor (0xb905 << (r3-10)).
     * The target then retains the low 10 bits.
     */
    for (bit = 25; bit >= 10; --bit) {
        if ((value & (1u << (unsigned)bit)) != 0u)
            value ^= (uint32_t)(0x0000b905u << (unsigned)(bit - 10));
    }
    return (uint16_t)(value & 0x03ffu);
}

uint16_t tdcsrx_original_bch_decode(uint32_t codeword26, int *corrected_bit)
{
    uint16_t target;
    int bit;
    uint32_t corrected = codeword26 & 0x03ffffffu;

    if (corrected_bit != NULL)
        *corrected_bit = -1;

    target = tdcsrx_original_syndrome(corrected);
    if (target == 0u)
        return (uint16_t)(corrected >> 10);

    /* The target precomputes all 26 single-bit syndromes, then selects first match. */
    for (bit = 0; bit < 26; ++bit) {
        if (tdcsrx_original_syndrome(1u << (unsigned)bit) == target) {
            corrected ^= 1u << (unsigned)bit;
            if (corrected_bit != NULL)
                *corrected_bit = bit;
            return (uint16_t)(corrected >> 10);
        }
    }
    return (uint16_t)(corrected >> 10);
}

uint16_t tdcsrx_reference_syndrome_5b9(uint32_t codeword26)
{
    int bit;
    uint32_t value = codeword26 & 0x03ffffffu;

    for (bit = 25; bit >= 10; --bit) {
        if ((value & (1u << (unsigned)bit)) != 0u)
            value ^= (uint32_t)(0x05b9u << (unsigned)(bit - 10));
    }
    return (uint16_t)(value & 0x03ffu);
}