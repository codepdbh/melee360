#include "jpeg_decode.h"

#include <string.h>

#define HUFF_LOOKUP_BITS 9
#define CONST_BITS 13
#define PASS1_BITS 2
#define DESCALE(x, n) (((x) + (1 << ((n) - 1))) >> (n))

static const uint8_t zigzag[64 + 16] = {
    0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6,  7,  14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63,
    63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63
};

struct huffman {
    uint8_t lookup_len[1 << HUFF_LOOKUP_BITS];
    uint8_t lookup_val[1 << HUFF_LOOKUP_BITS];
    int32_t maxcode[18];
    int32_t mincode[17];
    int32_t valptr[17];
    uint8_t values[256];
    int present;
};

struct bit_reader {
    const uint8_t *p;
    const uint8_t *end;
    uint32_t acc;
    int count;
    int stuffed;
};

struct decoder {
    struct m360_jpeg_info info;
    uint16_t quant[4][64];
    struct huffman dc[4];
    struct huffman ac[4];
    uint8_t component_id[M360_JPEG_MAX_COMPONENTS];
    uint8_t quant_index[M360_JPEG_MAX_COMPONENTS];
    uint8_t dc_table[M360_JPEG_MAX_COMPONENTS];
    uint8_t ac_table[M360_JPEG_MAX_COMPONENTS];
    unsigned scan_components;
    uint8_t scan_order[M360_JPEG_MAX_COMPONENTS];
    unsigned hmax;
    unsigned vmax;
    int have_frame;
};

static unsigned read16(const uint8_t *p)
{
    return ((unsigned)p[0] << 8) | p[1];
}

static int build_huffman(struct huffman *table, const uint8_t counts[16],
                         const uint8_t *values, unsigned total)
{
    unsigned code = 0;
    unsigned k = 0;
    unsigned length;
    memset(table, 0, sizeof(*table));
    if (total > 256)
        return 0;
    memcpy(table->values, values, total);
    for (length = 1; length <= 16; ++length) {
        unsigned i;
        table->valptr[length] = (int32_t)k;
        table->mincode[length] = (int32_t)code;
        for (i = 0; i < counts[length - 1]; ++i) {
            if (length <= HUFF_LOOKUP_BITS) {
                const unsigned shift = HUFF_LOOKUP_BITS - length;
                const unsigned first = code << shift;
                unsigned fill;
                for (fill = 0; fill < (1u << shift); ++fill) {
                    table->lookup_len[first + fill] = (uint8_t)length;
                    table->lookup_val[first + fill] = values[k];
                }
            }
            ++code;
            ++k;
        }
        table->maxcode[length] = counts[length - 1] ? (int32_t)code - 1 : -1;
        if (code > (1u << length))
            return 0;
        code <<= 1;
    }
    table->maxcode[17] = 0x7FFFFFFF;
    table->present = 1;
    return 1;
}

static void fill_bits(struct bit_reader *bits)
{
    while (bits->count <= 24) {
        unsigned byte = 0;
        if (bits->p < bits->end) {
            byte = *bits->p;
            if (bits->stuffed && byte == 0xFF) {
                const unsigned next = bits->p + 1 < bits->end ? bits->p[1] : 0xD9;
                if (next == 0x00)
                    bits->p += 2;
                else
                    byte = 0;
            } else {
                ++bits->p;
            }
        }
        bits->acc |= (uint32_t)byte << (24 - bits->count);
        bits->count += 8;
    }
}

static int get_bits(struct bit_reader *bits, unsigned count)
{
    uint32_t value;
    if (!count)
        return 0;
    fill_bits(bits);
    value = bits->acc >> (32 - count);
    bits->acc <<= count;
    bits->count -= (int)count;
    return (int)value;
}

static int extend(int value, unsigned size)
{
    return value < (1 << (size - 1)) ? value - (1 << size) + 1 : value;
}

