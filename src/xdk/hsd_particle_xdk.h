#ifndef MELEE360_XDK_HSD_PARTICLE_H
#define MELEE360_XDK_HSD_PARTICLE_H

#ifdef __cplusplus
extern "C" {
#endif

/* View-space quad corner for the native particle path. */
typedef struct M360ParticleVertex {
    float x, y, z;
    float r, g, b, a;
    float u, v;
} M360ParticleVertex;

typedef struct M360ParticleTexture {
    const void* image;
    unsigned format, width, height;
    const void* palette;
    unsigned paletteFormat, paletteEntries;
} M360ParticleTexture;

/* Draws one quad (4 corners, clockwise) modulating the texture (white if
 * none) by the vertex colour. blend 0: alpha, 1: additive, 2: alpha without
 * depth test (debug overlays). */
void M360_HsdDrawParticle(const M360ParticleVertex* corners, const M360ParticleTexture* texture,
                          unsigned blend, int depthWrite);

#ifdef __cplusplus
}
#endif

#endif
