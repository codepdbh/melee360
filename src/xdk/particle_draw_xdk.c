/* Native psDispParticles: draws the original HSD particle lists as textured,
 * camera-facing quads through the D3D HSD renderer instead of psdisp.c's GX
 * immediate-mode path. Covers position (world or appsrt-relative), size,
 * rotation, primary colour/alpha, texture group/pose/palette and the two
 * particle blend modes; custom polygon forms, trails, tornado paths and
 * environment-colour TEV modes are not reproduced. */
#include <float.h>
#include <math.h>
#include <string.h>

#pragma warning(push, 3)
#pragma warning(disable : 4244)
#include <dolphin/mtx.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/particle.h>
#include <sysdolphin/baselib/psstructs.h>
#pragma warning(pop)

#include "hsd_particle_xdk.h"

extern HSD_Particle* hsd_804D0908[16];

static void ToView(Mtx m, const Vec3* p, float* out)
{
    out[0] = m[0][0] * p->x + m[0][1] * p->y + m[0][2] * p->z + m[0][3];
    out[1] = m[1][0] * p->x + m[1][1] * p->y + m[1][2] * p->z + m[1][3];
    out[2] = m[2][0] * p->x + m[2][1] * p->y + m[2][2] * p->z + m[2][3];
}

static void ResolveTexture(const HSD_Particle* pp, M360ParticleTexture* tex)
{
    HSD_PSTexGroup* group;
    const DiscU32* table;
    memset(tex, 0, sizeof(*tex));
    if (!(pp->kind & DispTexture) || !psTexGroupArray[pp->bank])
        return;
    group = PS_TEXGROUP(pp->bank, pp->texGroup);
    if (!group || pp->poseNum >= group->num)
        return;
    table = (const DiscU32*) group->texTable;
    tex->image = (const void*) (uintptr_t) table[pp->poseNum].v;
    tex->format = group->fmt;
    tex->width = group->width;
    tex->height = group->height;
    if (group->fmt == 8 || group->fmt == 9) {
        const u32 palCount = (group->palflag & 1) ? 1
                             : (group->palnum ? group->palnum : group->num);
        u32 pal;
        if (pp->palNum != 0xFF)
            pal = pp->palNum;
        else if (!(pp->kind & ComTLUT))
            pal = pp->poseNum;
        else
            pal = 0;
        if (pal >= palCount) {
            tex->image = NULL;
            return;
        }
        tex->palette = (const void*) (uintptr_t) table[group->num + pal].v;
        tex->paletteFormat = group->tlutfmt;
        tex->paletteEntries = group->fmt == 8 ? 16 : 256;
    }
}

static void DrawParticle(HSD_Particle* pp, Mtx vmtx)
{
    M360ParticleVertex v[4];
    M360ParticleTexture tex;
    float center[3], half = pp->size, c, s;
    float scale = 1.0f;
    int i;
    if (pp->appsrt) {
        Mtx model, mv;
        Vec3 world;
        if (pp->appsrt->status != PS_APPSTATUS_STILL)
            HSD_MtxSRT(pp->appsrt->mmtx, &pp->appsrt->scale, (Vec3*) &pp->appsrt->rot,
                       &pp->appsrt->translate, NULL);
        if (pp->appsrt->status == PS_APPSTATUS_ONCE)
            pp->appsrt->status = PS_APPSTATUS_STILL;
        PSMTXCopy(pp->appsrt->mmtx, model);
        PSMTXMultVec(model, &pp->pos, &world);
        PSMTXConcat(vmtx, model, mv);
        scale = sqrtf(mv[0][0] * mv[0][0] + mv[1][0] * mv[1][0] + mv[2][0] * mv[2][0]);
        ToView(vmtx, &world, center);
    } else {
        ToView(vmtx, &pp->pos, center);
    }
    half *= scale;
    c = cosf(pp->rotate) * half;
    s = sinf(pp->rotate) * half;
    for (i = 0; i < 4; ++i) {
        static const float cx[4] = { -1.0f, 1.0f, 1.0f, -1.0f };
        static const float cy[4] = { 1.0f, 1.0f, -1.0f, -1.0f };
        v[i].x = center[0] + cx[i] * c - cy[i] * s;
        v[i].y = center[1] + cx[i] * s + cy[i] * c;
        v[i].z = center[2];
        v[i].r = pp->primCol.r / 255.0f;
        v[i].g = pp->primCol.g / 255.0f;
        v[i].b = pp->primCol.b / 255.0f;
        v[i].a = pp->primCol.a / 255.0f;
        v[i].u = cx[i] > 0.0f ? 1.0f : 0.0f;
        v[i].v = cy[i] > 0.0f ? 0.0f : 1.0f;
    }
    ResolveTexture(pp, &tex);
    M360_HsdDrawParticle(v, &tex, (pp->kind >> 22) & 3, (pp->kind & TexEdge) != 0);
}

/* Pass 1 draws edge (depth-writing) particles, pass 2 the rest; pass 0 only
 * advanced psdisp's frame counter. */
void psDispParticles(u32 target_link, u32 sw)
{
    HSD_CObj* cobj = HSD_CObjGetCurrent();
    Mtx vmtx;
    int link;
    if (sw == 0 || !cobj)
        return;
    HSD_CObjGetViewingMtx(cobj, vmtx);
    for (link = 0; link < 16; ++link) {
        HSD_Particle* pp;
        if (!(target_link & (1u << link)))
            continue;
        for (pp = hsd_804D0908[link]; pp; pp = pp->next) {
            const int edge = (pp->kind & TexEdge) != 0;
            if ((sw == 1) != edge || pp->size < FLT_EPSILON)
                continue;
            DrawParticle(pp, vmtx);
        }
    }
}
