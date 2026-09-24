#include <xtl.h>
#include <math.h>
#undef near
#undef far

#pragma warning(push, 3)
#include "gameplay_probe_compat.h"
extern "C" {
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/displayfunc.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/fog.h>
#include <sysdolphin/baselib/initialize.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/tobj.h>
#include <sysdolphin/baselib/video.h>
}
#pragma warning(pop)

#include "hsd_render_xdk.h"
#include "hsd_texture_xdk.h"
#include "hsd_particle_xdk.h"
#include "hsd_ps.h"
#include "hsd_vs.h"

void M360_Trace(const char* stage, unsigned value);
extern "C" void M360_HsdVideoInit(void);

namespace {

struct LightSlot {
    float pos[4];
    float dir[4];
    float color[4];
    float cosAttn[4];
    float distAttn[4];
};

struct GxLightObject {
    float pos[3];
    float dir[3];
    float a[3];
    float k[3];
    unsigned char color[4];
};

struct GxState {
    float viewport[6];
    unsigned scissor[4];
    float projection[4][4];
    unsigned projectionType;
    LightSlot lights[8];
    unsigned fogType;
    float fogStart, fogEnd;
    GXColor fogColor;
};

struct TevStage {
    float w[4][4];
    float k[4][4];
    float wa[2][4];
    float op[4];
    float opa[4];
};

struct HsdVertex {
    float px, py, pz;
    float nx, ny, nz;
    float r, g, b, a;
    float u0, v0, u1, v1;
};

struct TextureEntry {
    const void* image;
    const void* palette;
    IDirect3DTexture9* texture;
};

const unsigned kMaxVertices = 12288;
const unsigned kMaxTextures = 1024;
const unsigned kVerticesPerDraw = 1536;

IDirect3DDevice9* s_device;
IDirect3DVertexShader9* s_vertexShader;
IDirect3DPixelShader9* s_pixelShader;
IDirect3DVertexDeclaration9* s_declaration;
IDirect3DTexture9* s_white;
TextureEntry s_textures[kMaxTextures];
unsigned s_textureCount;
GxState s_gx;
GXColor s_eraseColor;
bool s_allowErase = true;
M360HsdRenderStats s_stats;
HsdVertex s_vertices[kMaxVertices];

float Clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

unsigned Read16(const unsigned char* p)
{
    return (static_cast<unsigned>(p[0]) << 8) | p[1];
}

unsigned Read32(const unsigned char* p)
{
    return (static_cast<unsigned>(p[0]) << 24) |
           (static_cast<unsigned>(p[1]) << 16) |
           (static_cast<unsigned>(p[2]) << 8) | p[3];
}

void MapRect(float x, float y, float w, float h, D3DVIEWPORT9* vp)
{
    vp->X = static_cast<DWORD>(160.0f + x * 1.5f + 0.5f);
    vp->Y = static_cast<DWORD>(y * 1.5f + 0.5f);
    vp->Width = static_cast<DWORD>(w * 1.5f + 0.5f);
    vp->Height = static_cast<DWORD>(h * 1.5f + 0.5f);
    vp->MinZ = 0.0f;
    vp->MaxZ = 1.0f;
}

void ApplyViewport()
{
    D3DVIEWPORT9 vp;
    MapRect(s_gx.viewport[0], s_gx.viewport[1], s_gx.viewport[2],
            s_gx.viewport[3], &vp);
    if (vp.Width && vp.Height)
        s_device->SetViewport(&vp);
    RECT r;
    r.left = static_cast<LONG>(160 + s_gx.scissor[0] * 3 / 2);
    r.top = static_cast<LONG>(s_gx.scissor[1] * 3 / 2);
    r.right = static_cast<LONG>(160 + (s_gx.scissor[0] + s_gx.scissor[2]) * 3 / 2);
    r.bottom = static_cast<LONG>((s_gx.scissor[1] + s_gx.scissor[3]) * 3 / 2);
    if (r.right > r.left && r.bottom > r.top) {
        s_device->SetScissorRect(&r);
        s_device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
    }
}

GxLightObject* LightObject(GXLightObj* obj)
{
    return reinterpret_cast<GxLightObject*>(obj);
}

int LightIndex(unsigned id)
{
    for (int i = 0; i < 8; ++i)
        if (id == (1u << i))
            return i;
    return -1;
}

const unsigned char* PaletteData(const HSD_TObj* tobj, unsigned* format,
                                 unsigned* entries)
{
    const HSD_Tlut* tlut = tobj->tlut;
    if (tobj->tluttbl && tobj->tlut_no != 0xFF && tobj->tluttbl[tobj->tlut_no])
        tlut = tobj->tluttbl[tobj->tlut_no];
    *format = tlut ? static_cast<unsigned>(tlut->fmt) : 0;
    *entries = tlut ? tlut->n_entries : 0;
    return tlut ? static_cast<const unsigned char*>(tlut->lut) : NULL;
}

IDirect3DTexture9* ResolveTexture(const HSD_TObj* tobj)
{
    const HSD_ImageDesc* image = tobj ? tobj->imagedesc : NULL;
    if (!image || !image->image_ptr)
        return NULL;
    unsigned paletteFormat, paletteEntries;
    const unsigned char* palette = PaletteData(tobj, &paletteFormat, &paletteEntries);
    for (unsigned i = 0; i < s_textureCount; ++i)
        if (s_textures[i].image == image->image_ptr &&
            s_textures[i].palette == palette)
            return s_textures[i].texture;
    IDirect3DTexture9* texture = NULL;
    unsigned* pixels = NULL;
    unsigned width = 0, height = 0;
    if (M360_HsdDecodeImage(image, tobj, &pixels, &width, &height) &&
        SUCCEEDED(s_device->CreateTexture(width, height, 1, 0,
            D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED, &texture, NULL))) {
        D3DLOCKED_RECT locked;
        if (SUCCEEDED(texture->LockRect(0, &locked, NULL, 0))) {
            for (unsigned y = 0; y < height; ++y)
                memcpy(static_cast<BYTE*>(locked.pBits) + y * locked.Pitch,
                       pixels + y * width, width * sizeof(unsigned));
            texture->UnlockRect(0);
        } else {
            texture->Release();
            texture = NULL;
        }
    }
    M360_HsdFreeDecoded(pixels);
    if (!texture && s_stats.decodeFailures++ < 16)
        M360_Trace("hsd.texture.decode_failed", image->format);
    if (s_textureCount < kMaxTextures) {
        s_textures[s_textureCount].image = image->image_ptr;
        s_textures[s_textureCount].palette = palette;
        s_textures[s_textureCount++].texture = texture;
    }
    return texture;
}

void ClearStage(TevStage* s)
{
    memset(s, 0, sizeof(*s));
    s->op[0] = s->opa[0] = 1.0f;
    s->op[2] = s->opa[2] = 1.0f;
    s->op[3] = s->opa[3] = 1.0f;
}

void ColorIn(TevStage* s, int i, float wPrev, float wX, float wXa,
             float r, float g, float b)
{
    s->w[i][0] = wPrev;
    s->w[i][1] = wX;
    s->w[i][2] = wXa;
    s->k[i][0] = r;
    s->k[i][1] = g;
    s->k[i][2] = b;
}

void AlphaIn(TevStage* s, int i, float wPrev, float wX, float k)
{
    s->wa[0][i] = wPrev;
    s->wa[1][i] = wX;
    s->k[i][3] = k;
}

void SetOp(float* dst, unsigned op, unsigned bias, unsigned scale, unsigned clamp)
{
    static const float biases[4] = { 0.0f, 0.5f, -0.5f, 0.0f };
    static const float scales[4] = { 1.0f, 2.0f, 4.0f, 0.5f };
    if (op > 1)
        ++s_stats.unsupported;
    dst[0] = op == 1 ? -1.0f : 1.0f;
    dst[1] = biases[bias & 3];
    dst[2] = scales[scale & 3];
    dst[3] = clamp ? 1.0f : 0.0f;
}

float U8(u8 v) { return v / 255.0f; }

void TObjTevColorInput(TevStage* s, int i, unsigned in, const HSD_TObjTev* t)
{
    switch (in) {
    case GX_CC_ZERO: ColorIn(s, i, 0, 0, 0, 0, 0, 0); break;
    case GX_CC_ONE: ColorIn(s, i, 0, 0, 0, 1, 1, 1); break;
    case GX_CC_HALF: ColorIn(s, i, 0, 0, 0, 0.5f, 0.5f, 0.5f); break;
    case GX_CC_TEXC: ColorIn(s, i, 0, 1, 0, 0, 0, 0); break;
    case GX_CC_TEXA: ColorIn(s, i, 0, 0, 1, 0, 0, 0); break;
    case TOBJ_TEV_CC_KONST_RGB:
        ColorIn(s, i, 0, 0, 0, U8(t->konst.r), U8(t->konst.g), U8(t->konst.b));
        break;
    case TOBJ_TEV_CC_KONST_RRR:
        ColorIn(s, i, 0, 0, 0, U8(t->konst.r), U8(t->konst.r), U8(t->konst.r));
        break;
    case TOBJ_TEV_CC_KONST_GGG:
        ColorIn(s, i, 0, 0, 0, U8(t->konst.g), U8(t->konst.g), U8(t->konst.g));
        break;
    case TOBJ_TEV_CC_KONST_BBB:
        ColorIn(s, i, 0, 0, 0, U8(t->konst.b), U8(t->konst.b), U8(t->konst.b));
        break;
    case TOBJ_TEV_CC_KONST_AAA:
        ColorIn(s, i, 0, 0, 0, U8(t->konst.a), U8(t->konst.a), U8(t->konst.a));
        break;
    case TOBJ_TEV_CC_TEX0_RGB:
        ColorIn(s, i, 0, 0, 0, U8(t->tev0.r), U8(t->tev0.g), U8(t->tev0.b));
        break;
    case TOBJ_TEV_CC_TEX0_AAA:
        ColorIn(s, i, 0, 0, 0, U8(t->tev0.a), U8(t->tev0.a), U8(t->tev0.a));
        break;
    case TOBJ_TEV_CC_TEX1_RGB:
        ColorIn(s, i, 0, 0, 0, U8(t->tev1.r), U8(t->tev1.g), U8(t->tev1.b));
        break;
    case TOBJ_TEV_CC_TEX1_AAA:
        ColorIn(s, i, 0, 0, 0, U8(t->tev1.a), U8(t->tev1.a), U8(t->tev1.a));
        break;
    default:
        ColorIn(s, i, 0, 0, 0, 0, 0, 0);
        ++s_stats.unsupported;
        break;
    }
}

void TObjTevAlphaInput(TevStage* s, int i, unsigned in, const HSD_TObjTev* t)
{
    switch (in) {
    case GX_CA_ZERO: AlphaIn(s, i, 0, 0, 0); break;
    case GX_CA_TEXA: AlphaIn(s, i, 0, 1, 0); break;
    case TOBJ_TEV_CA_KONST_R: AlphaIn(s, i, 0, 0, U8(t->konst.r)); break;
    case TOBJ_TEV_CA_KONST_G: AlphaIn(s, i, 0, 0, U8(t->konst.g)); break;
    case TOBJ_TEV_CA_KONST_B: AlphaIn(s, i, 0, 0, U8(t->konst.b)); break;
    case TOBJ_TEV_CA_KONST_A: AlphaIn(s, i, 0, 0, U8(t->konst.a)); break;
    case TOBJ_TEV_CA_TEX0_A: AlphaIn(s, i, 0, 0, U8(t->tev0.a)); break;
    case TOBJ_TEV_CA_TEX1_A: AlphaIn(s, i, 0, 0, U8(t->tev1.a)); break;
    default:
        AlphaIn(s, i, 0, 0, 0);
        ++s_stats.unsupported;
        break;
    }
}

bool BuildTObjTev(const HSD_TObj* tobj, TevStage* s)
{
    const HSD_TObjTev* t = tobj->tev;
    if (!t || !(t->active & (TOBJ_TEVREG_ACTIVE_COLOR_TEV |
                             TOBJ_TEVREG_ACTIVE_ALPHA_TEV)))
        return false;
    ClearStage(s);
    if (t->active & TOBJ_TEVREG_ACTIVE_COLOR_TEV) {
        const u8* in = &t->color_a;
        for (int i = 0; i < 4; ++i)
            TObjTevColorInput(s, i, in[i], t);
        SetOp(s->op, t->color_op, t->color_bias, t->color_scale, t->color_clamp);
    } else {
        ColorIn(s, 3, 0, 1, 0, 0, 0, 0);
    }
    if (t->active & TOBJ_TEVREG_ACTIVE_ALPHA_TEV) {
        const u8* in = &t->alpha_a;
        for (int i = 0; i < 4; ++i)
            TObjTevAlphaInput(s, i, in[i], t);
        SetOp(s->opa, t->alpha_op, t->alpha_bias, t->alpha_scale, t->alpha_clamp);
    } else {
        AlphaIn(s, 3, 0, 1, 0);
    }
    ++s_stats.tevStages;
    return true;
}

void BuildColorMap(const HSD_TObj* tobj, TevStage* s)
{
    ClearStage(s);
    const float blend = tobj->blending;
    switch (tobj_colormap(tobj)) {
    case TEX_COLORMAP_ALPHA_MASK:
        ColorIn(s, 0, 1, 0, 0, 0, 0, 0);
        ColorIn(s, 1, 0, 1, 0, 0, 0, 0);
        ColorIn(s, 2, 0, 0, 1, 0, 0, 0);
        break;
    case TEX_COLORMAP_RGB_MASK:
        ColorIn(s, 0, 1, 0, 0, 0, 0, 0);
        ColorIn(s, 1, 0, 1, 0, 0, 0, 0);
        ColorIn(s, 2, 0, 1, 0, 0, 0, 0);
        break;
    case TEX_COLORMAP_BLEND:
        ColorIn(s, 0, 1, 0, 0, 0, 0, 0);
        ColorIn(s, 1, 0, 1, 0, 0, 0, 0);
        ColorIn(s, 2, 0, 0, 0, blend, blend, blend);
        break;
    case TEX_COLORMAP_MODULATE:
        ColorIn(s, 1, 1, 0, 0, 0, 0, 0);
        ColorIn(s, 2, 0, 1, 0, 0, 0, 0);
        break;
    case TEX_COLORMAP_REPLACE:
        ColorIn(s, 3, 0, 1, 0, 0, 0, 0);
        break;
    case TEX_COLORMAP_ADD:
        ColorIn(s, 0, 0, 1, 0, 0, 0, 0);
        ColorIn(s, 3, 1, 0, 0, 0, 0, 0);
        break;
    case TEX_COLORMAP_SUB:
        ColorIn(s, 0, 0, 1, 0, 0, 0, 0);
        ColorIn(s, 3, 1, 0, 0, 0, 0, 0);
        s->op[0] = -1.0f;
        break;
    default:
        ColorIn(s, 3, 1, 0, 0, 0, 0, 0);
        break;
    }
    switch (tobj_alphamap(tobj)) {
    case TEX_ALPHAMAP_ALPHA_MASK:
        AlphaIn(s, 0, 1, 0, 0);
        AlphaIn(s, 1, 0, 1, 0);
        AlphaIn(s, 2, 0, 1, 0);
        break;
    case TEX_ALPHAMAP_BLEND:
        AlphaIn(s, 0, 1, 0, 0);
        AlphaIn(s, 1, 0, 1, 0);
        AlphaIn(s, 2, 0, 0, blend);
        break;
    case TEX_ALPHAMAP_MODULATE:
        AlphaIn(s, 1, 1, 0, 0);
        AlphaIn(s, 2, 0, 1, 0);
        break;
    case TEX_ALPHAMAP_REPLACE:
        AlphaIn(s, 3, 0, 1, 0);
        break;
    case TEX_ALPHAMAP_ADD:
        AlphaIn(s, 0, 0, 1, 0);
        AlphaIn(s, 3, 1, 0, 0);
        break;
    case TEX_ALPHAMAP_SUB:
        AlphaIn(s, 0, 0, 1, 0);
        AlphaIn(s, 3, 1, 0, 0);
        s->opa[0] = -1.0f;
        break;
    default:
        AlphaIn(s, 3, 1, 0, 0);
        break;
    }
}

void TextureMatrix(const HSD_TObj* tobj, float* rows)
{
    memset(rows, 0, 8 * sizeof(float));
    rows[0] = rows[5] = 1.0f;
    if (!tobj->repeat_s || !tobj->repeat_t)
        return;
    const float sx = fabsf(tobj->scale.x) < 1.0e-10f ? 0.0f : tobj->repeat_s / tobj->scale.x;
    const float sy = fabsf(tobj->scale.y) < 1.0e-10f ? 0.0f : tobj->repeat_t / tobj->scale.y;
    Vec3 rot = { tobj->rotate.x, tobj->rotate.y, -tobj->rotate.z };
    const float mirror = tobj->wrap_t == GX_MIRROR && sy != 0.0f ? 1.0f / sy : 0.0f;
    Mtx m, r, s;
    PSMTXTrans(m, -tobj->translate.x, -(tobj->translate.y + mirror), tobj->translate.z);
    HSD_MkRotationMtx(r, &rot);
    PSMTXConcat(r, m, m);
    PSMTXScale(s, sx, sy, tobj->scale.z);
    PSMTXConcat(s, m, m);
    for (int row = 0; row < 2; ++row)
        for (int col = 0; col < 4; ++col)
            rows[row * 4 + col] = m[row][col];
}

struct PixelState {
    bool blend;
    DWORD blendOp, src, dst;
    bool zEnable, zWrite;
    DWORD zFunc;
    DWORD colorWrite;
    bool alphaTest;
    float compare0[4], compare1[4], op[4];
};

void CompareWeights(unsigned func, unsigned ref, float* out)
{
    out[0] = (func & 1) ? 1.0f : 0.0f;
    out[1] = (func & 2) ? 1.0f : 0.0f;
    out[2] = (func & 4) ? 1.0f : 0.0f;
    out[3] = static_cast<float>(ref);
}

void BuildPixelState(const HSD_MObj* mobj, PixelState* ps)
{
    static const DWORD srcFactors[8] = {
        D3DBLEND_ZERO, D3DBLEND_ONE, D3DBLEND_DESTCOLOR, D3DBLEND_INVDESTCOLOR,
        D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, D3DBLEND_DESTALPHA,
        D3DBLEND_INVDESTALPHA
    };
    static const DWORD dstFactors[8] = {
        D3DBLEND_ZERO, D3DBLEND_ONE, D3DBLEND_SRCCOLOR, D3DBLEND_INVSRCCOLOR,
        D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, D3DBLEND_DESTALPHA,
        D3DBLEND_INVDESTALPHA
    };
    const u32 rm = mobj->rendermode;
    unsigned type, src, dst, zEnable, zFunc, zUpdate, colorUpdate, alphaUpdate;
    unsigned comp0, ref0, aop, comp1, ref1;
    if (const HSD_PEDesc* pe = mobj->pe) {
        colorUpdate = pe->flags & 1;
        alphaUpdate = pe->flags & 2;
        type = pe->type;
        src = pe->src_factor;
        dst = pe->dst_factor;
        zEnable = pe->flags & 0x10;
        zFunc = pe->z_comp;
        zUpdate = pe->flags & 0x20;
        comp0 = pe->alpha_comp0;
        ref0 = pe->ref0;
        aop = pe->alpha_op;
        comp1 = pe->alpha_comp1;
        ref1 = pe->ref1;
    } else {
        colorUpdate = 1;
        alphaUpdate = 0;
        type = (rm & RENDER_XLU) ? 1 : 0;
        src = 4;
        dst = 5;
        zEnable = 1;
        zFunc = (rm & RENDER_ZMODE_ALWAYS) ? 7 : 3;
        zUpdate = (rm & RENDER_NO_ZUPDATE) ? 0 : 1;
        aop = 0;
        ref0 = ref1 = 0;
        if (!(rm & RENDER_NO_ZUPDATE) && (rm & RENDER_XLU))
            comp0 = comp1 = 4;
        else
            comp0 = comp1 = 7;
    }
    ps->blend = type == 1 || type == 3;
    ps->blendOp = type == 3 ? D3DBLENDOP_REVSUBTRACT : D3DBLENDOP_ADD;
    ps->src = type == 3 ? D3DBLEND_ONE : srcFactors[src & 7];
    ps->dst = type == 3 ? D3DBLEND_ONE : dstFactors[dst & 7];
    ps->zEnable = zEnable != 0;
    ps->zWrite = zUpdate != 0;
    static const DWORD compares[8] = {
        D3DCMP_NEVER, D3DCMP_LESS, D3DCMP_EQUAL, D3DCMP_LESSEQUAL,
        D3DCMP_GREATER, D3DCMP_NOTEQUAL, D3DCMP_GREATEREQUAL, D3DCMP_ALWAYS
    };
    ps->zFunc = compares[zFunc & 7];
    ps->colorWrite = (colorUpdate ? (D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN |
                                     D3DCOLORWRITEENABLE_BLUE) : 0) |
                     (alphaUpdate ? D3DCOLORWRITEENABLE_ALPHA : 0);
    CompareWeights(comp0 & 7, ref0, ps->compare0);
    CompareWeights(comp1 & 7, ref1, ps->compare1);
    memset(ps->op, 0, sizeof(ps->op));
    ps->op[aop & 3] = 1.0f;
    const bool always0 = (comp0 & 7) == 7, always1 = (comp1 & 7) == 7;
    const bool alwaysPass = ((aop & 3) == 0 && always0 && always1) ||
                            ((aop & 3) == 1 && (always0 || always1));
    ps->alphaTest = !alwaysPass;
}

struct MatrixSet {
    Mtx pos[10];
    Mtx nrm[10];
    bool valid[10];
};

void SetMatrix(MatrixSet* set, int slot, MtxPtr m, bool normals)
{
    PSMTXCopy(m, set->pos[slot]);
    if (normals)
        HSD_MtxInverseTranspose(m, set->nrm[slot]);
    else
        PSMTXCopy(m, set->nrm[slot]);
    set->valid[slot] = true;
}

bool SetupMatrices(HSD_JObj* jobj, HSD_PObj* pobj, MtxPtr vmtx, MtxPtr pmtx,
                   bool normals, MatrixSet* set)
{
    memset(set->valid, 0, sizeof(set->valid));
    switch (pobj_type(pobj)) {
    case POBJ_SKIN:
        SetMatrix(set, 0, pmtx, normals);
        if (pobj->u.jobj) {
            Mtx m;
            HSD_JObjSetupMatrix(pobj->u.jobj);
            PSMTXConcat(vmtx, pobj->u.jobj->mtx, m);
            SetMatrix(set, 1, m, normals);
        }
        return true;
    case POBJ_ENVELOPE: {
        Mtx nodeMtx;
        MtxPtr right = _HSD_mkEnvelopeModelNodeMtx(jobj, nodeMtx);
        int index = 0;
        for (HSD_SList* list = pobj->u.envelope_list; index < 10 && list;
             ++index, list = list->next) {
            HSD_Envelope* envelope = static_cast<HSD_Envelope*>(list->data);
            Mtx mtx, tmp;
            MtxPtr mtxp = mtx;
            if (!envelope || !envelope->jobj)
                return false;
            if (envelope->weight >= 1.0f - 1.0e-7f) {
                HSD_JObjSetupMatrix(envelope->jobj);
                if (right && envelope->jobj->envelopemtx)
                    PSMTXConcat(envelope->jobj->mtx, envelope->jobj->envelopemtx, mtx);
                else
                    mtxp = envelope->jobj->mtx;
            } else {
                memset(mtx, 0, sizeof(mtx));
                for (; envelope; envelope = envelope->next) {
                    HSD_JObj* jp = envelope->jobj;
                    if (!jp || !jp->envelopemtx)
                        return false;
                    HSD_JObjSetupMatrix(jp);
                    PSMTXConcat(jp->mtx, jp->envelopemtx, tmp);
                    HSD_MtxScaledAdd(tmp, mtx, mtx, envelope->weight);
                }
            }
            if (right) {
                PSMTXConcat(mtxp, right, tmp);
                PSMTXCopy(tmp, mtx);
                mtxp = mtx;
            }
            PSMTXConcat(vmtx, mtxp, tmp);
            SetMatrix(set, index, tmp, normals);
        }
        return index > 0;
    }
    default:
        SetMatrix(set, 0, pmtx, normals);
        return true;
    }
}

unsigned ComponentSize(GXCompType type)
{
    return type == GX_U8 || type == GX_S8 ? 1u : (type == GX_F32 ? 4u : 2u);
}

float ReadComponent(const unsigned char* data, GXCompType type, unsigned frac)
{
    const float divisor = static_cast<float>(1u << frac);
    switch (type) {
    case GX_U8: return data[0] / divisor;
    case GX_S8: return static_cast<signed char>(data[0]) / divisor;
    case GX_U16: return Read16(data) / divisor;
    case GX_S16: return static_cast<short>(Read16(data)) / divisor;
    case GX_F32: {
        union { unsigned bits; float value; } v;
        v.bits = Read32(data);
        return v.value;
    }
    default: return 0.0f;
    }
}

unsigned DirectSize(const HSD_VtxDescList* d)
{
    if (d->attr >= GX_VA_PNMTXIDX && d->attr <= GX_VA_TEX7MTXIDX)
        return 1;
    if (d->attr == GX_VA_CLR0 || d->attr == GX_VA_CLR1) {
        switch (d->comp_type) {
        case GX_RGB565: case GX_RGBA4: return 2;
        case GX_RGB8: case GX_RGBA6: return 3;
        default: return 4;
        }
    }
    unsigned n = 1;
    if (d->attr == GX_VA_POS)
        n = d->comp_cnt == GX_POS_XYZ ? 3 : 2;
    else if (d->attr == GX_VA_NRM || d->attr == GX_VA_NBT)
        n = d->comp_cnt == GX_NRM_XYZ ? 3 : 9;
    else if (d->attr >= GX_VA_TEX0 && d->attr <= GX_VA_TEX7)
        n = d->comp_cnt == GX_TEX_ST ? 2 : 1;
    return n * ComponentSize(d->comp_type);
}

unsigned Expand4(unsigned v) { return (v << 4) | v; }

void DecodeColor(const unsigned char* p, GXCompType type, float* rgba)
{
    unsigned r = 255, g = 255, b = 255, a = 255;
    switch (type) {
    case GX_RGB565: {
        const unsigned v = Read16(p);
        r = ((v >> 11) & 31) * 255 / 31; g = ((v >> 5) & 63) * 255 / 63; b = (v & 31) * 255 / 31;
        break;
    }
    case GX_RGB8: case GX_RGBX8: r = p[0]; g = p[1]; b = p[2]; break;
    case GX_RGBA4:
        r = Expand4(p[0] >> 4); g = Expand4(p[0] & 15); b = Expand4(p[1] >> 4); a = Expand4(p[1] & 15);
        break;
    case GX_RGBA6: {
        const unsigned v = (p[0] << 16) | (p[1] << 8) | p[2];
        r = ((v >> 18) & 63) * 255 / 63; g = ((v >> 12) & 63) * 255 / 63;
        b = ((v >> 6) & 63) * 255 / 63; a = (v & 63) * 255 / 63;
        break;
    }
    case GX_RGBA8: r = p[0]; g = p[1]; b = p[2]; a = p[3]; break;
    default: break;
    }
    rgba[0] = r / 255.0f; rgba[1] = g / 255.0f; rgba[2] = b / 255.0f; rgba[3] = a / 255.0f;
}

struct RawVertex {
    float pos[3], nrm[3], color[4], uv0[2], uv1[2];
    unsigned matrix;
};

bool ParseVertex(const HSD_VtxDescList* descs, const unsigned char** cursor,
                 const unsigned char* end, RawVertex* v)
{
    const unsigned char* s = *cursor;
    memset(v, 0, sizeof(*v));
    v->color[0] = v->color[1] = v->color[2] = v->color[3] = 1.0f;
    unsigned count = 0;
    for (const HSD_VtxDescList* d = descs; d && d->attr != GX_VA_NULL && count < 32; ++d, ++count) {
        if (d->attr_type == GX_NONE)
            continue;
        const unsigned char* value = s;
        if (d->attr_type == GX_INDEX8) {
            if (s + 1 > end) return false;
            value = static_cast<const unsigned char*>(d->vertex) + *s * d->stride;
            s += 1;
        } else if (d->attr_type == GX_INDEX16) {
            if (s + 2 > end) return false;
            value = static_cast<const unsigned char*>(d->vertex) + Read16(s) * d->stride;
            s += 2;
        } else {
            const unsigned size = DirectSize(d);
            if (s + size > end) return false;
            s += size;
        }
        const unsigned cs = ComponentSize(d->comp_type);
        switch (d->attr) {
        case GX_VA_PNMTXIDX:
            v->matrix = value[0] / 3;
            break;
        case GX_VA_POS:
            v->pos[0] = ReadComponent(value, d->comp_type, d->frac);
            v->pos[1] = ReadComponent(value + cs, d->comp_type, d->frac);
            if (d->comp_cnt == GX_POS_XYZ)
                v->pos[2] = ReadComponent(value + cs * 2, d->comp_type, d->frac);
            break;
        case GX_VA_NRM: case GX_VA_NBT: {
            const unsigned frac = d->comp_type == GX_S8 ? 6 : (d->comp_type == GX_S16 ? 14 : d->frac);
            for (int i = 0; i < 3; ++i)
                v->nrm[i] = ReadComponent(value + cs * i, d->comp_type, frac);
            break;
        }
        case GX_VA_CLR0:
            DecodeColor(value, d->comp_type, v->color);
            break;
        case GX_VA_TEX0: case GX_VA_TEX1: {
            float* uv = d->attr == GX_VA_TEX0 ? v->uv0 : v->uv1;
            uv[0] = ReadComponent(value, d->comp_type, d->frac);
            if (d->comp_cnt == GX_TEX_ST)
                uv[1] = ReadComponent(value + cs, d->comp_type, d->frac);
            break;
        }
        default:
            break;
        }
    }
    *cursor = s;
    return true;
}

void Transform(const MatrixSet* set, const RawVertex& raw, HsdVertex* out)
{
    const unsigned slot = raw.matrix < 10 && set->valid[raw.matrix] ? raw.matrix : 0;
    const float (*m)[4] = set->pos[slot];
    const float (*n)[4] = set->nrm[slot];
    const float x = raw.pos[0], y = raw.pos[1], z = raw.pos[2];
    out->px = m[0][0] * x + m[0][1] * y + m[0][2] * z + m[0][3];
    out->py = m[1][0] * x + m[1][1] * y + m[1][2] * z + m[1][3];
    out->pz = m[2][0] * x + m[2][1] * y + m[2][2] * z + m[2][3];
    out->nx = n[0][0] * raw.nrm[0] + n[0][1] * raw.nrm[1] + n[0][2] * raw.nrm[2];
    out->ny = n[1][0] * raw.nrm[0] + n[1][1] * raw.nrm[1] + n[1][2] * raw.nrm[2];
    out->nz = n[2][0] * raw.nrm[0] + n[2][1] * raw.nrm[1] + n[2][2] * raw.nrm[2];
    out->r = raw.color[0]; out->g = raw.color[1]; out->b = raw.color[2]; out->a = raw.color[3];
    out->u0 = raw.uv0[0]; out->v0 = raw.uv0[1];
    out->u1 = raw.uv1[0]; out->v1 = raw.uv1[1];
}

void Flush(unsigned* count)
{
    for (unsigned first = 0; first < *count; first += kVerticesPerDraw) {
        unsigned n = *count - first;
        if (n > kVerticesPerDraw) n = kVerticesPerDraw;
        s_device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, n / 3, s_vertices + first,
                                  sizeof(HsdVertex));
        ++s_stats.drawCalls;
        s_stats.triangles += n / 3;
    }
    *count = 0;
}

