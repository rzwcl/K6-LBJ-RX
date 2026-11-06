#ifndef TDCSRX_RECOVERED_DSP_H
#define TDCSRX_RECOVERED_DSP_H

#include <stdint.h>

#include "tdcsrx_recovered_receiver.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TDCSRX_DSP_STATE_SIZE 0x108u
#define TDCSRX_DSP_SAMPLE_RATE 9600u
#define TDCSRX_DSP_BIT_RATE 1200u

typedef void (*tdcsrx_symbol_callback_t)(void *context,
                                         uint8_t path_index,
                                         uint8_t bit);

typedef struct {
    int32_t previous_difference;
    int32_t phase;
    uint8_t weight_1200;
    uint8_t weight_1800;
    uint8_t reserved[2];
} tdcsrx_discriminator_t;

/*
 * Parent state layout reconstructed from TDCS RX 0.3 Thumb instructions.
 * The receiver objects intentionally begin at offsets 0x88 and 0xC8,
 * matching the two collector-state pointers passed by the original DSP.
 */
typedef struct {
    int32_t dc_estimate;                 /* +0x00 */
    int32_t input_delay_1;               /* +0x04 */
    int32_t input_delay_2;               /* +0x08 */
    int32_t filter_y_1;                  /* +0x0C */
    int32_t filter_y_2;                  /* +0x10 */

    int32_t corr_1200_i;                 /* +0x14 */
    int32_t corr_1200_q;                 /* +0x18 */
    int32_t corr_1800_i;                 /* +0x1C */
    int32_t corr_1800_q;                 /* +0x20 */

    int32_t smooth_1200;                 /* +0x24 */
    int32_t smooth_1800;                 /* +0x28 */
    uint8_t phase_1200;                 /* +0x2C */
    uint8_t phase_1800;                 /* +0x2D */

    int16_t history[8][4];               /* +0x2E: 8-sample sliding window */
    uint8_t history_alignment[2];        /* +0x6E..+0x6F */

    tdcsrx_discriminator_t discriminator[2]; /* +0x70 and +0x7C */
    tdcsrx_receiver_t receiver;          /* +0x88: two 64-byte collectors */
} tdcsrx_audio_state_t;

/* Initialize the same 0x108-byte state block and constants as original code. */
void tdcsrx_audio_init(tdcsrx_audio_state_t *state);

/* Exact helper at original code offset 0x198: I/Q magnitude + adaptive baseline. */
int32_t tdcsrx_audio_magnitude(int32_t i_value,
                               int32_t q_value,
                               int32_t *baseline);

/*
 * Feed one 12-bit MCU ADC sample captured at 9,600 samples/s.
 * This reconstructs the original sample-to-bit detector, including both
 * independent 1200/1800 discriminators and both 26-bit frame collectors.
 */
void tdcsrx_audio_feed_sample(tdcsrx_audio_state_t *state,
                              uint16_t adc_sample,
                              tdcsrx_frame_callback_t callback,
                              void *context);

/* Test/diagnostic variant: observe each target slicer output before collection. */
void tdcsrx_audio_feed_sample_traced(tdcsrx_audio_state_t *state,
                                     uint16_t adc_sample,
                                     tdcsrx_symbol_callback_t symbol_callback,
                                     void *symbol_context,
                                     tdcsrx_frame_callback_t frame_callback,
                                     void *frame_context);

#ifdef __cplusplus
}
#endif

#endif /* TDCSRX_RECOVERED_DSP_H */
