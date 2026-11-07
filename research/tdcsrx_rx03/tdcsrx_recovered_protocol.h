#ifndef TDCSRX_RECOVERED_PROTOCOL_H
#define TDCSRX_RECOVERED_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Reconstructed from the published TDCS RX 0.3 FAP1 executable.
 * This is not the original author's C source.
 */
#define TDCSRX_FRAME_SIZE       45u
#define TDCSRX_FRAME_LAST_INDEX 44u
#define TDCSRX_PAYLOAD_SIZE     32u
#define TDCSRX_SYNC_WORD        0x0DD4259Fu
#define TDCSRX_CODEWORD_BITS    26u
#define TDCSRX_CODEWORD_DATA    16u
#define TDCSRX_CODEWORD_PARITY  10u
#define TDCSRX_BCH_GENERATOR_POLY 0x05b9u

/*
 * Exact 40-byte application record copied by the original RX 0.3 callback.
 * Every multi-byte value is stored as explicit little-endian byte arrays so
 * this ABI has no compiler-dependent padding. Bytes reserved_26_27 were not
 * written by the original parser; this safe host/K6 conversion zeros them.
 */
#define TDCSRX_APP_RECORD_SIZE 40u

typedef struct {
    uint8_t train_prefix[4];          /* +0x00..03 <- frame[0x0E..0x11] */
    uint8_t train_number_le[4];       /* +0x04..07 <- LE24 frame[0x12..0x14] */
    uint8_t mileage_magnitude_le[4];  /* +0x08..0B <- frame[0x19..0x1B]&0x3fffff */
    uint8_t locomotive_serial_le[2]; /* +0x0C..0D <- frame[0x17..0x18] */
    uint8_t total_weight_le[2];       /* +0x0E..0F <- frame[0x1F..0x20] */
    uint8_t calculated_length_le[2]; /* +0x10..11 <- frame[0x21..0x22] */
    uint8_t auxiliary_12_13[2];      /* +0x12..13 <- frame[0x28..0x29] */
    uint8_t speed_le[2];             /* +0x14..15 <- LE16(frame[0x1C..0x1D])&0x03ff */
    uint8_t locomotive_type_le[2];   /* +0x16..17 <- frame[0x15..0x16] */
    uint8_t locomotive_role;         /* +0x18 <- frame[0x0D]&0x03 */
    uint8_t mileage_present;         /* +0x19 <- 0 if mileage bytes all 0xff, else 1 */
    uint8_t mileage_negative;        /* +0x1A <- frame[0x1B] bit 7 */
    uint8_t mileage_direction;       /* +0x1B <- frame[0x1B] bit 6 */
    uint8_t car_count;               /* +0x1C <- frame[0x23] */
    uint8_t station_number_le[2];    /* +0x1D..1E <- frame[0x24..0x25] */
    uint8_t frame_26_copy;           /* +0x1F <- frame[0x26] */
    uint8_t frame_27_copy;           /* +0x20 <- frame[0x27] */
    uint8_t frame_2a_copy;           /* +0x21 <- frame[0x2A] */
    uint8_t signal_machine;          /* +0x22 <- frame[0x0B]&0x0F */
    uint8_t signal_kind;             /* +0x23 <- frame[0x0C]&0x07 */
    uint8_t header_variant;          /* +0x24 <- 1 for FF FF FF, 0 for 1F 8C 30 */
    uint8_t fixed_index_one;         /* +0x25 <- original parser writes constant 1 */
    uint8_t reserved_26_27[2];       /* +0x26..27 untouched by original parser */
} tdcsrx_app_record_t;

typedef char tdcsrx_app_record_size_must_be_40[
    (sizeof(tdcsrx_app_record_t) == TDCSRX_APP_RECORD_SIZE) ? 1 : -1];

