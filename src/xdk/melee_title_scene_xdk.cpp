#include <xtl.h>

#include "hsdjobj_xdk_compat.h"
#include "melee_archive_xdk.h"
#include "melee_title_scene_xdk.h"

extern "C" {
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/jobj.h>
}

namespace {

struct TitleModelSymbols {
    HSD_Joint* joint;
    HSD_AnimJoint* anim;
    HSD_MatAnimJoint* matAnim;
    HSD_ShapeAnimJoint* shapeAnim;
};

HSD_JObj* s_titleModels[2];
HSD_TObj* s_firstTexture;

unsigned Expand4To8(unsigned value) { return (value << 4) | value; }
unsigned Expand5To8(unsigned value) { return (value << 3) | (value >> 2); }
unsigned Expand6To8(unsigned value) { return (value << 2) | (value >> 4); }

unsigned DecodeRgb565(unsigned value)
{
    return 0xFF000000u | (Expand5To8((value >> 11) & 31) << 16) |
           (Expand6To8((value >> 5) & 63) << 8) | Expand5To8(value & 31);
}

unsigned DecodeRgb5A3(unsigned value)
{
    if (value & 0x8000)
        return 0xFF000000u | (Expand5To8((value >> 10) & 31) << 16) |
               (Expand5To8((value >> 5) & 31) << 8) |
               Expand5To8(value & 31);
    const unsigned alpha = ((value >> 12) & 7) * 255 / 7;
    return (alpha << 24) | (Expand4To8((value >> 8) & 15) << 16) |
           (Expand4To8((value >> 4) & 15) << 8) | Expand4To8(value & 15);
}

unsigned Read16(const unsigned char* data)
{
    return (static_cast<unsigned>(data[0]) << 8) | data[1];
}

void PutPixel(unsigned* pixels, unsigned width, unsigned height,
              unsigned x, unsigned y, unsigned color)
{
    if (x < width && y < height)
        pixels[y * width + x] = color;
}

unsigned PaletteColor(const HSD_TObj* texture, unsigned index)
{
    if (!texture->tlut || !texture->tlut->lut ||
        index >= texture->tlut->n_entries)
        return 0xFFFF00FFu;
    const unsigned value = Read16(
        static_cast<const unsigned char*>(texture->tlut->lut) + index * 2);
    switch (texture->tlut->fmt) {
    case GX_TL_IA8: {
        const unsigned intensity = value & 255;
        return ((value >> 8) << 24) | (intensity << 16) |
               (intensity << 8) | intensity;
    }
    case GX_TL_RGB565: return DecodeRgb565(value);
    case GX_TL_RGB5A3: return DecodeRgb5A3(value);
    default: return 0xFFFF00FFu;
    }
}

bool DecodeTexture(const HSD_TObj* texture, unsigned* pixels)
{
    const HSD_ImageDesc* image = texture ? texture->imagedesc : NULL;
    if (!image || !image->image_ptr || !pixels)
        return false;
    const unsigned char* source =
        static_cast<const unsigned char*>(image->image_ptr);
    const unsigned width = image->width;
    const unsigned height = image->height;
    const unsigned format = static_cast<unsigned>(image->format);
    unsigned offset = 0;

    if (format == GX_TF_I4 || format == GX_TF_C4) {
        for (unsigned by = 0; by < height; by += 8)
            for (unsigned bx = 0; bx < width; bx += 8) {
                for (unsigned p = 0; p < 64; ++p) {
                    const unsigned nibble = (source[offset + p / 2] >>
                        ((p & 1) ? 0 : 4)) & 15;
                    const unsigned color = format == GX_TF_C4
                        ? PaletteColor(texture, nibble)
                        : (0xFF000000u | (Expand4To8(nibble) * 0x010101u));
                    PutPixel(pixels, width, height, bx + p % 8, by + p / 8,
                             color);
                }
                offset += 32;
            }
        return true;
    }
    if (format == GX_TF_I8 || format == GX_TF_IA4 || format == GX_TF_C8) {
        for (unsigned by = 0; by < height; by += 4)
            for (unsigned bx = 0; bx < width; bx += 8) {
                for (unsigned p = 0; p < 32; ++p) {
                    const unsigned value = source[offset + p];
                    unsigned color;
                    if (format == GX_TF_C8)
                        color = PaletteColor(texture, value);
                    else if (format == GX_TF_IA4) {
                        const unsigned alpha = Expand4To8(value >> 4);
                        const unsigned intensity = Expand4To8(value & 15);
                        color = (alpha << 24) | (intensity * 0x010101u);
                    } else {
                        color = 0xFF000000u | (value * 0x010101u);
                    }
                    PutPixel(pixels, width, height, bx + p % 8, by + p / 8,
                             color);
                }
                offset += 32;
            }
        return true;
    }
    if (format == GX_TF_IA8 || format == GX_TF_RGB565 ||
        format == GX_TF_RGB5A3) {
        for (unsigned by = 0; by < height; by += 4)
            for (unsigned bx = 0; bx < width; bx += 4) {
                for (unsigned p = 0; p < 16; ++p) {
                    const unsigned value = Read16(source + offset + p * 2);
                    unsigned color;
                    if (format == GX_TF_RGB565)
                        color = DecodeRgb565(value);
                    else if (format == GX_TF_RGB5A3)
                        color = DecodeRgb5A3(value);
                    else {
                        const unsigned intensity = value & 255;
                        color = ((value >> 8) << 24) |
                                (intensity * 0x010101u);
                    }
                    PutPixel(pixels, width, height, bx + p % 4, by + p / 4,
                             color);
                }
                offset += 32;
            }
        return true;
    }
    if (format == GX_TF_RGBA8) {
        for (unsigned by = 0; by < height; by += 4)
            for (unsigned bx = 0; bx < width; bx += 4) {
                for (unsigned p = 0; p < 16; ++p) {
                    const unsigned alpha = source[offset + p * 2];
                    const unsigned red = source[offset + p * 2 + 1];
                    const unsigned green = source[offset + 32 + p * 2];
                    const unsigned blue = source[offset + 32 + p * 2 + 1];
                    PutPixel(pixels, width, height, bx + p % 4, by + p / 4,
                             (alpha << 24) | (red << 16) | (green << 8) | blue);
                }
                offset += 64;
            }
        return true;
    }
    if (format == GX_TF_CMPR) {
        for (unsigned by = 0; by < height; by += 8)
            for (unsigned bx = 0; bx < width; bx += 8) {
                for (unsigned sub = 0; sub < 4; ++sub) {
                    const unsigned char* block = source + offset + sub * 8;
                    const unsigned c0raw = Read16(block);
                    const unsigned c1raw = Read16(block + 2);
                    unsigned colors[4] = { DecodeRgb565(c0raw),
                                           DecodeRgb565(c1raw), 0, 0 };
                    if (c0raw > c1raw) {
                        for (unsigned channel = 0; channel < 3; ++channel) {
                            const unsigned shift = channel * 8;
                            const unsigned a = (colors[0] >> shift) & 255;
                            const unsigned b = (colors[1] >> shift) & 255;
                            colors[2] |= ((2 * a + b) / 3) << shift;
                            colors[3] |= ((a + 2 * b) / 3) << shift;
                        }
                        colors[2] |= 0xFF000000u;
                        colors[3] |= 0xFF000000u;
                    } else {
                        for (unsigned channel = 0; channel < 3; ++channel) {
                            const unsigned shift = channel * 8;
                            const unsigned a = (colors[0] >> shift) & 255;
                            const unsigned b = (colors[1] >> shift) & 255;
                            colors[2] |= ((a + b) / 2) << shift;
                        }
                        colors[2] |= 0xFF000000u;
                        colors[3] = 0;
                    }
                    const unsigned bits =
                        (static_cast<unsigned>(block[4]) << 24) |
                        (static_cast<unsigned>(block[5]) << 16) |
                        (static_cast<unsigned>(block[6]) << 8) | block[7];
                    const unsigned ox = (sub & 1) * 4;
                    const unsigned oy = (sub >> 1) * 4;
                    for (unsigned p = 0; p < 16; ++p) {
                        const unsigned index = (bits >> (30 - p * 2)) & 3;
                        PutPixel(pixels, width, height, bx + ox + p % 4,
                                 by + oy + p / 4, colors[index]);
                    }
                }
                offset += 32;
            }
        return true;
    }
    return false;
}

void CountTree(HSD_JObj* jobj, MeleeTitleSceneStatus* status)
{
    for (HSD_JObj* node = jobj;
         node && status->jointCount < 10000; node = node->next) {
        ++status->jointCount;
        if (!(node->flags & (JOBJ_SPLINE | JOBJ_PTCL))) {
            for (HSD_DObj* dobj = node->u.dobj; dobj; dobj = dobj->next) {
                ++status->displayObjectCount;
                if (dobj->mobj)
                    ++status->materialCount;
                if (dobj->mobj) {
                    for (HSD_TObj* tobj = dobj->mobj->tobj; tobj;
                         tobj = tobj->next) {
                        ++status->textureObjectCount;
                        if (tobj->imagedesc && tobj->imagedesc->image_ptr) {
                            ++status->textureImageCount;
                            if (!status->firstTextureWidth) {
                                s_firstTexture = tobj;
                                status->firstTextureWidth =
                                    tobj->imagedesc->width;
                                status->firstTextureHeight =
                                    tobj->imagedesc->height;
                                status->firstTextureFormat =
                                    static_cast<unsigned>(tobj->imagedesc->format);
                            }
                        }
                    }
                }
                for (HSD_PObj* pobj = dobj->pobj; pobj; pobj = pobj->next)
                    ++status->polygonObjectCount;
            }
        }
        if (!(node->flags & JOBJ_INSTANCE))
            CountTree(node->child, status);
    }
}

void* Resolve(const char* symbol, MeleeTitleSceneStatus* status)
{
    void* address = M360_GetHsdPublic(symbol);
    if (address)
        ++status->resolvedSymbols;
    return address;
}

} // namespace

