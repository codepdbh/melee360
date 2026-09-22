#include "gcm.h"
#include "jpeg_decode.h"
#include "mth.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MOVIE "MvOpen.mth"

static int failures;

static void check(int condition, const char *what)
{
    if (!condition) {
        printf("FAIL: %s\n", what);
        ++failures;
    }
}

static void reference_idct(const int32_t in[64], double out[64])
{
    const double pi = 3.14159265358979323846;
    unsigned x, y, u, v;
    for (y = 0; y < 8; ++y)
        for (x = 0; x < 8; ++x) {
            double sum = 0.0;
            for (v = 0; v < 8; ++v)
                for (u = 0; u < 8; ++u) {
                    const double cu = u ? 1.0 : 1.0 / sqrt(2.0);
                    const double cv = v ? 1.0 : 1.0 / sqrt(2.0);
                    sum += cu * cv * in[v * 8 + u] *
                           cos((2 * x + 1) * u * pi / 16.0) *
                           cos((2 * y + 1) * v * pi / 16.0);
                }
            out[y * 8 + x] = sum / 4.0 + 128.0;
        }
}

static void test_idct(void)
{
    int32_t block[64];
    uint8_t out[64];
    double ref[64];
    unsigned i, trial;
    int worst = 0;
    unsigned dc_sample;
    memset(block, 0, sizeof(block));
    block[0] = 80;
    m360_jpeg_idct_islow(block, out, 8);
    dc_sample = out[0];
    for (i = 0; i < 64; ++i)
        check(out[i] == 138, "DC-only IDCT gives 128 + DC/8");
    srand(360);
    for (trial = 0; trial < 2000; ++trial) {
        for (i = 0; i < 64; ++i)
            block[i] = (rand() % 5 == 0) ? (rand() % 257) - 128 : 0;
        block[0] = (rand() % 1025) - 512;
        m360_jpeg_idct_islow(block, out, 8);
        reference_idct(block, ref);
        for (i = 0; i < 64; ++i) {
            double r = ref[i] < 0 ? 0 : (ref[i] > 255 ? 255 : ref[i]);
            int diff = (int)out[i] - (int)floor(r + 0.5);
            if (diff < 0) diff = -diff;
            if (diff > worst) worst = diff;
        }
    }
    printf("idct: DC-only 80 -> %u, 2000 random blocks max |islow-float| = %d\n",
           dc_sample, worst);
    check(worst <= 1, "islow IDCT within 1 of float reference");
}

