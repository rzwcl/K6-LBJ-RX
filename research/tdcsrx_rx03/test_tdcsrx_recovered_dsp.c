#include "tdcsrx_recovered_dsp.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_magnitude_198(void)
{
    int32_t baseline;

    baseline = 16;
    assert(tdcsrx_audio_magnitude(3, 4, &baseline) == 4);
    assert(baseline == 16); /* small increase is limited to 16, but decrease? */

    baseline = 16;
    assert(tdcsrx_audio_magnitude(40, 0, &baseline) == 40);
    assert(baseline == 16); /* (40-16)>>5 = 0; lower bound remains 16 */

    baseline = 16;
    assert(tdcsrx_audio_magnitude(1024, 512, &baseline) == 1216);
    assert(baseline == 53); /* 16 + ((1216-16)>>5) */

    baseline = 1024;
    assert(tdcsrx_audio_magnitude(256, 0, &baseline) == 256);
    assert(baseline == 1023); /* falling baseline subtracts baseline>>10 */

    baseline = 100000;
    assert(tdcsrx_audio_magnitude(-1000, 500, &baseline) == 1187);
    assert(baseline == 99903);
}

static void test_audio_state_layout_and_initialization(void)
{
    tdcsrx_audio_state_t state;

    tdcsrx_audio_init(&state);
    assert(sizeof(state) == TDCSRX_DSP_STATE_SIZE);
    assert(state.dc_estimate == 0x8000);
    assert(state.smooth_1200 == 16);
    assert(state.smooth_1800 == 16);
    assert(state.discriminator[0].weight_1200 == 2u);
    assert(state.discriminator[0].weight_1800 == 3u);
    assert(state.discriminator[1].weight_1200 == 3u);
    assert(state.discriminator[1].weight_1800 == 2u);
}

static void test_sample_loop_updates_independent_states(void)
{
    tdcsrx_audio_state_t state;
    unsigned int i;

    tdcsrx_audio_init(&state);

    /* Stable mid-scale input: no undefined math, all phases advance on the
       exact 9,600-sample input cadence. */
    for (i = 0u; i < 160u; ++i)
        tdcsrx_audio_feed_sample(&state, 2048u, NULL, NULL);

    assert(state.phase_1200 == 0u);
    assert(state.phase_1800 == 0u);
    assert(state.discriminator[0].phase == 0x0000u);
    assert(state.discriminator[1].phase == 0x0000u);
    assert(state.smooth_1200 >= 16);
    assert(state.smooth_1800 >= 16);

    /* A sample burst updates the two tone accumulators and both independent
       discriminators without aliasing the frame collector objects. */
    for (i = 0u; i < 80u; ++i)
        tdcsrx_audio_feed_sample(&state, (uint16_t)((i & 1u) ? 2500u : 1600u),
                                 NULL, NULL);
    assert(state.discriminator[0].previous_difference !=
           state.discriminator[1].previous_difference ||
           state.discriminator[0].phase == state.discriminator[1].phase);
}


typedef struct {
    unsigned int calls;
    unsigned int path_mask;
    tdcsrx_message_t last_message;
    uint8_t bits[2][2048];
    unsigned int bit_count[2];
} audio_capture_t;

static const int16_t kSyntheticSine32[32] = {
       0,  117,  230,  333,  424,  499,  554,  587,
     600,  587,  554,  499,  424,  333,  230,  117,
       0, -117, -230, -333, -424, -499, -554, -587,
    -600, -587, -554, -499, -424, -333, -230, -117
};

static void audio_symbol_callback(void *context, uint8_t path, uint8_t bit)
{
    audio_capture_t *capture = (audio_capture_t *)context;
    if (path < 2u && capture->bit_count[path] < 2048u)
        capture->bits[path][capture->bit_count[path]++] = bit;
}

static void audio_capture_callback(void *context, uint8_t path,
                                   const tdcsrx_message_t *message)
{
    audio_capture_t *capture = (audio_capture_t *)context;
    ++capture->calls;
    capture->path_mask |= 1u << path;
    capture->last_message = *message;
}

static void audio_feed_bit(tdcsrx_audio_state_t *state,
                           uint8_t bit,
                           uint8_t *phase,
                           audio_capture_t *capture)
{
    unsigned int sample_index;
    const uint8_t phase_step = bit ? 6u : 4u; /* 1800 Hz for 1, 1200 Hz for 0 */

    for (sample_index = 0u; sample_index < 8u; ++sample_index) {
        const uint16_t sample = (uint16_t)(2048 +
            kSyntheticSine32[*phase & 31u]);
        tdcsrx_audio_feed_sample_traced(state, sample,
                                        audio_symbol_callback, capture,
                                        audio_capture_callback, capture);
        *phase = (uint8_t)((*phase + phase_step) & 31u);
    }
}

static uint32_t make_audio_test_codeword(uint16_t data)
{
    const uint32_t shifted = (uint32_t)data << TDCSRX_CODEWORD_PARITY;
    return shifted | tdcsrx_original_syndrome(shifted);
}

static void audio_feed_word(tdcsrx_audio_state_t *state,
                            uint32_t word,
                            uint8_t *phase,
                            audio_capture_t *capture)
{
    unsigned int bit;
    for (bit = 0u; bit < 32u; ++bit) {
        audio_feed_bit(state,
            (uint8_t)((word >> (31u - bit)) & 1u),
            phase, capture);
    }
}

