#ifndef MELEE360_HPS_H
#define MELEE360_HPS_H

#include <stddef.h>
#include <stdint.h>

#define M360_HPS_HEADER_SIZE 0x80u
#define M360_HPS_BLOCK_HEADER_SIZE 0x20u
#define M360_HPS_MAX_CHANNELS 2u
#define M360_HPS_NO_NEXT 0xFFFFFFFFu

struct m360_hps_header {
    uint32_t sample_rate;
    uint32_t channels;
    uint32_t end_nibble[M360_HPS_MAX_CHANNELS];
    int16_t coefs[M360_HPS_MAX_CHANNELS][16];
};

struct m360_hps_block {
    uint32_t offset;
    uint32_t size;
    uint32_t end_nibble;
    uint32_t next;
    uint16_t pred_scale[M360_HPS_MAX_CHANNELS];
    int16_t hist1[M360_HPS_MAX_CHANNELS];
    int16_t hist2[M360_HPS_MAX_CHANNELS];
};

struct m360_hps_stream {
    const uint8_t *file;
    uint32_t file_size;
    struct m360_hps_header header;
    struct m360_hps_block block;
    uint32_t frame;
    uint32_t frame_count;
    uint32_t frame_len;
    uint32_t frame_pos;
    int16_t hist1[M360_HPS_MAX_CHANNELS];
    int16_t hist2[M360_HPS_MAX_CHANNELS];
    int16_t pcm[M360_HPS_MAX_CHANNELS][14];
    uint32_t blocks_entered;
    uint32_t frames_decoded;
    uint32_t loops;
    uint32_t history_matches;
    uint32_t history_mismatches;
};

int m360_hps_parse_header(const uint8_t *file, uint32_t file_size,
                          struct m360_hps_header *header);
int m360_hps_parse_block(const uint8_t *file, uint32_t file_size,
                         uint32_t offset, uint32_t channels,
                         struct m360_hps_block *block);
uint32_t m360_hps_block_samples(uint32_t end_nibble);
int m360_hps_stream_open(struct m360_hps_stream *stream, const uint8_t *file,
                         uint32_t file_size);
size_t m360_hps_stream_read(struct m360_hps_stream *stream, int16_t *out,
                            size_t sample_frames);

#endif