bool M360_LoadTitleScene(MeleeTitleSceneStatus* status)
{
    if (!status)
        return false;
    ZeroMemory(status, sizeof(*status));
    status->expectedSymbols = 12;

    TitleModelSymbols title;
    title.joint = static_cast<HSD_Joint*>(
        Resolve("TtlMoji_Top_joint", status));
    title.anim = static_cast<HSD_AnimJoint*>(
        Resolve("TtlMoji_Top_animjoint", status));
    title.matAnim = static_cast<HSD_MatAnimJoint*>(
        Resolve("TtlMoji_Top_matanim_joint", status));
    title.shapeAnim = static_cast<HSD_ShapeAnimJoint*>(
        Resolve("TtlMoji_Top_shapeanim_joint", status));

    Resolve("ScTitle_cam_int1_camera", status);
    Resolve("ScTitle_scene_lights", status);
    Resolve("ScTitle_fog", status);

    TitleModelSymbols background;
    background.joint = static_cast<HSD_Joint*>(
        Resolve("TtlBg_Top_joint", status));
    background.anim = static_cast<HSD_AnimJoint*>(
        Resolve("TtlBg_Top_animjoint", status));
    background.matAnim = static_cast<HSD_MatAnimJoint*>(
        Resolve("TtlBg_Top_matanim_joint", status));
    background.shapeAnim = static_cast<HSD_ShapeAnimJoint*>(
        Resolve("TtlBg_Top_shapeanim_joint", status));
    Resolve("TitleMark_sobjdesc", status);

    status->symbolsResolved =
        status->resolvedSymbols == status->expectedSymbols;
    if (!status->symbolsResolved || !title.joint || !background.joint)
        return false;

    s_titleModels[0] = HSD_JObjLoadJoint(title.joint);
    s_titleModels[1] = HSD_JObjLoadJoint(background.joint);
    status->modelCount = (s_titleModels[0] ? 1u : 0u) +
                         (s_titleModels[1] ? 1u : 0u);
    status->modelsLoaded = status->modelCount == 2;
    if (!status->modelsLoaded)
        return false;

    HSD_JObjAddAnimAll(s_titleModels[0], title.anim, title.matAnim,
                       title.shapeAnim);
    HSD_JObjAddAnimAll(s_titleModels[1], background.anim, background.matAnim,
                       background.shapeAnim);
    HSD_JObjReqAnimAll(s_titleModels[0], 0.0f);
    HSD_JObjReqAnimAll(s_titleModels[1], 0.0f);
    HSD_JObjAnimAll(s_titleModels[0]);
    HSD_JObjAnimAll(s_titleModels[1]);
    status->animationsBound = true;

    CountTree(s_titleModels[0], status);
    CountTree(s_titleModels[1], status);
    return true;
}

void M360_AnimateTitleScene(void)
{
    if (s_titleModels[0])
        HSD_JObjAnimAll(s_titleModels[0]);
    if (s_titleModels[1])
        HSD_JObjAnimAll(s_titleModels[1]);
}

bool M360_DecodeFirstTitleTexture(unsigned** pixels, unsigned* width,
                                  unsigned* height)
{
    if (!pixels || !width || !height || !s_firstTexture ||
        !s_firstTexture->imagedesc)
        return false;
    *width = s_firstTexture->imagedesc->width;
    *height = s_firstTexture->imagedesc->height;
    const size_t count = static_cast<size_t>(*width) * *height;
    *pixels = static_cast<unsigned*>(malloc(count * sizeof(unsigned)));
    if (!*pixels)
        return false;
    memset(*pixels, 0, count * sizeof(unsigned));
    if (!DecodeTexture(s_firstTexture, *pixels)) {
        free(*pixels);
        *pixels = NULL;
        return false;
    }
    return true;
}

void M360_FreeDecodedTitleTexture(unsigned* pixels)
{
    free(pixels);
}
