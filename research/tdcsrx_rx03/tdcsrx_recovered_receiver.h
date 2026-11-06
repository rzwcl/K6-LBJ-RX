#ifndef TDCSRX_RECOVERED_RECEIVER_H
#define TDCSRX_RECOVERED_RECEIVER_H

#include <stdbool.h>
#include <stdint.h>

#include "tdcsrx_recovered_protocol.h"
#include "tdcsrx_recovered_stream.h"

#define TDCSRX_RX_PATH_COUNT 2u
#define TDCSRX_RX_FRAME_WORD_MAX 23u
#define TDCSRX_RX_FRAME_BUFFER_SIZE 46u

typedef struct {
    tdcsrx_stream_state_t stream;
    uint8_t frame[TDCSRX_RX_FRAME_BUFFER_SIZE];
    uint8_t byte_count;
    uint8_t word_count;
    uint8_t expected_words;
} tdcsrx_receiver_path_t;

typedef struct {
    tdcsrx_receiver_path_t path[TDCSRX_RX_PATH_COUNT];
} tdcsrx_receiver_t;

typedef void (*tdcsrx_frame_callback_t)(void *context,
                                        uint8_t path_index,
                                        const tdcsrx_message_t *message);

void tdcsrx_receiver_init(tdcsrx_receiver_t *receiver);

/* Feed one demodulator decision into one of the two recovered target paths. */
void tdcsrx_receiver_feed_bit(tdcsrx_receiver_t *receiver,
                              uint8_t path_index,
                              uint8_t bit,
                              tdcsrx_frame_callback_t callback,
                              void *context);

#endif /* TDCSRX_RECOVERED_RECEIVER_H */
