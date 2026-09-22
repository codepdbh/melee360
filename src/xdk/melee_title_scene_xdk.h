#ifndef MELEE360_XDK_TITLE_SCENE_H
#define MELEE360_XDK_TITLE_SCENE_H

struct MeleeTitleSceneStatus {
    bool symbolsResolved;
    bool modelsLoaded;
    bool animationsBound;
    unsigned expectedSymbols;
    unsigned resolvedSymbols;
    unsigned modelCount;
    unsigned jointCount;
    unsigned displayObjectCount;
    unsigned materialCount;
    unsigned polygonObjectCount;
    unsigned textureObjectCount;
    unsigned textureImageCount;
    unsigned firstTextureWidth;
    unsigned firstTextureHeight;
    unsigned firstTextureFormat;
    unsigned meshVertexCount;
};

struct MeleeTitleVertex {
    float x, y, z;
    float u, v;
    float u1, v1;
    float fog;
    unsigned color;
    const void* texture;
    const void* texture1;
    unsigned blend;
    unsigned cull;
};

bool M360_LoadTitleScene(MeleeTitleSceneStatus* status);
void M360_TitleEnter(bool openingMode);
bool M360_TitleUpdate(unsigned sceneTick);
unsigned M360_TitleClearColor(void);
bool M360_DecodeFirstTitleTexture(unsigned** pixels, unsigned* width,
                                  unsigned* height);
void M360_FreeDecodedTitleTexture(unsigned* pixels);
unsigned M360_TitleTextureWrap(const void* texture);
unsigned M360_BuildTitleMesh(MeleeTitleVertex* vertices, unsigned capacity);
bool M360_DecodeTitleTexture(const void* texture, unsigned** pixels,
                            unsigned* width, unsigned* height);

#endif