void Emit(unsigned* count, const HsdVertex& a, const HsdVertex& b, const HsdVertex& c)
{
    if (*count + 3 > kMaxVertices)
        Flush(count);
    s_vertices[(*count)++] = a;
    s_vertices[(*count)++] = b;
    s_vertices[(*count)++] = c;
}

void DrawPObj(HSD_PObj* pobj, const MatrixSet* set, unsigned* count)
{
    if (!pobj->verts || !pobj->display || pobj->n_display > 0x4000)
        return;
    const unsigned char* s = pobj->display;
    const unsigned char* end = s + (static_cast<unsigned>(pobj->n_display) << 5);
    while (s + 3 <= end) {
        const unsigned primitive = *s & 0xF8;
        if (primitive == 0)
            break;
        const unsigned n = Read16(s + 1);
        s += 3;
        HsdVertex first, previous, current, ring[4];
        RawVertex raw;
        for (unsigned i = 0; i < n; ++i) {
            if (!ParseVertex(pobj->verts, &s, end, &raw))
                return;
            Transform(set, raw, &current);
            ++s_stats.vertices;
            if (pobj_type(pobj) == POBJ_ENVELOPE)
                ++s_stats.envelopeVertices;
            switch (primitive) {
            case GX_TRIANGLES:
                ring[i % 3] = current;
                if (i % 3 == 2)
                    Emit(count, ring[0], ring[1], ring[2]);
                break;
            case GX_QUADS:
                ring[i % 4] = current;
                if (i % 4 == 3) {
                    Emit(count, ring[0], ring[1], ring[2]);
                    Emit(count, ring[0], ring[2], ring[3]);
                }
                break;
            case GX_TRIANGLESTRIP:
                if (i >= 2) {
                    if (i & 1)
                        Emit(count, previous, first, current);
                    else
                        Emit(count, first, previous, current);
                }
                first = i == 0 ? current : previous;
                previous = current;
                break;
            case GX_TRIANGLEFAN:
                if (i == 0)
                    first = current;
                else if (i >= 2)
                    Emit(count, first, previous, current);
                previous = current;
                break;
            default:
                break;
            }
        }
    }
}

