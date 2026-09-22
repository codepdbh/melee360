#include "hps.h"
#include "dsp_adpcm.h"

#include <string.h>

static const uint8_t k_hps_magic[8] = { ' ', 'H', 'A', 'L', 'P', 'S', 'T', 0 };

static uint32_t be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static uint16_t be16(const uint8_t *p)
{
    return (uint16_t)(((unsigned)p[0] << 8) | (unsigned)p[1]);
}

int m360_hps_parse_header(const uint8_t *file, uint32_t file_size,
                          struct m360_hps_header *header)
{
    uint32_t c;
    unsigned i;

    if (!file || !header || file_size < M360_HPS_HEADER_SIZE ||
        memcmp(file, k_hps_magic, sizeof(k_hps_magic)) != 0)
        return 0;
    memset(header, 0, sizeof(*header));
    header->sample_rate = be32(file + 8);
    header->channels = be32(file + 12);
    if (header->sample_rate == 0 || header->channels == 0 ||
        header->channels > M360_HPS_MAX_CHANNELS)
        return 0;
    for (c = 0; c < header->channels; ++c) {
        const uint8_t *ch = file + 0x10 + c * 0x38;
        if (be16(ch + 2) != 0)
            return 0;
        header->end_nibble[c] = be32(ch + 8);
        for (i = 0; i < 16; ++i)
            header->coefs[c][i] = (int16_t)be16(ch + 16 + i * 2);
    }
    return 1;
}

int m360_hps_parse_block(const uint8_t *file, uint32_t file_size,
                         uint32_t offset, uint32_t channels,
                         struct m360_hps_block *block)
{
    const uint8_t *p;
    uint32_t c;

    if (!file || !block || channels == 0 ||
        channels > M360_HPS_MAX_CHANNELS || offset < M360_HPS_HEADER_SIZE ||
        offset > file_size || file_size - offset < M360_HPS_BLOCK_HEADER_SIZE)
        return 0;
    p = file + offset;
    memset(block, 0, sizeof(*block));
    block->offset = offset;
    block->size = be32(p);
    block->end_nibble = be32(p + 4);
    block->next = be32(p + 8);
    if (block->size == 0 || block->size % (channels * 8u) != 0 ||
        block->size > file_size - offset - M360_HPS_BLOCK_HEADER_SIZE ||
        block->end_nibble >= block->size * 2u / channels ||
        block->end_nibble < 2u)
        return 0;
    for (c = 0; c < channels; ++c) {
        block->pred_scale[c] = be16(p + 12 + c * 8);
        block->hist1[c] = (int16_t)be16(p + 14 + c * 8);
        block->hist2[c] = (int16_t)be16(p + 16 + c * 8);
    }
    return 1;
}

uint32_t m360_hps_block_samples(uint32_t end_nibble)
{
    const uint32_t nibbles = end_nibble + 1u;
    const uint32_t tail = nibbles % 16u;
    return (nibbles / 16u) * 14u + (tail > 2u ? tail - 2u : 0u);
}

static int enter_block(struct m360_hps_stream *stream, uint32_t offset)
{
    struct m360_hps_block block;
    uint32_t c;
    const int linear = stream->blocks_entered != 0 &&
                       offset > stream->block.offset;

    if (!m360_hps_parse_block(stream->file, stream->file_size, offset,
                              stream->header.channels, &block))
        return 0;
    for (c = 0; c < stream->header.channels; ++c) {
        if (linear) {
            if (stream->hist1[c] == block.hist1[c] &&
                stream->hist2[c] == block.hist2[c])
                ++stream->history_matches;
            else
                ++stream->history_mismatches;
        }
        stream->hist1[c] = block.hist1[c];
        stream->hist2[c] = block.hist2[c];
    }
    if (stream->blocks_entered != 0 && !linear)
        ++stream->loops;
    stream->block = block;
    stream->frame = 0;
    stream->frame_count = (block.end_nibble + 16u) / 16u;
    stream->frame_len = 0;
    stream->frame_pos = 0;
    ++stream->blocks_entered;
    return 1;
}

int m360_hps_stream_open(struct m360_hps_stream *stream, const uint8_t *file,
                         uint32_t file_size)
{
    if (!stream)
        return 0;
    memset(stream, 0, sizeof(*stream));
    stream->file = file;
    stream->file_size = file_size;
    if (!m360_hps_parse_header(file, file_size, &stream->header))
        return 0;
    return enter_block(stream, M360_HPS_HEADER_SIZE);
}

static int decode_next_frame(struct m360_hps_stream *stream)
{
    const struct m360_hps_block *block = &stream->block;
    uint32_t channel_bytes;
    uint32_t count;
    uint32_t c;

    while (stream->frame >= stream->frame_count) {
        uint32_t next = block->next;
        if (next == M360_HPS_NO_NEXT)
            next = M360_HPS_HEADER_SIZE;
        if (!enter_block(stream, next))
            return 0;
    }
    channel_bytes = block->size / stream->header.channels;
    count = block->end_nibble + 1u - stream->frame * 16u;
    count = count >= 16u ? 14u : (count > 2u ? count - 2u : 0u);
    for (c = 0; c < stream->header.channels; ++c) {
        const uint8_t *frame = stream->file + block->offset +
                               M360_HPS_BLOCK_HEADER_SIZE + c * channel_bytes +
                               stream->frame * M360_DSP_FRAME_BYTES;
        m360_dsp_decode_frame(frame, stream->header.coefs[c],
                              &stream->hist1[c], &stream->hist2[c],
                              stream->pcm[c], count);
    }
    ++stream->frame;
    ++stream->frames_decoded;
    stream->frame_len = count;
    stream->frame_pos = 0;
    return 1;
}

size_t m360_hps_stream_read(struct m360_hps_stream *stream, int16_t *out,
                            size_t sample_frames)
{
    size_t done = 0;
    uint32_t c;

    if (!stream || !stream->file || !out)
        return 0;
    while (done < sample_frames) {
        if (stream->frame_pos >= stream->frame_len) {
            if (!decode_next_frame(stream))
                break;
            continue;
        }
        for (c = 0; c < stream->header.channels; ++c)
            *out++ = stream->pcm[c][stream->frame_pos];
        ++stream->frame_pos;
        ++done;
    }
    return done;
}
