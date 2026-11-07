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

bool tdcsrx_build_app_record(const uint8_t *frame, size_t frame_length,
                             tdcsrx_app_record_t *out)
{
    tdcsrx_message_t validated;
    uint16_t speed;
    bool mileage_is_all_ff;

    if (out == NULL || !tdcsrx_parse_frame(frame, frame_length, &validated))
        return false;

    memset(out, 0, sizeof(*out));

    memcpy(out->train_prefix, frame + 0x0e, sizeof(out->train_prefix));
    memcpy(out->train_number_le, frame + 0x12, 3u);
    out->train_number_le[3] = 0u;

    out->mileage_magnitude_le[0] = frame[0x19];
    out->mileage_magnitude_le[1] = frame[0x1a];
    out->mileage_magnitude_le[2] = (uint8_t)(frame[0x1b] & 0x3fu);
    out->mileage_magnitude_le[3] = 0u;

    memcpy(out->locomotive_serial_le, frame + 0x17, 2u);
    memcpy(out->total_weight_le, frame + 0x1f, 2u);
    memcpy(out->calculated_length_le, frame + 0x21, 2u);

    out->auxiliary_12_13[0] = frame[0x28];
    out->auxiliary_12_13[1] = frame[0x29];

    speed = (uint16_t)((uint16_t)frame[0x1c] |
                       ((uint16_t)frame[0x1d] << 8));
    speed &= 0x03ffu;
    out->speed_le[0] = (uint8_t)speed;
    out->speed_le[1] = (uint8_t)(speed >> 8);

    memcpy(out->locomotive_type_le, frame + 0x15, 2u);
    out->locomotive_role = (uint8_t)(frame[0x0d] & 0x03u);

    mileage_is_all_ff = frame[0x19] == 0xffu &&
                        frame[0x1a] == 0xffu &&
                        frame[0x1b] == 0xffu;
    out->mileage_present = mileage_is_all_ff ? 0u : 1u;
    out->mileage_negative = (uint8_t)(frame[0x1b] >> 7);
    out->mileage_direction = (uint8_t)((frame[0x1b] >> 6) & 1u);

    out->car_count = frame[0x23];
    memcpy(out->station_number_le, frame + 0x24, 2u);
    out->frame_26_copy = frame[0x26];
    out->frame_27_copy = frame[0x27];
    out->frame_2a_copy = frame[0x2a];
    out->signal_machine = (uint8_t)(frame[0x0b] & 0x0fu);
    out->signal_kind = (uint8_t)(frame[0x0c] & 0x07u);
    out->header_variant =
        (uint8_t)(frame[7] == 0xffu && frame[8] == 0xffu &&
                  frame[9] == 0xffu);
    out->fixed_index_one = 1u;

    /*
     * Original parser leaves +0x26..+0x27 untouched. Zero them here so the
     * callback-compatible record is deterministic and never leaks stale RAM.
     */
    out->reserved_26_27[0] = 0u;
    out->reserved_26_27[1] = 0u;
    return true;
}

uint16_t tdcsrx_original_syndrome(uint32_t codeword26)
{
    int bit;
    uint32_t value = codeword26 & 0x03ffffffu;

    /*
     * Literal and loop bounds reproduced from offsets 0x1d4..0x1fc:
     * r3=25 down through 10; if bit r3 is set, xor (0x05b9 << (r3-10)).
     * The target then retains the low 10 bits.
     */
    for (bit = 25; bit >= 10; --bit) {
        if ((value & (1u << (unsigned)bit)) != 0u)
            value ^= (uint32_t)(0x000005b9u << (unsigned)(bit - 10));
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