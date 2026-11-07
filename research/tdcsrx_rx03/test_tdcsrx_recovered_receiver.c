#include "tdcsrx_recovered_receiver.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    unsigned int calls;
    uint8_t path;
    tdcsrx_message_t message;
} capture_t;

static void capture_frame(void *context, uint8_t path,
                          const tdcsrx_message_t *message)
{
    capture_t *capture = (capture_t *)context;
    ++capture->calls;
    capture->path = path;
    capture->message = *message;
}

static void put_le16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void put_le24(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
}

static void make_frame(uint8_t frame[TDCSRX_FRAME_SIZE], int ff_header)
{
    uint8_t *payload;
    uint16_t crc;

    memset(frame, 0, TDCSRX_FRAME_SIZE);
    frame[0] = 0x0c;
    frame[1] = 0x2b;
    if (ff_header) {
        frame[7] = 0xff;
        frame[8] = 0xff;
        frame[9] = 0xff;
    } else {
        frame[7] = 0x1f;
        frame[8] = 0x8c;
        frame[9] = 0x30;
    }
    frame[10] = TDCSRX_PAYLOAD_SIZE;
    payload = frame + 11;

    payload[0] = 0xf2;
    payload[1] = 0xfd;
    payload[2] = 0xd5;
    memcpy(payload + 3, "T123", 4);
    put_le24(payload + 7, 7654321u);
    put_le16(payload + 10, 2390u);
    put_le16(payload + 12, 1234u);
    put_le24(payload + 14, 0x00c12345u);
    put_le24(payload + 17, 0x00a5fc7cu);
    put_le16(payload + 20, 800u);
    put_le16(payload + 22, 412u);
    payload[24] = 17u;
    put_le16(payload + 25, 321u);
    payload[27] = 4u;
    payload[28] = 5u;
    put_le24(payload + 29, 654321u);

    crc = tdcsrx_crc16_xmodem(frame, 43u);
    frame[43] = (uint8_t)(crc >> 8);
    frame[44] = (uint8_t)crc;
}

static uint32_t make_bch_codeword(uint16_t data)
{
    uint32_t shifted = (uint32_t)data << TDCSRX_CODEWORD_PARITY;
    return shifted | tdcsrx_original_syndrome(shifted);
}

static void feed_frame(tdcsrx_receiver_t *receiver,
                       uint8_t path_index,
                       const uint8_t frame[TDCSRX_FRAME_SIZE],
                       bool inverted,
                       unsigned int sync_error_mask,
                       int corrupt_codeword_bit,
                       tdcsrx_frame_callback_t callback,
                       void *context)
{
    unsigned int bit;
    unsigned int i;
    uint8_t padded[TDCSRX_RX_FRAME_BUFFER_SIZE];

    memset(padded, 0, sizeof(padded));
    memcpy(padded, frame, TDCSRX_FRAME_SIZE);

    /* The target searches the sync in a rolling 32-bit window with <=2 errors. */
    for (bit = 0u; bit < 32u; ++bit) {
        uint8_t value = (uint8_t)((TDCSRX_SYNC_WORD >> (31u - bit)) & 1u);
        if ((sync_error_mask & (1u << bit)) != 0u)
            value ^= 1u;
        if (inverted)
            value ^= 1u;
        tdcsrx_receiver_feed_bit(receiver, path_index, value,
                                 callback, context);
    }

    /* 45 app bytes are carried in 23 MSB-first 26-bit BCH words (46 bytes
       after word decode; the final low byte is padding). */
    for (i = 0u; i < TDCSRX_RX_FRAME_BUFFER_SIZE; i += 2u) {
        uint16_t word = (uint16_t)(((uint16_t)padded[i] << 8) | padded[i + 1u]);
        uint32_t codeword = make_bch_codeword(word);

        if (i == 18u && corrupt_codeword_bit >= 0)
            codeword ^= 1u << (unsigned int)corrupt_codeword_bit;

        for (bit = 0u; bit < TDCSRX_CODEWORD_BITS; ++bit) {
            uint8_t value =
                (uint8_t)((codeword >> (TDCSRX_CODEWORD_BITS - 1u - bit)) & 1u);
            if (inverted)
                value ^= 1u;
            tdcsrx_receiver_feed_bit(receiver, path_index, value,
                                     callback, context);
        }
    }
}

