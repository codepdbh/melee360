#include "hsd_texture_xdk.h"

static unsigned Expand4(unsigned v) { return (v << 4) | v; }
static unsigned Expand5(unsigned v) { return (v << 3) | (v >> 2); }
static unsigned Expand6(unsigned v) { return (v << 2) | (v >> 4); }
static unsigned Expand3(unsigned v) { return (v << 5) | (v << 2) | (v >> 1); }

static unsigned Read16(const unsigned char* p)
{
    return ((unsigned) p[0] << 8) | p[1];
}

static unsigned Rgb565(unsigned v)
{
    return 0xFF000000u | (Expand5((v >> 11) & 31) << 16) |
           (Expand6((v >> 5) & 63) << 8) | Expand5(v & 31);
}

static unsigned Rgb5a3(unsigned v)
{
    if (v & 0x8000)
        return 0xFF000000u | (Expand5((v >> 10) & 31) << 16) |
               (Expand5((v >> 5) & 31) << 8) | Expand5(v & 31);
    return (Expand3((v >> 12) & 7) << 24) | (Expand4((v >> 8) & 15) << 16) |
           (Expand4((v >> 4) & 15) << 8) | Expand4(v & 15);
}

static unsigned Ia(unsigned alpha, unsigned intensity)
{
    return (alpha << 24) | (intensity << 16) | (intensity << 8) | intensity;
}

static unsigned PaletteColor(const unsigned char* palette, unsigned format,
                             unsigned entries, unsigned index)
{
    unsigned v;
    if (!palette || index >= entries)
        return 0xFFFF00FFu;
    v = Read16(palette + index * 2);
    switch (format) {
    case M360_GX_TL_IA8: return Ia(v >> 8, v & 255);
    case M360_GX_TL_RGB565: return Rgb565(v);
    case M360_GX_TL_RGB5A3: return Rgb5a3(v);
    default: return 0xFFFF00FFu;
    }
}

static void TileSize(unsigned format, unsigned* tw, unsigned* th,
                     unsigned* bytes)
{
    switch (format) {
    case M360_GX_TF_I4: case M360_GX_TF_C4: case M360_GX_TF_CMPR:
        *tw = 8; *th = 8; *bytes = 32; break;
    case M360_GX_TF_I8: case M360_GX_TF_IA4: case M360_GX_TF_C8:
        *tw = 8; *th = 4; *bytes = 32; break;
    case M360_GX_TF_RGBA8:
        *tw = 4; *th = 4; *bytes = 64; break;
    default:
        *tw = 4; *th = 4; *bytes = 32; break;
    }
}

unsigned M360_GxTextureSize(unsigned width, unsigned height, unsigned format)
{
    unsigned tw, th, bytes;
    TileSize(format, &tw, &th, &bytes);
    return ((width + tw - 1) / tw) * ((height + th - 1) / th) * bytes;
}

static void Put(unsigned* argb, unsigned width, unsigned height, unsigned x,
                unsigned y, unsigned color)
{
    if (x < width && y < height)
        argb[y * width + x] = color;
}

static void DecodeCmprBlock(const unsigned char* block, unsigned* out)
{
    const unsigned c0 = Read16(block), c1 = Read16(block + 2);
    unsigned colors[4];
    unsigned bits, p, ch;
    colors[0] = Rgb565(c0);
    colors[1] = Rgb565(c1);
    colors[2] = 0xFF000000u;
    colors[3] = 0xFF000000u;
    for (ch = 0; ch < 24; ch += 8) {
        const unsigned a = (colors[0] >> ch) & 255, b = (colors[1] >> ch) & 255;
        if (c0 > c1) {
            colors[2] |= ((2 * a + b) / 3) << ch;
            colors[3] |= ((a + 2 * b) / 3) << ch;
        } else {
            colors[2] |= ((a + b) / 2) << ch;
        }
    }
    if (c0 <= c1)
        colors[3] = 0;
    bits = ((unsigned) block[4] << 24) | ((unsigned) block[5] << 16) |
           ((unsigned) block[6] << 8) | block[7];
    for (p = 0; p < 16; ++p)
        out[p] = colors[(bits >> (30 - p * 2)) & 3];
}