void SetLightsAndChannels(const HSD_MObj* mobj, float* vs)
{
    const u32 rm = mobj->rendermode;
    const HSD_Material* mat = mobj->mat;
    float* ambient = vs + 5 * 4;
    float* material = vs + 6 * 4;
    float* mode = vs + 7 * 4;
    ambient[0] = ambient[1] = ambient[2] = ambient[3] = 0.0f;
    material[0] = material[1] = material[2] = material[3] = 1.0f;
    mode[0] = mode[1] = mode[2] = mode[3] = 0.0f;
    unsigned diffuseMask = 0, alphaMask = 0, specMask = 0;
    if ((rm & RENDER_SPECULAR) && mat) {
        specMask = HSD_LObjGetLightMaskSpecular();
        for (int i = 0; i < HSD_LObjGetNbActive(); ++i) {
            HSD_LObj* lobj = HSD_LObjGetActiveByIndex(i);
            if (lobj)
                HSD_LObjSetup(lobj, lobj->color, mat->shininess);
        }
        mode[2] = 1.0f;
    }
    switch (rm & 7) {
    case 2:
        mode[0] = 1.0f;
        mode[1] = 0.0f;
        break;
    case 4: {
        HSD_LObj* amb = HSD_LObjGetActiveByID(static_cast<GXLightID>(0x100));
        if (amb && (amb->flags & LOBJ_DIFFUSE) && mat) {
            ambient[0] = mat->ambient.r * amb->color.r / 65025.0f;
            ambient[1] = mat->ambient.g * amb->color.g / 65025.0f;
            ambient[2] = mat->ambient.b * amb->color.b / 65025.0f;
        }
        diffuseMask = HSD_LObjGetLightMaskDiffuse();
        alphaMask = HSD_LObjGetLightMaskAlpha();
        mode[0] = 2.0f;
        if (amb && (amb->flags & LOBJ_ALPHA)) {
            ambient[3] = amb->color.a / 255.0f;
            mode[1] = 1.0f;
        } else if (alphaMask) {
            mode[1] = 1.0f;
        }
        break;
    }
    default:
        break;
    }
    float* lights = vs + 16 * 4;
    for (int i = 0; i < 8; ++i) {
        float* l = lights + i * 24;
        memcpy(l, &s_gx.lights[i], sizeof(LightSlot));
        l[20] = (diffuseMask & (1u << i)) ? 1.0f : 0.0f;
        l[21] = (specMask & (1u << i)) ? 1.0f : 0.0f;
        l[22] = (alphaMask & (1u << i)) ? 1.0f : 0.0f;
        l[23] = 0.0f;
    }
}

