#ifndef TDCS_PROTOCOL_H
#define TDCS_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * The 46 decoded BCH bytes contain a 45-byte TDCS application frame plus
 * one trailing byte from the final 26-bit codeword.
 *
 * The 40-byte frame body is application-frame offsets [3..42]:
 *   body[0..7]  = destination/control/format header
 *   body[8..39] = the actual 32-byte TDCS payload
 */
#define TDCS_FRAME_BODY_SIZE  40u
#define TDCS_DATA_SIZE        32u
#define TDCS_INFO_SIZE        46u
#define TDCS_CODEWORD_COUNT   23u
#define TDCS_CODEWORD_BITS    26u
#define TDCS_FRAME_CODED_BITS (TDCS_CODEWORD_COUNT * TDCS_CODEWORD_BITS)
#define TDCS_FRAME_CODED_BYTES ((TDCS_FRAME_CODED_BITS + 7u) / 8u)

typedef enum {
    TDCS_DECODE_OK = 0,
    TDCS_DECODE_BAD_ARGUMENT,
    TDCS_DECODE_INSUFFICIENT_BITS,
    TDCS_DECODE_BCH_UNCORRECTABLE,
    TDCS_DECODE_BAD_HEADER,
    TDCS_DECODE_BAD_CRC
} TDCS_DecodeResult;

typedef struct {
    uint8_t frame_body[TDCS_FRAME_BODY_SIZE];
    uint16_t crc16;
    uint8_t corrected_bits;
} TDCS_Frame;

typedef struct {
    uint8_t event_code;
    uint8_t signal_type_code;
    uint8_t class_flags;

    uint8_t train_type_id[4];
    char train_prefix;
    uint32_t train_number;

    uint8_t loco_model;
    uint8_t loco_model_ext;
    uint16_t loco_number;

    uint32_t mileage_raw;
    uint32_t mileage_m;
    uint8_t mileage_negative;
    uint8_t mileage_increasing;
    uint8_t mileage_test_frame;
    bool mileage_valid;

    uint32_t speed_raw;
    uint16_t speed_kmh;

    uint16_t weight_t;
    uint16_t length_tenths;
    uint8_t car_count;

    uint16_t station_code;
    uint8_t route_number;
    uint8_t actual_route_number;
    uint32_t driver_number;
} TDCS_PayloadFields;

/*
 * Decode one TDCS FFSK burst after the radio has acquired its sync word.
 *
 * packed_bits contains 23 consecutive 26-bit BCH codewords (598 bits).
 * With lsb_first=false, the first received bit is bit 7 of packed_bits[0].
 * With lsb_first=true, the first received bit is bit 0 of packed_bits[0].
 * bit_offset allows callers to skip leading bits in the FIFO byte stream.
 *
 * On success, result contains frame bytes [3..42] (8 header bytes followed
 * by the actual 32-byte payload), the CRC-16/XMODEM value, and the number
 * of corrected BCH bits. The 46th decoded byte is outside the 45-byte
 * application frame and is intentionally ignored.
 */
TDCS_DecodeResult TDCS_DecodePackedFrame(
    const uint8_t *packed_bits,
    size_t packed_size,
    size_t bit_offset,
    bool lsb_first,
    TDCS_Frame *result);

/* Parse the actual 32-byte TDCS payload (application-frame bytes [11..42]). */
bool TDCS_ParsePayload(
    const uint8_t payload[TDCS_DATA_SIZE],
    TDCS_PayloadFields *fields);

uint16_t TDCS_CRC16XModem(const uint8_t *data, size_t length);

#endif