static int decode_huffman(struct bit_reader *bits, const struct huffman *table)
{
    unsigned peek;
    unsigned length;
    fill_bits(bits);
    peek = bits->acc >> (32 - HUFF_LOOKUP_BITS);
    length = table->lookup_len[peek];
    if (length) {
        bits->acc <<= length;
        bits->count -= (int)length;
        return table->lookup_val[peek];
    }
    for (length = HUFF_LOOKUP_BITS + 1; length <= 16; ++length) {
        const int32_t code = (int32_t)(bits->acc >> (32 - length));
        if (code <= table->maxcode[length]) {
            const int32_t index =
                table->valptr[length] + code - table->mincode[length];
            bits->acc <<= length;
            bits->count -= (int)length;
            return index >= 0 && index < 256 ? table->values[index] : -1;
        }
    }
    return -1;
}

static uint8_t clamp_sample(int32_t value)
{
    return (uint8_t)(value < 0 ? 0 : (value > 255 ? 255 : value));
}

void m360_jpeg_idct_islow(const int32_t in[64], uint8_t *out, unsigned pitch)
{
    int32_t workspace[64];
    int column;
    int row;
    for (column = 0; column < 8; ++column) {
        const int32_t *c = in + column;
        int32_t *w = workspace + column;
        int32_t tmp0, tmp1, tmp2, tmp3, tmp10, tmp11, tmp12, tmp13;
        int32_t z1, z2, z3, z4, z5;
        if (!c[8] && !c[16] && !c[24] && !c[32] && !c[40] && !c[48] && !c[56]) {
            const int32_t dc = c[0] * (1 << PASS1_BITS);
            for (row = 0; row < 8; ++row)
                w[row * 8] = dc;
            continue;
        }
        z2 = c[16];
        z3 = c[48];
        z1 = (z2 + z3) * 4433;
        tmp2 = z1 + z3 * -15137;
        tmp3 = z1 + z2 * 6270;
        tmp0 = (c[0] + c[32]) * (1 << CONST_BITS);
        tmp1 = (c[0] - c[32]) * (1 << CONST_BITS);
        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;
        tmp0 = c[56];
        tmp1 = c[40];
        tmp2 = c[24];
        tmp3 = c[8];
        z1 = tmp0 + tmp3;
        z2 = tmp1 + tmp2;
        z3 = tmp0 + tmp2;
        z4 = tmp1 + tmp3;
        z5 = (z3 + z4) * 9633;
        tmp0 *= 2446;
        tmp1 *= 16819;
        tmp2 *= 25172;
        tmp3 *= 12299;
        z1 *= -7373;
        z2 *= -20995;
        z3 = z3 * -16069 + z5;
        z4 = z4 * -3196 + z5;
        tmp0 += z1 + z3;
        tmp1 += z2 + z4;
        tmp2 += z2 + z3;
        tmp3 += z1 + z4;
        w[0] = DESCALE(tmp10 + tmp3, CONST_BITS - PASS1_BITS);
        w[56] = DESCALE(tmp10 - tmp3, CONST_BITS - PASS1_BITS);
        w[8] = DESCALE(tmp11 + tmp2, CONST_BITS - PASS1_BITS);
        w[48] = DESCALE(tmp11 - tmp2, CONST_BITS - PASS1_BITS);
        w[16] = DESCALE(tmp12 + tmp1, CONST_BITS - PASS1_BITS);
        w[40] = DESCALE(tmp12 - tmp1, CONST_BITS - PASS1_BITS);
        w[24] = DESCALE(tmp13 + tmp0, CONST_BITS - PASS1_BITS);
        w[32] = DESCALE(tmp13 - tmp0, CONST_BITS - PASS1_BITS);
    }
    for (row = 0; row < 8; ++row) {
        const int32_t *w = workspace + row * 8;
        uint8_t *o = out + row * pitch;
        int32_t tmp0, tmp1, tmp2, tmp3, tmp10, tmp11, tmp12, tmp13;
        int32_t z1, z2, z3, z4, z5;
        const int shift = CONST_BITS + PASS1_BITS + 3;
        if (!w[1] && !w[2] && !w[3] && !w[4] && !w[5] && !w[6] && !w[7]) {
            const uint8_t value =
                clamp_sample(DESCALE(w[0], PASS1_BITS + 3) + 128);
            memset(o, value, 8);
            continue;
        }
        z2 = w[2];
        z3 = w[6];
        z1 = (z2 + z3) * 4433;
        tmp2 = z1 + z3 * -15137;
        tmp3 = z1 + z2 * 6270;
        tmp0 = (w[0] + w[4]) * (1 << CONST_BITS);
        tmp1 = (w[0] - w[4]) * (1 << CONST_BITS);
        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;
        tmp0 = w[7];
        tmp1 = w[5];
        tmp2 = w[3];
        tmp3 = w[1];
        z1 = tmp0 + tmp3;
        z2 = tmp1 + tmp2;
        z3 = tmp0 + tmp2;
        z4 = tmp1 + tmp3;
        z5 = (z3 + z4) * 9633;
        tmp0 *= 2446;
        tmp1 *= 16819;
        tmp2 *= 25172;
        tmp3 *= 12299;
        z1 *= -7373;
        z2 *= -20995;
        z3 = z3 * -16069 + z5;
        z4 = z4 * -3196 + z5;
        tmp0 += z1 + z3;
        tmp1 += z2 + z4;
        tmp2 += z2 + z3;
        tmp3 += z1 + z4;
        o[0] = clamp_sample(DESCALE(tmp10 + tmp3, shift) + 128);
        o[7] = clamp_sample(DESCALE(tmp10 - tmp3, shift) + 128);
        o[1] = clamp_sample(DESCALE(tmp11 + tmp2, shift) + 128);
        o[6] = clamp_sample(DESCALE(tmp11 - tmp2, shift) + 128);
        o[2] = clamp_sample(DESCALE(tmp12 + tmp1, shift) + 128);
        o[5] = clamp_sample(DESCALE(tmp12 - tmp1, shift) + 128);
        o[3] = clamp_sample(DESCALE(tmp13 + tmp0, shift) + 128);
        o[4] = clamp_sample(DESCALE(tmp13 - tmp0, shift) + 128);
    }
}

