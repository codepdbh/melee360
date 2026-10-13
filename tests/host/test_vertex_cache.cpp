#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#define _DOLPHIN_TYPES_H_
typedef unsigned char u8;
typedef unsigned int u32;
#include <dolphin/gx/GXEnum.h>
const u32 TEX_LIGHTMAP_DIFFUSE = 1u << 4, TEX_LIGHTMAP_SPECULAR = 1u << 5;
const u32 TEX_LIGHTMAP_AMBIENT = 1u << 6, TEX_LIGHTMAP_EXT = 1u << 7;
typedef float Mtx[3][4];
struct HSD_VtxDescList {
    GXAttr attr; GXAttrType attr_type; GXCompCnt comp_cnt; GXCompType comp_type;
    unsigned char frac; unsigned short stride; const void* vertex;
};
#include "gx_vertex_original.h"
enum { POBJ_SKIN = 0, POBJ_ENVELOPE = 1, POBJ_SHAPEANIM = 2 };
struct HSD_PObj {
    HSD_VtxDescList* verts; const unsigned char* display; unsigned n_display, type;
};
unsigned pobj_type(const HSD_PObj* p) { return p->type; }
static bool failAllocation;
static unsigned allocations;
static void* CacheAlloc(unsigned bytes) {
    ++allocations;
    return failAllocation ? nullptr : std::malloc(bytes);
}
#define malloc CacheAlloc
#include "hsd_vertex_cache.h"
#undef malloc

