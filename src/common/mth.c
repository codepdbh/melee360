#include "mth.h"

static uint32_t read_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

int m360_mth_parse_header(const uint8_t *data, size_t size,
                          struct m360_mth_header *header)
{
    if (!data || size < M360_MTH_HEADER_SIZE || data[0] != 'M' ||
        data[1] != 'T' || data[2] != 'H' || data[3] != 'P')
        return 0;
    header->version = read_be32(data + 0x08);
    header->buffer_size = read_be32(data + 0x0C);
    header->width = read_be32(data + 0x10);
    header->height = read_be32(data + 0x14);
    header->frame_rate = read_be32(data + 0x18);
    header->frame_count = read_be32(data + 0x1C);
    header->first_frame = read_be32(data + 0x20);
    header->frame_offsets = read_be32(data + 0x24);
    header->first_frame_size = read_be32(data + 0x28);
    return header->version <= 2 && header->width && header->height &&
           header->frame_count && header->first_frame >= M360_MTH_HEADER_SIZE &&
           header->first_frame_size > M360_MTH_FRAME_PREFIX;
}

uint32_t m360_mth_next_frame_size(const uint8_t *frame)
{
    return read_be32(frame);
}

uint32_t m360_mth_frame_for_tick(const uint32_t *rate_table, uint32_t tick)
{
    uint32_t frame = 0;
    if (!rate_table)
        return tick;
    for (;; rate_table += 2) {
        const uint32_t total = rate_table[0] * rate_table[1];
        if (tick < total || !rate_table[1])
            return frame + (rate_table[1] ? tick / rate_table[1] : 0);
        frame += rate_table[0];
        tick -= total;
    }
}

uint32_t m360_mth_ticks_for_frames(const uint32_t *rate_table,
                                   uint32_t frames)
{
    uint32_t ticks = 0;
    if (!rate_table)
        return frames;
    for (;; rate_table += 2) {
        if (frames <= rate_table[0] || !rate_table[1])
            return ticks + frames * rate_table[1];
        ticks += rate_table[0] * rate_table[1];
        frames -= rate_table[0];
    }
}