static int parse_headers(struct decoder *d, const uint8_t *data, size_t size,
                         const uint8_t **entropy)
{
    const uint8_t *p = data;
    const uint8_t *end = data + size;
    memset(d, 0, sizeof(*d));
    if (size < 4 || p[0] != 0xFF || p[1] != 0xD8)
        return 0;
    p += 2;
    for (;;) {
        unsigned marker;
        unsigned length;
        const uint8_t *segment;
        if (p + 4 > end || *p != 0xFF)
            return 0;
        while (p < end && *p == 0xFF)
            ++p;
        if (p + 3 > end)
            return 0;
        marker = *p++;
        if (marker == 0xD8 || (marker >= 0xD0 && marker <= 0xD7))
            continue;
        length = read16(p);
        if (length < 2 || p + length > end)
            return 0;
        segment = p + 2;
        p += length;
        length -= 2;
        if (marker == 0xDB) {
            const uint8_t *q = segment;
            while (q < segment + length) {
                const unsigned precision = *q >> 4;
                const unsigned index = *q & 15;
                unsigned i;
                ++q;
                if (index > 3 || q + (precision ? 128 : 64) > segment + length)
                    return 0;
                for (i = 0; i < 64; ++i) {
                    d->quant[index][i] = (uint16_t)(precision ? read16(q + i * 2) : q[i]);
                }
                q += precision ? 128 : 64;
            }
        } else if (marker == 0xC4) {
            const uint8_t *q = segment;
            while (q + 17 <= segment + length) {
                const unsigned table_class = *q >> 4;
                const unsigned index = *q & 15;
                unsigned total = 0;
                unsigned i;
                for (i = 0; i < 16; ++i)
                    total += q[1 + i];
                if (index > 3 || table_class > 1 || total > 256 ||
                    q + 17 + total > segment + length)
                    return 0;
                if (!build_huffman(table_class ? &d->ac[index] : &d->dc[index],
                                   q + 1, q + 17, total))
                    return 0;
                q += 17 + total;
            }
        } else if (marker == 0xC0 || marker == 0xC1) {
            unsigned i;
            if (length < 6 || segment[0] != 8)
                return 0;
            d->info.height = read16(segment + 1);
            d->info.width = read16(segment + 3);
            d->info.components = segment[5];
            if ((d->info.components != 1 && d->info.components != 3) ||
                length < 6 + d->info.components * 3 ||
                !d->info.width || !d->info.height)
                return 0;
            d->hmax = d->vmax = 1;
            for (i = 0; i < d->info.components; ++i) {
                const uint8_t *c = segment + 6 + i * 3;
                d->component_id[i] = c[0];
                d->info.h[i] = c[1] >> 4;
                d->info.v[i] = c[1] & 15;
                d->quant_index[i] = c[2];
                if (d->info.h[i] < 1 || d->info.h[i] > 2 || d->info.v[i] < 1 ||
                    d->info.v[i] > 2 || c[2] > 3)
                    return 0;
                if (d->info.h[i] > d->hmax) d->hmax = d->info.h[i];
                if (d->info.v[i] > d->vmax) d->vmax = d->info.v[i];
            }
            for (i = 0; i < d->info.components; ++i) {
                d->info.plane_width[i] =
                    (d->info.width * d->info.h[i] + d->hmax - 1) / d->hmax;
                d->info.plane_height[i] =
                    (d->info.height * d->info.v[i] + d->vmax - 1) / d->vmax;
            }
            d->have_frame = 1;
        } else if (marker == 0xDD) {
            if (length < 2)
                return 0;
            d->info.restart_interval = read16(segment);
        } else if (marker == 0xDA) {
            unsigned i;
            if (!d->have_frame || length < 1)
                return 0;
            d->scan_components = segment[0];
            if (d->scan_components < 1 ||
                d->scan_components > d->info.components ||
                length < 4 + d->scan_components * 2)
                return 0;
            for (i = 0; i < d->scan_components; ++i) {
                const uint8_t id = segment[1 + i * 2];
                const uint8_t tables = segment[2 + i * 2];
                unsigned c;
                for (c = 0; c < d->info.components; ++c)
                    if (d->component_id[c] == id)
                        break;
                if (c == d->info.components || (tables >> 4) > 3 || (tables & 15) > 3)
                    return 0;
                d->scan_order[i] = (uint8_t)c;
                d->dc_table[c] = (uint8_t)(tables >> 4);
                d->ac_table[c] = (uint8_t)(tables & 15);
            }
            if (d->scan_components != d->info.components)
                return 0;
            *entropy = p;
            return 1;
        } else if (marker == 0xC2 || marker == 0xC3 || (marker >= 0xC5 && marker <= 0xCF &&
                   marker != 0xC8 && marker != 0xCC)) {
            return 0;
        } else if (marker == 0xD9) {
            return 0;
        }
    }
}

