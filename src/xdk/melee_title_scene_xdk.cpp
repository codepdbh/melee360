#include <xtl.h>
#include <math.h>
#undef near
#undef far

#include "hsdjobj_xdk_compat.h"
#include "melee_archive_xdk.h"
#include "melee_title_scene_xdk.h"

extern "C" {
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/wobj.h>
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
const HSD_TObj* s_drawTexture;
HSD_CameraDescPerspective* s_camera;

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
            triangle[i].x = 640 + 105*s_camera->aspect *
                ((c*vx+s*vy)*focal/(depth*s_camera->aspect));
            triangle[i].y = 404 - 105*((-s*vx+c*vy)*focal/depth);
            triangle[i].z = 0;
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
    float x, y, z, u, v;
    unsigned color;
};

const unsigned char* ParseVertex(const HSD_VtxDescList* descriptors,
                                 const unsigned char* stream,
                                 ParsedVertex* vertex)
{
    vertex->x = vertex->y = vertex->z = vertex->u = vertex->v = 0.0f;
    vertex->color = 0xFFFFFFFFu;
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
        } else if (desc->attr == GX_VA_CLR0) {
            vertex->color = DecodeColor(value, desc->comp_type);
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

void EmitVertex(MeleeTitleVertex* output, unsigned capacity, unsigned* count,
                const ParsedVertex& source, unsigned materialColor)
{
    if (*count >= capacity)
        return;
    MeleeTitleVertex& target = output[(*count)++];
    target.x = source.x;
    target.y = source.y;
    target.z = source.z;
    target.u = source.u;
    target.v = source.v;
    target.color = source.color == 0xFFFFFFFFu ? materialColor : source.color;
    target.texture = s_drawTexture;
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

void DecodeJObj(HSD_JObj* jobj, MeleeTitleVertex* output,
                unsigned capacity, unsigned* count)
{
    for (HSD_JObj* node = jobj; node && *count < capacity; node = node->next) {
        HSD_JObjSetupMatrix(node);
        if (!(node->flags & (JOBJ_SPLINE | JOBJ_PTCL))) {
            for (HSD_DObj* dobj = node->u.dobj; dobj; dobj = dobj->next)
                for (HSD_PObj* pobj = dobj->pobj; pobj; pobj = pobj->next)
                    DecodePObj(node, dobj, pobj, output, capacity, count);
        }
        if (!(node->flags & JOBJ_INSTANCE))
            DecodeJObj(node->child, output, capacity, count);
    }
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

void M360_FreeDecodedTitleTexture(unsigned* pixels)
{
    free(pixels);
}

unsigned M360_BuildTitleMesh(MeleeTitleVertex* vertices, unsigned capacity)
{
    if (!vertices || capacity < 3)
        return 0;
    unsigned count = 0;
    DecodeJObj(s_titleModels[0], vertices, capacity, &count);
    DecodeJObj(s_titleModels[1], vertices, capacity, &count);
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
    const float scaleX = 560.0f / rangeX;
    const float scaleY = 210.0f / rangeY;
    const float scale = scaleX < scaleY ? scaleX : scaleY;
    const float centerX = (minX + maxX) * 0.5f;
    const float centerY = (minY + maxY) * 0.5f;
    for (unsigned i = 0; i < count; ++i) {
        vertices[i].x = 640.0f + (vertices[i].x - centerX) * scale;
        vertices[i].y = 404.0f - (vertices[i].y - centerY) * scale;
        vertices[i].z = 0.0f;
    }
    return count - (count % 3);
}
