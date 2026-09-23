#ifndef MELEE360_XDK_HSD_TEXTURE_H
#define MELEE360_XDK_HSD_TEXTURE_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
    M360_GX_TF_I4 = 0x0,
    M360_GX_TF_I8 = 0x1,
    M360_GX_TF_IA4 = 0x2,
    M360_GX_TF_IA8 = 0x3,
    M360_GX_TF_RGB565 = 0x4,
    M360_GX_TF_RGB5A3 = 0x5,
    M360_GX_TF_RGBA8 = 0x6,
    M360_GX_TF_C4 = 0x8,
    M360_GX_TF_C8 = 0x9,
    M360_GX_TF_C14X2 = 0xA,
    M360_GX_TF_CMPR = 0xE
};

enum {
    M360_GX_TL_IA8 = 0,
    M360_GX_TL_RGB565 = 1,
    M360_GX_TL_RGB5A3 = 2
};

unsigned M360_GxTextureSize(unsigned width, unsigned height, unsigned format);
int M360_DecodeGxTexture(const unsigned char* data, unsigned width,
                         unsigned height, unsigned format,
                         const unsigned char* palette, unsigned paletteFormat,
                         unsigned paletteEntries, unsigned* argb);

#ifdef __cplusplus
}
#endif

#endif
