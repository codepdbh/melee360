#include "hsdmath_xdk_compat.h"

#include <math.h>
#include <stddef.h>

#include <dolphin/mtx.h>

extern "C" {
#include <sysdolphin/baselib/fobj.h>
#include <sysdolphin/baselib/spline.h>
}

#if defined(_XBOX)
typedef char M360_AssertSplineSize[(sizeof(HSD_Spline) == 0x18) ? 1 : -1];
typedef char M360_AssertFObjDescSize[(sizeof(HSD_FObjDesc) == 0x14) ? 1 : -1];
typedef char M360_AssertMtxSize[(sizeof(Mtx) == 48) ? 1 : -1];
typedef char M360_AssertVec3Size[(sizeof(Vec3) == 12) ? 1 : -1];
#endif

/* Scalar float replacements for the paired-single Dolphin PSMTX and PSVEC
 * routines that mtx.c calls; every routine tolerates aliased outputs. */
extern "C" {

void PSMTXIdentity(Mtx m)
{
    m[0][0] = 1.0f; m[0][1] = 0.0f; m[0][2] = 0.0f; m[0][3] = 0.0f;
    m[1][0] = 0.0f; m[1][1] = 1.0f; m[1][2] = 0.0f; m[1][3] = 0.0f;
    m[2][0] = 0.0f; m[2][1] = 0.0f; m[2][2] = 1.0f; m[2][3] = 0.0f;
}

void PSMTXCopy(Mtx src, Mtx dst)
{
    if (src == dst) {
        return;
    }
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) {
            dst[r][c] = src[r][c];
        }
    }
}

void PSMTXConcat(Mtx mA, Mtx mB, Mtx mAB)
{
    Mtx tmp;
    for (int r = 0; r < 3; ++r) {
        tmp[r][0] = mA[r][0] * mB[0][0] + mA[r][1] * mB[1][0] +
                    mA[r][2] * mB[2][0];
        tmp[r][1] = mA[r][0] * mB[0][1] + mA[r][1] * mB[1][1] +
                    mA[r][2] * mB[2][1];
        tmp[r][2] = mA[r][0] * mB[0][2] + mA[r][1] * mB[1][2] +
                    mA[r][2] * mB[2][2];
        tmp[r][3] = mA[r][0] * mB[0][3] + mA[r][1] * mB[1][3] +
                    mA[r][2] * mB[2][3] + mA[r][3];
    }
    PSMTXCopy(tmp, mAB);
}

void PSMTXTrans(Mtx m, f32 xT, f32 yT, f32 zT)
{
    PSMTXIdentity(m);
    m[0][3] = xT;
    m[1][3] = yT;
    m[2][3] = zT;
}

void PSMTXScale(Mtx m, f32 xS, f32 yS, f32 zS)
{
    PSMTXIdentity(m);
    m[0][0] = xS;
    m[1][1] = yS;
    m[2][2] = zS;
}

void PSMTXQuat(Mtx m, QuaternionPtr q)
{
    const f32 norm = q->x * q->x + q->y * q->y + q->z * q->z + q->w * q->w;
    if (norm == 0.0f) {
        PSMTXIdentity(m);
        return;
    }

    const f32 s = 2.0f / norm;
    const f32 xs = q->x * s;
    const f32 ys = q->y * s;
    const f32 zs = q->z * s;
    const f32 wx = q->w * xs;
    const f32 wy = q->w * ys;
    const f32 wz = q->w * zs;
    const f32 xx = q->x * xs;
    const f32 xy = q->x * ys;
    const f32 xz = q->x * zs;
    const f32 yy = q->y * ys;
    const f32 yz = q->y * zs;
    const f32 zz = q->z * zs;

    m[0][0] = 1.0f - (yy + zz);
    m[0][1] = xy - wz;
    m[0][2] = xz + wy;
    m[0][3] = 0.0f;
    m[1][0] = xy + wz;
    m[1][1] = 1.0f - (xx + zz);
    m[1][2] = yz - wx;
    m[1][3] = 0.0f;
    m[2][0] = xz - wy;
    m[2][1] = yz + wx;
    m[2][2] = 1.0f - (xx + yy);
    m[2][3] = 0.0f;
}

