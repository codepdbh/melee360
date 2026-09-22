#ifndef MELEE360_XDK_SPRITE_RENDERER_H
#define MELEE360_XDK_SPRITE_RENDERER_H

#include <xtl.h>

struct MeleeTitleVertex;

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
    void AddTitleMesh(const MeleeTitleVertex* vertices, unsigned count);
    void End(IDirect3DDevice9* device);
    void Shutdown();
    bool UsesExternalAtlas() const;

private:
    IDirect3DTexture9* ResolveTitleTexture(IDirect3DDevice9* device, const void* key);
    void DrawTitleMesh(IDirect3DDevice9* device);
    const void* textureKeys_[128];
    IDirect3DTexture9* titleTextures_[128];
    unsigned textureCount_;
    const void* vertexTextures_[32766];
    const void* vertexTextures1_[32766];
    unsigned vertexBlend_[32766];
    struct TitleVertex {
        float x, y, z, w;
        float red, green, blue, alpha;
        float u, v;
        float u1, v1, fog, unused;
    };
    TitleVertex titleVertices_[32766];
    IDirect3DVertexShader9* titleVertexShader_;
    IDirect3DPixelShader9* titlePixelShader_;
    IDirect3DVertexDeclaration9* titleDeclaration_;
    IDirect3DTexture9* whiteTexture_;
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
    static const unsigned kMaxTitleVertices = 32766;
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
    unsigned titleVertexCount_;
    bool bannerQueued_;
    bool gameTextureQueued_;
    bool externalAtlas_;
};

#endif