void BindSampler(DWORD stage, const HSD_TObj* tobj)
{
    static const DWORD modes[4] = { D3DTADDRESS_CLAMP, D3DTADDRESS_WRAP,
                                    D3DTADDRESS_MIRROR, D3DTADDRESS_CLAMP };
    IDirect3DTexture9* texture = tobj ? ResolveTexture(tobj) : NULL;
    s_device->SetTexture(stage, texture ? texture : s_white);
    s_device->SetSamplerState(stage, D3DSAMP_ADDRESSU,
                              modes[tobj ? (tobj->wrap_s & 3) : 0]);
    s_device->SetSamplerState(stage, D3DSAMP_ADDRESSV,
                              modes[tobj ? (tobj->wrap_t & 3) : 0]);
    const bool nearMag = tobj && tobj->magFilt == GX_NEAR;
    const bool nearMin = tobj && tobj->lod &&
        (tobj->lod->minFilt == GX_NEAR || tobj->lod->minFilt == GX_NEAR_MIP_NEAR ||
         tobj->lod->minFilt == GX_NEAR_MIP_LIN);
    s_device->SetSamplerState(stage, D3DSAMP_MAGFILTER, nearMag ? D3DTEXF_POINT : D3DTEXF_LINEAR);
    s_device->SetSamplerState(stage, D3DSAMP_MINFILTER, nearMin ? D3DTEXF_POINT : D3DTEXF_LINEAR);
    s_device->SetSamplerState(stage, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
}

void RenderDObj(HSD_JObj* jobj, HSD_DObj* dobj, MtxPtr vmtx, MtxPtr pmtx)
{
    HSD_MObj* mobj = dobj->mobj;
    if (!mobj || !s_device)
        return;
    const u32 rm = mobj->rendermode;
    const HSD_Material* mat = mobj->mat;

    float vs[64 * 4];
    memset(vs, 0, sizeof(vs));
    memcpy(vs, s_gx.projection, sizeof(s_gx.projection));
    vs[16] = s_gx.fogStart;
    vs[17] = s_gx.fogEnd;
    vs[18] = static_cast<float>(s_gx.fogType & 7);
    SetLightsAndChannels(mobj, vs);

    float ps[54 * 4];
    memset(ps, 0, sizeof(ps));
    BOOL flags[11];
    memset(flags, 0, sizeof(flags));

    HSD_TObj* slots[2] = { NULL, NULL };
    unsigned used = 0;
    for (HSD_TObj* t = mobj->tobj; t && used < 2; t = t->next) {
        if (t->id == GX_TEXMAP_NULL || !t->imagedesc)
            continue;
        const u32 lm = t->flags & (TEX_LIGHTMAP_DIFFUSE | TEX_LIGHTMAP_AMBIENT |
                                   TEX_LIGHTMAP_SPECULAR | TEX_LIGHTMAP_EXT);
        if (!lm || tobj_coord(t) == TEX_COORD_TOON || tobj_bump(t))
            continue;
        TevStage* tev = reinterpret_cast<TevStage*>(ps + used * 24 * 4);
        TevStage* map = reinterpret_cast<TevStage*>(ps + (used * 24 + 12) * 4);
        flags[4 + used] = BuildTObjTev(t, tev);
        BuildColorMap(t, map);
        const bool post = (lm & TEX_LIGHTMAP_EXT) && !(lm & (TEX_LIGHTMAP_DIFFUSE | TEX_LIGHTMAP_AMBIENT));
        flags[(post ? 2 : 0) + used] = TRUE;
        TextureMatrix(t, vs + (8 + used * 2) * 4);
        const unsigned coord = tobj_coord(t);
        if (coord == TEX_COORD_REFLECTION)
            vs[12 * 4 + used] = 2.0f;
        else if (coord != TEX_COORD_UV)
            ++s_stats.unsupported;
        else
            vs[12 * 4 + used] = t->src == GX_TG_TEX1 ? 1.0f : 0.0f;
        slots[used++] = t;
    }
    flags[6] = (rm & RENDER_VERTEX) != 0;
    flags[7] = (rm & RENDER_DIFFUSE) != 0;
    flags[8] = (rm & RENDER_SPECULAR) != 0;
    if (mat) {
        ps[48 * 4 + 0] = U8(mat->diffuse.r);
        ps[48 * 4 + 1] = U8(mat->diffuse.g);
        ps[48 * 4 + 2] = U8(mat->diffuse.b);
        ps[48 * 4 + 3] = Clamp01(mat->alpha);
        ps[49 * 4 + 0] = U8(mat->specular.r);
        ps[49 * 4 + 1] = U8(mat->specular.g);
        ps[49 * 4 + 2] = U8(mat->specular.b);
    } else {
        ps[48 * 4 + 0] = ps[48 * 4 + 1] = ps[48 * 4 + 2] = ps[48 * 4 + 3] = 1.0f;
    }
    ps[50 * 4 + 0] = U8(s_gx.fogColor.r);
    ps[50 * 4 + 1] = U8(s_gx.fogColor.g);
    ps[50 * 4 + 2] = U8(s_gx.fogColor.b);
    ps[50 * 4 + 3] = 1.0f;
    flags[10] = (s_gx.fogType & 7) >= 2;

    PixelState pe;
    BuildPixelState(mobj, &pe);
    flags[9] = pe.alphaTest;
    memcpy(ps + 51 * 4, pe.compare0, sizeof(pe.compare0));
    memcpy(ps + 52 * 4, pe.compare1, sizeof(pe.compare1));
    memcpy(ps + 53 * 4, pe.op, sizeof(pe.op));

    s_device->SetVertexShader(s_vertexShader);
    s_device->SetPixelShader(s_pixelShader);
    s_device->SetVertexDeclaration(s_declaration);
    s_device->SetVertexShaderConstantF(0, vs, 64);
    s_device->SetPixelShaderConstantF(0, ps, 54);
    s_device->SetPixelShaderConstantB(0, flags, 11);
    BindSampler(0, slots[0]);
    BindSampler(1, slots[1]);
    s_device->SetRenderState(D3DRS_ALPHABLENDENABLE, pe.blend);
    s_device->SetRenderState(D3DRS_BLENDOP, pe.blendOp);
    s_device->SetRenderState(D3DRS_SRCBLEND, pe.src);
    s_device->SetRenderState(D3DRS_DESTBLEND, pe.dst);
    s_device->SetRenderState(D3DRS_ZENABLE, pe.zEnable ? D3DZB_TRUE : D3DZB_FALSE);
    s_device->SetRenderState(D3DRS_ZWRITEENABLE, pe.zWrite);
    s_device->SetRenderState(D3DRS_ZFUNC, pe.zFunc);
    s_device->SetRenderState(D3DRS_COLORWRITEENABLE, pe.colorWrite);
    s_device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

    const bool normals = (rm & (RENDER_DIFFUSE | RENDER_SPECULAR)) != 0 ||
                         vs[12 * 4] > 1.5f || vs[12 * 4 + 1] > 1.5f;
    MatrixSet set;
    unsigned count = 0;
    for (HSD_PObj* pobj = dobj->pobj; pobj; pobj = pobj->next) {
        const unsigned cull = pobj->flags & (POBJ_CULLFRONT | POBJ_CULLBACK);
        if (cull == (POBJ_CULLFRONT | POBJ_CULLBACK))
            continue;
        if (pobj_type(pobj) == POBJ_SHAPEANIM)
            ++s_stats.unsupported;
        if (!SetupMatrices(jobj, pobj, vmtx, pmtx, normals, &set))
            continue;
        const DWORD mode = cull == POBJ_CULLBACK ? D3DCULL_CCW :
                           (cull == POBJ_CULLFRONT ? D3DCULL_CW : D3DCULL_NONE);
        Flush(&count);
        s_device->SetRenderState(D3DRS_CULLMODE, mode);
        DrawPObj(pobj, &set, &count);
        Flush(&count);
    }
}

} // namespace

