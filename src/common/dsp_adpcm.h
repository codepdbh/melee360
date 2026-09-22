#ifndef MELEE360_DSP_ADPCM_H
#define MELEE360_DSP_ADPCM_H

#include <stdint.h>

#define M360_DSP_FRAME_BYTES 8u
#define M360_DSP_FRAME_SAMPLES 14u

void m360_dsp_decode_frame(const uint8_t *frame, const int16_t *coefs,
                           int16_t *hist1, int16_t *hist2, int16_t *out,
                           unsigned count);

#endif
