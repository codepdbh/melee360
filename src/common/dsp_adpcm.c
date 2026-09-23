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
    unsigned i;

    if (count > M360_DSP_FRAME_SAMPLES)
        count = M360_DSP_FRAME_SAMPLES;
    for (i = 0; i < count; ++i) {
        const uint8_t byte = frame[1u + i / 2u];
        const int nibble = (i & 1u) ? (byte & 15) : (byte >> 4);
        out[i] = m360_dsp_decode_nibble(frame[0], nibble, coefs, hist1, hist2);
    }
}

int16_t m360_dsp_decode_nibble(uint8_t pred_scale, int nibble,
                               const int16_t *coefs,
                               int16_t *hist1, int16_t *hist2)
{
    const unsigned predictor = (pred_scale >> 4) & 7u;
    const int64_t scale = (int64_t)1 << (pred_scale & 15u);
    const int64_t c0 = coefs[predictor * 2u];
    const int64_t c1 = coefs[predictor * 2u + 1u];
    const int64_t acc = (int64_t)(nibble >= 8 ? nibble - 16 : nibble) *
                        scale * 2048 + 1024 + c0 * *hist1 + c1 * *hist2;
    const int16_t sample = clamp16(acc >> 11);
    *hist2 = *hist1;
    *hist1 = sample;
    return sample;
}

unsigned m360_dsp_decode_range(const uint8_t *data, unsigned bytes,
                               uint32_t start_nibble, uint32_t end_nibble,
                               uint8_t pred_scale, const int16_t *coefs,
                               int16_t *hist1, int16_t *hist2,
                               int16_t *out, unsigned capacity)
{
    uint32_t current = start_nibble;
    unsigned count = 0;
    if (!data || !coefs || !hist1 || !hist2 || !out ||
        current > end_nibble || (end_nibble >> 1) >= bytes)
        return 0;
    while (count < capacity) {
        unsigned nibble;
        if ((current & 15u) < 2u) {
            pred_scale = data[(current & ~15u) >> 1];
            if (end_nibble - current < 2u - (current & 15u))
                break;
            current += 2u - (current & 15u);
        }
        nibble = data[current >> 1];
        nibble = (current & 1u) ? (nibble & 15u) : (nibble >> 4);
        out[count++] = m360_dsp_decode_nibble(pred_scale, (int)nibble,
                                              coefs, hist1, hist2);
        if (current == end_nibble)
            break;
        ++current;
    }
    return count;
}
