#include <assert.h>
#include <stdio.h>
struct IDirect3DTexture9 {
    unsigned releases;
    IDirect3DTexture9() : releases(0) {}
    void Release() { ++releases; }
};
#include "texture_cache_original.h"

int main(void)
{
    int image, palette;
    IDirect3DTexture9 a, b, c, d, e;
    CacheTexture(&image, &palette, 32, 32, 8, 2, 16, &a);
    assert(TextureMatches(s_textures[0], &image, &palette, 32, 32, 8, 2, 16));
    assert(!TextureMatches(s_textures[0], &image, &palette, 64, 32, 8, 2, 16));
    assert(!TextureMatches(s_textures[0], &image, &palette, 32, 64, 8, 2, 16));
    assert(!TextureMatches(s_textures[0], &image, &palette, 32, 32, 9, 2, 16));
    assert(!TextureMatches(s_textures[0], &image, &palette, 32, 32, 8, 0, 16));
    assert(!TextureMatches(s_textures[0], &image, &palette, 32, 32, 8, 2, 256));
    assert(!TextureMatches(s_textures[0], &image, 0, 32, 32, 8, 2, 16));
    CacheTexture(&image, 0, 1, 1, 0, 0, 0, 0);
    assert(s_textureCount == 1); // failed uploads are retried instead of cached
    CacheTexture(&image, &palette, 64, 32, 8, 2, 16, &b);
    CacheTexture(&image, &palette, 32, 64, 8, 2, 16, &c);
    CacheTexture(&image, &palette, 32, 32, 9, 2, 16, &d);
    s_textures[0].age = ++s_textureAge; // A was used again
    CacheTexture(&image, &palette, 32, 32, 8, 0, 16, &e);
    assert(s_textureCount == kMaxTextures);
    assert(a.releases == 0 && b.releases == 1 && c.releases == 0 && d.releases == 0);
    assert(s_textures[1].texture == &e);
    M360_HsdRenderClearTextures();
    assert(!s_textureCount && !s_textureAge);
    assert(a.releases == 1 && b.releases == 1 && c.releases == 1 && d.releases == 1 && e.releases == 1);
    puts("PASS: texture descriptor identity, retry, LRU eviction and scene cleanup");
    return 0;
}