typedef struct {
    uint8_t signal_machine;       /* payload[0] & 0x0f */
    uint8_t signal_kind;          /* payload[1] & 0x07 */
    uint8_t locomotive_role;      /* payload[2] & 0x03 */

    uint8_t train_prefix[4];      /* payload[3..6], transmitted bytes */
    uint32_t train_number;        /* payload[7..9], little-endian 24-bit */
    uint16_t locomotive_type;     /* payload[10..11], little-endian */
    uint16_t locomotive_serial;   /* payload[12..13], little-endian */

    uint32_t mileage_raw24;       /* payload[14..16], all 24 bits */
    uint32_t mileage_value22;     /* mileage_raw24 & 0x3fffff */
    bool mileage_negative;        /* payload[16] bit 7 */
    bool mileage_direction;       /* payload[16] bit 6 */
    bool mileage_present;         /* false only for 0xffffff in recovered parser */

    uint16_t speed_kmh;           /* LE16(payload[17..18]) & 0x03ff; payload[19] ignored */
    uint16_t weight;              /* payload[20..21], little-endian */
    uint16_t calculated_length;   /* payload[22..23], little-endian; display scales by 10 */
    uint8_t car_count;            /* payload[24] */
    uint16_t station_number;      /* payload[25..26], little-endian */
    uint8_t track_number;         /* payload[27] */
    uint8_t actual_track;         /* payload[28] */
    uint32_t driver_number;       /* payload[29..31], little-endian 24-bit */

    uint8_t raw_payload[TDCSRX_PAYLOAD_SIZE];
    uint8_t raw_frame[TDCSRX_FRAME_SIZE];
    uint16_t received_crc;
    uint16_t calculated_crc;
} tdcsrx_message_t;

/* CRC-16/XMODEM as recovered from the target executable: poly=0x1021, init=0. */
uint16_t tdcsrx_crc16_xmodem(const uint8_t *data, size_t length);

/*
 * Recovered frame parser.
 * Requires exactly 45 bytes and validates:
 *   frame[0] == 0x0c, frame[1] == 0x2b, frame[10] == 0x20
 *   frame[7..9] == ff ff ff OR 1f 8c 30
 *   big-endian CRC at [43..44] over bytes [0..42]
 *
 * The target function receives a last-index value of 44 rather than a byte
 * count. This reconstruction uses a conventional byte-count API instead.
 */
bool tdcsrx_parse_frame(const uint8_t *frame, size_t frame_length,
                        tdcsrx_message_t *out);

/*
 * Build the original callback's 40-byte record ABI from a CRC/header-validated
 * 45-byte frame. This is deliberately separate from tdcsrx_message_t, which
 * is a larger host diagnostics structure and is NOT callback-ABI compatible.
 * Unwritten original tail bytes +0x26..+0x27 are zeroed for deterministic use.
 */
bool tdcsrx_build_app_record(const uint8_t *frame, size_t frame_length,
                             tdcsrx_app_record_t *out);


/*
 * Exact polynomial-remainder operation observed in TDCSRX RX 0.3 machine code.
 * The LDR at code offset 0x1dc resolves to the literal at code offset 0x200:
 * bytes B9 05 00 00, i.e. little-endian uint32_t 0x000005b9.
 * This produces 26 distinct single-bit syndrome columns, matching BCH(26,16).
 */
uint16_t tdcsrx_original_syndrome(uint32_t codeword26);

/*
 * Emulates the target's single-bit lookup/flip policy. Returns the 16 data bits
 * (codeword >> 10); corrected_bit is set to 0..25 if a matching one-bit error
 * was found, or -1 if the word was left unchanged.
 */
uint16_t tdcsrx_original_bch_decode(uint32_t codeword26, int *corrected_bit);

/*
 * Independent reference polynomial division using 0x5b9 (as in OnAir's
 * railair-terminal-derived implementation). Its 26 single-bit syndromes are
 * unique. This is a reference candidate, not asserted to be the original app's
 * exact implementation.
 */
uint16_t tdcsrx_reference_syndrome_5b9(uint32_t codeword26);

#ifdef __cplusplus
}
#endif

#endif /* TDCSRX_RECOVERED_PROTOCOL_H */