int m360_jpeg_read_info(const uint8_t *data, size_t size,
                        struct m360_jpeg_info *info)
{
    struct decoder d;
    const uint8_t *entropy;
    if (!parse_headers(&d, data, size, &entropy))
        return 0;
    *info = d.info;
    return 1;
}

static int decode_block(struct decoder *d, struct bit_reader *bits,
                        unsigned component, int *predictor, int32_t block[64],
                        int *dc_only)
{
    const struct huffman *dc = &d->dc[d->dc_table[component]];
    const struct huffman *ac = &d->ac[d->ac_table[component]];
    const uint16_t *q = d->quant[d->quant_index[component]];
    int symbol = decode_huffman(bits, dc);
    unsigned k;
    int value;
    if (symbol < 0 || symbol > 15)
        return 0;
    value = symbol ? extend(get_bits(bits, (unsigned)symbol), (unsigned)symbol) : 0;
    *predictor += value;
    memset(block, 0, 64 * sizeof(block[0]));
    value = *predictor * q[0];
    block[0] = value < -8192 ? -8192 : (value > 8191 ? 8191 : value);
    *dc_only = 1;
    for (k = 1; k < 64;) {
        unsigned run;
        unsigned size;
        symbol = decode_huffman(bits, ac);
        if (symbol < 0)
            return 0;
        run = (unsigned)symbol >> 4;
        size = (unsigned)symbol & 15;
        if (!size) {
            if (run != 15)
                break;
            k += 16;
            continue;
        }
        k += run;
        if (k > 63)
            return 0;
        value = extend(get_bits(bits, size), size) * q[k];
        block[zigzag[k]] = value < -8192 ? -8192 : (value > 8191 ? 8191 : value);
        *dc_only = 0;
        ++k;
    }
    return 1;
}

