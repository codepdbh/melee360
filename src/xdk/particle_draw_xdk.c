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

/* Debug capsule overlay (hitbox/hurtbox view): a camera-facing disc at each
 * end and a band between them, drawn without depth test (blend mode 2). */
static void DebugTri(const float* a, const float* b, const float* c, const float* rgba)
{
    M360ParticleVertex v[4];
    const float* p[4];
    int i;
    p[0] = a;
    p[1] = b;
    p[2] = c;
    p[3] = c;
    for (i = 0; i < 4; ++i) {
        v[i].x = p[i][0];
        v[i].y = p[i][1];
        v[i].z = p[i][2];
        v[i].r = rgba[0];
        v[i].g = rgba[1];
        v[i].b = rgba[2];
        v[i].a = rgba[3];
        v[i].u = v[i].v = 0.0f;
    }
    M360_HsdDrawParticle(v, NULL, 2, 0);
}

static void DebugDisc(const float* c, float radius, const float* rgba)
{
    enum { kSides = 12 };
    float prev[3], cur[3];
    int i;
    prev[0] = c[0] + radius;
    prev[1] = c[1];
    prev[2] = c[2];
    for (i = 1; i <= kSides; ++i) {
        const float t = 6.2831853f * (float) i / kSides;
        cur[0] = c[0] + radius * (float) cos(t);
        cur[1] = c[1] + radius * (float) sin(t);
        cur[2] = c[2];
        DebugTri(c, prev, cur, rgba);
        prev[0] = cur[0];
        prev[1] = cur[1];
    }
}

void M360_DrawDebugCapsule(const Vec3* a, const Vec3* b, float radius, float r, float g,
                           float bl, float alpha)
{
    HSD_CObj* cobj = HSD_CObjGetCurrent();
    Mtx vmtx;
    float va[3], vb[3], rgba[4], dx, dy, len, nx, ny;
    if (!cobj || radius <= 0.0f)
        return;
    HSD_CObjGetViewingMtx(cobj, vmtx);
    ToView(vmtx, a, va);
    ToView(vmtx, b, vb);
    rgba[0] = r;
    rgba[1] = g;
    rgba[2] = bl;
    rgba[3] = alpha;
    DebugDisc(va, radius, rgba);
    dx = vb[0] - va[0];
    dy = vb[1] - va[1];
    len = (float) sqrt(dx * dx + dy * dy);
    if (len > 0.01f) {
        float q[4][3];
        nx = -dy / len * radius;
        ny = dx / len * radius;
        q[0][0] = va[0] + nx; q[0][1] = va[1] + ny; q[0][2] = va[2];
        q[1][0] = vb[0] + nx; q[1][1] = vb[1] + ny; q[1][2] = vb[2];
        q[2][0] = vb[0] - nx; q[2][1] = vb[1] - ny; q[2][2] = vb[2];
        q[3][0] = va[0] - nx; q[3][1] = va[1] - ny; q[3][2] = va[2];
        DebugTri(q[0], q[1], q[2], rgba);
        DebugTri(q[0], q[2], q[3], rgba);
        DebugDisc(vb, radius, rgba);
    }
}