static void feed_frame_after_preamble_sync(tdcsrx_receiver_t *receiver,
                                                uint8_t path_index,
                                                const uint8_t frame[TDCSRX_FRAME_SIZE],
                                                bool inverted,
                                                uint32_t sync_error_mask,
                                                tdcsrx_frame_callback_t callback,
                                                void *context)
{
    unsigned int bit;
    unsigned int i;
    uint8_t padded[TDCSRX_RX_FRAME_BUFFER_SIZE];

    memset(padded, 0, sizeof(padded));
    memcpy(padded, frame, TDCSRX_FRAME_SIZE);

    /*
     * The published railway protocol specifies 51 alternating bit-sync bits.
     * Model the BK4819 having consumed the first 16 as its two-byte hardware
     * sync: 35 alternating bits remain in the FIFO ahead of the 31-bit
     * application frame sync. This deliberately keeps the application sync
     * in the software receiver so its <=2-bit tolerance is retained.
     */
    for (bit = 0u; bit < 35u; ++bit) {
        uint8_t value = (uint8_t)(bit & 1u);
        if (inverted)
            value ^= 1u;
        tdcsrx_receiver_feed_bit(receiver, path_index, value,
                                 callback, context);
    }

    for (bit = 0u; bit < 31u; ++bit) {
        uint8_t value =
            (uint8_t)((TDCSRX_SYNC_WORD >> (30u - bit)) & 1u);
        if ((sync_error_mask & (1u << bit)) != 0u)
            value ^= 1u;
        if (inverted)
            value ^= 1u;
        tdcsrx_receiver_feed_bit(receiver, path_index, value,
                                 callback, context);
    }

    /* The 45-byte application frame is carried in 23 MSB-first 26-bit words. */
    for (i = 0u; i < TDCSRX_RX_FRAME_BUFFER_SIZE; i += 2u) {
        uint16_t word = (uint16_t)(((uint16_t)padded[i] << 8) | padded[i + 1u]);
        uint32_t codeword = make_bch_codeword(word);

        for (bit = 0u; bit < TDCSRX_CODEWORD_BITS; ++bit) {
            uint8_t value =
                (uint8_t)((codeword >> (TDCSRX_CODEWORD_BITS - 1u - bit)) & 1u);
            if (inverted)
                value ^= 1u;
            tdcsrx_receiver_feed_bit(receiver, path_index, value,
                                     callback, context);
        }
    }
}

static void test_two_byte_preamble_sync_handoff(void)
{
    tdcsrx_receiver_t receiver;
    capture_t capture;
    uint8_t frame_a[TDCSRX_FRAME_SIZE];
    uint8_t frame_b[TDCSRX_FRAME_SIZE];

    memset(&capture, 0, sizeof(capture));
    tdcsrx_receiver_init(&receiver);
    make_frame(frame_a, 0);
    make_frame(frame_b, 1);

    feed_frame_after_preamble_sync(&receiver, 0u, frame_a, false,
                                   (1u << 4) | (1u << 19),
                                   capture_frame, &capture);
    feed_frame_after_preamble_sync(&receiver, 1u, frame_b, true, 0u,
                                   capture_frame, &capture);

    assert(capture.calls == 2u);
    assert(capture.path == 1u);
    assert(capture.message.train_number == 7654321u);
    assert(capture.message.received_crc == capture.message.calculated_crc);
}

static void test_complete_frame_path_a(void)
{
    tdcsrx_receiver_t receiver;
    capture_t capture;
    uint8_t frame[TDCSRX_FRAME_SIZE];

    memset(&capture, 0, sizeof(capture));
    tdcsrx_receiver_init(&receiver);
    make_frame(frame, 0);

    feed_frame(&receiver, 0u, frame, false, 0u, -1,
               capture_frame, &capture);

    assert(capture.calls == 1u);
    assert(capture.path == 0u);
    assert(capture.message.train_number == 7654321u);
    assert(capture.message.locomotive_type == 2390u);
    assert(capture.message.speed_kmh == 124u);
    assert(capture.message.driver_number == 654321u);
}

static void test_complete_inverted_frame_path_b(void)
{
    tdcsrx_receiver_t receiver;
    capture_t capture;
    uint8_t frame[TDCSRX_FRAME_SIZE];

    memset(&capture, 0, sizeof(capture));
    tdcsrx_receiver_init(&receiver);
    make_frame(frame, 1);

    feed_frame(&receiver, 1u, frame, true, (1u << 3) | (1u << 19), 7,
               capture_frame, &capture);

    assert(capture.calls == 1u);
    assert(capture.path == 1u);
    assert(capture.message.train_number == 7654321u);
    assert(capture.message.received_crc == capture.message.calculated_crc);
}

static void test_bad_crc_not_delivered(void)
{
    tdcsrx_receiver_t receiver;
    capture_t capture;
    uint8_t frame[TDCSRX_FRAME_SIZE];

    memset(&capture, 0, sizeof(capture));
    tdcsrx_receiver_init(&receiver);
    make_frame(frame, 0);
    frame[44] ^= 1u;

    feed_frame(&receiver, 0u, frame, false, 0u, -1,
               capture_frame, &capture);

    assert(capture.calls == 0u);
}

