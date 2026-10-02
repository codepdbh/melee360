#include <assert.h>
#include <stdio.h>
#include <string.h>
// Use the original enum values without pulling in Dolphin's compiler headers.
#define _DOLPHIN_TYPES_H_
typedef unsigned char u8;
#include <dolphin/gx/GXEnum.h>
typedef float Mtx[3][4];
struct HSD_VtxDescList {
    GXAttr attr; GXAttrType attr_type; GXCompCnt comp_cnt; GXCompType comp_type;
    unsigned char frac; unsigned short stride; const void* vertex;
};
#include "gx_vertex_original.h"

int main() {
    const signed char normals[] = { 64, 0, 0, 0, 64, 0, 0, 0, 64 };
    const unsigned char uv[] = { 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0 };
    HSD_VtxDescList desc[] = {
        { GX_VA_NRM, GX_INDEX8, GX_NRM_NBT3, GX_S8, 0, 3, normals },
        { GX_VA_TEX7, GX_INDEX8, GX_TEX_ST, GX_S16, 8, 4, uv },
        { GX_VA_NULL, GX_NONE, GX_POS_XYZ, GX_F32, 0, 0, NULL }
    };
    const unsigned char bytes8[] = { 1, 2, 0, 2, 0, 2, 1, 1 };
    const unsigned char* cursor = bytes8;
    RawVertex raw;
    assert(ParseVertex(desc, &cursor, bytes8 + sizeof(bytes8), &raw));
    assert(cursor == bytes8 + 4 && raw.nrm[1] == 1.0f);
    assert(raw.uv[7][0] == 5.0f && raw.uv[7][1] == 6.0f);
    assert(ParseVertex(desc, &cursor, bytes8 + sizeof(bytes8), &raw));
    assert(cursor == bytes8 + 8 && raw.nrm[0] == 1.0f && raw.uv[7][0] == 3.0f);
    for (unsigned length = 0; length < 4; ++length) {
        cursor = bytes8;
        assert(!ParseVertex(desc, &cursor, bytes8 + length, &raw));
        assert(cursor == bytes8);
    }
    desc[0].attr_type = desc[1].attr_type = GX_INDEX16;
    const unsigned char bytes16[] = { 0, 1, 0, 2, 0, 0, 0, 2 };
    cursor = bytes16;
    assert(ParseVertex(desc, &cursor, bytes16 + sizeof(bytes16), &raw));
    assert(cursor == bytes16 + 8 && raw.nrm[1] == 1.0f && raw.uv[7][0] == 5.0f);
    for (unsigned length = 0; length < 8; ++length) {
        cursor = bytes16;
        assert(!ParseVertex(desc, &cursor, bytes16 + length, &raw));
        assert(cursor == bytes16);
    }
    desc[0].vertex = NULL;
    cursor = bytes16;
    assert(!ParseVertex(desc, &cursor, bytes16 + sizeof(bytes16), &raw));

    desc[0].attr_type = desc[1].attr_type = GX_DIRECT;
    desc[0].attr = GX_VA_NBT;
    desc[1].attr = GX_VA_TEX3;
    const unsigned char direct[] = { 64, 0, 0, 0, 64, 0, 0, 0, 64, 4, 0, 5, 0 };
    cursor = direct;
    assert(DirectSize(desc) == 9);
    assert(ParseVertex(desc, &cursor, direct + sizeof(direct), &raw));
    assert(cursor == direct + 13 && raw.nrm[0] == 1.0f && raw.uv[3][0] == 4.0f);

    HSD_VtxDescList tex[9];
    unsigned char stream[32];
    memset(tex, 0, sizeof(tex));
    for (unsigned i = 0; i < 8; ++i) {
        tex[i].attr = static_cast<GXAttr>(GX_VA_TEX0 + i);
        tex[i].attr_type = GX_DIRECT;
        tex[i].comp_cnt = GX_TEX_ST; tex[i].comp_type = GX_S16; tex[i].frac = 8;
        stream[i * 4] = static_cast<unsigned char>(i + 1); stream[i * 4 + 1] = 0;
        stream[i * 4 + 2] = static_cast<unsigned char>(i + 2); stream[i * 4 + 3] = 0;
    }
    tex[8].attr = GX_VA_NULL;
    cursor = stream;
    assert(ParseVertex(tex, &cursor, stream + sizeof(stream), &raw));
    for (unsigned i = 0; i < 8; ++i) assert(raw.uv[i][0] == i + 1.0f);
    MatrixSet matrices;
    memset(&matrices, 0, sizeof(matrices));
    matrices.valid[0] = true;
    for (unsigned i = 0; i < 3; ++i) matrices.pos[0][i][i] = matrices.nrm[0][i][i] = 1;
    const unsigned sources[] = { 6, 3 };
    HsdVertex vertex;
    Transform(&matrices, raw, &vertex, sources);
    assert(vertex.u0 == 7 && vertex.v0 == 8 && vertex.u1 == 4 && vertex.v1 == 5);
    assert(sizeof(vertex) == 56);
    puts("PASS: NBT3 index8/index16 alignment, truncated tuples, direct normals and TEX0-TEX7 routing");
}
