#ifndef MELEE360_DSP_ADPCM_H
#define MELEE360_DSP_ADPCM_H

#include <stdint.h>

#define M360_DSP_FRAME_BYTES 8u
#define M360_DSP_FRAME_SAMPLES 14u

void m360_dsp_decode_frame(const uint8_t *frame, const int16_t *coefs,
                           int16_t *hist1, int16_t *hist2, int16_t *out,
                           unsigned count);
int16_t m360_dsp_decode_nibble(uint8_t pred_scale, int nibble,
                               const int16_t *coefs,
                               int16_t *hist1, int16_t *hist2);

/* AX ADPCM addresses are absolute nibble offsets; end_nibble is inclusive.
 * pred_scale and histories describe the state at start_nibble. */
unsigned m360_dsp_decode_range(const uint8_t *data, unsigned bytes,
                               uint32_t start_nibble, uint32_t end_nibble,
                               uint8_t pred_scale, const int16_t *coefs,
                               int16_t *hist1, int16_t *hist2,
                               int16_t *out, unsigned capacity);

#endif
