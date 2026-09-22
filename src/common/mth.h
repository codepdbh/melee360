#ifndef MELEE360_MTH_H
#define MELEE360_MTH_H

#include <stddef.h>
#include <stdint.h>

#define M360_MTH_HEADER_SIZE 0x40u
#define M360_MTH_FRAME_PREFIX 4u

struct m360_mth_header {
    uint32_t version;
    uint32_t buffer_size;
    uint32_t width;
    uint32_t height;
    uint32_t frame_rate;
    uint32_t frame_count;
    uint32_t first_frame;
    uint32_t frame_offsets;
    uint32_t first_frame_size;
};

int m360_mth_parse_header(const uint8_t *data, size_t size,
                          struct m360_mth_header *header);
uint32_t m360_mth_next_frame_size(const uint8_t *frame);
uint32_t m360_mth_frame_for_tick(const uint32_t *rate_table, uint32_t tick);
uint32_t m360_mth_ticks_for_frames(const uint32_t *rate_table,
                                   uint32_t frames);

#endif
