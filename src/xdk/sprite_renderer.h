#ifndef MELEE360_XDK_SPRITE_RENDERER_H
#define MELEE360_XDK_SPRITE_RENDERER_H

#include <xtl.h>

struct SpriteColor {
    float red;
    float green;
    float blue;
    float alpha;
};

class SpriteRenderer {
public:
    SpriteRenderer();
    bool Initialize(IDirect3DDevice9* device);
    void Begin();
    void AddQuad(float x, float y, float width, float height,
                 const SpriteColor& color);
    void AddGradientQuad(float x, float y, float width, float height,
                         const SpriteColor& top,
                         const SpriteColor& bottom);
    void AddSprite(float x, float y, float width, float height,
                   float u0, float v0, float u1, float v1,
                   const SpriteColor& tint);
    bool UploadBanner(IDirect3DDevice9* device, const unsigned* pixels);
    void AddBanner(float x, float y, float width, float height);
    bool UploadGameTexture(IDirect3DDevice9* device, const unsigned* pixels,
                           unsigned width, unsigned height);
    void AddGameTexture(float x, float y, float width, float height);
    void End(IDirect3DDevice9* device);
    void Shutdown();
    bool UsesExternalAtlas() const;

private:
    struct Vertex {
        float x;
        float y;
        float z;
        float w;
        float red;
        float green;
        float blue;
        float alpha;
        float u;
        float v;
    };

    static const unsigned kMaxQuads = 16384;
    Vertex vertices_[kMaxQuads * 4];
    unsigned quadCount_;
    IDirect3DVertexShader9* vertexShader_;
    IDirect3DPixelShader9* pixelShader_;
    IDirect3DVertexDeclaration9* declaration_;
    IDirect3DTexture9* atlas_;
    IDirect3DTexture9* bannerTexture_;
    IDirect3DTexture9* gameTexture_;
    Vertex bannerVertices_[4];
    Vertex gameVertices_[4];
    bool bannerQueued_;
    bool gameTextureQueued_;
    bool externalAtlas_;
};

#endif
