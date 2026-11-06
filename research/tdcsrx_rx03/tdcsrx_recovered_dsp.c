#include "tdcsrx_recovered_dsp.h"

#include <stddef.h>
#include <string.h>

#define TDCSRX_FILTER_X_COEFF  0x0000140a
#define TDCSRX_FILTER_Y1_COEFF 0x00003246
#define TDCSRX_FILTER_Y2_COEFF (-6125)

static const int8_t kTone1200[8] = {
    127, 90, 0, -90, -127, -90, 0, 90
};

static const int8_t kTone1800[16] = {
    127, 49, -90, -117, 0, 117, 90, -49,
    -127, -49, 90, 117, 0, -117, -90, 49
};

/* Interpret a 32-bit two's-complement bit pattern without signed overflow. */
static int32_t from_u32(uint32_t value)
{
    int32_t result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

static int32_t wrap_add_i32(int32_t a, int32_t b)
{
    return from_u32((uint32_t)a + (uint32_t)b);
}

static int32_t wrap_sub_i32(int32_t a, int32_t b)
{
    return from_u32((uint32_t)a - (uint32_t)b);
}

static int32_t wrap_mul_i32(int32_t a, int32_t b)
{
    return from_u32((uint32_t)a * (uint32_t)b);
}

/* Explicit arithmetic right shift, matching the ARM ASRS instructions. */
static int32_t arithmetic_shift_right(int32_t value, unsigned int shift)
{
    uint32_t bits;

    if (shift == 0u)
        return value;
    if (shift >= 32u)
        return value < 0 ? -1 : 0;

    bits = (uint32_t)value;
    if (value >= 0)
        return (int32_t)(bits >> shift);

    return from_u32(~((~bits) >> shift));
}

static uint32_t magnitude_i32(int32_t value)
{
    const uint32_t bits = (uint32_t)value;
    return value < 0 ? (0u - bits) : bits;
}

static int32_t target_sample_filter(tdcsrx_audio_state_t *state,
                                    uint16_t adc_sample)
{
    int32_t dc_delta;
    int32_t input;
    int32_t x_difference;
    int32_t sum;

    /* dc += (((sample << 4) - dc) >> 6); */
    dc_delta = wrap_sub_i32((int32_t)adc_sample << 4,
                            state->dc_estimate);
    state->dc_estimate = wrap_add_i32(
        state->dc_estimate, arithmetic_shift_right(dc_delta, 6u));

    input = wrap_sub_i32((int32_t)adc_sample,
                         arithmetic_shift_right(state->dc_estimate, 4u));

    /*
     * Original biquad:
     * y = (0x140A*(x[n]-x[n-2]) + 0x3246*y[n-1] - 0x17ED*y[n-2]) >> 14
     *
     * 0xFFFFE813 is the signed -0x17ED literal from the binary.
     */
    x_difference = wrap_sub_i32(input, state->input_delay_2);
    sum = wrap_mul_i32(TDCSRX_FILTER_X_COEFF, x_difference);
    sum = wrap_add_i32(sum,
        wrap_mul_i32(TDCSRX_FILTER_Y1_COEFF, state->filter_y_1));
    sum = wrap_add_i32(sum,
        wrap_mul_i32(TDCSRX_FILTER_Y2_COEFF, state->filter_y_2));

    {
        int32_t filtered = arithmetic_shift_right(sum, 14u);

        state->input_delay_2 = state->input_delay_1;
        state->input_delay_1 = input;
        state->filter_y_2 = state->filter_y_1;
        state->filter_y_1 = filtered;
        return filtered;
    }
}

int32_t tdcsrx_audio_magnitude(int32_t i_value,
                               int32_t q_value,
                               int32_t *baseline)
{
    uint32_t absolute_i;
    uint32_t absolute_q;
    uint32_t maximum;
    uint32_t minimum;
    uint32_t magnitude;
    int32_t previous;
    int32_t next;

    if (baseline == NULL)
        return 0;

    absolute_i = magnitude_i32(i_value);
    absolute_q = magnitude_i32(q_value);
    maximum = absolute_i > absolute_q ? absolute_i : absolute_q;
    minimum = absolute_i > absolute_q ? absolute_q : absolute_i;

    /* target helper 0x198: max + min/4 + min/8 (arithmetic shifts). */
    magnitude = maximum + (minimum >> 2) + (minimum >> 3);
    previous = *baseline;

    /*
     * Original asymmetric smoother:
     *   rising:  baseline += max((magnitude-baseline)>>5, 16)
     *   falling: baseline -= baseline>>10
     */
    if (previous >= (int32_t)magnitude) {
        next = wrap_sub_i32(previous,
                            arithmetic_shift_right(previous, 10u));
    } else {
        const int32_t delta =
            wrap_sub_i32((int32_t)magnitude, previous);
        const int32_t step = arithmetic_shift_right(delta, 5u);
        next = wrap_add_i32(previous, step);
        if (next < 16)
            next = 16;
    }

    *baseline = next;
    return (int32_t)magnitude;
}

static void update_correlators(tdcsrx_audio_state_t *state,
                               int32_t filtered)
{
    const uint8_t phase_1200 = state->phase_1200 & 7u;
    const uint8_t phase_1800 = state->phase_1800 & 15u;
    int16_t *history = state->history[phase_1200];
    int32_t contribution;

    /* 1200 Hz I/Q: 8-sample table and a -90 degree quadrature tap. */
    contribution = arithmetic_shift_right(
        wrap_mul_i32(filtered, kTone1200[phase_1200]), 8u);
    state->corr_1200_i = wrap_add_i32(
        state->corr_1200_i,
        wrap_sub_i32(contribution, history[0]));
    history[0] = (int16_t)contribution;

    contribution = arithmetic_shift_right(
        wrap_mul_i32(filtered, kTone1200[(phase_1200 + 6u) & 7u]), 8u);
    state->corr_1200_q = wrap_add_i32(
        state->corr_1200_q,
        wrap_sub_i32(contribution, history[1]));
    history[1] = (int16_t)contribution;

    /* 1800 Hz I/Q: 16-entry third-harmonic table and +90 degree tap. */
    contribution = arithmetic_shift_right(
        wrap_mul_i32(filtered, kTone1800[phase_1800]), 8u);
    state->corr_1800_i = wrap_add_i32(
        state->corr_1800_i,
        wrap_sub_i32(contribution, history[2]));
    history[2] = (int16_t)contribution;

    contribution = arithmetic_shift_right(
        wrap_mul_i32(filtered, kTone1800[(phase_1800 + 4u) & 15u]), 8u);
    state->corr_1800_q = wrap_add_i32(
        state->corr_1800_q,
        wrap_sub_i32(contribution, history[3]));
    history[3] = (int16_t)contribution;

    state->phase_1200 = (uint8_t)((phase_1200 + 1u) & 7u);
    state->phase_1800 = (uint8_t)((phase_1800 + 1u) & 15u);
}

static void update_discriminator(tdcsrx_audio_state_t *state,
                                 unsigned int path_index,
                                 int32_t normalized_1200,
                                 int32_t normalized_1800,
                                 tdcsrx_frame_callback_t callback,
                                 void *context)
{
    tdcsrx_discriminator_t *lane = &state->discriminator[path_index];
    int32_t positive_term;
    int32_t negative_term;
    int32_t difference;
    int32_t phase;
    uint8_t current_bit;
    uint8_t previous_bit;

    /* Target instruction order: (1800 * baseline1200 * weight1800)
       minus (1200 * baseline1800 * weight1200). */
    positive_term = wrap_mul_i32(normalized_1800, lane->weight_1800);
    negative_term = wrap_mul_i32(normalized_1200, lane->weight_1200);
    difference = wrap_sub_i32(positive_term, negative_term);

    /* The target's ASRS/SUBS/LSRS sequence is exactly (difference > 0). */
    current_bit = (uint8_t)(difference > 0);
    previous_bit = (uint8_t)(lane->previous_difference > 0);

    phase = wrap_add_i32(lane->phase, 0x2000);
    if (current_bit != previous_bit) {
        const int32_t phase_error = wrap_sub_i32(0x9000, phase);
        phase = wrap_add_i32(
            phase, arithmetic_shift_right(phase_error, 2u));
    }

    lane->previous_difference = difference;

    if (phase >= 0x10000) {
        phase = wrap_sub_i32(phase, 0x10000);
        tdcsrx_receiver_feed_bit(&state->receiver,
                                 (uint8_t)path_index,
                                 current_bit,
                                 callback,
                                 context);
    }

    lane->phase = phase;
}

void tdcsrx_audio_init(tdcsrx_audio_state_t *state)
{
    if (state == NULL)
        return;

    memset(state, 0, sizeof(*state));
    state->dc_estimate = 0x8000;
    state->smooth_1200 = 16;
    state->smooth_1800 = 16;

    state->discriminator[0].weight_1200 = 2u;
    state->discriminator[0].weight_1800 = 3u;
    state->discriminator[1].weight_1200 = 3u;
    state->discriminator[1].weight_1800 = 2u;

    tdcsrx_receiver_init(&state->receiver);
}

void tdcsrx_audio_feed_sample(tdcsrx_audio_state_t *state,
                              uint16_t adc_sample,
                              tdcsrx_frame_callback_t callback,
                              void *context)
{
    int32_t filtered;
    int32_t magnitude_1200;
    int32_t magnitude_1800;
    int32_t normalized_1200;
    int32_t normalized_1800;

    if (state == NULL)
        return;

    filtered = target_sample_filter(state, adc_sample);
    update_correlators(state, filtered);

    magnitude_1200 = tdcsrx_audio_magnitude(
        state->corr_1200_i, state->corr_1200_q, &state->smooth_1200);
    magnitude_1800 = tdcsrx_audio_magnitude(
        state->corr_1800_i, state->corr_1800_q, &state->smooth_1800);

    /*
     * The target cross-normalizes each tone by the opposite tone's smoothed
     * baseline before applying the two alternative weight pairs:
     *
     *   normalized_1200 = magnitude_1200 * smooth_1800
     *   normalized_1800 = magnitude_1800 * smooth_1200
     *
     * These are 32-bit MULS operations in the original Thumb code.
     */
    normalized_1200 = wrap_mul_i32(magnitude_1200, state->smooth_1800);
    normalized_1800 = wrap_mul_i32(magnitude_1800, state->smooth_1200);

    update_discriminator(state, 0u, normalized_1200, normalized_1800,
                         callback, context);
    update_discriminator(state, 1u, normalized_1200, normalized_1800,
                         callback, context);
}

/* These checks pin this C state layout to the original 0x108-byte block. */
typedef char tdcsrx_assert_lane_a_offset[
    (offsetof(tdcsrx_audio_state_t, discriminator[0]) == 0x70u) ? 1 : -1];
typedef char tdcsrx_assert_lane_b_offset[
    (offsetof(tdcsrx_audio_state_t, discriminator[1]) == 0x7cu) ? 1 : -1];
typedef char tdcsrx_assert_receiver_offset[
    (offsetof(tdcsrx_audio_state_t, receiver) == 0x88u) ? 1 : -1];
typedef char tdcsrx_assert_path_size[
    (sizeof(tdcsrx_receiver_path_t) == 0x40u) ? 1 : -1];
typedef char tdcsrx_assert_state_size[
    (sizeof(tdcsrx_audio_state_t) == TDCSRX_DSP_STATE_SIZE) ? 1 : -1];