static void test_invalid_header_not_delivered(void)
{
    tdcsrx_receiver_t receiver;
    capture_t capture;
    uint8_t frame[TDCSRX_FRAME_SIZE];

    memset(&capture, 0, sizeof(capture));
    tdcsrx_receiver_init(&receiver);
    make_frame(frame, 0);
    frame[0] = 0x0du;
    {
        uint16_t crc = tdcsrx_crc16_xmodem(frame, 43u);
        frame[43] = (uint8_t)(crc >> 8);
        frame[44] = (uint8_t)crc;
    }

    feed_frame(&receiver, 0u, frame, false, 0u, -1,
               capture_frame, &capture);

    assert(capture.calls == 0u);
}

static void build_bitstream(const uint8_t frame[TDCSRX_FRAME_SIZE],
                              bool inverted,
                              uint8_t bits[32u + TDCSRX_RX_FRAME_WORD_MAX *
                                                TDCSRX_CODEWORD_BITS])
{
    unsigned int bit;
    unsigned int i;
    uint8_t padded[TDCSRX_RX_FRAME_BUFFER_SIZE];

    memset(padded, 0, sizeof(padded));
    memcpy(padded, frame, TDCSRX_FRAME_SIZE);

    for (bit = 0u; bit < 32u; ++bit) {
        uint8_t value = (uint8_t)((TDCSRX_SYNC_WORD >> (31u - bit)) & 1u);
        bits[bit] = (uint8_t)(value ^ (inverted ? 1u : 0u));
    }

    for (i = 0u; i < TDCSRX_RX_FRAME_BUFFER_SIZE; i += 2u) {
        uint16_t word = (uint16_t)(((uint16_t)padded[i] << 8) | padded[i + 1u]);
        uint32_t codeword = make_bch_codeword(word);
        unsigned int base = 32u + (i / 2u) * TDCSRX_CODEWORD_BITS;

        for (bit = 0u; bit < TDCSRX_CODEWORD_BITS; ++bit) {
            uint8_t value =
                (uint8_t)((codeword >> (TDCSRX_CODEWORD_BITS - 1u - bit)) & 1u);
            bits[base + bit] = (uint8_t)(value ^ (inverted ? 1u : 0u));
        }
    }
}

static void test_two_paths_interleaved(void)
{
    enum { STREAM_BITS = 32u + TDCSRX_RX_FRAME_WORD_MAX * TDCSRX_CODEWORD_BITS };
    tdcsrx_receiver_t receiver;
    capture_t capture;
    uint8_t frame_a[TDCSRX_FRAME_SIZE];
    uint8_t frame_b[TDCSRX_FRAME_SIZE];
    uint8_t bits_a[STREAM_BITS];
    uint8_t bits_b[STREAM_BITS];
    unsigned int i;

    memset(&capture, 0, sizeof(capture));
    tdcsrx_receiver_init(&receiver);
    make_frame(frame_a, 0);
    make_frame(frame_b, 1);
    build_bitstream(frame_a, false, bits_a);
    build_bitstream(frame_b, true, bits_b);

    /* Two target discriminator lanes advance independently from the same
       sample cadence; interleave their bits to expose accidental shared state. */
    for (i = 0u; i < STREAM_BITS; ++i) {
        tdcsrx_receiver_feed_bit(&receiver, 0u, bits_a[i],
                                 capture_frame, &capture);
        tdcsrx_receiver_feed_bit(&receiver, 1u, bits_b[i],
                                 capture_frame, &capture);
    }

    assert(capture.calls == 2u);
    /* Last callback is path B because its bit was fed second. */
    assert(capture.path == 1u);
    assert(capture.message.train_number == 7654321u);
}

static void test_reacquire_after_frame(void)
{
    tdcsrx_receiver_t receiver;
    capture_t capture;
    uint8_t frame[TDCSRX_FRAME_SIZE];

    memset(&capture, 0, sizeof(capture));
    tdcsrx_receiver_init(&receiver);
    make_frame(frame, 0);

    feed_frame(&receiver, 0u, frame, false, 0u, -1,
               capture_frame, &capture);
    feed_frame(&receiver, 0u, frame, false, 0u, -1,
               capture_frame, &capture);

    assert(capture.calls == 2u);
    assert(capture.path == 0u);
    assert(capture.message.train_number == 7654321u);
}

int main(void)
{
    test_two_byte_preamble_sync_handoff();
    test_complete_frame_path_a();
    test_complete_inverted_frame_path_b();
    test_bad_crc_not_delivered();
    test_invalid_header_not_delivered();
    test_two_paths_interleaved();
    test_reacquire_after_frame();

    puts("TDCSRX recovered dual-path frame assembler host tests: PASS");
    puts("- 23 decoded 16-bit words assemble the 45-byte frame");
    puts("- both independent collector paths");
    puts("- inverted sync and two-bit sync tolerance");
    puts("- single-bit BCH correction before frame CRC");
    puts("- invalid CRC/header rejected");
    puts("- two independent paths interleaved without state sharing");
    puts("- sync reacquisition after a completed frame");
    puts("- 35-bit residual alternating preamble after a 16-bit hardware sync");
    return 0;
}
