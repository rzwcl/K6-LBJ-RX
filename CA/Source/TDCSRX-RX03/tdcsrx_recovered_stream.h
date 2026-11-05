#ifndef TDCSRX_RECOVERED_STREAM_H
#define TDCSRX_RECOVERED_STREAM_H

#include <stdbool.h>
#include <stdint.h>

#include "tdcsrx_recovered_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TDCSRX_SYNC_MAX_ERRORS 2u

typedef enum {
    TDCSRX_STREAM_NO_EVENT = 0,
    TDCSRX_STREAM_SYNC_ACQUIRED,
    TDCSRX_STREAM_CODEWORD_READY
} tdcsrx_stream_event_t;

typedef struct {
    uint32_t sync_shift;
    uint32_t codeword_shift;
    uint8_t codeword_bit_count;
    bool synchronized;
    bool inverted;
} tdcsrx_stream_state_t;

/* Initialize all sync-search and codeword-collection state. */
void tdcsrx_stream_init(tdcsrx_stream_state_t *state);

/*
 * Feed one demodulated binary bit (0 or 1).
 *
 * Before synchronization, the target compares four Hamming-distance
 * candidates: normal/inverted sync over 31 and 32 effective bits. A candidate
 * is accepted when its distance is <= TDCSRX_SYNC_MAX_ERRORS.
 *
 * After synchronization, input bits are polarity-corrected and collected
 * MSB-first into a 26-bit BCH word. On the 26th bit, the original BCH(26,16)
 * correction routine is applied and a 16-bit data word is returned.
 *
 * A SYNC_ACQUIRED event does not consume its triggering bit as BCH payload;
 * the next input bit starts the first codeword.
 */
tdcsrx_stream_event_t tdcsrx_stream_feed_bit(tdcsrx_stream_state_t *state,
                                             uint8_t input_bit,
                                             uint16_t *data_word,
                                             int *corrected_bit);

#ifdef __cplusplus
}
#endif

#endif /* TDCSRX_RECOVERED_STREAM_H */
