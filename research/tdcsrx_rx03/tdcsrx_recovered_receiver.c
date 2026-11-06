#include "tdcsrx_recovered_receiver.h"

#include <string.h>

static void reset_frame_buffer(tdcsrx_receiver_path_t *path)
{
    memset(path->frame, 0, sizeof(path->frame));
    path->byte_count = 0u;
    path->word_count = 0u;
    path->expected_words = 0u;
}

static void restart_path(tdcsrx_receiver_path_t *path)
{
    tdcsrx_stream_release_sync(&path->stream);
    reset_frame_buffer(path);
}

void tdcsrx_receiver_init(tdcsrx_receiver_t *receiver)
{
    unsigned int i;

    if (receiver == NULL)
        return;

    memset(receiver, 0, sizeof(*receiver));
    for (i = 0u; i < TDCSRX_RX_PATH_COUNT; ++i)
        tdcsrx_stream_init(&receiver->path[i].stream);
}

void tdcsrx_receiver_feed_bit(tdcsrx_receiver_t *receiver,
                              uint8_t path_index,
                              uint8_t bit,
                              tdcsrx_frame_callback_t callback,
                              void *context)
{
    tdcsrx_receiver_path_t *path;
    tdcsrx_stream_event_t event;
    uint16_t data_word = 0u;
    int corrected_bit = -1;

    if (receiver == NULL || path_index >= TDCSRX_RX_PATH_COUNT)
        return;

    path = &receiver->path[path_index];
    event = tdcsrx_stream_feed_bit(&path->stream, bit,
                                   &data_word, &corrected_bit);

    if (event == TDCSRX_STREAM_SYNC_ACQUIRED) {
        reset_frame_buffer(path);
        return;
    }

    if (event != TDCSRX_STREAM_CODEWORD_READY)
        return;

    if ((unsigned int)path->byte_count + 2u > sizeof(path->frame)) {
        restart_path(path);
        return;
    }

    /* The target stores each corrected 16-bit BCH word high byte first. */
    path->frame[path->byte_count++] = (uint8_t)(data_word >> 8);
    path->frame[path->byte_count++] = (uint8_t)data_word;
    ++path->word_count;

    if (path->word_count == 1u) {
        /* Header bytes recovered from the first 26-bit BCH codeword. */
        if (path->frame[0] != 0x0cu || path->frame[1] != 0x2bu) {
            restart_path(path);
            return;
        }

        /* Target: expected words = (frame[1] + 3) >> 1. */
        path->expected_words = (uint8_t)((path->frame[1] + 3u) >> 1);
        if (path->expected_words < 3u ||
            path->expected_words > TDCSRX_RX_FRAME_WORD_MAX) {
            restart_path(path);
            return;
        }
    }

    if (path->expected_words != 0u &&
        path->word_count >= path->expected_words) {
        tdcsrx_message_t message;
        bool accepted = false;

        /* The 45-byte target frame occupies 23 16-bit words; the final
           low byte is padding and is not part of the CRC/application frame. */
        if (path->expected_words == TDCSRX_RX_FRAME_WORD_MAX &&
            path->byte_count == TDCSRX_RX_FRAME_BUFFER_SIZE &&
            (size_t)path->frame[1] + 2u == TDCSRX_FRAME_SIZE) {
            accepted = tdcsrx_parse_frame(path->frame,
                                          TDCSRX_FRAME_SIZE,
                                          &message);
        }

        if (accepted && callback != NULL)
            callback(context, path_index, &message);

        restart_path(path);
    }
}
