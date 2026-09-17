#include "sprite_renderer.h"

#include "sprite_ps.h"
#include "sprite_vs.h"

SpriteRenderer::SpriteRenderer()
    : quadCount_(0), vertexShader_(0), pixelShader_(0), declaration_(0)
{
}

bool SpriteRenderer::Initialize(IDirect3DDevice9* device)
{
    if (FAILED(device->CreateVertexShader(
            reinterpret_cast<const DWORD*>(g_melee360SpriteVS),
            &vertexShader_)))
        return false;
    if (FAILED(device->CreatePixelShader(
            reinterpret_cast<const DWORD*>(g_melee360SpritePS),
            &pixelShader_)))
        return false;

    static const D3DVERTEXELEMENT9 elements[] = {
        { 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT,
          D3DDECLUSAGE_POSITION, 0 },
        { 0, 16, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT,
          D3DDECLUSAGE_COLOR, 0 },
        D3DDECL_END()
    };
    return SUCCEEDED(device->CreateVertexDeclaration(elements, &declaration_));
}

void SpriteRenderer::Begin()
{
    quadCount_ = 0;
}

void SpriteRenderer::AddQuad(float x, float y, float width, float height,
                             const SpriteColor& color)
{
    AddGradientQuad(x, y, width, height, color, color);
}

void SpriteRenderer::AddGradientQuad(float x, float y, float width,
                                     float height, const SpriteColor& top,
                                     const SpriteColor& bottom)
{
    if (quadCount_ >= kMaxQuads || width <= 0.0f || height <= 0.0f)
        return;

    Vertex* vertices = &vertices_[quadCount_ * 4];
    const float right = x + width;
    const float bottomY = y + height;
    const float positions[4][2] = {
        { x, y }, { right, y }, { right, bottomY }, { x, bottomY }
    };
    for (unsigned i = 0; i < 4; ++i) {
        const SpriteColor& color = i < 2 ? top : bottom;
        vertices[i].x = positions[i][0];
        vertices[i].y = positions[i][1];
        vertices[i].z = 0.0f;
        vertices[i].w = 1.0f;
        vertices[i].red = color.red;
        vertices[i].green = color.green;
        vertices[i].blue = color.blue;
        vertices[i].alpha = color.alpha;
    }
    ++quadCount_;
}

void SpriteRenderer::End(IDirect3DDevice9* device)
{
    if (!quadCount_)
        return;

    device->SetVertexShader(vertexShader_);
    device->SetPixelShader(pixelShader_);
    device->SetVertexDeclaration(declaration_);
    device->SetRenderState(D3DRS_VIEWPORTENABLE, TRUE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->DrawPrimitiveUP(D3DPT_QUADLIST, quadCount_, vertices_,
                            sizeof(Vertex));
}

void SpriteRenderer::Shutdown()
{
    if (declaration_)
        declaration_->Release();
    if (pixelShader_)
        pixelShader_->Release();
    if (vertexShader_)
        vertexShader_->Release();
    declaration_ = 0;
    pixelShader_ = 0;
    vertexShader_ = 0;
}
