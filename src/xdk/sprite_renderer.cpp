#include "sprite_renderer.h"

#include <d3dx9.h>

#include "sprite_ps.h"
#include "sprite_vs.h"

namespace {

const unsigned kAtlasWidth = 256;
const unsigned kAtlasHeight = 128;

void FillRect(DWORD* pixels, unsigned pitch, int x, int y, int width,
              int height, DWORD color)
{
    for (int row = y; row < y + height; ++row) {
        DWORD* destination = reinterpret_cast<DWORD*>(
            reinterpret_cast<BYTE*>(pixels) + row * pitch);
        for (int column = x; column < x + width; ++column)
            destination[column] = color;
    }
}

void FillEllipse(DWORD* pixels, unsigned pitch, int centerX, int centerY,
                 int radiusX, int radiusY, DWORD color)
{
    for (int y = -radiusY; y <= radiusY; ++y) {
        for (int x = -radiusX; x <= radiusX; ++x) {
            if (x * x * radiusY * radiusY + y * y * radiusX * radiusX <=
                radiusX * radiusX * radiusY * radiusY) {
                DWORD* destination = reinterpret_cast<DWORD*>(
                    reinterpret_cast<BYTE*>(pixels) + (centerY + y) * pitch);
                destination[centerX + x] = color;
            }
        }
    }
}

void CopyRegion(DWORD* pixels, unsigned pitch, int sourceX, int sourceY,
                int destinationX, int destinationY, int width, int height)
{
    for (int y = 0; y < height; ++y) {
        DWORD* source = reinterpret_cast<DWORD*>(
            reinterpret_cast<BYTE*>(pixels) + (sourceY + y) * pitch);
        DWORD* destination = reinterpret_cast<DWORD*>(
            reinterpret_cast<BYTE*>(pixels) + (destinationY + y) * pitch);
        for (int x = 0; x < width; ++x)
            destination[destinationX + x] = source[sourceX + x];
    }
}

bool BuildAtlas(IDirect3DDevice9* device, IDirect3DTexture9** atlas)
{
    if (FAILED(device->CreateTexture(kAtlasWidth, kAtlasHeight, 1, 0,
            D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED, atlas, 0)))
        return false;

    D3DLOCKED_RECT locked;
    if (FAILED((*atlas)->LockRect(0, &locked, 0, 0)))
        return false;
    ZeroMemory(locked.pBits, locked.Pitch * kAtlasHeight);
    DWORD* pixels = reinterpret_cast<DWORD*>(locked.pBits);

    FillRect(pixels, locked.Pitch, 0, 0, 4, 4,
             D3DCOLOR_ARGB(255, 255, 255, 255));

    /* Original cyan fighter sprite, stored in atlas columns 8..55. */
    FillEllipse(pixels, locked.Pitch, 32, 15, 13, 13,
                D3DCOLOR_ARGB(255, 225, 242, 255));
    FillRect(pixels, locked.Pitch, 21, 11, 5, 4,
             D3DCOLOR_ARGB(255, 13, 32, 58));
    FillRect(pixels, locked.Pitch, 37, 11, 5, 4,
             D3DCOLOR_ARGB(255, 13, 32, 58));
    FillRect(pixels, locked.Pitch, 26, 21, 12, 3,
             D3DCOLOR_ARGB(255, 70, 203, 247));
    FillRect(pixels, locked.Pitch, 17, 27, 30, 23,
             D3DCOLOR_ARGB(255, 29, 133, 218));
    FillRect(pixels, locked.Pitch, 20, 29, 24, 7,
             D3DCOLOR_ARGB(255, 86, 218, 255));
    FillRect(pixels, locked.Pitch, 10, 29, 8, 18,
             D3DCOLOR_ARGB(255, 72, 199, 244));
    FillRect(pixels, locked.Pitch, 47, 29, 8, 18,
             D3DCOLOR_ARGB(255, 72, 199, 244));
    FillRect(pixels, locked.Pitch, 20, 50, 10, 11,
             D3DCOLOR_ARGB(255, 107, 232, 52));
    FillRect(pixels, locked.Pitch, 35, 50, 10, 11,
             D3DCOLOR_ARGB(255, 107, 232, 52));
    FillRect(pixels, locked.Pitch, 12, 25, 41, 3,
             D3DCOLOR_ARGB(230, 255, 85, 137));

    CopyRegion(pixels, locked.Pitch, 8, 0, 64, 64, 48, 64);
    CopyRegion(pixels, locked.Pitch, 8, 0, 120, 64, 48, 64);
    CopyRegion(pixels, locked.Pitch, 8, 0, 176, 64, 48, 64);
    FillRect(pixels, locked.Pitch, 68, 108, 10, 12,
             D3DCOLOR_ARGB(255, 107, 232, 52));
    FillRect(pixels, locked.Pitch, 95, 112, 10, 9,
             D3DCOLOR_ARGB(255, 107, 232, 52));
    FillRect(pixels, locked.Pitch, 128, 116, 10, 8,
             D3DCOLOR_ARGB(255, 107, 232, 52));
    FillRect(pixels, locked.Pitch, 151, 116, 10, 8,
             D3DCOLOR_ARGB(255, 107, 232, 52));
    FillRect(pixels, locked.Pitch, 215, 91, 26, 9,
             D3DCOLOR_ARGB(255, 255, 226, 89));

    /* Original training dummy sprite, stored in columns 68..115. */
    FillEllipse(pixels, locked.Pitch, 92, 15, 12, 13,
                D3DCOLOR_ARGB(255, 244, 183, 91));
    FillRect(pixels, locked.Pitch, 86, 11, 4, 4,
             D3DCOLOR_ARGB(255, 48, 25, 34));
    FillRect(pixels, locked.Pitch, 96, 11, 4, 4,
             D3DCOLOR_ARGB(255, 48, 25, 34));
    FillRect(pixels, locked.Pitch, 80, 27, 24, 27,
             D3DCOLOR_ARGB(255, 169, 55, 76));
    FillRect(pixels, locked.Pitch, 73, 31, 7, 20,
             D3DCOLOR_ARGB(255, 223, 78, 91));
    FillRect(pixels, locked.Pitch, 104, 31, 7, 20,
             D3DCOLOR_ARGB(255, 223, 78, 91));
    FillRect(pixels, locked.Pitch, 82, 54, 9, 8,
             D3DCOLOR_ARGB(255, 93, 42, 64));
    FillRect(pixels, locked.Pitch, 94, 54, 9, 8,
             D3DCOLOR_ARGB(255, 93, 42, 64));
    FillRect(pixels, locked.Pitch, 78, 27, 28, 4,
             D3DCOLOR_ARGB(240, 255, 199, 79));

    (*atlas)->UnlockRect(0);
    return true;
}

} // namespace