extern "C" {

HSD_RenderPass HSD_GetCurrentRenderPass(void)
{
    return HSD_RP_SCREEN;
}

u32 VIGetNextField(void)
{
    return 0;
}

f32 pc_widescreen_cobj_scale(struct HSD_CObj* cobj)
{
    (void) cobj;
    return 1.0f;
}

void pc_widescreen_widen(f32 scale, f32 center, f32* left, f32* right)
{
    *left = center + (*left - center) * scale;
    *right = center + (*right - center) * scale;
}

void _HSD_ZListSort(void) {}
void _HSD_ZListDisp(void) {}
void _HSD_ZListClear(void) {}

void GXSetViewport(f32 left, f32 top, f32 wd, f32 ht, f32 nearz, f32 farz)
{
    s_gx.viewport[0] = left; s_gx.viewport[1] = top;
    s_gx.viewport[2] = wd; s_gx.viewport[3] = ht;
    s_gx.viewport[4] = nearz; s_gx.viewport[5] = farz;
    if (s_device)
        ApplyViewport();
}

void GXSetViewportJitter(f32 left, f32 top, f32 wd, f32 ht, f32 nearz,
                         f32 farz, u32 field)
{
    (void) field;
    GXSetViewport(left, top, wd, ht, nearz, farz);
}

void GXSetScissor(u32 left, u32 top, u32 wd, u32 ht)
{
    s_gx.scissor[0] = left; s_gx.scissor[1] = top;
    s_gx.scissor[2] = wd; s_gx.scissor[3] = ht;
    if (s_device)
        ApplyViewport();
}

void GXSetProjection(f32 mtx[4][4], GXProjectionType type)
{
    memcpy(s_gx.projection, mtx, sizeof(s_gx.projection));
    s_gx.projectionType = static_cast<unsigned>(type);
}

void GXGetProjectionv(f32* p)
{
    const float (*m)[4] = s_gx.projection;
    p[0] = static_cast<f32>(s_gx.projectionType);
    p[1] = m[0][0];
    p[3] = m[1][1];
    p[5] = m[2][2];
    p[6] = m[2][3];
    if (s_gx.projectionType == GX_PERSPECTIVE) {
        p[2] = m[0][2];
        p[4] = m[1][2];
    } else {
        p[2] = m[0][3];
        p[4] = m[1][3];
    }
}

void GXGetViewportv(f32* vp)
{
    memcpy(vp, s_gx.viewport, sizeof(s_gx.viewport));
}

void GXSetFog(GXFogType type, f32 startz, f32 endz, f32 nearz, f32 farz,
              GXColor color)
{
    (void) nearz;
    (void) farz;
    s_gx.fogType = static_cast<unsigned>(type);
    s_gx.fogStart = startz;
    s_gx.fogEnd = endz;
    s_gx.fogColor = color;
}

void GXInitFogAdjTable(GXFogAdjTable* table, u16 width, f32 projmtx[4][4])
{
    (void) width;
    (void) projmtx;
    memset(table, 0, sizeof(*table));
}

void GXSetFogRangeAdj(GXBool enable, u16 center, GXFogAdjTable* table)
{
    (void) enable;
    (void) center;
    (void) table;
}

void GXInitLightAttn(GXLightObj* obj, f32 a0, f32 a1, f32 a2, f32 k0,
                     f32 k1, f32 k2)
{
    GxLightObject* l = LightObject(obj);
    l->a[0] = a0; l->a[1] = a1; l->a[2] = a2;
    l->k[0] = k0; l->k[1] = k1; l->k[2] = k2;
}

void GXInitLightSpot(GXLightObj* obj, f32 cutoff, GXSpotFn fn)
{
    GxLightObject* l = LightObject(obj);
    if (cutoff <= 0.0f || cutoff > 90.0f)
        fn = GX_SP_OFF;
    const float d = cosf(cutoff * 3.14159265f / 180.0f);
    float a0 = 1.0f, a1 = 0.0f, a2 = 0.0f, t;
    switch (fn) {
    case GX_SP_FLAT: a0 = -1000.0f * d; a1 = 1000.0f; break;
    case GX_SP_COS: a0 = -d / (1.0f - d); a1 = 1.0f / (1.0f - d); break;
    case GX_SP_COS2: a0 = 0.0f; a1 = -d / (1.0f - d); a2 = 1.0f / (1.0f - d); break;
    case GX_SP_SHARP:
        t = (1.0f - d) * (1.0f - d);
        a0 = d * (d - 2.0f) / t; a1 = 2.0f / t; a2 = -1.0f / t;
        break;
    case GX_SP_RING1:
        t = (1.0f - d) * (1.0f - d);
        a0 = -4.0f * d / t; a1 = 4.0f * (1.0f + d) / t; a2 = -4.0f / t;
        break;
    case GX_SP_RING2:
        t = (1.0f - d) * (1.0f - d);
        a0 = 1.0f - 2.0f * d * d / t; a1 = 4.0f * d / t; a2 = -2.0f / t;
        break;
    default: break;
    }
    l->a[0] = a0; l->a[1] = a1; l->a[2] = a2;
}

void GXInitLightDistAttn(GXLightObj* obj, f32 refDist, f32 refBright,
                         GXDistAttnFn fn)
{
    GxLightObject* l = LightObject(obj);
    if (refDist < 0.0f || refBright <= 0.0f || refBright >= 1.0f)
        fn = GX_DA_OFF;
    float k0 = 1.0f, k1 = 0.0f, k2 = 0.0f;
    switch (fn) {
    case GX_DA_GENTLE: k1 = (1.0f - refBright) / (refBright * refDist); break;
    case GX_DA_MEDIUM:
        k1 = 0.5f * (1.0f - refBright) / (refBright * refDist);
        k2 = 0.5f * (1.0f - refBright) / (refBright * refDist * refDist);
        break;
    case GX_DA_STEEP: k2 = (1.0f - refBright) / (refBright * refDist * refDist); break;
    default: break;
    }
    l->k[0] = k0; l->k[1] = k1; l->k[2] = k2;
}

void GXInitLightPos(GXLightObj* obj, f32 x, f32 y, f32 z)
{
    GxLightObject* l = LightObject(obj);
    l->pos[0] = x; l->pos[1] = y; l->pos[2] = z;
}

void GXInitLightDir(GXLightObj* obj, f32 nx, f32 ny, f32 nz)
{
    GxLightObject* l = LightObject(obj);
    l->dir[0] = -nx; l->dir[1] = -ny; l->dir[2] = -nz;
}

void GXInitLightColor(GXLightObj* obj, GXColor color)
{
    GxLightObject* l = LightObject(obj);
    l->color[0] = color.r; l->color[1] = color.g;
    l->color[2] = color.b; l->color[3] = color.a;
}

void GXLoadLightObjImm(GXLightObj* obj, GXLightID id)
{
    const int index = LightIndex(static_cast<unsigned>(id));
    if (index < 0)
        return;
    const GxLightObject* l = LightObject(obj);
    LightSlot* slot = &s_gx.lights[index];
    for (int i = 0; i < 3; ++i) {
        slot->pos[i] = l->pos[i];
        slot->dir[i] = l->dir[i];
        slot->cosAttn[i] = l->a[i];
        slot->distAttn[i] = l->k[i];
    }
    for (int i = 0; i < 4; ++i)
        slot->color[i] = l->color[i] / 255.0f;
}

void HSD_SetEraseColor(u8 r, u8 g, u8 b, u8 a)
{
    s_eraseColor.r = r; s_eraseColor.g = g;
    s_eraseColor.b = b; s_eraseColor.a = a;
}

void HSD_EraseRect(f32 top, f32 bottom, f32 left, f32 right, f32 z,
                   int enable_color, int enable_alpha, int enable_depth)
{
    (void) top; (void) bottom; (void) left; (void) right; (void) z;
    if (!s_device || !s_allowErase)
        return;
    DWORD flags = 0;
    if (enable_color || enable_alpha)
        flags |= D3DCLEAR_TARGET;
    if (enable_depth)
        flags |= D3DCLEAR_ZBUFFER;
    if (!flags)
        return;
    D3DVIEWPORT9 vp;
    MapRect(s_gx.viewport[0], s_gx.viewport[1], s_gx.viewport[2],
            s_gx.viewport[3], &vp);
    D3DRECT rect = { static_cast<LONG>(vp.X), static_cast<LONG>(vp.Y),
                     static_cast<LONG>(vp.X + vp.Width),
                     static_cast<LONG>(vp.Y + vp.Height) };
    s_device->Clear(1, &rect, flags,
                    D3DCOLOR_ARGB(255, s_eraseColor.r, s_eraseColor.g, s_eraseColor.b),
                    1.0f, 0);
    ++s_stats.erases;
}

void HSD_JObjDispSub(HSD_JObj* jobj, MtxPtr vmtx, MtxPtr pmtx,
                     HSD_TrspMask trsp_mask, u32 rendermode)
{
    if (rendermode & RENDER_SHADOW)
        return;
    HSD_JObjSetCurrent(jobj);
    if (jobj->flags & JOBJ_SPECULAR)
        HSD_LObjSetupSpecularInit(pmtx);
    const u32 dobjTrsp = static_cast<u32>(trsp_mask) << DOBJ_TRSP_SHIFT;
    for (HSD_DObj* dobj = jobj->u.dobj; dobj; dobj = dobj->next) {
        if ((dobj->flags & DOBJ_HIDDEN) || !(dobj->flags & dobjTrsp))
            continue;
        HSD_DObjSetCurrent(dobj);
        RenderDObj(jobj, dobj, vmtx, pmtx);
    }
    HSD_DObjSetCurrent(NULL);
    HSD_JObjSetCurrent(NULL);
}

void HSD_JObjDisp(HSD_JObj* jobj, MtxPtr vmtx, HSD_TrspMask trsp_mask,
                  u32 rendermode)
{
    if (!jobj || !union_type_dobj(jobj) || (jobj->flags & JOBJ_HIDDEN))
        return;
    const u32 bits = jobj->flags & (static_cast<u32>(trsp_mask) << JOBJ_TRSP_SHIFT);
    if (!bits)
        return;
    HSD_JObjSetupMatrix(jobj);
    if (!vmtx) {
        HSD_CObj* cobj = HSD_CObjGetCurrent();
        if (!cobj)
            return;
        vmtx = HSD_CObjGetViewingMtxPtrDirect(cobj);
    }
    Mtx pmtx;
    HSD_JObjMakePositionMtx(jobj, vmtx, pmtx);
    if (bits & JOBJ_OPA)
        HSD_JObjDispSub(jobj, vmtx, pmtx, HSD_TRSP_OPA, rendermode);
    if (bits & JOBJ_TEXEDGE)
        HSD_JObjDispSub(jobj, vmtx, pmtx, HSD_TRSP_TEXEDGE, rendermode);
    if (bits & JOBJ_XLU)
        HSD_JObjDispSub(jobj, vmtx, pmtx, HSD_TRSP_XLU, rendermode);
}

} // extern "C"