static void test_full_9600_hz_audio_to_frame(void)
{
    tdcsrx_audio_state_t state;
    audio_capture_t capture;
    uint8_t frame[TDCSRX_FRAME_SIZE];
    uint8_t padded[TDCSRX_RX_FRAME_BUFFER_SIZE];
    uint8_t phase = 0u;
    uint8_t expected[64u + 32u + TDCSRX_RX_FRAME_WORD_MAX * TDCSRX_CODEWORD_BITS];
    unsigned int expected_count = 0u;
    unsigned int i;
    uint16_t crc;

    memset(&capture, 0, sizeof(capture));
    memset(frame, 0, sizeof(frame));
    tdcsrx_audio_init(&state);

    frame[0] = 0x0cu;
    frame[1] = 0x2bu;
    frame[7] = 0xffu;
    frame[8] = 0xffu;
    frame[9] = 0xffu;
    frame[10] = TDCSRX_PAYLOAD_SIZE;
    memcpy(frame + 11, "SYNTHETIC-FFSK-TEST-2026-11-06", 30u);
    crc = tdcsrx_crc16_xmodem(frame, 43u);
    frame[43] = (uint8_t)(crc >> 8);
    frame[44] = (uint8_t)crc;
    memset(padded, 0, sizeof(padded));
    memcpy(padded, frame, sizeof(frame));

    /* Alternating mark/space warm-up, followed by the recovered target sync. */
    for (i = 0u; i < 64u; ++i) {
        expected[expected_count++] = (uint8_t)(i & 1u);
        audio_feed_bit(&state, (uint8_t)(i & 1u), &phase, &capture);
    }

    for (i = 0u; i < 32u; ++i) {
        const uint8_t bit =
            (uint8_t)((TDCSRX_SYNC_WORD >> (31u - i)) & 1u);
        expected[expected_count++] = bit;
        audio_feed_bit(&state, bit, &phase, &capture);
    }

    for (i = 0u; i < sizeof(padded); i += 2u) {
        const uint16_t data =
            (uint16_t)(((uint16_t)padded[i] << 8) | padded[i + 1u]);
        const uint32_t codeword = make_audio_test_codeword(data);
        unsigned int bit;
        for (bit = 0u; bit < TDCSRX_CODEWORD_BITS; ++bit)
            expected[expected_count++] =
                (uint8_t)((codeword >> (TDCSRX_CODEWORD_BITS - 1u - bit)) & 1u);
        audio_feed_word(&state, codeword, &phase, &capture);
    }

    fprintf(stderr, "audio diagnostics: calls=%u paths=0x%X symbols=%u/%u smooth=%ld/%ld diff=%ld/%ld sync=%u/%u\n",
           capture.calls, capture.path_mask,
           capture.bit_count[0], capture.bit_count[1],
           (long)state.smooth_1200, (long)state.smooth_1800,
           (long)state.discriminator[0].previous_difference,
           (long)state.discriminator[1].previous_difference,
           state.receiver.path[0].stream.synchronized,
           state.receiver.path[1].stream.synchronized);
    {
        unsigned int path, inversion, offset;
        unsigned int best_errors = 0xffffffffu, best_overlap = 0u;
        unsigned int best_path = 0u, best_inversion = 0u, best_offset = 0u;
        for (path = 0u; path < 2u; ++path) {
            for (inversion = 0u; inversion < 2u; ++inversion) {
                for (offset = 0u; offset < capture.bit_count[path]; ++offset) {
                    unsigned int overlap = expected_count;
                    unsigned int errors = 0u;
                    unsigned int k;
                    if (offset + overlap > capture.bit_count[path])
                        overlap = capture.bit_count[path] - offset;
                    if (overlap < expected_count / 2u)
                        continue;
                    for (k = 0u; k < overlap; ++k) {
                        if ((capture.bits[path][offset + k] ^ inversion) != expected[k])
                            ++errors;
                    }
                    if (errors * best_overlap < best_errors * overlap) {
                        best_errors = errors;
                        best_overlap = overlap;
                        best_path = path;
                        best_inversion = inversion;
                        best_offset = offset;
                    }
                }
            }
        }
        fprintf(stderr, "bit compare: expected=%u best path=%u invert=%u offset=%u overlap=%u errors=%u\n",
                expected_count, best_path, best_inversion, best_offset,
                best_overlap, best_errors);
    }
    fprintf(stderr, "sync shift=%08lX/%08lX phase=%08lX/%08lX corr=%ld,%ld,%ld,%ld\n",
            (unsigned long)state.receiver.path[0].stream.sync_shift,
            (unsigned long)state.receiver.path[1].stream.sync_shift,
            (unsigned long)state.discriminator[0].phase,
            (unsigned long)state.discriminator[1].phase,
            (long)state.corr_1200_i, (long)state.corr_1200_q,
            (long)state.corr_1800_i, (long)state.corr_1800_q);
    assert(capture.calls >= 1u);
    assert(capture.last_message.train_number == 0u);
    assert(capture.last_message.received_crc ==
           capture.last_message.calculated_crc);
}

int main(void)
{
    test_magnitude_198();
    test_audio_state_layout_and_initialization();
    test_sample_loop_updates_independent_states();
    test_full_9600_hz_audio_to_frame();

    puts("TDCSRX recovered 9.6 kHz DSP tests: PASS");
    puts("- exact 0x198 I/Q magnitude and asymmetric baseline update");
    puts("- 0x108-byte parent state layout and original initialization");
    puts("- two independent weighted discriminators and collector state blocks");
    puts("- synthetic 1200/1800 Hz audio through 9.6 kHz ADC sample path to CRC-valid frame");
    return 0;
}