SpriteRenderer::SpriteRenderer()
    : quadCount_(0), vertexShader_(0), pixelShader_(0), declaration_(0),
      atlas_(0), bannerTexture_(0), gameTexture_(0), bannerQueued_(false),
      gameTextureQueued_(false), externalAtlas_(false)
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
        { 0, 32, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT,
          D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END()
    };
    if (FAILED(device->CreateVertexDeclaration(elements, &declaration_)))
        return false;
    if (SUCCEEDED(D3DXCreateTextureFromFile(
            device, "game:\\assets\\sprite_atlas.png", &atlas_))) {
        externalAtlas_ = true;
        OutputDebugStringA("[M360][XEX] loaded external sprite atlas\n");
        return true;
    }
    OutputDebugStringA("[M360][XEX] external atlas missing; using fallback\n");
    return BuildAtlas(device, &atlas_);
}

void SpriteRenderer::Begin()
{
    quadCount_ = 0;
    bannerQueued_ = false;
    gameTextureQueued_ = false;
}

void SpriteRenderer::AddQuad(float x, float y, float width, float height,
                             const SpriteColor& color)
{
    const float whiteU = 2.0f / static_cast<float>(kAtlasWidth);
    const float whiteV = 2.0f / static_cast<float>(kAtlasHeight);
    AddSprite(x, y, width, height, whiteU, whiteV, whiteU, whiteV, color);
}