static const uint8_t dc_bits[16] = { 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
static const uint8_t dc_vals[12] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
static const uint8_t ac_bits[16] = { 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
static const uint8_t ac_vals[162] = {
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06,
    0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
    0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0, 0x24, 0x33, 0x62, 0x72,
    0x82, 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45,
    0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75,
    0x76, 0x77, 0x78, 0x79, 0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3,
    0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
    0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9,
    0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
    0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1, 0xf2, 0xf3, 0xf4,
    0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa
};

struct writer {
    uint8_t data[4096];
    size_t size;
    uint32_t acc;
    unsigned count;
};

static void put_byte(struct writer *w, unsigned byte)
{
    w->data[w->size++] = (uint8_t)byte;
}

static void put_bits(struct writer *w, uint32_t value, unsigned count)
{
    while (count--) {
        w->acc = (w->acc << 1) | ((value >> count) & 1);
        if (++w->count == 8) {
            put_byte(w, w->acc & 0xFF);
            if ((w->acc & 0xFF) == 0xFF)
                put_byte(w, 0);
            w->acc = 0;
            w->count = 0;
        }
    }
}

static void code_for(const uint8_t bits[16], const uint8_t *vals,
                     unsigned symbol, uint32_t *code, unsigned *length)
{
    uint32_t c = 0;
    unsigned k = 0, len, i;
    for (len = 1; len <= 16; ++len) {
        for (i = 0; i < bits[len - 1]; ++i, ++k, ++c)
            if (vals[k] == symbol) {
                *code = c;
                *length = len;
                return;
            }
        c <<= 1;
    }
    *length = 0;
}

static void put_symbol(struct writer *w, const uint8_t bits[16],
                       const uint8_t *vals, unsigned symbol)
{
    uint32_t code = 0;
    unsigned length = 0;
    code_for(bits, vals, symbol, &code, &length);
    put_bits(w, code, length);
}

static void put_dc(struct writer *w, int diff)
{
    unsigned size = 0;
    int magnitude = diff < 0 ? -diff : diff;
    while (magnitude >> size)
        ++size;
    put_symbol(w, dc_bits, dc_vals, size);
    if (size)
        put_bits(w, (uint32_t)(diff < 0 ? diff + (1 << size) - 1 : diff), size);
}

static void put_segment(struct writer *w, unsigned marker, const uint8_t *body,
                        unsigned length)
{
    put_byte(w, 0xFF);
    put_byte(w, marker);
    put_byte(w, (length + 2) >> 8);
    put_byte(w, (length + 2) & 0xFF);
    memcpy(w->data + w->size, body, length);
    w->size += length;
}

static void test_hand_jpeg(void)
{
    static const int y_dc[4] = { 80, -40, 400, 0 };
    static const int cb_dc = -160;
    static const int cr_dc = 240;
    struct writer w;
    uint8_t body[256];
    uint8_t y[16 * 16], cb[8 * 8], cr[8 * 8];
    uint8_t *planes[3];
    unsigned pitches[3] = { 16, 8, 8 };
    struct m360_jpeg_info info;
    unsigned i;
    int prev = 0;
    int ok;
    memset(&w, 0, sizeof(w));
    put_byte(&w, 0xFF);
    put_byte(&w, 0xD8);
    body[0] = 0;
    for (i = 0; i < 64; ++i)
        body[1 + i] = 1;
    put_segment(&w, 0xDB, body, 65);
    {
        static const uint8_t sof[] = { 8, 0, 16, 0, 16, 3, 1, 0x22, 0,
                                       2, 0x11, 0, 3, 0x11, 0 };
        put_segment(&w, 0xC0, sof, sizeof(sof));
    }
    body[0] = 0x00;
    memcpy(body + 1, dc_bits, 16);
    memcpy(body + 17, dc_vals, 12);
    put_segment(&w, 0xC4, body, 29);
    body[0] = 0x10;
    memcpy(body + 1, ac_bits, 16);
    memcpy(body + 17, ac_vals, 162);
    put_segment(&w, 0xC4, body, 179);
    {
        static const uint8_t sos[] = { 3, 1, 0x00, 2, 0x00, 3, 0x00, 0, 63, 0 };
        put_segment(&w, 0xDA, sos, sizeof(sos));
    }
    for (i = 0; i < 4; ++i) {
        put_dc(&w, y_dc[i] - prev);
        prev = y_dc[i];
        put_symbol(&w, ac_bits, ac_vals, 0x00);
    }
    put_dc(&w, cb_dc);
    put_symbol(&w, ac_bits, ac_vals, 0x00);
    put_dc(&w, cr_dc);
    put_symbol(&w, ac_bits, ac_vals, 0x00);
    if (w.count)
        put_bits(&w, 0x7F, 8 - w.count);
    put_byte(&w, 0xFF);
    put_byte(&w, 0xD9);

    planes[0] = y;
    planes[1] = cb;
    planes[2] = cr;
    ok = m360_jpeg_decode(w.data, w.size, 0, planes, pitches, &info);
    check(ok && info.width == 16 && info.height == 16 && info.h[0] == 2 &&
              info.v[0] == 2 && info.plane_width[1] == 8,
          "hand JPEG header");
    ok = ok && y[0] == 138 && y[8] == 123 && y[16 * 8] == 178 &&
         y[16 * 8 + 8] == 128 && y[16 * 16 - 1] == 128 && cb[0] == 108 &&
         cb[63] == 108 && cr[0] == 158;
    printf("hand 4:2:0 JPEG (%u bytes): Y %u %u %u %u Cb %u Cr %u\n",
           (unsigned)w.size, y[0], y[8], y[128], y[136], cb[0], cr[0]);
    check(ok, "hand-encoded Huffman DC/EOB JPEG decodes to known samples");
}

static size_t stuff_thp(const uint8_t *jpeg, size_t size, uint8_t *out)
{
    const uint8_t *p = jpeg + 2;
    size_t o = 0;
    size_t end = size;
    while (end > 2 && !(jpeg[end - 2] == 0xFF && jpeg[end - 1] == 0xD9))
        --end;
    end -= 2;
    for (;;) {
        unsigned marker = p[1];
        unsigned length = ((unsigned)p[2] << 8) | p[3];
        p += 2 + length;
        if (marker == 0xDA)
            break;
    }
    memcpy(out, jpeg, (size_t)(p - jpeg));
    o = (size_t)(p - jpeg);
    for (; p < jpeg + end; ++p) {
        out[o++] = *p;
        if (*p == 0xFF)
            out[o++] = 0;
    }
    out[o++] = 0xFF;
    out[o++] = 0xD9;
    return o;
}

static void write_bmp(const char *path, const uint8_t *y, const uint8_t *u,
                      const uint8_t *v, unsigned width, unsigned height)
{
    FILE *file = fopen(path, "wb");
    uint8_t header[54];
    unsigned row_size = width * 3;
    unsigned image = row_size * height;
    unsigned x, row;
    uint8_t *line;
    if (!file)
        return;
    memset(header, 0, sizeof(header));
    header[0] = 'B';
    header[1] = 'M';
#define LE32(p, v) (p)[0] = (uint8_t)(v); (p)[1] = (uint8_t)((v) >> 8); \
    (p)[2] = (uint8_t)((v) >> 16); (p)[3] = (uint8_t)((v) >> 24)
    LE32(header + 2, 54 + image);
    LE32(header + 10, 54);
    LE32(header + 14, 40);
    LE32(header + 18, width);
    LE32(header + 22, height);
    header[26] = 1;
    header[28] = 24;
    LE32(header + 34, image);
    fwrite(header, 1, sizeof(header), file);
    line = (uint8_t *)malloc(row_size);
    for (row = 0; row < height; ++row) {
        const unsigned sy = height - 1 - row;
        for (x = 0; x < width; ++x) {
            const double yy = y[sy * width + x];
            const double cb = u[(sy / 2) * (width / 2) + x / 2] - 128.0;
            const double cr = v[(sy / 2) * (width / 2) + x / 2] - 128.0;
            double rgb[3];
            unsigned c;
            rgb[0] = yy + 1.402 * cr;
            rgb[1] = yy - 0.344136 * cb - 0.714136 * cr;
            rgb[2] = yy + 1.772 * cb;
            for (c = 0; c < 3; ++c) {
                double value = rgb[c] < 0 ? 0 : (rgb[c] > 255 ? 255 : rgb[c]);
                line[x * 3 + 2 - c] = (uint8_t)(value + 0.5);
            }
        }
        fwrite(line, 1, row_size, file);
    }
    free(line);
    fclose(file);
}

static void test_movie(const char *iso_path, const char *out_dir)
{
    static const uint32_t rate_table[] = { 1250, 2, 394, 1, 65536, 2 };
    static const unsigned samples[] = { 60, 400, 1000, 1500, 2000, 2600, 2780, 3035 };
    FILE *image = fopen(iso_path, "rb");
    struct m360_gcm gcm;
    struct m360_gcm_file file;
    struct m360_mth_header header;
    uint8_t head[M360_MTH_HEADER_SIZE];
    uint8_t *frame;
    uint8_t *stuffed;
    uint8_t *planes[3];
    uint8_t *check_planes[3];
    unsigned pitches[3];
    uint32_t offset, size, max_size = 0, min_size = 0xFFFFFFFFu, frames = 0;
    unsigned decoded = 0, sample = 0, i;
    double decode_seconds = 0.0;
    clock_t start;

    check(image != NULL, "open ISO");
    if (!image)
        return;
    check(m360_gcm_mount(&gcm, image) == 0, "mount FST");
    check(m360_gcm_find(&gcm, MOVIE, &file), "find " MOVIE);
    check(m360_gcm_read(&gcm, &file, 0, head, sizeof(head)) == sizeof(head),
          "read MTH header");
    check(m360_mth_parse_header(head, sizeof(head), &header), "parse MTH header");
    printf("%s: %u bytes, version %u, %ux%u, %u fps, %u frames, first %u (+%u), "
           "buffer %u, frame offsets %u\n",
           MOVIE, file.size, header.version, header.width, header.height,
           header.frame_rate, header.frame_count, header.first_frame,
           header.first_frame_size, header.buffer_size, header.frame_offsets);
    check(header.width == 640 && header.height == 480 && header.frame_rate == 30 &&
              header.frame_count == 3036 && header.frame_offsets == 0,
          "MvOpen.mth header fields");

    frame = (uint8_t *)malloc(1 << 20);
    stuffed = (uint8_t *)malloc(1 << 21);
    for (i = 0; i < 3; ++i) {
        const unsigned w = i ? header.width / 2 : header.width;
        const unsigned h = i ? header.height / 2 : header.height;
        pitches[i] = w;
        planes[i] = (uint8_t *)malloc(w * h);
        check_planes[i] = (uint8_t *)malloc(w * h);
    }
    offset = header.first_frame;
    size = header.first_frame_size;
    start = clock();
    while (frames < header.frame_count && offset + size <= file.size && size > 4) {
        struct m360_jpeg_info info;
        int ok;
        clock_t t0;
        if (m360_gcm_read(&gcm, &file, offset, frame, size) != size)
            break;
        if (size > max_size) max_size = size;
        if (size < min_size) min_size = size;
        check(frame[4] == 0xFF && frame[5] == 0xD8, "frame starts with SOI");
        t0 = clock();
        ok = m360_jpeg_decode(frame + 4, size - 4, M360_JPEG_THP, planes, pitches,
                              &info);
        decode_seconds += (double)(clock() - t0) / CLOCKS_PER_SEC;
        if (ok && info.width == header.width && info.height == header.height &&
            info.h[0] == 2 && info.v[0] == 2 && info.h[1] == 1)
            ++decoded;
        else
            printf("frame %u decode failed (ok=%d)\n", frames, ok);
        if (sample < sizeof(samples) / sizeof(samples[0]) && frames == samples[sample]) {
            double sum = 0.0, sq = 0.0;
            unsigned lo = 255, hi = 0, p;
            char path[512];
            size_t n = (size_t)header.width * header.height;
            for (p = 0; p < n; ++p) {
                const unsigned value = planes[0][p];
                sum += value;
                sq += (double)value * value;
                if (value < lo) lo = value;
                if (value > hi) hi = value;
            }
            printf("frame %4u tick %4u: %6u bytes  Y mean %.1f sd %.1f range %u..%u  "
                   "Cb[0] %u Cr[0] %u\n",
                   frames, m360_mth_ticks_for_frames(rate_table, frames), size,
                   sum / n, sqrt(sq / n - (sum / n) * (sum / n)), lo, hi,
                   planes[1][0], planes[2][0]);
            check(hi > lo + 40, "decoded frame is not uniform");
            {
                size_t stuffed_size = stuff_thp(frame + 4, size - 4, stuffed);
                int same = m360_jpeg_decode(stuffed, stuffed_size, 0, check_planes,
                                            pitches, NULL);
                for (i = 0; i < 3 && same; ++i)
                    same = memcmp(planes[i], check_planes[i],
                                  pitches[i] * (i ? header.height / 2 : header.height)) == 0;
                check(same, "byte-stuffed re-encode decodes identically");
                if (out_dir) {
                    FILE *jpg;
                    snprintf(path, sizeof(path), "%s/mvopen_%04u.jpg", out_dir, frames);
                    jpg = fopen(path, "wb");
                    if (jpg) {
                        fwrite(stuffed, 1, stuffed_size, jpg);
                        fclose(jpg);
                    }
                    snprintf(path, sizeof(path), "%s/mvopen_%04u.y", out_dir, frames);
                    jpg = fopen(path, "wb");
                    if (jpg) {
                        fwrite(planes[0], 1, n, jpg);
                        fclose(jpg);
                    }
                    snprintf(path, sizeof(path), "%s/mvopen_%04u.bmp", out_dir, frames);
                    write_bmp(path, planes[0], planes[1], planes[2], header.width,
                              header.height);
                }
            }
            ++sample;
        }
        offset += size;
        size = m360_mth_next_frame_size(frame);
        ++frames;
    }
    printf("walked %u frames, end offset %u of %u, frame sizes %u..%u, "
           "decoded %u, host decode %.2f ms/frame, total %.1f s\n",
           frames, offset, file.size, min_size, max_size, decoded,
           frames ? decode_seconds * 1000.0 / frames : 0.0,
           (double)(clock() - start) / CLOCKS_PER_SEC);
    check(frames == header.frame_count, "frame chain has frame_count frames");
    check(offset == file.size, "frame chain ends exactly at EOF");
    check(decoded == header.frame_count, "every frame decodes at 640x480 4:2:0");
    check(m360_mth_frame_for_tick(rate_table, 2499) == 1249 &&
              m360_mth_frame_for_tick(rate_table, 2500) == 1250 &&
              m360_mth_frame_for_tick(rate_table, 2893) == 1643 &&
              m360_mth_frame_for_tick(rate_table, 2894) == 1644 &&
              m360_mth_frame_for_tick(rate_table, 5130) == 2762 &&
              m360_mth_ticks_for_frames(rate_table, 3036) == 5678,
          "lbMthp rate table mapping");
    printf("rate table: tick 5130 -> frame %u, last frame at tick %u\n",
           m360_mth_frame_for_tick(rate_table, 5130),
           m360_mth_ticks_for_frames(rate_table, header.frame_count));
    for (i = 0; i < 3; ++i) {
        free(planes[i]);
        free(check_planes[i]);
    }
    free(frame);
    free(stuffed);
    m360_gcm_unmount(&gcm);
    fclose(image);
}

int main(int argc, char **argv)
{
    test_idct();
    test_hand_jpeg();
    if (argc > 1)
        test_movie(argv[1], argc > 2 ? argv[2] : NULL);
    else
        check(0, "usage: test_movie <iso> [output-dir]");
    printf(failures ? "[M360][MOVIE] %d failure(s)\n" : "[M360][MOVIE] all checks passed\n",
           failures);
    return failures ? 1 : 0;
}