bool M360_HsdDecodeImage(const void* imageHandle, const void* tobjHandle,
                         unsigned** pixels, unsigned* width, unsigned* height)
{
    const HSD_ImageDesc* image = static_cast<const HSD_ImageDesc*>(imageHandle);
    const HSD_TObj* tobj = static_cast<const HSD_TObj*>(tobjHandle);
    if (!pixels || !width || !height || !image || !image->image_ptr)
        return false;
    *pixels = NULL;
    *width = image->width;
    *height = image->height;
    if (!*width || !*height || *width > 1024 || *height > 1024)
        return false;
    unsigned paletteFormat = 0, paletteEntries = 0;
    const unsigned char* palette = tobj ? PaletteData(tobj, &paletteFormat, &paletteEntries) : NULL;
    *pixels = static_cast<unsigned*>(malloc(*width * *height * sizeof(unsigned)));
    if (!*pixels)
        return false;
    memset(*pixels, 0, *width * *height * sizeof(unsigned));
    if (!M360_DecodeGxTexture(static_cast<const unsigned char*>(image->image_ptr),
                              *width, *height, static_cast<unsigned>(image->format),
                              palette, paletteFormat, paletteEntries, *pixels)) {
        free(*pixels);
        *pixels = NULL;
        return false;
    }
    return true;
}

