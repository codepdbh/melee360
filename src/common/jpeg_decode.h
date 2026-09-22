#ifndef MELEE360_JPEG_DECODE_H
#define MELEE360_JPEG_DECODE_H

#include <stddef.h>
#include <stdint.h>

#define M360_JPEG_THP 1u
#define M360_JPEG_MAX_COMPONENTS 3u

struct m360_jpeg_info {
    unsigned width;
    unsigned height;
    unsigned components;
    unsigned h[M360_JPEG_MAX_COMPONENTS];
    unsigned v[M360_JPEG_MAX_COMPONENTS];
    unsigned plane_width[M360_JPEG_MAX_COMPONENTS];
    unsigned plane_height[M360_JPEG_MAX_COMPONENTS];
    unsigned restart_interval;
};

int m360_jpeg_read_info(const uint8_t *data, size_t size,
                        struct m360_jpeg_info *info);
int m360_jpeg_decode(const uint8_t *data, size_t size, unsigned flags,
                     uint8_t *const planes[M360_JPEG_MAX_COMPONENTS],
                     const unsigned pitches[M360_JPEG_MAX_COMPONENTS],
                     struct m360_jpeg_info *info);
void m360_jpeg_idct_islow(const int32_t in[64], uint8_t *out,
                          unsigned pitch);

#endif