static void store_block(const int32_t block[64], int dc_only, uint8_t *plane,
                        unsigned pitch, unsigned x, unsigned y,
                        unsigned width, unsigned height)
{
    uint8_t temp[64];
    unsigned row;
    if (x >= width || y >= height)
        return;
    if (dc_only) {
        const uint8_t value = clamp_sample(DESCALE(block[0], 3) + 128);
        const unsigned w = width - x < 8 ? width - x : 8;
        const unsigned h = height - y < 8 ? height - y : 8;
        for (row = 0; row < h; ++row)
            memset(plane + (y + row) * pitch + x, value, w);
        return;
    }
    if (x + 8 <= width && y + 8 <= height) {
        m360_jpeg_idct_islow(block, plane + y * pitch + x, pitch);
        return;
    }
    m360_jpeg_idct_islow(block, temp, 8);
    for (row = 0; row < 8 && y + row < height; ++row) {
        const unsigned w = width - x < 8 ? width - x : 8;
        memcpy(plane + (y + row) * pitch + x, temp + row * 8, w);
    }
}

static void restart(struct bit_reader *bits, int predictors[M360_JPEG_MAX_COMPONENTS])
{
    predictors[0] = predictors[1] = predictors[2] = 0;
    if (!bits->stuffed) {
        const int drop = bits->count & 7;
        bits->acc <<= drop;
        bits->count -= drop;
        return;
    }
    bits->acc = 0;
    bits->count = 0;
    if (bits->p + 1 < bits->end && bits->p[0] == 0xFF &&
        bits->p[1] >= 0xD0 && bits->p[1] <= 0xD7)
        bits->p += 2;
}

int m360_jpeg_decode(const uint8_t *data, size_t size, unsigned flags,
                     uint8_t *const planes[M360_JPEG_MAX_COMPONENTS],
                     const unsigned pitches[M360_JPEG_MAX_COMPONENTS],
                     struct m360_jpeg_info *info)
{
    struct decoder d;
    struct bit_reader bits;
    const uint8_t *entropy;
    int predictors[M360_JPEG_MAX_COMPONENTS] = { 0, 0, 0 };
    int32_t block[64];
    unsigned mcus_x;
    unsigned mcus_y;
    unsigned mx;
    unsigned my;
    unsigned c;
    unsigned remaining;
    if (!parse_headers(&d, data, size, &entropy))
        return 0;
    for (c = 0; c < d.info.components; ++c) {
        if (!planes[c] || pitches[c] < d.info.plane_width[c] ||
            !d.dc[d.dc_table[c]].present || !d.ac[d.ac_table[c]].present)
            return 0;
    }
    bits.p = entropy;
    bits.end = data + size;
    bits.acc = 0;
    bits.count = 0;
    bits.stuffed = (flags & M360_JPEG_THP) ? 0 : 1;
    if (d.info.components == 1) {
        d.hmax = d.vmax = d.info.h[0] = d.info.v[0] = 1;
    }
    mcus_x = (d.info.width + 8 * d.hmax - 1) / (8 * d.hmax);
    mcus_y = (d.info.height + 8 * d.vmax - 1) / (8 * d.vmax);
    remaining = d.info.restart_interval;
    for (my = 0; my < mcus_y; ++my) {
        for (mx = 0; mx < mcus_x; ++mx) {
            if (d.info.restart_interval) {
                if (!remaining) {
                    restart(&bits, predictors);
                    remaining = d.info.restart_interval;
                }
                --remaining;
            }
            for (c = 0; c < d.info.components; ++c) {
                const unsigned h = d.info.h[c];
                const unsigned v = d.info.v[c];
                unsigned by;
                unsigned bx;
                for (by = 0; by < v; ++by) {
                    for (bx = 0; bx < h; ++bx) {
                        int dc_only;
                        if (!decode_block(&d, &bits, c, &predictors[c], block,
                                          &dc_only))
                            return 0;
                        store_block(block, dc_only, planes[c], pitches[c],
                                    (mx * h + bx) * 8, (my * v + by) * 8,
                                    d.info.plane_width[c],
                                    d.info.plane_height[c]);
                    }
                }
            }
        }
    }
    if (info)
        *info = d.info;
    return 1;
}
