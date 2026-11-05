#include "tdcsrx_recovered_stream.h"

#include <stddef.h>

#define TDCSRX_SYNC_WORD_31_INVERTED 0x722bda60u
#define TDCSRX_SYNC_WORD_INVERTED    0xf22bda60u
#define TDCSRX_SYNC_MASK_31          0x7fffffffu

static unsigned int popcount32(uint32_t value)
{
    /* Same SWAR population-count sequence as the target routine at 0x3e4. */
    value -= (value >> 1) & 0x55555555u;
    value = (value & 0x33333333u) + ((value >> 2) & 0x33333333u);
    value = (value + (value >> 4)) & 0x0f0f0f0fu;
    value *= 0x01010101u;
    return (unsigned int)(value >> 24);
}

static bool find_sync(uint32_t rolling, bool *inverted)
{
    uint32_t rolling31 = (rolling << 1) >> 1;
    unsigned int d_normal31 =
        popcount32(TDCSRX_SYNC_WORD ^ rolling31);
    unsigned int d_inverted31 =
        popcount32(TDCSRX_SYNC_WORD_31_INVERTED ^ rolling31);
    unsigned int d_normal32 =
        popcount32(TDCSRX_SYNC_WORD ^ rolling);
    unsigned int d_inverted32 =
        popcount32(TDCSRX_SYNC_WORD_INVERTED ^ rolling);
    unsigned int best = d_normal31;
    bool best_inverted = false;

    /* Preserve the original target's strict-greater comparisons and tie order. */
    if (best > d_inverted31) {
        best = d_inverted31;
        best_inverted = true;
    }
    if (best > d_normal32) {
        best = d_normal32;
        best_inverted = false;
    }
    if (best > d_inverted32) {
        best = d_inverted32;
        best_inverted = true;
    }

    if (best > TDCSRX_SYNC_MAX_ERRORS)
        return false;

    *inverted = best_inverted;
    return true;
}

void tdcsrx_stream_init(tdcsrx_stream_state_t *state)
{
    if (state == NULL)
        return;

    state->sync_shift = 0u;
    state->codeword_shift = 0u;
    state->codeword_bit_count = 0u;
    state->synchronized = false;
    state->inverted = false;
}

tdcsrx_stream_event_t tdcsrx_stream_feed_bit(tdcsrx_stream_state_t *state,
                                             uint8_t input_bit,
                                             uint16_t *data_word,
                                             int *corrected_bit)
{
    uint32_t rolling;
    uint8_t bit;

    if (state == NULL)
        return TDCSRX_STREAM_NO_EVENT;

    bit = (uint8_t)(input_bit & 1u);
    rolling = (state->sync_shift << 1) | bit;
    state->sync_shift = rolling;

    if (!state->synchronized) {
        bool inverted = false;
        if (!find_sync(rolling, &inverted))
            return TDCSRX_STREAM_NO_EVENT;

        state->synchronized = true;
        state->inverted = inverted;
        state->codeword_shift = 0u;
        state->codeword_bit_count = 0u;
        if (corrected_bit != NULL)
            *corrected_bit = -1;
        return TDCSRX_STREAM_SYNC_ACQUIRED;
    }

    if (state->inverted)
        bit ^= 1u;

    state->codeword_shift =
        ((state->codeword_shift << 1) | bit) & 0x03ffffffu;
    ++state->codeword_bit_count;

    if (state->codeword_bit_count < TDCSRX_CODEWORD_BITS)
        return TDCSRX_STREAM_NO_EVENT;

    {
        int corrected = -1;
        uint16_t decoded =
            tdcsrx_original_bch_decode(state->codeword_shift, &corrected);

        state->codeword_shift = 0u;
        state->codeword_bit_count = 0u;

        if (data_word != NULL)
            *data_word = decoded;
        if (corrected_bit != NULL)
            *corrected_bit = corrected;
    }

    return TDCSRX_STREAM_CODEWORD_READY;
}
