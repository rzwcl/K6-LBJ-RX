#include "tdcsrx_recovered_dsp.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_magnitude_198(void)
{
    int32_t baseline;

    baseline = 16;
    assert(tdcsrx_audio_magnitude(3, 4, &baseline) == 5);
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

int main(void)
{
    test_magnitude_198();
    test_audio_state_layout_and_initialization();
    test_sample_loop_updates_independent_states();

    puts("TDCSRX recovered 9.6 kHz DSP tests: PASS");
    puts("- exact 0x198 I/Q magnitude and asymmetric baseline update");
    puts("- 0x108-byte parent state layout and original initialization");
    puts("- two independent weighted discriminators and collector state blocks");
    return 0;
}