int M360_DecodeGxTexture(const unsigned char* data, unsigned width,
                         unsigned height, unsigned format,
                         const unsigned char* palette, unsigned paletteFormat,
                         unsigned paletteEntries, unsigned* argb)
{
    unsigned tw, th, bytes, bx, by, p;
    const unsigned char* src = data;
    if (!data || !argb || !width || !height)
        return 0;
    if (format > M360_GX_TF_CMPR || format == 7 || format == 0xB ||
        format == 0xC || format == 0xD)
        return 0;
    TileSize(format, &tw, &th, &bytes);
    for (by = 0; by < height; by += th) {
        for (bx = 0; bx < width; bx += tw) {
            switch (format) {
            case M360_GX_TF_I4: case M360_GX_TF_C4:
                for (p = 0; p < 64; ++p) {
                    const unsigned n = (src[p / 2] >> ((p & 1) ? 0 : 4)) & 15;
                    Put(argb, width, height, bx + p % 8, by + p / 8,
                        format == M360_GX_TF_C4
                            ? PaletteColor(palette, paletteFormat, paletteEntries, n)
                            : Expand4(n) * 0x01010101u);
                }
                break;
            case M360_GX_TF_I8:
                for (p = 0; p < 32; ++p)
                    Put(argb, width, height, bx + p % 8, by + p / 8,
                        src[p] * 0x01010101u);
                break;
            case M360_GX_TF_IA4:
                for (p = 0; p < 32; ++p)
                    Put(argb, width, height, bx + p % 8, by + p / 8,
                        Ia(Expand4(src[p] >> 4), Expand4(src[p] & 15)));
                break;
            case M360_GX_TF_C8:
                for (p = 0; p < 32; ++p)
                    Put(argb, width, height, bx + p % 8, by + p / 8,
                        PaletteColor(palette, paletteFormat, paletteEntries, src[p]));
                break;
            case M360_GX_TF_IA8:
                for (p = 0; p < 16; ++p)
                    Put(argb, width, height, bx + p % 4, by + p / 4,
                        Ia(src[p * 2], src[p * 2 + 1]));
                break;
            case M360_GX_TF_RGB565:
                for (p = 0; p < 16; ++p)
                    Put(argb, width, height, bx + p % 4, by + p / 4,
                        Rgb565(Read16(src + p * 2)));
                break;
            case M360_GX_TF_RGB5A3:
                for (p = 0; p < 16; ++p)
                    Put(argb, width, height, bx + p % 4, by + p / 4,
                        Rgb5a3(Read16(src + p * 2)));
                break;
            case M360_GX_TF_C14X2:
                for (p = 0; p < 16; ++p)
                    Put(argb, width, height, bx + p % 4, by + p / 4,
                        PaletteColor(palette, paletteFormat, paletteEntries,
                                     Read16(src + p * 2) & 0x3FFF));
                break;
            case M360_GX_TF_RGBA8:
                for (p = 0; p < 16; ++p)
                    Put(argb, width, height, bx + p % 4, by + p / 4,
                        ((unsigned) src[p * 2] << 24) |
                        ((unsigned) src[p * 2 + 1] << 16) |
                        ((unsigned) src[32 + p * 2] << 8) | src[32 + p * 2 + 1]);
                break;
            case M360_GX_TF_CMPR: {
                unsigned sub;
                for (sub = 0; sub < 4; ++sub) {
                    unsigned block[16];
                    DecodeCmprBlock(src + sub * 8, block);
                    for (p = 0; p < 16; ++p)
                        Put(argb, width, height, bx + (sub & 1) * 4 + p % 4,
                            by + (sub >> 1) * 4 + p / 4, block[p]);
                }
                break;
            }
            default:
                return 0;
            }
            src += bytes;
        }
    }
    return 1;
}
