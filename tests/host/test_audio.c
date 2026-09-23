#include "dsp_adpcm.h"
#include "gcm.h"
#include "hps.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRACK "audio/menu01.hps"
#define PREVIEW_SECONDS 20u
#define SOAK_SECONDS 70u

static int failures;

static void check(int condition, const char *what)
{
    if (!condition) {
        printf("FAIL: %s\n", what);
        ++failures;
    }
}

static int test_hand_frame(void)
{
    static const uint8_t frame[8] = { 0x21, 0x7F, 0x80, 0x12, 0x3F,
                                      0x00, 0xFF, 0xE9 };
    static const int16_t expected[14] = { 74, 10, -29, -17, 1,  9,  10,
                                          1,  -2, -1,  -2,  -3, -5, -16 };
    static const uint8_t loud[8] = { 0x0F, 0x77, 0x77, 0x88, 0x88,
                                     0x00, 0x00, 0x00 };
    int16_t coefs[16];
    int16_t out[14];
    int16_t h1 = 100;
    int16_t h2 = -40;
    unsigned i;
    int ok = 1;

    memset(coefs, 0, sizeof(coefs));
    coefs[4] = 1024;
    coefs[5] = -512;
    m360_dsp_decode_frame(frame, coefs, &h1, &h2, out, 14);
    for (i = 0; i < 14; ++i)
        ok &= out[i] == expected[i];
    ok &= h1 == -16 && h2 == -5;
    printf("hand frame: %d %d %d ... %d %d (history %d %d)\n", out[0], out[1],
           out[2], out[12], out[13], h1, h2);
    check(ok, "hand-computed ADPCM frame");

    h1 = 100;
    h2 = -40;
    check(m360_dsp_decode_range(frame, sizeof(frame), 2, 15, frame[0],
                                coefs, &h1, &h2, out, 14) == 14 &&
              memcmp(out, expected, sizeof(expected)) == 0,
          "SSM nibble 2 starts after header; end address is inclusive");
    h1 = 100;
    h2 = -40;
    check(m360_dsp_decode_range(frame, sizeof(frame), 2, 2, frame[0],
                                coefs, &h1, &h2, out, 14) == 1 && out[0] == 74,
          "single-nibble voice retains its last sample");
    check(m360_dsp_decode_range(frame, sizeof(frame), 2, 16, frame[0],
                                coefs, &h1, &h2, out, 14) == 0,
          "SSM address outside sample data is rejected");

    h1 = 0;
    h2 = 0;
    m360_dsp_decode_frame(loud, coefs, &h1, &h2, out, 14);
    check(out[0] == 32767 && out[3] == 32767 && out[4] == -32768 &&
              out[7] == -32768,
          "ADPCM clamps to s16");
    return ok;
}

static int cmp_u32(const void *a, const void *b)
{
    const unsigned x = *(const unsigned *)a;
    const unsigned y = *(const unsigned *)b;
    return x < y ? -1 : (x > y ? 1 : 0);
}

static void put_le16(unsigned char *p, unsigned v)
{
    p[0] = (unsigned char)(v & 255u);
    p[1] = (unsigned char)((v >> 8) & 255u);
}

static void put_le32(unsigned char *p, unsigned v)
{
    put_le16(p, v & 0xFFFFu);
    put_le16(p + 2, v >> 16);
}

static int write_wav(const char *path, const int16_t *pcm, unsigned frames,
                     unsigned channels, unsigned rate)
{
    unsigned char header[44];
    const unsigned bytes = frames * channels * 2u;
    FILE *out = fopen(path, "wb");
    unsigned i;

    if (!out)
        return 0;
    memcpy(header, "RIFF", 4);
    put_le32(header + 4, 36u + bytes);
    memcpy(header + 8, "WAVEfmt ", 8);
    put_le32(header + 16, 16);
    put_le16(header + 20, 1);
    put_le16(header + 22, channels);
    put_le32(header + 24, rate);
    put_le32(header + 28, rate * channels * 2u);
    put_le16(header + 32, channels * 2u);
    put_le16(header + 34, 16);
    memcpy(header + 36, "data", 4);
    put_le32(header + 40, bytes);
    fwrite(header, 1, sizeof(header), out);
    for (i = 0; i < frames * channels; ++i) {
        unsigned char s[2];
        put_le16(s, (unsigned)(uint16_t)pcm[i]);
        fwrite(s, 1, 2, out);
    }
    return fclose(out) == 0;
}

