#include <xtl.h>
#include <math.h>
#undef near
#undef far

#include "gameplay_probe_compat.h"
#include "melee_archive_xdk.h"
#include "melee_title_scene_xdk.h"

extern "C" {
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/wobj.h>
#include <sysdolphin/baselib/fog.h>
#include <melee/mn/types.h>
float mn_8022ED6C(HSD_JObj*, AnimLoopSettings*);
float mn_8022F298(HSD_JObj*);
}

namespace {

struct TitleModelSymbols {
    HSD_Joint* joint;
    HSD_AnimJoint* anim;
    HSD_MatAnimJoint* matAnim;
    HSD_ShapeAnimJoint* shapeAnim;
};

enum { kLogo, kBackground, kFlash, kModelCount };

HSD_JObj* s_titleModels[kModelCount];
bool s_visible[kModelCount];
bool s_openingMode;
bool s_backgroundStarted;
AnimLoopSettings s_logoLoop = { 0, 1600.0f, 400.0f };
AnimLoopSettings s_backgroundLoop = { 0, 1330.0f, 130.0f };
HSD_FogDesc* s_fog;
HSD_TObj* s_firstTexture;
const HSD_TObj* s_drawTexture;
unsigned s_drawBlend;
unsigned s_drawCull;
bool s_drawVertexColor;
HSD_CameraDescPerspective* s_camera;
HSD_CameraDescPerspective* s_menuCamera;
HSD_FogDesc* s_menuFog;
const char* const s_menuNames[] = {
    "MenMainBack", "MenMainPanel", "MenMainConTop", "MenMainCursor",
    "MenMainConRl", "MenMainCursorRl", "MenMainNmRl",
    "MenMainCursorTr01", "MenMainCursorTr02", "MenMainCursorTr03",
    "MenMainCursorTr04", "MenMainCursorRl01", "MenMainCursorRl02",
    "MenMainCursorRl03", "MenMainCursorRl04", "MenMainCursorRl05",
    "MenMainConIs", "MenMainCursorIs", "MenMainConSs", "MenMainCursorSs"
};
HSD_JObj* s_menuModels[20];

bool ProjectCamera(MeleeTitleVertex* vertices, unsigned* count)
{
    if (!s_camera || s_camera->projection_type != PROJ_PERSPECTIVE ||
        !s_camera->eyepos || !s_camera->interest ||
        !(s_camera->fov > 0 && s_camera->fov < 179) ||
        !(s_camera->aspect > 0) || !(s_camera->nnear > 0))
        return false;
    const HSD_WObjDesc* eye = s_camera->eyepos;
    const HSD_WObjDesc* target = s_camera->interest;
    float z[3] = { eye->pos.x - target->pos.x,
                   eye->pos.y - target->pos.y,
                   eye->pos.z - target->pos.z };
    float length = sqrtf(z[0]*z[0] + z[1]*z[1] + z[2]*z[2]);
    if (!(length > 0.00001f)) return false;
    for (unsigned i = 0; i < 3; ++i) z[i] /= length;
    float up[3] = { 0, 1, 0 };
    if (s_camera->up_vector) {
        up[0] = s_camera->up_vector->x;
        up[1] = s_camera->up_vector->y;
        up[2] = s_camera->up_vector->z;
    }
    float x[3] = { up[1]*z[2]-up[2]*z[1], up[2]*z[0]-up[0]*z[2],
                   up[0]*z[1]-up[1]*z[0] };
    length = sqrtf(x[0]*x[0] + x[1]*x[1] + x[2]*x[2]);
    if (!(length > 0.00001f)) return false;
    for (unsigned i = 0; i < 3; ++i) x[i] /= length;
    float y[3] = { z[1]*x[2]-z[2]*x[1], z[2]*x[0]-z[0]*x[2],
                   z[0]*x[1]-z[1]*x[0] };
    const float c = cosf(s_camera->roll), s = sinf(s_camera->roll);
    const float focal = 1.0f / tanf(s_camera->fov * 0.00872664626f);
    unsigned output = 0;
    for (unsigned first = 0; first + 2 < *count; first += 3) {
        MeleeTitleVertex triangle[3];
        bool visible = true;
        for (unsigned i = 0; i < 3; ++i) {
            triangle[i] = vertices[first+i];
            float d[3] = { triangle[i].x-eye->pos.x,
                           triangle[i].y-eye->pos.y, triangle[i].z-eye->pos.z };
            const float depth = -(d[0]*z[0]+d[1]*z[1]+d[2]*z[2]);
            if (!(depth >= s_camera->nnear && depth <= s_camera->ffar)) {
                visible = false; break;
            }
            const float vx = d[0]*x[0]+d[1]*x[1]+d[2]*x[2];
            const float vy = d[0]*y[0]+d[1]*y[1]+d[2]*y[2];
            triangle[i].x = 640 + 480 *
                ((c*vx+s*vy)*focal/(depth*s_camera->aspect));
            triangle[i].y = 360 - 360*((-s*vx+c*vy)*focal/depth);
            triangle[i].z = 0;
            triangle[i].fog = 0.0f;
            if (s_fog && s_fog->end > s_fog->start) {
                const float fog = (depth - s_fog->start) / (s_fog->end - s_fog->start);
                triangle[i].fog = fog < 0.0f ? 0.0f : (fog > 1.0f ? 1.0f : fog);
            }
        }
        if (visible && triangle[0].cull) {
            const float cross =
                (triangle[1].x - triangle[0].x) * (triangle[2].y - triangle[0].y) -
                (triangle[1].y - triangle[0].y) * (triangle[2].x - triangle[0].x);
            if ((triangle[0].cull & 2) && cross < 0.0f)
                visible = false;
            if ((triangle[0].cull & 1) && cross > 0.0f)
                visible = false;
        }
        if (visible)
            for (unsigned i = 0; i < 3; ++i) vertices[output++] = triangle[i];
    }
    *count = output;
    return true;
}

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
                        : Expand4To8(nibble) * 0x01010101u;
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
                        color = value * 0x01010101u;
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

unsigned Read32(const unsigned char* data)
{
    return (static_cast<unsigned>(data[0]) << 24) |
           (static_cast<unsigned>(data[1]) << 16) |
           (static_cast<unsigned>(data[2]) << 8) | data[3];
}

float ReadComponent(const unsigned char* data, GXCompType type, unsigned frac)
{
    const float divisor = static_cast<float>(1u << frac);
    switch (type) {
    case GX_U8: return data[0] / divisor;
    case GX_S8: return static_cast<const signed char*>(
        static_cast<const void*>(data))[0] / divisor;
    case GX_U16: return Read16(data) / divisor;
    case GX_S16: return static_cast<short>(Read16(data)) / divisor;
    case GX_F32: {
        union { unsigned bits; float value; } converted;
        converted.bits = Read32(data);
        return converted.value;
    }
    default: return 0.0f;
    }
}

unsigned ComponentSize(GXCompType type)
{
    return type == GX_U8 || type == GX_S8 ? 1u :
           (type == GX_F32 ? 4u : 2u);
}

unsigned DirectSize(const HSD_VtxDescList* desc)
{
    if (desc->attr >= GX_VA_PNMTXIDX && desc->attr <= GX_VA_TEX7MTXIDX)
        return 1;
    if (desc->attr == GX_VA_CLR0 || desc->attr == GX_VA_CLR1) {
        switch (desc->comp_type) {
        case GX_RGB565: case GX_RGBA4: return 2;
        case GX_RGB8: case GX_RGBA6: return 3;
        default: return 4;
        }
    }
    unsigned components = 1;
    if (desc->attr == GX_VA_POS)
        components = desc->comp_cnt == GX_POS_XYZ ? 3 : 2;
    else if (desc->attr == GX_VA_NRM || desc->attr == GX_VA_NBT)
        components = desc->comp_cnt == GX_NRM_NBT ? 9 : 3;
    else if (desc->attr >= GX_VA_TEX0 && desc->attr <= GX_VA_TEX7)
        components = desc->comp_cnt == GX_TEX_ST ? 2 : 1;
    return components * ComponentSize(desc->comp_type);
}

unsigned DecodeColor(const unsigned char* data, GXCompType type)
{
    switch (type) {
    case GX_RGB565: return DecodeRgb565(Read16(data));
    case GX_RGB8: return 0xFF000000u | (data[0] << 16) | (data[1] << 8) | data[2];
    case GX_RGBX8: return 0xFF000000u | (data[0] << 16) | (data[1] << 8) | data[2];
    case GX_RGBA4:
        return (Expand4To8(data[1] & 15) << 24) |
               (Expand4To8(data[0] >> 4) << 16) |
               (Expand4To8(data[0] & 15) << 8) | Expand4To8(data[1] >> 4);
    case GX_RGBA6: {
        const unsigned bits = (data[0] << 16) | (data[1] << 8) | data[2];
        return (((bits & 63) * 255 / 63) << 24) |
               ((((bits >> 18) & 63) * 255 / 63) << 16) |
               ((((bits >> 12) & 63) * 255 / 63) << 8) |
               (((bits >> 6) & 63) * 255 / 63);
    }
    case GX_RGBA8:
        return (data[3] << 24) | (data[0] << 16) | (data[1] << 8) | data[2];
    default: return 0xFFFFFFFFu;
    }
}

struct ParsedVertex {
    float x, y, z, u, v, u1, v1;
    unsigned color;
    bool hasColor;
};

const unsigned char* ParseVertex(const HSD_VtxDescList* descriptors,
                                 const unsigned char* stream,
                                 ParsedVertex* vertex)
{
    vertex->x = vertex->y = vertex->z = vertex->u = vertex->v = 0.0f;
    vertex->u1 = vertex->v1 = 0.0f;
    vertex->color = 0xFFFFFFFFu;
    vertex->hasColor = false;
    unsigned descriptorCount = 0;
    for (const HSD_VtxDescList* desc = descriptors;
         desc && desc->attr != GX_VA_NULL && descriptorCount < 32;
         ++desc, ++descriptorCount) {
        if (desc->attr_type == GX_NONE)
            continue;
        const unsigned char* value = stream;
        if (desc->attr_type == GX_INDEX8) {
            const unsigned index = *stream++;
            value = static_cast<const unsigned char*>(desc->vertex) +
                    index * desc->stride;
        } else if (desc->attr_type == GX_INDEX16) {
            const unsigned index = Read16(stream);
            stream += 2;
            value = static_cast<const unsigned char*>(desc->vertex) +
                    index * desc->stride;
        } else {
            stream += DirectSize(desc);
        }
        if (desc->attr == GX_VA_POS) {
            const unsigned size = ComponentSize(desc->comp_type);
            vertex->x = ReadComponent(value, desc->comp_type, desc->frac);
            vertex->y = ReadComponent(value + size, desc->comp_type, desc->frac);
            if (desc->comp_cnt == GX_POS_XYZ)
                vertex->z = ReadComponent(value + size * 2,
                                          desc->comp_type, desc->frac);
        } else if (desc->attr == GX_VA_TEX0) {
            const unsigned size = ComponentSize(desc->comp_type);
            vertex->u = ReadComponent(value, desc->comp_type, desc->frac);
            if (desc->comp_cnt == GX_TEX_ST)
                vertex->v = ReadComponent(value + size,
                                          desc->comp_type, desc->frac);
        } else if (desc->attr == GX_VA_TEX1) {
            const unsigned size = ComponentSize(desc->comp_type);
            vertex->u1 = ReadComponent(value, desc->comp_type, desc->frac);
            if (desc->comp_cnt == GX_TEX_ST)
                vertex->v1 = ReadComponent(value + size,
                                           desc->comp_type, desc->frac);
        } else if (desc->attr == GX_VA_CLR0) {
            vertex->color = DecodeColor(value, desc->comp_type);
            vertex->hasColor = true;
        }
    }
    return stream;
}

unsigned VertexStreamSize(const HSD_VtxDescList* descriptors)
{
    unsigned size = 0;
    unsigned descriptorCount = 0;
    for (const HSD_VtxDescList* desc = descriptors;
         desc && desc->attr != GX_VA_NULL && descriptorCount < 32;
         ++desc, ++descriptorCount) {
        if (desc->attr_type == GX_NONE)
            continue;
        if (desc->attr_type == GX_INDEX8)
            size += 1;
        else if (desc->attr_type == GX_INDEX16)
            size += 2;
        else if (desc->attr_type == GX_DIRECT)
            size += DirectSize(desc);
        else
            return 0;
    }
    return size;
}

void TransformVertex(const HSD_JObj* jobj, ParsedVertex* vertex)
{
    const float x = vertex->x, y = vertex->y, z = vertex->z;
    vertex->x = jobj->mtx[0][0] * x + jobj->mtx[0][1] * y +
                jobj->mtx[0][2] * z + jobj->mtx[0][3];
    vertex->y = jobj->mtx[1][0] * x + jobj->mtx[1][1] * y +
                jobj->mtx[1][2] * z + jobj->mtx[1][3];
    vertex->z = jobj->mtx[2][0] * x + jobj->mtx[2][1] * y +
                jobj->mtx[2][2] * z + jobj->mtx[2][3];
}

void TextureCoord(const HSD_TObj* t, const ParsedVertex& source, float* outU,
                  float* outV)
{
    const bool second = t && t->src == GX_TG_TEX1;
    *outU = second ? source.u1 : source.u;
    *outV = second ? source.v1 : source.v;
    if (!t || !t->repeat_s || !t->repeat_t)
        return;
    const float scaleU = fabsf(t->scale.x) < 1e-6f ? 0.0f : t->repeat_s / t->scale.x;
    const float scaleV = fabsf(t->scale.y) < 1e-6f ? 0.0f : t->repeat_t / t->scale.y;
    const float u = *outU - t->translate.x;
    const float v = *outV - t->translate.y -
        (t->wrap_t == GX_MIRROR && scaleV != 0.0f ? 1.0f / scaleV : 0.0f);
    const float c = cosf(-t->rotate.z), s = sinf(-t->rotate.z);
    *outU = (c * u - s * v) * scaleU;
    *outV = (s * u + c * v) * scaleV;
}

void EmitVertex(MeleeTitleVertex* output, unsigned capacity, unsigned* count,
                const ParsedVertex& source, unsigned materialColor)
{
    if (*count >= capacity)
        return;
    MeleeTitleVertex& target = output[(*count)++];
    target.x = source.x;
    target.y = source.y;
    target.z = source.z;
    TextureCoord(s_drawTexture, source, &target.u, &target.v);
    target.texture1 = s_drawTexture ? s_drawTexture->next : 0;
    TextureCoord(static_cast<const HSD_TObj*>(target.texture1), source,
                 &target.u1, &target.v1);
    target.fog = 0.0f;
    target.color = s_drawVertexColor && source.hasColor ? source.color
                                                        : materialColor;
    target.texture = s_drawTexture;
    target.blend = s_drawBlend;
    target.cull = s_drawCull;
}

void EmitTriangle(MeleeTitleVertex* output, unsigned capacity, unsigned* count,
                  const ParsedVertex& a, const ParsedVertex& b,
                  const ParsedVertex& c, unsigned color)
{
    EmitVertex(output, capacity, count, a, color);
    EmitVertex(output, capacity, count, b, color);
    EmitVertex(output, capacity, count, c, color);
}

void DecodePObj(const HSD_JObj* jobj, const HSD_DObj* dobj,
                const HSD_PObj* pobj, MeleeTitleVertex* output,
                unsigned capacity, unsigned* count)
{
    if (!pobj->verts || !pobj->display)
        return;
    unsigned materialColor = 0xFFFFFFFFu;
    s_drawTexture = dobj->mobj ? dobj->mobj->tobj : 0;
    s_drawCull = (pobj->flags >> 14) & 3;
    s_drawVertexColor = dobj->mobj && (dobj->mobj->rendermode & RENDER_VERTEX);
    const HSD_PEDesc* pe = dobj->mobj ? dobj->mobj->pe : 0;
    s_drawBlend = pe ? 0x80000000u | (static_cast<unsigned>(pe->type) << 16) |
                       (static_cast<unsigned>(pe->src_factor) << 8) |
                       pe->dst_factor
                     : 0;
    if (dobj->mobj && dobj->mobj->mat) {
        const HSD_Material* mat = dobj->mobj->mat;
        const unsigned alpha = static_cast<unsigned>(mat->alpha * 255.0f);
        materialColor = ((alpha > 255 ? 255 : alpha) << 24) |
                        (mat->diffuse.r << 16) | (mat->diffuse.g << 8) |
                        mat->diffuse.b;
    }
    const unsigned vertexSize = VertexStreamSize(pobj->verts);
    if (!vertexSize || pobj->n_display > 0x4000)
        return;
    const unsigned displayBytes = static_cast<unsigned>(pobj->n_display) << 5;
    const unsigned char* stream = pobj->display;
    const unsigned char* end = stream + displayBytes;
    while (stream + 3 <= end && *count < capacity) {
        const unsigned primitive = *stream++ & 0xF8;
        if (primitive == 0) /* GX_NOP padding */
            break;
        const unsigned vertexCount = Read16(stream);
        stream += 2;
        const unsigned remaining = static_cast<unsigned>(end - stream);
        if (vertexCount > remaining / vertexSize)
            break;
        ParsedVertex first, previous, current, quad[4];
        for (unsigned i = 0; i < vertexCount; ++i) {
            stream = ParseVertex(pobj->verts, stream, &current);
            TransformVertex(jobj, &current);
            if (primitive == GX_TRIANGLES) {
                quad[i % 3] = current;
                if (i % 3 == 2)
                    EmitTriangle(output, capacity, count, quad[0], quad[1],
                                 quad[2], materialColor);
            } else if (primitive == GX_QUADS) {
                quad[i % 4] = current;
                if (i % 4 == 3) {
                    EmitTriangle(output, capacity, count, quad[0], quad[1],
                                 quad[2], materialColor);
                    EmitTriangle(output, capacity, count, quad[0], quad[2],
                                 quad[3], materialColor);
                }
            } else if (primitive == GX_TRIANGLESTRIP) {
                if (i >= 2) {
                    if (i & 1)
                        EmitTriangle(output, capacity, count, previous, first,
                                     current, materialColor);
                    else
                        EmitTriangle(output, capacity, count, first, previous,
                                     current, materialColor);
                }
                first = previous;
                previous = current;
                if (i == 0)
                    first = previous = current;
            } else if (primitive == GX_TRIANGLEFAN) {
                if (i == 0)
                    first = current;
                else if (i >= 2)
                    EmitTriangle(output, capacity, count, first, previous,
                                 current, materialColor);
                previous = current;
            }
        }
    }
}

void DecodeJObj(HSD_JObj* jobj, unsigned pass, MeleeTitleVertex* output,
                unsigned capacity, unsigned* count)
{
    for (HSD_JObj* node = jobj; node && *count < capacity; node = node->next) {
        if (node->flags & JOBJ_INSTANCE)
            continue;
        HSD_JObjSetupMatrix(node);
        if ((node->flags & (pass << 18)) && !(node->flags & JOBJ_HIDDEN) &&
            !(node->flags & (JOBJ_SPLINE | JOBJ_PTCL))) {
            for (HSD_DObj* dobj = node->u.dobj; dobj; dobj = dobj->next) {
                if ((dobj->flags & DOBJ_HIDDEN) || !(dobj->flags & (pass << 1)))
                    continue;
                for (HSD_PObj* pobj = dobj->pobj; pobj; pobj = pobj->next)
                    DecodePObj(node, dobj, pobj, output, capacity, count);
            }
        }
        if (node->flags & (pass << 28))
            DecodeJObj(node->child, pass, output, capacity, count);
    }
}

HSD_JObj* JointByIndex(HSD_JObj* root, int target)
{
    HSD_JObj* jobj = root;
    for (int index = 0; jobj && index != target; ++index) {
        if (!(jobj->flags & JOBJ_INSTANCE) && jobj->child) {
            jobj = jobj->child;
            continue;
        }
        while (jobj && !jobj->next)
            jobj = jobj->parent;
        jobj = jobj ? jobj->next : NULL;
    }
    return jobj;
}

void HideJoint(HSD_JObj* root, int index)
{
    HSD_JObj* jobj = JointByIndex(root, index);
    if (jobj)
        HSD_JObjSetFlagsAll(jobj, JOBJ_HIDDEN);
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

    s_camera = static_cast<HSD_CameraDescPerspective*>(
        Resolve("ScTitle_cam_int1_camera", status));
    Resolve("ScTitle_scene_lights", status);
    s_fog = static_cast<HSD_FogDesc*>(Resolve("ScTitle_fog", status));

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

    s_titleModels[kLogo] = HSD_JObjLoadJoint(title.joint);
    s_titleModels[kBackground] = HSD_JObjLoadJoint(background.joint);
    s_titleModels[kFlash] = HSD_JObjLoadJoint(title.joint);
    status->modelCount = (s_titleModels[kLogo] ? 1u : 0u) +
                         (s_titleModels[kBackground] ? 1u : 0u);
    status->modelsLoaded = status->modelCount == 2 && s_titleModels[kFlash];
    if (!status->modelsLoaded)
        return false;

    HSD_JObjAddAnimAll(s_titleModels[kLogo], title.anim, title.matAnim,
                       title.shapeAnim);
    HSD_JObjAddAnimAll(s_titleModels[kBackground], background.anim,
                       background.matAnim, background.shapeAnim);
    HSD_JObjAddAnimAll(s_titleModels[kFlash], title.anim, title.matAnim,
                       title.shapeAnim);
    HSD_JObjReqAnimAll(s_titleModels[kFlash], s_logoLoop.loop_frame);
    HSD_JObjAnimAll(s_titleModels[kFlash]);
    Vec3 flashOffset = { 0.0f, -3.0f, 0.0f };
    HSD_JObjSetTranslate(s_titleModels[kFlash], &flashOffset);
    HideJoint(s_titleModels[kFlash], 3);
    HideJoint(s_titleModels[kFlash], 1);
    HideJoint(s_titleModels[kLogo], 7);
    M360_TitleEnter(false);
    status->animationsBound = true;

    CountTree(s_titleModels[kLogo], status);
    CountTree(s_titleModels[kBackground], status);
    return true;
}

void M360_TitleEnter(bool openingMode)
{
    s_openingMode = openingMode;
    s_backgroundStarted = !openingMode;
    s_visible[kLogo] = !openingMode;
    s_visible[kBackground] = !openingMode;
    s_visible[kFlash] = false;
    if (!s_titleModels[kLogo] || !s_titleModels[kBackground])
        return;
    HSD_JObjReqAnimAll(s_titleModels[kLogo],
                       openingMode ? s_logoLoop.start_frame : s_logoLoop.loop_frame);
    HSD_JObjAnimAll(s_titleModels[kLogo]);
    HSD_JObjReqAnimAll(s_titleModels[kBackground],
                       openingMode ? s_backgroundLoop.start_frame : 130.0f);
    HSD_JObjAnimAll(s_titleModels[kBackground]);
}

void M360_Trace(const char* stage, unsigned value);
static void DebugDump(HSD_JObj* root)
{
    int index = 0;
    for (HSD_JObj* j = root; j; j = JointByIndex(root, ++index)) {
        M360_Trace("dbg.joint", index);
        M360_Trace("dbg.flags", j->flags);
        unsigned tris = 0;
        for (HSD_DObj* d = (j->flags & (JOBJ_SPLINE | JOBJ_PTCL)) ? 0 : j->u.dobj; d; d = d->next) {
            M360_Trace("dbg.dobj.flags", d->flags);
            if (d->mobj) {
                M360_Trace("dbg.rm", d->mobj->rendermode);
                if (d->mobj->mat) {
                    M360_Trace("dbg.diffuse", (d->mobj->mat->diffuse.r << 16) | (d->mobj->mat->diffuse.g << 8) | d->mobj->mat->diffuse.b);
                    M360_Trace("dbg.alpha100", static_cast<unsigned>(d->mobj->mat->alpha * 100));
                }
            }
            ++tris;
        }
        if (index > 40) break;
    }
}

bool M360_TitleUpdate(unsigned sceneTick)
{
    static unsigned debugFrames = 0;
    if (!s_openingMode && ++debugFrames == 90)
        DebugDump(s_titleModels[kLogo]);
    if (!s_titleModels[kLogo] || !s_titleModels[kBackground])
        return false;
    if (!s_openingMode) {
        mn_8022ED6C(s_titleModels[kLogo], &s_logoLoop);
        mn_8022ED6C(s_titleModels[kBackground], &s_backgroundLoop);
        return true;
    }
    s_visible[kFlash] = sceneTick >= 0x3B6 && sceneTick < 0x3CE;
    if (sceneTick < 0x140A)
        return false;
    s_visible[kLogo] = true;
    if (sceneTick > 5400) {
        mn_8022ED6C(s_titleModels[kLogo], &s_logoLoop);
    } else {
        HSD_JObjReqAnimAll(s_titleModels[kLogo],
                           static_cast<float>(sceneTick - 5130));
        HSD_JObjAnimAll(s_titleModels[kLogo]);
    }
    if (s_backgroundStarted) {
        mn_8022ED6C(s_titleModels[kBackground], &s_backgroundLoop);
    } else if (mn_8022F298(s_titleModels[kLogo]) >= 270.0f) {
        s_backgroundStarted = true;
        s_visible[kBackground] = true;
    }
    return s_backgroundStarted;
}

unsigned M360_TitleClearColor(void)
{
    if (!s_fog)
        return 0xFF000000u;
    return 0xFF000000u | (s_fog->color.r << 16) | (s_fog->color.g << 8) |
           s_fog->color.b;
}

bool M360_DecodeFirstTitleTexture(unsigned** pixels, unsigned* width,
                                  unsigned* height)
{
    return M360_DecodeTitleTexture(s_firstTexture, pixels, width, height);
}

bool M360_DecodeTitleTexture(const void* handle, unsigned** pixels,
                            unsigned* width, unsigned* height)
{
    const HSD_TObj* texture = static_cast<const HSD_TObj*>(handle);
    if (!pixels || !width || !height || !texture || !texture->imagedesc)
        return false;
    *pixels = 0;
    *width = texture->imagedesc->width;
    *height = texture->imagedesc->height;
    if (!*width || !*height || *width > 2048 || *height > 2048)
        return false;
    const size_t count = static_cast<size_t>(*width) * *height;
    *pixels = static_cast<unsigned*>(malloc(count * sizeof(unsigned)));
    if (!*pixels)
        return false;
    memset(*pixels, 0, count * sizeof(unsigned));
    if (!DecodeTexture(texture, *pixels)) {
        free(*pixels);
        *pixels = NULL;
        return false;
    }
    return true;
}

unsigned M360_TitleTextureWrap(const void* handle)
{
    const HSD_TObj* texture = static_cast<const HSD_TObj*>(handle);
    return texture ? (static_cast<unsigned>(texture->wrap_s) & 3) |
                     ((static_cast<unsigned>(texture->wrap_t) & 3) << 2) : 0;
}

void M360_FreeDecodedTitleTexture(unsigned* pixels)
{
    free(pixels);
}

unsigned M360_BuildTitleMesh(MeleeTitleVertex* vertices, unsigned capacity)
{
    if (!vertices || capacity < 3)
        return 0;
    static const unsigned passes[3] = { 1, 4, 2 };
    static const unsigned order[kModelCount] = { kBackground, kLogo, kFlash };
    unsigned count = 0;
    for (unsigned pass = 0; pass < 3; ++pass)
        for (unsigned model = 0; model < kModelCount; ++model)
            if (s_visible[order[model]])
                DecodeJObj(s_titleModels[order[model]], passes[pass], vertices,
                           capacity, &count);
    if (!count)
        return 0;
    if (ProjectCamera(vertices, &count))
        return count;
    float minX = vertices[0].x, maxX = vertices[0].x;
    float minY = vertices[0].y, maxY = vertices[0].y;
    for (unsigned i = 1; i < count; ++i) {
        if (vertices[i].x < minX) minX = vertices[i].x;
        if (vertices[i].x > maxX) maxX = vertices[i].x;
        if (vertices[i].y < minY) minY = vertices[i].y;
        if (vertices[i].y > maxY) maxY = vertices[i].y;
    }
    const float rangeX = maxX - minX;
    const float rangeY = maxY - minY;
    if (rangeX <= 0.0001f || rangeY <= 0.0001f)
        return 0;
    const float scaleX = 900.0f / rangeX;
    const float scaleY = 680.0f / rangeY;
    const float scale = scaleX < scaleY ? scaleX : scaleY;
    const float centerX = (minX + maxX) * 0.5f;
    const float centerY = (minY + maxY) * 0.5f;
    for (unsigned i = 0; i < count; ++i) {
        vertices[i].x = 640.0f + (vertices[i].x - centerX) * scale;
        vertices[i].y = 360.0f - (vertices[i].y - centerY) * scale;
        vertices[i].z = 0.0f;
    }
    return count - (count % 3);
}

bool M360_LoadMenuModels(unsigned* loadedModels, unsigned* loadedJoints)
{
    if (!loadedModels || !loadedJoints)
        return false;
    *loadedModels = *loadedJoints = 0;
    s_menuCamera = static_cast<HSD_CameraDescPerspective*>(
        M360_GetMenuHsdPublic("ScMenMain_cam_int1_camera"));
    s_menuFog = static_cast<HSD_FogDesc*>(
        M360_GetMenuHsdPublic("ScMenMain_fog"));
    static const char* const suffixes[4] = {
        "_Top_joint", "_Top_animjoint", "_Top_matanim_joint",
        "_Top_shapeanim_joint"
    };
    for (unsigned model = 0; model < 20; ++model) {
        char symbol[80];
        void* roots[4];
        for (unsigned part = 0; part < 4; ++part) {
            strcpy(symbol, s_menuNames[model]);
            strcat(symbol, suffixes[part]);
            roots[part] = M360_GetMenuHsdPublic(symbol);
        }
        if (!roots[0])
            continue;
        s_menuModels[model] = HSD_JObjLoadJoint(static_cast<HSD_Joint*>(roots[0]));
        if (!s_menuModels[model])
            continue;
        ++*loadedModels;
        HSD_JObjAddAnimAll(s_menuModels[model],
            static_cast<HSD_AnimJoint*>(roots[1]),
            static_cast<HSD_MatAnimJoint*>(roots[2]),
            static_cast<HSD_ShapeAnimJoint*>(roots[3]));
        HSD_JObjReqAnimAll(s_menuModels[model], 0.0f);
        HSD_JObjAnimAll(s_menuModels[model]);
        MeleeTitleSceneStatus counts;
        ZeroMemory(&counts, sizeof(counts));
        CountTree(s_menuModels[model], &counts);
        *loadedJoints += counts.jointCount;
    }
    return *loadedModels == 20 && s_menuCamera;
}

unsigned M360_BuildMenuMesh(MeleeTitleVertex* vertices, unsigned capacity)
{
    if (!vertices || capacity < 3 || !s_menuCamera)
        return 0;
    HSD_CameraDescPerspective* previousCamera = s_camera;
    HSD_FogDesc* previousFog = s_fog;
    s_camera = s_menuCamera;
    s_fog = s_menuFog;
    static const unsigned passes[] = { 1, 4, 2 };
    unsigned count = 0;
    // mnMain_Scene_OnEnter creates Back (GX link 2), Panel (3), then
    // ConTop (4) for MENU_KIND_MAIN. The other archive models belong to
    // submenus; drawing all twenty at once overlays unrelated screens.
    for (unsigned pass = 0; pass < 3; ++pass)
        for (unsigned model = 0; model < 3 && count + 3 <= capacity; ++model)
            if (s_menuModels[model])
                DecodeJObj(s_menuModels[model], passes[pass], vertices,
                           capacity, &count);
    ProjectCamera(vertices, &count);
    s_camera = previousCamera;
    s_fog = previousFog;
    return count;
}

void M360_UpdateMenuModels(void)
{
    // mn_8022EAE0 advances the original background JObj on every frame.
    if (s_menuModels[0])
        HSD_JObjAnimAll(s_menuModels[0]);
}