/* Native particle quads (see particle_draw_xdk.c). Textures are cached like
 * material textures, keyed by image and palette. */
static IDirect3DTexture9* ResolveParticleTexture(const M360ParticleTexture* tex)
{
    if (!tex || !tex->image || !tex->width || !tex->height ||
        tex->width > 1024 || tex->height > 1024)
        return NULL;
    for (unsigned i = 0; i < s_textureCount; ++i)
        if (s_textures[i].image == tex->image && s_textures[i].palette == tex->palette)
            return s_textures[i].texture;
    IDirect3DTexture9* texture = NULL;
    unsigned* pixels = static_cast<unsigned*>(malloc(tex->width * tex->height * sizeof(unsigned)));
    if (pixels) {
        memset(pixels, 0, tex->width * tex->height * sizeof(unsigned));
        if (M360_DecodeGxTexture(static_cast<const unsigned char*>(tex->image), tex->width,
                                 tex->height, tex->format,
                                 static_cast<const unsigned char*>(tex->palette),
                                 tex->paletteFormat, tex->paletteEntries, pixels) &&
            SUCCEEDED(s_device->CreateTexture(tex->width, tex->height, 1, 0,
                D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED, &texture, NULL))) {
            D3DLOCKED_RECT locked;
            if (SUCCEEDED(texture->LockRect(0, &locked, NULL, 0))) {
                for (unsigned y = 0; y < tex->height; ++y)
                    memcpy(static_cast<BYTE*>(locked.pBits) + y * locked.Pitch,
                           pixels + y * tex->width, tex->width * sizeof(unsigned));
                texture->UnlockRect(0);
            } else {
                texture->Release();
                texture = NULL;
            }
        }
        free(pixels);
    }
    if (!texture && s_stats.decodeFailures++ < 16)
        M360_Trace("hsd.particle.decode_failed", tex->format);
    if (s_textureCount < kMaxTextures) {
        s_textures[s_textureCount].image = tex->image;
        s_textures[s_textureCount].palette = tex->palette;
        s_textures[s_textureCount++].texture = texture;
    }
    return texture;
}

