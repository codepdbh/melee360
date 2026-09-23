#ifndef MELEE360_XDK_HSD_RENDER_H
#define MELEE360_XDK_HSD_RENDER_H

struct IDirect3DDevice9;

struct M360HsdRenderStats {
    unsigned drawCalls;
    unsigned triangles;
    unsigned vertices;
    unsigned textures;
    unsigned decodeFailures;
    unsigned envelopeVertices;
    unsigned erases;
    unsigned tevStages;
    unsigned unsupported;
};

bool M360_HsdRenderInit(IDirect3DDevice9* device);
void M360_HsdRenderShutdown(void);
void M360_HsdRenderBeginFrame(void);
void M360_HsdRenderEndFrame(void);
void M360_HsdRenderAllowErase(bool allow);
void M360_HsdRenderGetStats(M360HsdRenderStats* stats);
unsigned M360_HsdRenderEraseColor(void);
bool M360_HsdDecodeTexture(const void* tobj, unsigned** pixels,
                           unsigned* width, unsigned* height);
bool M360_HsdDecodeImage(const void* imagedesc, const void* tlut,
                         unsigned** pixels, unsigned* width, unsigned* height);
void M360_HsdFreeDecoded(unsigned* pixels);

#endif