void SpriteRenderer::AddGradientQuad(float x, float y, float width,
                                     float height, const SpriteColor& top,
                                     const SpriteColor& bottom)
{
    if (quadCount_ >= kMaxQuads || width <= 0.0f || height <= 0.0f)
        return;
    const float whiteU = 2.0f / static_cast<float>(kAtlasWidth);
    const float whiteV = 2.0f / static_cast<float>(kAtlasHeight);
    AddSprite(x, y, width, height, whiteU, whiteV, whiteU, whiteV, top);
    Vertex* vertices = &vertices_[(quadCount_ - 1) * 4];
    vertices[2].red = vertices[3].red = bottom.red;
    vertices[2].green = vertices[3].green = bottom.green;
    vertices[2].blue = vertices[3].blue = bottom.blue;
    vertices[2].alpha = vertices[3].alpha = bottom.alpha;
}

void SpriteRenderer::AddSprite(float x, float y, float width, float height,
                               float u0, float v0, float u1, float v1,
                               const SpriteColor& tint)
{
    if (quadCount_ >= kMaxQuads || width <= 0.0f || height <= 0.0f)
        return;

    Vertex* vertices = &vertices_[quadCount_ * 4];
    const float positions[4][2] = {
        { x, y }, { x + width, y },
        { x + width, y + height }, { x, y + height }
    };
    const float texcoords[4][2] = {
        { u0, v0 }, { u1, v0 }, { u1, v1 }, { u0, v1 }
    };
    for (unsigned i = 0; i < 4; ++i) {
        vertices[i].x = positions[i][0];
        vertices[i].y = positions[i][1];
        vertices[i].z = 0.0f;
        vertices[i].w = 1.0f;
        vertices[i].red = tint.red;
        vertices[i].green = tint.green;
        vertices[i].blue = tint.blue;
        vertices[i].alpha = tint.alpha;
        vertices[i].u = texcoords[i][0];
        vertices[i].v = texcoords[i][1];
    }
    ++quadCount_;
}

bool SpriteRenderer::UploadBanner(IDirect3DDevice9* device,
                                  const unsigned* pixels)
{
    if (!pixels)
        return false;
    if (bannerTexture_) {
        bannerTexture_->Release();
        bannerTexture_ = 0;
    }
    if (FAILED(device->CreateTexture(96, 32, 1, 0,
            D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED, &bannerTexture_, 0)))
        return false;
    D3DLOCKED_RECT locked;
    if (FAILED(bannerTexture_->LockRect(0, &locked, 0, 0)))
        return false;
    for (unsigned y = 0; y < 32; ++y) {
        memcpy(reinterpret_cast<BYTE*>(locked.pBits) + y * locked.Pitch,
               pixels + y * 96, 96 * sizeof(unsigned));
    }
    bannerTexture_->UnlockRect(0);
    return true;
}

void SpriteRenderer::AddBanner(float x, float y, float width, float height)
{
    if (!bannerTexture_)
        return;
    const float positions[4][2] = {
        { x, y }, { x + width, y },
        { x + width, y + height }, { x, y + height }
    };
    const float texcoords[4][2] = {
        { 0.0f, 0.0f }, { 1.0f, 0.0f },
        { 1.0f, 1.0f }, { 0.0f, 1.0f }
    };
    for (unsigned i = 0; i < 4; ++i) {
        bannerVertices_[i].x = positions[i][0];
        bannerVertices_[i].y = positions[i][1];
        bannerVertices_[i].z = 0.0f;
        bannerVertices_[i].w = 1.0f;
        bannerVertices_[i].red = 1.0f;
        bannerVertices_[i].green = 1.0f;
        bannerVertices_[i].blue = 1.0f;
        bannerVertices_[i].alpha = 1.0f;
        bannerVertices_[i].u = texcoords[i][0];
        bannerVertices_[i].v = texcoords[i][1];
    }
    bannerQueued_ = true;
}