void M360_HsdDrawParticle(const M360ParticleVertex* corners, const M360ParticleTexture* tex,
                          unsigned blend, int depthWrite)
{
    if (!s_device || !corners)
        return;
    float vs[64 * 4];
    memset(vs, 0, sizeof(vs));
    memcpy(vs, s_gx.projection, sizeof(s_gx.projection));
    vs[7 * 4 + 0] = 1.0f;               /* channel colour = vertex colour */
    vs[5 * 4 + 3] = 0.0f;
    vs[6 * 4 + 0] = vs[6 * 4 + 1] = vs[6 * 4 + 2] = vs[6 * 4 + 3] = 1.0f;
    vs[8 * 4 + 0] = 1.0f;               /* identity texture matrix, uv0 */
    vs[9 * 4 + 1] = 1.0f;

    float ps[54 * 4];
    memset(ps, 0, sizeof(ps));
    /* Stage 12 (slot 0 pre-lighting): colour = prev * tex, alpha = prev * tex. */
    TevStage* stage = reinterpret_cast<TevStage*>(ps + 12 * 4);
    ClearStage(stage);
    stage->w[1][0] = 1.0f;              /* in1 = prev */
    stage->w[2][1] = 1.0f;              /* in2 = texture */
    stage->wa[0][1] = 1.0f;             /* alpha in1 = prev.a */
    stage->wa[1][2] = 1.0f;             /* alpha in2 = texture.a */
    ps[48 * 4 + 0] = ps[48 * 4 + 1] = ps[48 * 4 + 2] = ps[48 * 4 + 3] = 1.0f;
    BOOL flags[11];
    memset(flags, 0, sizeof(flags));
    flags[0] = TRUE;                    /* slot0Pre */
    flags[6] = TRUE;                    /* vertexBase */

    IDirect3DTexture9* texture = ResolveParticleTexture(tex);
    s_device->SetVertexShader(s_vertexShader);
    s_device->SetPixelShader(s_pixelShader);
    s_device->SetVertexDeclaration(s_declaration);
    s_device->SetVertexShaderConstantF(0, vs, 64);
    s_device->SetPixelShaderConstantF(0, ps, 54);
    s_device->SetPixelShaderConstantB(0, flags, 11);
    s_device->SetTexture(0, texture ? texture : s_white);
    s_device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    s_device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    s_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    s_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    s_device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    s_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    s_device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    s_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    s_device->SetRenderState(D3DRS_DESTBLEND, blend == 1 ? D3DBLEND_ONE : D3DBLEND_INVSRCALPHA);
    s_device->SetRenderState(D3DRS_ZENABLE, blend == 2 ? D3DZB_FALSE : D3DZB_TRUE);
    s_device->SetRenderState(D3DRS_ZWRITEENABLE, depthWrite ? TRUE : FALSE);
    s_device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    s_device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALL);
    s_device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    s_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    HsdVertex quad[6];
    static const int order[6] = { 0, 1, 2, 0, 2, 3 };
    for (int i = 0; i < 6; ++i) {
        const M360ParticleVertex& c = corners[order[i]];
        HsdVertex& v = quad[i];
        v.px = c.x; v.py = c.y; v.pz = c.z;
        v.nx = 0.0f; v.ny = 0.0f; v.nz = 1.0f;
        v.r = c.r; v.g = c.g; v.b = c.b; v.a = c.a;
        v.u0 = c.u; v.v0 = c.v; v.u1 = c.u; v.v1 = c.v;
    }
    s_device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, quad, sizeof(HsdVertex));
    ++s_stats.drawCalls;
    s_stats.triangles += 2;
}

bool M360_HsdDecodeTexture(const void* tobjHandle, unsigned** pixels,
                           unsigned* width, unsigned* height)
{
    const HSD_TObj* tobj = static_cast<const HSD_TObj*>(tobjHandle);
    return tobj && M360_HsdDecodeImage(tobj->imagedesc, tobj, pixels, width, height);
}

void M360_HsdFreeDecoded(unsigned* pixels)
{
    free(pixels);
}

bool M360_HsdRenderInit(IDirect3DDevice9* device)
{
    static const D3DVERTEXELEMENT9 elements[] = {
        { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
        { 0, 24, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
        { 0, 40, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        { 0, 48, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
        D3DDECL_END()
    };
    s_device = device;
    M360_HsdVideoInit();
    GXSetViewport(0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    GXSetScissor(0, 0, 640, 480);
    if (FAILED(device->CreateVertexShader(reinterpret_cast<const DWORD*>(g_melee360HsdVS),
                                          &s_vertexShader)) ||
        FAILED(device->CreatePixelShader(reinterpret_cast<const DWORD*>(g_melee360HsdPS),
                                         &s_pixelShader)) ||
        FAILED(device->CreateVertexDeclaration(elements, &s_declaration)) ||
        FAILED(device->CreateTexture(1, 1, 1, 0, D3DFMT_LIN_A8R8G8B8,
                                     D3DPOOL_MANAGED, &s_white, NULL)))
        return false;
    D3DLOCKED_RECT locked;
    if (FAILED(s_white->LockRect(0, &locked, NULL, 0)))
        return false;
    *static_cast<DWORD*>(locked.pBits) = 0xFFFFFFFFu;
    s_white->UnlockRect(0);
    return true;
}

void M360_HsdRenderShutdown(void)
{
    for (unsigned i = 0; i < s_textureCount; ++i)
        if (s_textures[i].texture)
            s_textures[i].texture->Release();
    s_textureCount = 0;
    if (s_white) s_white->Release();
    if (s_declaration) s_declaration->Release();
    if (s_pixelShader) s_pixelShader->Release();
    if (s_vertexShader) s_vertexShader->Release();
    s_white = NULL;
    s_declaration = NULL;
    s_pixelShader = NULL;
    s_vertexShader = NULL;
    s_device = NULL;
}

void M360_HsdRenderBeginFrame(void)
{
    const unsigned failures = s_stats.decodeFailures;
    memset(&s_stats, 0, sizeof(s_stats));
    s_stats.decodeFailures = failures;
    s_stats.textures = s_textureCount;
}

void M360_HsdRenderEndFrame(void)
{
    if (!s_device)
        return;
    D3DVIEWPORT9 vp = { 0, 0, 1280, 720, 0.0f, 1.0f };
    s_device->SetViewport(&vp);
    s_device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    s_device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
    s_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    s_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    s_device->SetRenderState(D3DRS_COLORWRITEENABLE,
                             D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN |
                             D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
    s_device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    s_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    s_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    s_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    s_device->SetTexture(0, NULL);
    s_device->SetTexture(1, NULL);
    s_stats.textures = s_textureCount;
}

void M360_HsdRenderAllowErase(bool allow)
{
    s_allowErase = allow;
}

void M360_HsdRenderGetStats(M360HsdRenderStats* stats)
{
    if (stats)
        *stats = s_stats;
}

unsigned M360_HsdRenderEraseColor(void)
{
    return 0xFF000000u | (s_eraseColor.r << 16) | (s_eraseColor.g << 8) |
           s_eraseColor.b;
}
