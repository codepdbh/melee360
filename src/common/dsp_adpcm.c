#include "dsp_adpcm.h"

static int16_t clamp16(int64_t value)
{
    if (value > 32767)
        return 32767;
    if (value < -32768)
        return -32768;
    return (int16_t)value;
}

void m360_dsp_decode_frame(const uint8_t *frame, const int16_t *coefs,
                           int16_t *hist1, int16_t *hist2, int16_t *out,
                           unsigned count)
{
    const unsigned predictor = (frame[0] >> 4) & 7u;
    const int64_t scale = (int64_t)1 << (frame[0] & 15u);
    const int64_t c0 = coefs[predictor * 2u];
    const int64_t c1 = coefs[predictor * 2u + 1u];
    int64_t h1 = *hist1;
    int64_t h2 = *hist2;
    unsigned i;

    if (count > M360_DSP_FRAME_SAMPLES)
        count = M360_DSP_FRAME_SAMPLES;
    for (i = 0; i < count; ++i) {
        const uint8_t byte = frame[1u + i / 2u];
        int nibble = (i & 1u) ? (byte & 15) : (byte >> 4);
        int64_t acc;
        int16_t sample;

        if (nibble >= 8)
            nibble -= 16;
        acc = (int64_t)nibble * scale * 2048 + 1024 + c0 * h1 + c1 * h2;
        sample = clamp16(acc >> 11);
        h2 = h1;
        h1 = sample;
        out[i] = sample;
    }
    *hist1 = (int16_t)h1;
    *hist2 = (int16_t)h2;
}
