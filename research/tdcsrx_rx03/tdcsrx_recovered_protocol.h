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