int main()
{
    HSD_VtxDescList desc[] = {
        {GX_VA_PNMTXIDX, GX_DIRECT, GX_POS_XY, GX_U8, 0, 0, nullptr},
        {GX_VA_POS, GX_DIRECT, GX_POS_XYZ, GX_S16, 0, 0, nullptr},
        {GX_VA_CLR0, GX_DIRECT, GX_CLR_RGBA, GX_RGBA8, 0, 0, nullptr},
        {GX_VA_TEX7, GX_DIRECT, GX_TEX_ST, GX_U8, 0, 0, nullptr},
        {GX_VA_NULL, GX_NONE, GX_POS_XY, GX_U8, 0, 0, nullptr}
    };
    unsigned char display[128] = {GX_TRIANGLES, 0, 3};
    for (unsigned i = 0; i < 3; ++i) {
        unsigned char* p = display + 3 + i * 13;
        p[0] = static_cast<unsigned char>(i * 3);
        p[2] = static_cast<unsigned char>(i + 1); p[4] = 2; p[6] = 3;
        p[7] = 255; p[8] = static_cast<unsigned char>(i * 20); p[9] = 128; p[10] = 255;
        p[11] = static_cast<unsigned char>(i); p[12] = 7;
    }
    // A second primitive tests stream alignment across command headers.
    display[42] = GX_TRIANGLESTRIP; display[44] = 3;
    std::memcpy(display + 45, display + 3, 39);
    HSD_PObj mesh = {desc, display, 4, POBJ_ENVELOPE};
    VertexCacheEntry* cached = GetVertexCache(&mesh);
    assert(cached && cached->vertices == 6 && cached->stride == 13);
    const unsigned char* cursor = display + 3;
    for (unsigned i = 0; i < 6; ++i) {
        if (i == 3) cursor += 3;
        RawVertex raw;
        assert(ParseVertex(desc, &cursor, display + sizeof(display), &raw));
        assert(std::memcmp(&raw, &cached->raw[i], sizeof(raw)) == 0);
    }
    const unsigned built = allocations;
    assert(GetVertexCache(&mesh) == cached && allocations == built);
    // Current animation matrices and TEX selection apply after cache lookup.
    MatrixSet matrices = {};
    matrices.valid[0] = true;
    for (unsigned axis = 0; axis < 3; ++axis)
        matrices.pos[0][axis][axis] = matrices.nrm[0][axis][axis] = 1;
    const unsigned sources[] = {7, 0};
    HsdVertex a, b;
    Transform(&matrices, cached->raw[0], &a, sources);
    matrices.pos[0][0][3] = 19;
    Transform(&matrices, cached->raw[0], &b, sources);
    assert(b.px == a.px + 19 && b.u0 == a.u0 && b.v0 == 7);
    assert(s_vertexCacheBytes == 6 * sizeof(RawVertex));
    ForgetVertexCache(&mesh);
    assert(s_vertexCacheBytes == 0);
    // Same owner/source addresses can be reused after destruction.
    display[5] = 33;
    cached = GetVertexCache(&mesh);
    assert(cached && cached->raw[0].pos[0] == 33);
    mesh.type = POBJ_SHAPEANIM;
    assert(!GetVertexCache(&mesh) && s_vertexCacheBytes == 0);
    mesh.type = POBJ_SKIN;
    failAllocation = true;
    assert(!GetVertexCache(&mesh) && !s_vertexCacheBytes);
    failAllocation = false;
    s_vertexCacheBytes = kVertexCacheBytes;
    assert(!GetVertexCache(&mesh));
    s_vertexCacheBytes = 0;
    cached = GetVertexCache(&mesh);
    assert(cached);
    unsigned char other[128];
    std::memcpy(other, display, sizeof(other)); other[5] = 42;
    mesh.display = other;
    cached = GetVertexCache(&mesh);
    assert(cached && cached->raw[0].pos[0] == 42 && s_vertexCacheBytes == 6 * sizeof(RawVertex));
    ClearVertexCache();
    assert(!s_vertexCacheBytes);
    // Truncated tuples, invalid attributes and pathological command counts
    // fall back to the existing decoder, without publishing partial entries.
    unsigned char broken[32] = {GX_TRIANGLES, 0, 3};
    mesh.display = broken; mesh.n_display = 1;
    assert(!GetVertexCache(&mesh));
    mesh.display = other; mesh.n_display = 4;
    desc[0].attr_type = static_cast<GXAttrType>(99);
    assert(!GetVertexCache(&mesh)); desc[0].attr_type = GX_DIRECT;
    mesh.n_display = 0x4001;
    assert(!GetVertexCache(&mesh)); mesh.n_display = 4;
    // Hash collisions survive deletion of the first member in the chain.
    HSD_PObj owners[1200] = {};
    unsigned first = 0, second = 0;
    bool collision = false;
    for (unsigned i = 1; i < 1200 && !collision; ++i)
        for (unsigned j = 0; j < i; ++j)
            if (VertexCacheHash(&owners[i]) == VertexCacheHash(&owners[j])) {
                first = j; second = i; collision = true; break;
            }
    assert(collision);
    owners[first] = owners[second] = mesh;
    assert(GetVertexCache(&owners[first]) && GetVertexCache(&owners[second]));
    ForgetVertexCache(&owners[first]);
    const unsigned before = allocations;
    assert(GetVertexCache(&owners[second]) && allocations == before);
    ClearVertexCache();
    const unsigned char positions[] = {0, 9, 0, 8, 0, 7};
    const signed char normals[] = {64, 0, 0, 0, 64, 0, 0, 0, 64};
    const unsigned char colors[] = {255, 20, 30, 255};
    const unsigned char uv[] = {4, 5};
    HSD_VtxDescList indexed[] = {
        {GX_VA_PNMTXIDX, GX_DIRECT, GX_POS_XY, GX_U8, 0, 0, nullptr},
        {GX_VA_POS, GX_INDEX8, GX_POS_XYZ, GX_S16, 0, 6, positions},
        {GX_VA_NRM, GX_INDEX8, GX_NRM_NBT3, GX_S8, 0, 3, normals},
        {GX_VA_CLR0, GX_INDEX8, GX_CLR_RGBA, GX_RGBA8, 0, 4, colors},
        {GX_VA_TEX7, GX_INDEX8, GX_TEX_ST, GX_U8, 0, 2, uv},
        {GX_VA_NULL, GX_NONE, GX_POS_XY, GX_U8, 0, 0, nullptr}
    };
    unsigned char indices[32] = {GX_QUADS, 0, 4};
    for (unsigned i = 0; i < 4; ++i) {
        indices[3 + i * 7 + 2] = 1;
        indices[3 + i * 7 + 3] = 2;
    }
    HSD_PObj nbtMesh = {indexed, indices, 1, POBJ_ENVELOPE};
    cached = GetVertexCache(&nbtMesh);
    assert(cached && cached->vertices == 4 && cached->stride == 7);
    cursor = indices + 3;
    for (unsigned i = 0; i < 4; ++i) {
        RawVertex raw;
        assert(ParseVertex(indexed, &cursor, indices + sizeof(indices), &raw));
        assert(std::memcmp(&raw, &cached->raw[i], sizeof(raw)) == 0);
    }
    ClearVertexCache();
    std::puts("PASS: exact decoded vertices, current transforms, bounded cache, removal/reuse, shape bypass, failures and hash chains");
}