int main(int argc, char **argv)
{
    struct m360_gcm gcm;
    struct m360_gcm_file file;
    struct m360_hps_header header;
    struct m360_hps_block block;
    struct m360_hps_stream stream;
    FILE *image;
    uint8_t *data;
    int16_t *pcm;
    unsigned *deltas;
    unsigned preview_frames, got, i, c, blocks = 0, loop_block = 0;
    unsigned long long nibble_sum = 0, samples_total = 0, loop_sample = 0;
    uint32_t offset, ps_mismatch = 0;
    double sum_sq[2] = { 0, 0 };
    unsigned peak[2] = { 0, 0 }, clipped = 0, small = 0, delta_count;

    if (argc < 2) {
        fprintf(stderr, "usage: %s melee.iso [preview.wav]\n", argv[0]);
        return 2;
    }
    test_hand_frame();

    image = fopen(argv[1], "rb");
    check(image != NULL, "open ISO");
    if (!image)
        return 1;
    check(m360_gcm_mount(&gcm, image) == 0, "mount FST");
    check(m360_gcm_find(&gcm, TRACK, &file), "find " TRACK);
    if (failures)
        return 1;
    printf("file: /%s size=%u disc_offset=0x%X\n", TRACK, file.size,
           file.offset);
    data = (uint8_t *)malloc(file.size);
    check(data && m360_gcm_read(&gcm, &file, 0, data, file.size) == file.size,
          "read track");
    m360_gcm_unmount(&gcm);
    fclose(image);
    if (failures)
        return 1;

    check(m360_hps_parse_header(data, file.size, &header), "HALPST header");
    printf("header: rate=%u channels=%u end_nibble=0x%X/0x%X\n",
           header.sample_rate, header.channels, header.end_nibble[0],
           header.end_nibble[1]);
    check(header.sample_rate == 32000u, "sample rate 32000");
    check(header.channels == 2u, "stereo");

    offset = M360_HPS_HEADER_SIZE;
    for (;;) {
        if (!m360_hps_parse_block(data, file.size, offset, header.channels,
                                  &block)) {
            check(0, "block parse");
            break;
        }
        for (c = 0; c < header.channels; ++c) {
            const uint8_t first = data[offset + M360_HPS_BLOCK_HEADER_SIZE +
                                       c * (block.size / header.channels)];
            if (block.pred_scale[c] != first)
                ++ps_mismatch;
        }
        nibble_sum += block.end_nibble + 1u;
        samples_total += m360_hps_block_samples(block.end_nibble);
        ++blocks;
        if (block.next == M360_HPS_NO_NEXT || block.next <= offset)
            break;
        offset = block.next;
    }
    check(blocks > 1 && block.next != M360_HPS_NO_NEXT, "looping chain");
    check(block.offset + M360_HPS_BLOCK_HEADER_SIZE + block.size == file.size,
          "last block ends at EOF");
    check(nibble_sum - 1u == header.end_nibble[0],
          "sum of block nibbles matches header end address");
    check(ps_mismatch == 0, "block pred/scale equals first frame header");
    offset = M360_HPS_HEADER_SIZE;
    while (offset != block.next) {
        struct m360_hps_block b;
        m360_hps_parse_block(data, file.size, offset, header.channels, &b);
        loop_sample += m360_hps_block_samples(b.end_nibble);
        ++loop_block;
        offset = b.next;
    }
    printf("blocks=%u samples=%llu (%.3f s) nibbles=%llu loop->block %u at "
           "sample %llu (%.3f s)\n",
           blocks, samples_total, (double)samples_total / header.sample_rate,
           nibble_sum, loop_block, loop_sample,
           (double)loop_sample / header.sample_rate);

    preview_frames = PREVIEW_SECONDS * header.sample_rate;
    pcm = (int16_t *)malloc((size_t)SOAK_SECONDS * header.sample_rate *
                            header.channels * sizeof(int16_t));
    check(pcm != NULL && m360_hps_stream_open(&stream, data, file.size),
          "stream open");
    if (failures)
        return 1;
    got = (unsigned)m360_hps_stream_read(&stream, pcm,
                                         SOAK_SECONDS * header.sample_rate);
    check(got == SOAK_SECONDS * header.sample_rate, "decode across loop");
    printf("decoded %u frames/ch (%u s): adpcm_frames=%u blocks_entered=%u "
           "loops=%u history carry match=%u mismatch=%u\n",
           got, SOAK_SECONDS, stream.frames_decoded, stream.blocks_entered,
           stream.loops, stream.history_matches, stream.history_mismatches);
    check(stream.loops == 1, "wrapped exactly once in 70 s");
    check(stream.history_mismatches == 0 && stream.history_matches > 0,
          "decoded history equals next block header history");

    delta_count = (preview_frames - 1u) * header.channels;
    deltas = (unsigned *)malloc(delta_count * sizeof(unsigned));
    for (c = 0; c < header.channels; ++c) {
        for (i = 0; i < preview_frames; ++i) {
            const int s = pcm[i * header.channels + c];
            const unsigned mag = (unsigned)(s < 0 ? -s : s);
            sum_sq[c] += (double)s * s;
            if (mag > peak[c])
                peak[c] = mag;
            if (s == 32767 || s == -32768)
                ++clipped;
            if (i) {
                const int d = s - pcm[(i - 1) * header.channels + c];
                const unsigned ad = (unsigned)(d < 0 ? -d : d);
                deltas[c * (preview_frames - 1u) + i - 1u] = ad;
                if (ad < 4096u)
                    ++small;
            }
        }
    }
    qsort(deltas, delta_count, sizeof(unsigned), cmp_u32);
    printf("first %u s: rms L=%.1f R=%.1f peak L=%u R=%u clipped=%u\n",
           PREVIEW_SECONDS, sqrt(sum_sq[0] / preview_frames),
           sqrt(sum_sq[1] / preview_frames), peak[0], peak[1], clipped);
    printf("|delta|: median=%u p90=%u p99=%u max=%u  <4096: %.4f%%\n",
           deltas[delta_count / 2], deltas[delta_count * 9 / 10],
           deltas[delta_count * 99 / 100], deltas[delta_count - 1],
           100.0 * small / delta_count);
    check(sqrt(sum_sq[0] / preview_frames) > 500.0 &&
              sqrt(sum_sq[1] / preview_frames) > 500.0,
          "RMS well above silence");
    check(deltas[delta_count / 2] < 512u, "median |delta| < 512 (1/64 FS)");
    check(deltas[delta_count * 99 / 100] < 4096u, "p99 |delta| < 1/8 FS");
    check(clipped < preview_frames / 1000u, "clipping < 0.1%");

    if (argc > 2) {
        check(write_wav(argv[2], pcm, preview_frames, header.channels,
                        header.sample_rate),
              "write preview WAV");
        printf("wav: %s (%u frames, %u Hz, %u ch, 16-bit)\n", argv[2],
               preview_frames, header.sample_rate, header.channels);
    }
    free(deltas);
    free(pcm);
    free(data);
    if (failures) {
        printf("FAILED: %d check(s)\n", failures);
        return 1;
    }
    puts("PASS: HPS/DSP-ADPCM audio decode");
    return 0;
}
