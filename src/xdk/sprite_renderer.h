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
    void End(IDirect3DDevice9* device);
    void Shutdown();

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
    };

    static const unsigned kMaxQuads = 4096;
    Vertex vertices_[kMaxQuads * 4];
    unsigned quadCount_;
    IDirect3DVertexShader9* vertexShader_;
    IDirect3DPixelShader9* pixelShader_;
    IDirect3DVertexDeclaration9* declaration_;
};

#endif