u32 PSMTXInverse(Mtx src, Mtx inv)
{
    const f32 det = src[0][0] * (src[1][1] * src[2][2] - src[2][1] * src[1][2]) -
                    src[0][1] * (src[1][0] * src[2][2] - src[2][0] * src[1][2]) +
                    src[0][2] * (src[1][0] * src[2][1] - src[2][0] * src[1][1]);
    if (det == 0.0f) {
        return 0;
    }

    const f32 r = 1.0f / det;
    Mtx tmp;
    tmp[0][0] = (src[1][1] * src[2][2] - src[2][1] * src[1][2]) * r;
    tmp[0][1] = -(src[0][1] * src[2][2] - src[2][1] * src[0][2]) * r;
    tmp[0][2] = (src[0][1] * src[1][2] - src[1][1] * src[0][2]) * r;
    tmp[1][0] = -(src[1][0] * src[2][2] - src[2][0] * src[1][2]) * r;
    tmp[1][1] = (src[0][0] * src[2][2] - src[2][0] * src[0][2]) * r;
    tmp[1][2] = -(src[0][0] * src[1][2] - src[1][0] * src[0][2]) * r;
    tmp[2][0] = (src[1][0] * src[2][1] - src[2][0] * src[1][1]) * r;
    tmp[2][1] = -(src[0][0] * src[2][1] - src[2][0] * src[0][1]) * r;
    tmp[2][2] = (src[0][0] * src[1][1] - src[1][0] * src[0][1]) * r;
    for (int i = 0; i < 3; ++i) {
        tmp[i][3] = -(tmp[i][0] * src[0][3] + tmp[i][1] * src[1][3] +
                      tmp[i][2] * src[2][3]);
    }
    PSMTXCopy(tmp, inv);
    return 1;
}

void PSMTXMultVec(Mtx44 m, Vec* src, Vec* dst)
{
    const f32 x = src->x;
    const f32 y = src->y;
    const f32 z = src->z;
    dst->x = m[0][0] * x + m[0][1] * y + m[0][2] * z + m[0][3];
    dst->y = m[1][0] * x + m[1][1] * y + m[1][2] * z + m[1][3];
    dst->z = m[2][0] * x + m[2][1] * y + m[2][2] * z + m[2][3];
}

void PSMTXRotAxisRad(Mtx m, Vec* axis, f32 rad)
{
    Vec n;
    PSVECNormalize(axis, &n);
    const f32 s = sinf(rad);
    const f32 c = cosf(rad);
    const f32 t = 1.0f - c;
    m[0][0] = t * n.x * n.x + c;
    m[0][1] = t * n.x * n.y - s * n.z;
    m[0][2] = t * n.x * n.z + s * n.y;
    m[0][3] = 0.0f;
    m[1][0] = t * n.x * n.y + s * n.z;
    m[1][1] = t * n.y * n.y + c;
    m[1][2] = t * n.y * n.z - s * n.x;
    m[1][3] = 0.0f;
    m[2][0] = t * n.x * n.z - s * n.y;
    m[2][1] = t * n.y * n.z + s * n.x;
    m[2][2] = t * n.z * n.z + c;
    m[2][3] = 0.0f;
}

void PSVECAdd(Vec* a, Vec* b, Vec* a_b)
{
    a_b->x = a->x + b->x;
    a_b->y = a->y + b->y;
    a_b->z = a->z + b->z;
}

void PSVECSubtract(Vec* a, Vec* b, Vec* a_b)
{
    a_b->x = a->x - b->x;
    a_b->y = a->y - b->y;
    a_b->z = a->z - b->z;
}

void PSVECScale(Vec* src, Vec* dst, f32 scale)
{
    dst->x = src->x * scale;
    dst->y = src->y * scale;
    dst->z = src->z * scale;
}

f32 PSVECDotProduct(Vec* vec1, Vec* vec2)
{
    return vec1->x * vec2->x + vec1->y * vec2->y + vec1->z * vec2->z;
}

void PSVECCrossProduct(Vec* vec1, Vec* vec2, Vec* dst)
{
    Vec tmp;
    tmp.x = vec1->y * vec2->z - vec1->z * vec2->y;
    tmp.y = vec1->z * vec2->x - vec1->x * vec2->z;
    tmp.z = vec1->x * vec2->y - vec1->y * vec2->x;
    *dst = tmp;
}

f32 PSVECMag(Vec* v)
{
    return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
}

void PSVECNormalize(Vec* vec1, Vec* dst)
{
    const f32 mag = PSVECMag(vec1);
    if (mag == 0.0f) {
        dst->x = 0.0f;
        dst->y = 0.0f;
        dst->z = 0.0f;
        return;
    }

    const f32 inv = 1.0f / mag;
    dst->x = vec1->x * inv;
    dst->y = vec1->y * inv;
    dst->z = vec1->z * inv;
}

} /* extern "C" */