bool SpriteRenderer::UploadGameTexture(IDirect3DDevice9* device,
                                       const unsigned* pixels,
                                       unsigned width, unsigned height)
{
    if (!pixels || !width || !height)
        return false;
    if (gameTexture_) {
        gameTexture_->Release();
        gameTexture_ = 0;
    }
    if (FAILED(device->CreateTexture(width, height, 1, 0,
            D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED, &gameTexture_, 0)))
        return false;
    D3DLOCKED_RECT locked;
    if (FAILED(gameTexture_->LockRect(0, &locked, 0, 0)))
        return false;
    for (unsigned y = 0; y < height; ++y) {
        memcpy(reinterpret_cast<BYTE*>(locked.pBits) + y * locked.Pitch,
               pixels + y * width, width * sizeof(unsigned));
    }
    gameTexture_->UnlockRect(0);
    return true;
}

void SpriteRenderer::AddGameTexture(float x, float y, float width,
                                    float height)
{
    if (!gameTexture_)
        return;
    const float positions[4][2] = {
        { x, y }, { x + width, y },
        { x + width, y + height }, { x, y + height }
    };
    const float texcoords[4][2] = {
        { 0.0f, 0.0f }, { 1.0f, 0.0f },
        { 1.0f, 1.0f }, { 0.0f, 1.0f }
    };
    for (unsigned i = 0; i < 4; ++i) {
        gameVertices_[i].x = positions[i][0];
        gameVertices_[i].y = positions[i][1];
        gameVertices_[i].z = 0.0f;
        gameVertices_[i].w = 1.0f;
        gameVertices_[i].red = gameVertices_[i].green =
            gameVertices_[i].blue = gameVertices_[i].alpha = 1.0f;
        gameVertices_[i].u = texcoords[i][0];
        gameVertices_[i].v = texcoords[i][1];
    }
    gameTextureQueued_ = true;
}

void SpriteRenderer::End(IDirect3DDevice9* device)
{
    if (!quadCount_ && !bannerQueued_ && !gameTextureQueued_)
        return;

    device->SetVertexShader(vertexShader_);
    device->SetPixelShader(pixelShader_);
    device->SetVertexDeclaration(declaration_);
    device->SetRenderState(D3DRS_VIEWPORTENABLE, TRUE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    /* The quad batch includes the opaque backdrop: submit it before the mesh. */
    if (quadCount_) {
        device->SetTexture(0, atlas_);
        for (unsigned first = 0; first < quadCount_; first += 256) {
            unsigned count = quadCount_ - first;
            if (count > 256) count = 256;
            device->DrawPrimitiveUP(D3DPT_QUADLIST, count,
                                    vertices_ + first * 4, sizeof(Vertex));
        }
    }
    if (bannerQueued_) {
        device->SetTexture(0, bannerTexture_);
        device->DrawPrimitiveUP(D3DPT_QUADLIST, 1, bannerVertices_,
                                sizeof(Vertex));
    }
    if (gameTextureQueued_) {
        device->SetTexture(0, gameTexture_);
        device->DrawPrimitiveUP(D3DPT_QUADLIST, 1, gameVertices_,
                                sizeof(Vertex));
    }
}

void SpriteRenderer::Shutdown()
{
    if (atlas_)
        atlas_->Release();
    if (bannerTexture_)
        bannerTexture_->Release();
    if (gameTexture_)
        gameTexture_->Release();
    if (declaration_)
        declaration_->Release();
    if (pixelShader_)
        pixelShader_->Release();
    if (vertexShader_)
        vertexShader_->Release();
    declaration_ = 0;
    pixelShader_ = 0;
    vertexShader_ = 0;
    atlas_ = 0;
    bannerTexture_ = 0;
    gameTexture_ = 0;
    externalAtlas_ = false;
}

bool SpriteRenderer::UsesExternalAtlas() const
{
    return externalAtlas_;
}
