#include "tdcsrx_recovered_dsp.h"

#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    unsigned int calls;
    unsigned int path_mask;
    unsigned int symbol_count[2];
    unsigned int symbol_ones[2];
    char bits[2][4096];
    tdcsrx_message_t last;
} capture_t;

static void on_symbol(void *context, uint8_t path, uint8_t bit)
{
    capture_t *capture = (capture_t *)context;
    if (path >= 2u)
        return;
    ++capture->symbol_count[path];
    capture->symbol_ones[path] += bit != 0u;
    if (capture->symbol_count[path] < sizeof(capture->bits[path]) - 1u) {
        capture->bits[path][capture->symbol_count[path] - 1u] =
            bit != 0u ? '1' : '0';
        capture->bits[path][capture->symbol_count[path]] = '\0';
    }
}

static void write_bitstreams(const capture_t *capture, double scale)
{
    unsigned int path;
    char filename[80];

    for (path = 0u; path < 2u; ++path) {
        FILE *file;
        (void)snprintf(filename, sizeof(filename),
                       "tdcsrx_path%u_scale_%03u.bits",
                       path, (unsigned int)(scale * 100.0));
        file = fopen(filename, "wb");
        if (file != NULL) {
            (void)fwrite(capture->bits[path], 1u,
                         strlen(capture->bits[path]), file);
            (void)fputc('\n', file);
            fclose(file);
        }
    }
}

static void on_frame(void *context, uint8_t path,
                     const tdcsrx_message_t *message)
{
    capture_t *capture = (capture_t *)context;
    ++capture->calls;
    capture->path_mask |= 1u << path;
    capture->last = *message;
    printf("FRAME path=%u train=%lu speed=%u loco_type=%u serial=%u "
           "mileage=%lu crc=%04X/%04X\n",
           (unsigned int)path,
           (unsigned long)message->train_number,
           (unsigned int)message->speed_kmh,
           (unsigned int)message->locomotive_type,
           (unsigned int)message->locomotive_serial,
           (unsigned long)message->mileage_value22,
           (unsigned int)message->received_crc,
           (unsigned int)message->calculated_crc);
}

static int read_samples(const char *path, uint16_t **samples_out,
                        size_t *count_out)
{
    FILE *file = fopen(path, "rb");
    long size;
    uint8_t *bytes;
    uint16_t *samples;
    size_t count;
    size_t i;

    if (file == NULL) {
        fprintf(stderr, "cannot open %s: %s\n", path, strerror(errno));
        return 0;
    }
    if (fseek(file, 0, SEEK_END) != 0 || (size = ftell(file)) < 0 ||
        fseek(file, 0, SEEK_SET) != 0 || (size & 1L) != 0) {
        fprintf(stderr, "invalid sample file size\n");
        fclose(file);
        return 0;
    }
    bytes = (uint8_t *)malloc((size_t)size);
    if (bytes == NULL) {
        fclose(file);
        return 0;
    }
    if (fread(bytes, 1u, (size_t)size, file) != (size_t)size) {
        fprintf(stderr, "short read from sample file\n");
        free(bytes);
        fclose(file);
        return 0;
    }
    fclose(file);

    count = (size_t)size / 2u;
    samples = (uint16_t *)malloc(count * sizeof(*samples));
    if (samples == NULL) {
        free(bytes);
        return 0;
    }
    for (i = 0u; i < count; ++i)
        samples[i] = (uint16_t)((uint16_t)bytes[2u * i] |
                               ((uint16_t)bytes[2u * i + 1u] << 8));

    free(bytes);
    *samples_out = samples;
    *count_out = count;
    return 1;
}

