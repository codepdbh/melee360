#include "../../src/xdk/hsd_texture_xdk.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    unsigned char tile[32];
    unsigned pixels[66];
    unsigned i;
    memset(tile, 0, sizeof(tile));
    /* Four subblocks, red->black. Each row selects endpoints then gradients. */
    for (i = 0; i < 4; ++i) {
        tile[i * 8] = 0xF8;
        tile[i * 8 + 1] = 0;
        tile[i * 8 + 4] = tile[i * 8 + 5] =
        tile[i * 8 + 6] = tile[i * 8 + 7] = 0x1B;
    }
    assert(M360_GxTextureSize(8, 8, M360_GX_TF_CMPR) == sizeof(tile));
    assert(M360_DecodeGxTexture(tile, 8, 8, M360_GX_TF_CMPR, 0, 0, 0, pixels));
    for (i = 0; i < 64; i += 4) {
        assert(pixels[i] == 0xFFFF0000u);
        assert(pixels[i + 1] == 0xFF000000u);
        assert(pixels[i + 2] == 0xFF9F0000u); /* 159, not DXT1's 170 */
        assert(pixels[i + 3] == 0xFF5F0000u); /* 95, not DXT1's 85 */
    }
    /* Black->white transparent mode: index 3 keeps RGB 127 and alpha 0. */
    for (i = 0; i < 4; ++i) {
        tile[i * 8] = tile[i * 8 + 1] = 0;
        tile[i * 8 + 2] = tile[i * 8 + 3] = 0xFF;
    }
    assert(M360_DecodeGxTexture(tile, 8, 8, M360_GX_TF_CMPR, 0, 0, 0, pixels));
    for (i = 0; i < 64; i += 4) {
        assert(pixels[i] == 0xFF000000u);
        assert(pixels[i + 1] == 0xFFFFFFFFu);
        assert(pixels[i + 2] == 0xFF7F7F7Fu);
        assert(pixels[i + 3] == 0x007F7F7Fu);
    }
    /* Partial tiles still consume a full block without writing beyond output. */
    pixels[0] = pixels[16] = 0xDEADBEEFu;
    assert(M360_DecodeGxTexture(tile, 5, 3, M360_GX_TF_CMPR, 0, 0, 0, pixels + 1));
    assert(pixels[0] == 0xDEADBEEFu && pixels[16] == 0xDEADBEEFu);
    puts("PASS: GX CMPR gradients, transparent RGB and partial-tile bounds");
    return 0;
}