int main(int argc, char **argv)
{
    static const double scales[] = {0.5, 0.75, 1.0, 1.25, 1.5, 2.0};
    uint16_t *samples = NULL;
    size_t sample_count = 0u;
    size_t scale_index;
    int found = 0;

    if (argc != 2) {
        fprintf(stderr, "usage: %s SAMPLE_ADC_U16LE.bin\n", argv[0]);
        return 2;
    }
    if (!read_samples(argv[1], &samples, &sample_count))
        return 2;

    printf("real-wav TDCSRX probe: samples=%lu rate=%u expected_train=319 "
           "expected_crc=B27A\n",
           (unsigned long)sample_count, (unsigned int)TDCSRX_DSP_SAMPLE_RATE);

    for (scale_index = 0u;
         scale_index < sizeof(scales) / sizeof(scales[0]);
         ++scale_index) {
        tdcsrx_audio_state_t state;
        capture_t capture;
        size_t i;
        FILE *trace = NULL;

        memset(&capture, 0, sizeof(capture));
        tdcsrx_audio_init(&state);
        printf("SCALE %.2f\n", scales[scale_index]);
        if (scales[scale_index] == 1.0) {
            trace = fopen("tdcsrx_real_wav_trace.csv", "w");
            if (trace != NULL) {
                fputs("sample,adc,filter_y1,corr1200_i,corr1200_q,"
                      "corr1800_i,corr1800_q,smooth1200,smooth1800,"
                      "phase1200,phase1800,diff_a,phase_a,diff_b,phase_b\n",
                      trace);
            }
        }

        for (i = 0u; i < sample_count; ++i) {
            int32_t centred = (int32_t)samples[i] - 2048;
            int32_t adjusted = 2048 + (int32_t)lround((double)centred * scales[scale_index]);
            if (adjusted < 0)
                adjusted = 0;
            else if (adjusted > 4095)
                adjusted = 4095;

            tdcsrx_audio_feed_sample_traced(
                &state, (uint16_t)adjusted, on_symbol, &capture,
                on_frame, &capture);
            if (trace != NULL) {
                (void)fprintf(trace,
                    "%lu,%d,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%u,%u,%ld,%ld,%ld,%ld\n",
                    (unsigned long)i, adjusted,
                    (long)state.filter_y_1,
                    (long)state.corr_1200_i, (long)state.corr_1200_q,
                    (long)state.corr_1800_i, (long)state.corr_1800_q,
                    (long)state.smooth_1200, (long)state.smooth_1800,
                    (unsigned int)state.phase_1200,
                    (unsigned int)state.phase_1800,
                    (long)state.discriminator[0].previous_difference,
                    (long)state.discriminator[0].phase,
                    (long)state.discriminator[1].previous_difference,
                    (long)state.discriminator[1].phase);
            }
        }
        if (trace != NULL)
            fclose(trace);

        write_bitstreams(&capture, scales[scale_index]);
        printf("SUMMARY calls=%u paths=0x%X symbols=%u/%u ones=%u/%u "
               "smooth=%ld/%ld diff=%ld/%ld sync=%u/%u\n",
               capture.calls, capture.path_mask,
               capture.symbol_count[0], capture.symbol_count[1],
               capture.symbol_ones[0], capture.symbol_ones[1],
               (long)state.smooth_1200, (long)state.smooth_1800,
               (long)state.discriminator[0].previous_difference,
               (long)state.discriminator[1].previous_difference,
               state.receiver.path[0].stream.synchronized,
               state.receiver.path[1].stream.synchronized);

        if (capture.calls != 0u &&
            capture.last.train_number == 319u &&
            capture.last.received_crc == 0xb27au &&
            capture.last.calculated_crc == 0xb27au &&
            capture.last.speed_kmh == 20u) {
            found = 1;
            printf("REAL_WAV_DECODER_PASS scale=%.2f pathmask=0x%X\n",
                   scales[scale_index], capture.path_mask);
            break;
        }
    }

    free(samples);
    if (!found) {
        fprintf(stderr, "REAL_WAV_DECODER_FAIL: no frame matched X319, "
                        "20 km/h, CRC B27A at tested ADC scales\n");
        return 1;
    }
    return 0;
}
