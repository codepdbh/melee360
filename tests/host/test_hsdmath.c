#include "hsdmath_xdk_compat.h"

#include "fobj.h"
#include "mtx.h"
#include "quatlib.h"
#include "random.h"
#include "spline.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

extern void M360_HSD_HeapInit(void);

static int fail_count = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            fprintf(stderr, "[M360][MATH][FAIL] %s (%s:%d)\n", #cond,        \
                    __FILE__, __LINE__);                                     \
            fail_count++;                                                    \
        }                                                                    \
    } while (0)

#define CHECK_NEAR(a, b, eps)                                                \
    do {                                                                     \
        double a_ = (double) (a);                                            \
        double b_ = (double) (b);                                            \
        if (!(fabs(a_ - b_) <= (double) (eps))) {                            \
            fprintf(stderr,                                                  \
                    "[M360][MATH][FAIL] %s=%.9g vs %s=%.9g (%s:%d)\n", #a,   \
                    a_, #b, b_, __FILE__, __LINE__);                         \
            fail_count++;                                                    \
        }                                                                    \
    } while (0)

#define EPS 1e-5

static void check_mtx_near(Mtx a, Mtx b, double eps, int line)
{
    int r, c;
    for (r = 0; r < 3; r++) {
        for (c = 0; c < 4; c++) {
            if (!(fabs((double) a[r][c] - (double) b[r][c]) <= eps)) {
                fprintf(stderr,
                        "[M360][MATH][FAIL] mtx[%d][%d]=%.9g vs %.9g (line "
                        "%d)\n",
                        r, c, (double) a[r][c], (double) b[r][c], line);
                fail_count++;
                return;
            }
        }
    }
}

#define CHECK_MTX(a, b, eps) check_mtx_near((a), (b), (eps), __LINE__)

static int host_is_little_endian(void)
{
    u32 probe = 1;
    u8 first;
    memcpy(&first, &probe, 1);
    return first == 1;
}

static void quat_near(Quaternion* a, Quaternion* b, double eps, int line)
{
    double dp = (double) a->x * b->x + (double) a->y * b->y +
                (double) a->z * b->z + (double) a->w * b->w;
    double sign = dp < 0.0 ? -1.0 : 1.0;
    if (!(fabs(a->x - sign * b->x) <= eps &&
          fabs(a->y - sign * b->y) <= eps &&
          fabs(a->z - sign * b->z) <= eps && fabs(a->w - sign * b->w) <= eps))
    {
        fprintf(stderr,
                "[M360][MATH][FAIL] quat (%g %g %g %g) vs (%g %g %g %g) "
                "(line %d)\n",
                (double) a->x, (double) a->y, (double) a->z, (double) a->w,
                (double) b->x, (double) b->y, (double) b->z, (double) b->w,
                line);
        fail_count++;
    }
}

#define CHECK_QUAT(a, b, eps) quat_near((a), (b), (eps), __LINE__)

static void test_primitives(void)
{
    Mtx id, t, s, ts, st, a;
    Vec3 v1 = { 1.0f, 2.0f, 2.0f };
    Vec3 v2 = { 0.0f, 1.0f, 0.0f };
    Vec3 out;

    PSMTXIdentity(id);
    CHECK(id[0][0] == 1.0f && id[1][1] == 1.0f && id[2][2] == 1.0f);
    CHECK(id[0][1] == 0.0f && id[0][2] == 0.0f && id[0][3] == 0.0f);
    CHECK(id[1][0] == 0.0f && id[2][3] == 0.0f);

    PSMTXTrans(t, 1.0f, 2.0f, 3.0f);
    PSMTXScale(s, 2.0f, 3.0f, 4.0f);

    {
        Mtx expect_ts = { { 2.0f, 0.0f, 0.0f, 1.0f },
                          { 0.0f, 3.0f, 0.0f, 2.0f },
                          { 0.0f, 0.0f, 4.0f, 3.0f } };
        Mtx expect_st = { { 2.0f, 0.0f, 0.0f, 2.0f },
                          { 0.0f, 3.0f, 0.0f, 6.0f },
                          { 0.0f, 0.0f, 4.0f, 12.0f } };
        PSMTXConcat(t, s, ts);
        PSMTXConcat(s, t, st);
        CHECK_MTX(ts, expect_ts, 0.0);
        CHECK_MTX(st, expect_st, 0.0);

        PSMTXCopy(t, a);
        PSMTXConcat(a, s, a);
        CHECK_MTX(a, expect_ts, 0.0);
        PSMTXCopy(s, a);
        PSMTXConcat(t, a, a);
        CHECK_MTX(a, expect_ts, 0.0);
    }

    CHECK_NEAR(PSVECMag(&v1), 3.0, EPS);
    PSVECNormalize(&v1, &out);
    CHECK_NEAR(out.x, 1.0 / 3.0, EPS);
    CHECK_NEAR(out.y, 2.0 / 3.0, EPS);
    CHECK_NEAR(out.z, 2.0 / 3.0, EPS);
    PSVECNormalize(&v1, &v1);
    CHECK_NEAR(PSVECMag(&v1), 1.0, EPS);

    {
        Vec3 x = { 1.0f, 0.0f, 0.0f };
        Vec3 y = { 0.0f, 1.0f, 0.0f };
        PSVECCrossProduct(&x, &y, &out);
        CHECK(out.x == 0.0f && out.y == 0.0f && out.z == 1.0f);
        PSVECCrossProduct(&x, &y, &x);
        CHECK(x.z == 1.0f && x.x == 0.0f);
    }

    CHECK_NEAR(PSVECDotProduct(&v1, &v2), v1.y, 0.0);
    PSVECScale(&v2, &out, 5.0f);
    CHECK(out.y == 5.0f && out.x == 0.0f);
    PSVECSubtract(&out, &v2, &out);
    CHECK(out.y == 4.0f);

    {
        Vec3 zero = { 0.0f, 0.0f, 0.0f };
        PSVECNormalize(&zero, &out);
        CHECK(out.x == 0.0f && out.y == 0.0f && out.z == 0.0f);
    }
}

static void make_trs(Mtx m, Vec3* scale, Vec3* rot, Vec3* trans)
{
    HSD_MtxSRT(m, scale, rot, trans, NULL);
}

static void test_mtx(void)
{
    Mtx m, inv, prod, id, t2, expect;
    Vec3 scale = { 2.0f, 3.0f, 0.5f };
    Vec3 rot = { 0.3f, -0.4f, 0.5f };
    Vec3 trans = { 10.0f, -20.0f, 30.0f };
    Vec3 got;
    int r, c;

    PSMTXIdentity(id);

    make_trs(m, &scale, &rot, &trans);

    {
        Vec3 unit = { 1.0f, 1.0f, 1.0f };
        HSD_MtxSRT(t2, &scale, &rot, &trans, &unit);
        CHECK_MTX(t2, m, 1e-6);
    }

    HSD_MtxInverse(m, inv);
    PSMTXConcat(m, inv, prod);
    CHECK_MTX(prod, id, 1e-4);
    PSMTXConcat(inv, m, prod);
    CHECK_MTX(prod, id, 1e-4);

    PSMTXCopy(m, t2);
    HSD_MtxInverse(t2, t2);
    for (r = 0; r < 3; r++) {
        for (c = 0; c < 3; c++) {
            CHECK_NEAR(t2[r][c], inv[r][c], 1e-6);
        }
    }
    CHECK_NEAR(t2[0][3], inv[0][3], 1e-6);

    {
        Mtx singular = { { 1.0f, 2.0f, 3.0f, 4.0f },
                         { 2.0f, 4.0f, 6.0f, 5.0f },
                         { 3.0f, 6.0f, 9.0f, 6.0f } };
        HSD_MtxInverse(singular, t2);
        CHECK_MTX(t2, id, 0.0);
    }

    {
        Mtx a, b, rhs, lhs;
        Vec3 sa = { 1.5f, 1.0f, 2.0f };
        Vec3 ra = { -0.2f, 0.7f, 0.1f };
        Vec3 ta = { 1.0f, 2.0f, 3.0f };
        make_trs(a, &sa, &ra, &ta);
        HSD_MtxInverse(a, rhs);
        PSMTXConcat(rhs, m, lhs);
        HSD_MtxInverseConcat(a, m, b);
        CHECK_MTX(b, lhs, 1e-4);

        PSMTXCopy(m, t2);
        HSD_MtxInverseConcat(a, t2, t2);
        CHECK_MTX(t2, lhs, 1e-4);

        PSMTXCopy(a, t2);
        HSD_MtxInverseConcat(t2, m, t2);
        CHECK_MTX(t2, lhs, 1e-4);
    }

    {
        Mtx diag, it;
        Vec3 s = { 2.0f, 4.0f, 5.0f };
        Vec3 zero = { 0.0f, 0.0f, 0.0f };
        make_trs(diag, &s, &zero, &zero);
        HSD_MtxInverseTranspose(diag, it);
        CHECK_NEAR(it[0][0], 0.5, EPS);
        CHECK_NEAR(it[1][1], 0.25, EPS);
        CHECK_NEAR(it[2][2], 0.2, EPS);
        CHECK(it[0][3] == 0.0f && it[1][3] == 0.0f && it[2][3] == 0.0f);

        {
            Mtx rotm, rit;
            HSD_MkRotationMtx(rotm, &rot);
            HSD_MtxInverseTranspose(rotm, rit);
            CHECK_MTX(rit, rotm, 1e-5);
        }
    }

    HSD_MtxGetTranslate(m, &got);
    CHECK_NEAR(got.x, 10.0, EPS);
    CHECK_NEAR(got.y, -20.0, EPS);
    CHECK_NEAR(got.z, 30.0, EPS);

    HSD_MtxGetScale(m, &got);
    CHECK_NEAR(got.x, 2.0, 1e-4);
    CHECK_NEAR(got.y, 3.0, 1e-4);
    CHECK_NEAR(got.z, 0.5, 1e-4);

    HSD_MtxGetRotation(m, &got);
    CHECK_NEAR(got.x, rot.x, 1e-5);
    CHECK_NEAR(got.y, rot.y, 1e-5);
    CHECK_NEAR(got.z, rot.z, 1e-5);

    {
        Mtx rz;
        Vec3 e = { 0.0f, 0.0f, (f32) (M_PI / 2) };
        HSD_MkRotationMtx(rz, &e);
        CHECK_NEAR(rz[0][0], 0.0, EPS);
        CHECK_NEAR(rz[1][0], 1.0, EPS);
        CHECK_NEAR(rz[0][1], -1.0, EPS);
        CHECK_NEAR(rz[1][1], 0.0, EPS);
        CHECK_NEAR(rz[2][2], 1.0, EPS);
    }

    {
        Mtx rm;
        Vec3 e = { 1.1f, -0.6f, 2.3f };
        f32 dotc;
        HSD_MkRotationMtx(rm, &e);
        for (r = 0; r < 3; r++) {
            for (c = 0; c < 3; c++) {
                dotc = rm[0][r] * rm[0][c] + rm[1][r] * rm[1][c] +
                       rm[2][r] * rm[2][c];
                CHECK_NEAR(dotc, r == c ? 1.0 : 0.0, 1e-5);
            }
        }
    }

    {
        Quaternion q = { 0.0f, 0.0f, (f32) sin(M_PI / 4), (f32) cos(M_PI / 4) };
        Mtx expect_z = { { 0.0f, -1.0f, 0.0f, 0.0f },
                         { 1.0f, 0.0f, 0.0f, 0.0f },
                         { 0.0f, 0.0f, 1.0f, 0.0f } };
        HSD_MtxQuat(t2, &q);
        CHECK_MTX(t2, expect_z, 1e-6);
    }

    {
        Mtx m0, m1, out;
        int i;
        for (i = 0; i < 12; i++) {
            (&m0[0][0])[i] = (f32) i;
            (&m1[0][0])[i] = (f32) (100 + i);
            (&expect[0][0])[i] = (f32) (100 + i) + 0.5f * (f32) i;
        }
        HSD_MtxScaledAdd(m0, m1, out, 0.5f);
        CHECK_MTX(out, expect, 1e-6);
    }

    {
        Vec3 unit_scale = { 1.0f, 1.0f, 1.0f };
        Quaternion q;
        Mtx via_quat, via_euler;
        EulerToQuat(&rot, &q);
        HSD_MtxSRTQuat(via_quat, &scale, &q, &trans, NULL);
        HSD_MtxSRT(via_euler, &scale, &rot, &trans, NULL);
        CHECK_MTX(via_quat, via_euler, 1e-4);

        HSD_MtxSRTQuat(via_quat, &unit_scale, &q, &trans, &unit_scale);
        {
            Mtx pure;
            HSD_MtxSRT(pure, &unit_scale, &rot, &trans, NULL);
            CHECK_MTX(via_quat, pure, 1e-4);
        }
    }

    {
        void* mtxs[3];
        void* vecs[3];
        int i;
        HSD_MtxInitAllocData();
        HSD_VecInitAllocData();
        CHECK(HSD_MtxGetAllocData() != NULL);
        CHECK(HSD_VecGetAllocData() != NULL);
        for (i = 0; i < 3; i++) {
            mtxs[i] = HSD_MtxAlloc();
            vecs[i] = HSD_VecAlloc();
            CHECK(mtxs[i] != NULL && vecs[i] != NULL);
        }
        CHECK(HSD_ObjAllocGetUsing(HSD_MtxGetAllocData()) == 3);
        CHECK(HSD_ObjAllocGetUsing(HSD_VecGetAllocData()) == 3);
        for (i = 0; i < 3; i++) {
            HSD_MtxFree(mtxs[i]);
            HSD_VecFree(vecs[i]);
        }
        HSD_MtxFree(NULL);
        HSD_VecFree(NULL);
        CHECK(HSD_ObjAllocGetUsing(HSD_MtxGetAllocData()) == 0);
        CHECK(HSD_ObjAllocGetUsing(HSD_VecGetAllocData()) == 0);
    }
}

static void test_quat(void)
{
    static const Vec3 eulers[] = {
        { 0.3f, -0.4f, 0.5f }, { -1.2f, 0.2f, 2.5f }, { 2.9f, 1.0f, -0.7f },
        { 0.0f, 0.0f, 0.0f },  { 1.0f, 0.0f, 0.0f },
    };
    size_t i;
    Quaternion q, q2, p;
    Mtx m, m2, rm;
    Vec3 e, back;

    for (i = 0; i < sizeof(eulers) / sizeof(eulers[0]); i++) {
        e = eulers[i];
        CHECK(EulerToQuat(&e, &q) == 0);
        CHECK_NEAR(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w, 1.0, 1e-5);
        HSD_MtxQuat(m, &q);
        HSD_MkRotationMtx(rm, &e);
        CHECK_MTX(m, rm, 1e-5);

        CHECK(MatToQuat(m, &q2) == 0);
        CHECK_QUAT(&q2, &q, 1e-5);

        CHECK(HSD_QuatLib_8037EB28(m, &back) == 0);
        HSD_MkRotationMtx(m2, &back);
        CHECK_MTX(m2, m, 1e-5);
    }

    {
        Vec3 e2 = { 0.3f, -0.4f, 0.5f };
        HSD_MkRotationMtx(m, &e2);
        HSD_QuatLib_8037EB28(m, &back);
        CHECK_NEAR(back.x, e2.x, 1e-5);
        CHECK_NEAR(back.y, e2.y, 1e-5);
        CHECK_NEAR(back.z, e2.z, 1e-5);
    }

    {
        Vec3 gimbal = { 0.2f, (f32) (M_PI / 2), 0.3f };
        HSD_MkRotationMtx(m, &gimbal);
        HSD_QuatLib_8037EB28(m, &back);
        CHECK_NEAR(back.y, M_PI / 2, 1e-4);
        CHECK(back.z == 0.0f);
        HSD_MkRotationMtx(m2, &back);
        CHECK_MTX(m2, m, 1e-3);
    }

    {
        static const Quaternion half_turns[] = {
            { 1.0f, 0.0f, 0.0f, 0.0f },
            { 0.0f, 1.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 1.0f, 0.0f },
        };
        for (i = 0; i < 3; i++) {
            q = half_turns[i];
            HSD_MtxQuat(m, &q);
            CHECK(MatToQuat(m, &q2) == 0);
            CHECK_QUAT(&q2, &q, 1e-5);
        }
    }

    {
        Vec3 axis = { 0.0f, 0.0f, 2.0f };
        Vec3 zero = { 0.0f, 0.0f, 0.0f };
        Vec3 axis2 = { 1.0f, 1.0f, 1.0f };
        CHECK(HSD_QuatLib_8037ECE0(&axis, &q, (f32) M_PI) == 0);
        CHECK_NEAR(q.x, 0.0, EPS);
        CHECK_NEAR(q.y, 0.0, EPS);
        CHECK_NEAR(q.z, 1.0, EPS);
        CHECK_NEAR(q.w, 0.0, 1e-6);
        CHECK(HSD_QuatLib_8037ECE0(&zero, &q, 1.0f) == -1);

        HSD_QuatLib_8037ECE0(&axis2, &q, 1.0f);
        CHECK_NEAR(q.w, cos(0.5), 1e-6);
        CHECK_NEAR(q.x, sin(0.5) / sqrt(3.0), 1e-6);
    }

    {
        Vec3 ea = { 0.4f, 0.1f, -0.9f };
        Vec3 eb = { -0.3f, 0.8f, 0.2f };
        Quaternion qa, qb, qab;
        Mtx ma, mb, mab, mprod;
        EulerToQuat(&ea, &qa);
        EulerToQuat(&eb, &qb);
        HSD_QuatLib_8037EC4C(&qa, &qb, &qab);
        HSD_MtxQuat(ma, &qa);
        HSD_MtxQuat(mb, &qb);
        HSD_MtxQuat(mab, &qab);
        PSMTXConcat(ma, mb, mprod);
        CHECK_MTX(mab, mprod, 1e-5);

        qa = qab;
        HSD_QuatLib_8037EC4C(&qa, &qb, &qa);
        HSD_MtxQuat(ma, &qa);
        HSD_MtxQuat(mb, &qb);
        PSMTXConcat(mab, mb, mprod);
        CHECK_MTX(ma, mprod, 1e-5);
    }

    {
        Quaternion ident = { 0.0f, 0.0f, 0.0f, 1.0f };
        Quaternion quarter = { 0.0f, 0.0f, (f32) sin(M_PI / 4),
                               (f32) cos(M_PI / 4) };
        Quaternion out;
        Quaternion expect_mid = { 0.0f, 0.0f, (f32) sin(M_PI / 8),
                                  (f32) cos(M_PI / 8) };

        CHECK(HSD_QuatLib_8037EF28(&ident, &quarter, &out, 0.0f) == 0);
        CHECK_QUAT(&out, &ident, 1e-6);
        HSD_QuatLib_8037EF28(&ident, &quarter, &out, 1.0f);
        CHECK_QUAT(&out, &quarter, 1e-6);
        HSD_QuatLib_8037EF28(&ident, &quarter, &out, 0.5f);
        CHECK_QUAT(&out, &expect_mid, 1e-6);
        CHECK_NEAR(out.x * out.x + out.y * out.y + out.z * out.z +
                       out.w * out.w,
                   1.0, 1e-6);

        p = ident;
        HSD_QuatLib_8037EF28(&p, &p, &out, 0.37f);
        CHECK_QUAT(&out, &ident, 1e-6);
    }
}

static void put_f32(u8* dst, f32 value)
{
    u32 bits;
    memcpy(&bits, &value, sizeof(bits));
    dst[0] = (u8) (bits & 0xFF);
    dst[1] = (u8) ((bits >> 8) & 0xFF);
    dst[2] = (u8) ((bits >> 16) & 0xFF);
    dst[3] = (u8) ((bits >> 24) & 0xFF);
}

static double hermite_ref(double T, double t, double p0, double p1, double d0,
                          double d1)
{
    double s = t / T;
    double h00 = 2 * s * s * s - 3 * s * s + 1;
    double h10 = s * s * s - 2 * s * s + s;
    double h01 = -2 * s * s * s + 3 * s * s;
    double h11 = s * s * s - s * s;
    return h00 * p0 + h10 * T * d0 + h01 * p1 + h11 * T * d1;
}

static DiscVec3 spl_cv[16];
static DiscF32 spl_seg[8];
static DiscF32 spl_poly[16];

static void set_cv(int i, f32 x, f32 y, f32 z)
{
    spl_cv[i].x = x;
    spl_cv[i].y = y;
    spl_cv[i].z = z;
}

static void init_spline(HSD_Spline* s, u8 type, s16 numcv, f32 tension)
{
    memset(s, 0, sizeof(*s));
    s->type = type;
    s->numcv = numcv;
    s->tension = tension;
    s->cv = spl_cv;
    s->segLength = spl_seg;
    s->segPoly = spl_poly;
}

static void test_spline(void)
{
    HSD_Spline s;
    Vec3 p;
    int i;

    CHECK_NEAR(splGetHelmite(0.1f, 0.0f, 3.0f, 7.0f, 1.0f, -2.0f), 3.0, EPS);
    CHECK_NEAR(splGetHelmite(0.1f, 10.0f, 3.0f, 7.0f, 1.0f, -2.0f), 7.0, 1e-4);
    CHECK_NEAR(splGetHelmite(0.1f, 5.0f, 0.0f, 10.0f, 0.0f, 0.0f), 5.0, 1e-5);
    CHECK_NEAR(splGetHelmite(0.1f, 3.0f, 0.0f, 10.0f, 2.0f, -1.0f),
               hermite_ref(10.0, 3.0, 0.0, 10.0, 2.0, -1.0), 1e-4);
    CHECK_NEAR(splGetHelmite(0.25f, 1.5f, -4.0f, 9.0f, 0.5f, 3.0f),
               hermite_ref(4.0, 1.5, -4.0, 9.0, 0.5, 3.0), 1e-4);

    memset(spl_cv, 0, sizeof(spl_cv));

    /* lerp reads four points from idx, so pad past numcv. */
    set_cv(0, 0.0f, 0.0f, 0.0f);
    set_cv(1, 2.0f, 4.0f, 6.0f);
    for (i = 2; i < 8; i++) {
        set_cv(i, 10.0f, 4.0f, -6.0f);
    }
    init_spline(&s, 0, 3, 0.0f);
    splGetSplinePoint(&p, &s, 0.0f);
    CHECK(p.x == 0.0f && p.y == 0.0f && p.z == 0.0f);
    splGetSplinePoint(&p, &s, 0.25f);
    CHECK_NEAR(p.x, 1.0, EPS);
    CHECK_NEAR(p.y, 2.0, EPS);
    CHECK_NEAR(p.z, 3.0, EPS);
    splGetSplinePoint(&p, &s, 0.5f);
    CHECK(p.x == 2.0f && p.y == 4.0f && p.z == 6.0f);
    splGetSplinePoint(&p, &s, 0.75f);
    CHECK_NEAR(p.x, 6.0, EPS);
    CHECK_NEAR(p.z, 0.0, EPS);
    splGetSplinePoint(&p, &s, 1.0f);
    CHECK(p.x == 10.0f && p.y == 4.0f && p.z == -6.0f);

    p.x = 77.0f;
    splGetSplinePoint(&p, &s, -0.1f);
    CHECK(p.x == 77.0f);
    splGetSplinePoint(&p, &s, 1.1f);
    CHECK(p.x == 77.0f);

    /* bezier, one segment: numcv=2 addresses four control points. */
    set_cv(0, 0.0f, 0.0f, 0.0f);
    set_cv(1, 1.0f, 2.0f, 0.0f);
    set_cv(2, 3.0f, 2.0f, 0.0f);
    set_cv(3, 4.0f, 0.0f, 0.0f);
    init_spline(&s, 1, 2, 0.0f);
    splGetSplinePoint(&p, &s, 0.0f);
    CHECK(p.x == 0.0f && p.y == 0.0f);
    splGetSplinePoint(&p, &s, 0.5f);
    CHECK_NEAR(p.x, (0.0 + 3 * 1.0 + 3 * 3.0 + 4.0) / 8.0, EPS);
    CHECK_NEAR(p.y, (0.0 + 3 * 2.0 + 3 * 2.0 + 0.0) / 8.0, EPS);
    splGetSplinePoint(&p, &s, 1.0f);
    CHECK(p.x == 4.0f && p.y == 0.0f);

    /* uniform cubic B-spline over collinear control points is linear. */
    for (i = 0; i < 8; i++) {
        set_cv(i, (f32) i, 0.0f, 0.0f);
    }
    init_spline(&s, 2, 3, 0.0f);
    splGetSplinePoint(&p, &s, 0.0f);
    CHECK_NEAR(p.x, 1.0, EPS);
    splGetSplinePoint(&p, &s, 0.25f);
    CHECK_NEAR(p.x, 1.5, EPS);
    splGetSplinePoint(&p, &s, 0.5f);
    CHECK_NEAR(p.x, 2.0, EPS);
    splGetSplinePoint(&p, &s, 1.0f);
    CHECK_NEAR(p.x, 3.0, EPS);

    /* cardinal, tension 0.5 (Catmull-Rom), is linear on collinear data. */
    init_spline(&s, 3, 3, 0.5f);
    splGetSplinePoint(&p, &s, 0.0f);
    CHECK_NEAR(p.x, 1.0, EPS);
    splGetSplinePoint(&p, &s, 0.75f);
    CHECK_NEAR(p.x, 2.5, EPS);
    splGetSplinePoint(&p, &s, 1.0f);
    CHECK_NEAR(p.x, 3.0, EPS);
    set_cv(2, 2.0f, 5.0f, 0.0f);
    splGetSplinePoint(&p, &s, 0.5f);
    CHECK_NEAR(p.x, 2.0, EPS);
    CHECK_NEAR(p.y, 5.0, EPS);

    /* arc length, lerp: 1 then 3 units, cumulative fractions 0/.25/1. */
    set_cv(0, 0.0f, 0.0f, 0.0f);
    set_cv(1, 1.0f, 0.0f, 0.0f);
    for (i = 2; i < 8; i++) {
        set_cv(i, 4.0f, 0.0f, 0.0f);
    }
    spl_seg[0].v = 0.0f;
    spl_seg[1].v = 0.25f;
    spl_seg[2].v = 1.0f;
    init_spline(&s, 0, 3, 0.0f);
    s.totalLength = 4.0f;
    CHECK(splArcLengthGetParameter(&s, 0.0f) == 0.0f);
    CHECK(splArcLengthGetParameter(&s, 1.0f) == 1.0f);
    CHECK_NEAR(splArcLengthGetParameter(&s, 0.125f), 0.25, EPS);
    CHECK_NEAR(splArcLengthGetParameter(&s, 0.625f), 0.75, EPS);
    splArcLengthPoint(&p, &s, 0.625f);
    CHECK_NEAR(p.x, 2.5, EPS);
    splArcLengthPoint(&p, &s, 0.125f);
    CHECK_NEAR(p.x, 0.5, EPS);

    /* arc length, bezier x(t)=t^2 (control x = 0,0,1/3,1): speed^2 = 4t^2,
     * so arc-length fraction s maps to t = sqrt(s) and x = s. */
    set_cv(0, 0.0f, 0.0f, 0.0f);
    set_cv(1, 0.0f, 0.0f, 0.0f);
    set_cv(2, 1.0f / 3.0f, 0.0f, 0.0f);
    set_cv(3, 1.0f, 0.0f, 0.0f);
    spl_seg[0].v = 0.0f;
    spl_seg[1].v = 1.0f;
    spl_poly[0].v = 0.0f;
    spl_poly[1].v = 0.0f;
    spl_poly[2].v = 4.0f;
    spl_poly[3].v = 0.0f;
    spl_poly[4].v = 0.0f;
    init_spline(&s, 1, 2, 0.0f);
    s.totalLength = 1.0f;
    CHECK_NEAR(splArcLengthGetParameter(&s, 0.25f), 0.5, 2e-3);
    CHECK_NEAR(splArcLengthGetParameter(&s, 0.81f), 0.9, 2e-3);
    splArcLengthPoint(&p, &s, 0.25f);
    CHECK_NEAR(p.x, 0.25, 2e-3);
    splArcLengthPoint(&p, &s, 0.5f);
    CHECK_NEAR(p.x, 0.5, 2e-3);
}

typedef struct UpdateLog {
    int calls;
    int last_type;
    f32 last_value;
    f32 by_type[8];
} UpdateLog;

static void on_update(void* obj, enum_t type, HSD_ObjData* fval)
{
    UpdateLog* log = (UpdateLog*) obj;
    log->calls++;
    log->last_type = (int) type;
    log->last_value = fval->fv;
    if (type >= 0 && type < 8) {
        log->by_type[type] = fval->fv;
    }
}

static void fill_desc(HSD_FObjDesc* d, u8* stream, u32 length, u8 type,
                      u8 frac_value, u8 frac_slope, HSD_FObjDesc* next)
{
    memset(d, 0, sizeof(*d));
    d->next = next;
    d->length = length;
    d->startframe = 0.0f;
    d->type = type;
    d->frac_value = frac_value;
    d->frac_slope = frac_slope;
    d->ad = stream;
}

static f32 frame_value(HSD_FObj* fobj, UpdateLog* log, f32 rate)
{
    log->calls = 0;
    HSD_FObjInterpretAnim(fobj, log, on_update, rate);
    CHECK(log->calls == 1);
    return log->last_value;
}

static void test_fobj(void)
{
    UpdateLog log;
    HSD_FObj* fobj;
    HSD_FObjDesc desc;
    int k;

    HSD_FObjInitAllocData();
    memset(&log, 0, sizeof(log));

    /* Stream layout: [op | (pack-1) << 4][values...][wait varint], values
     * byte-assembled from little-endian bytes, so the decode is identical on
     * big- and little-endian hosts. Linear, float: 0.0 -> 100.0 in 10. */
    {
        u8 lin[11];
        lin[0] = 0x12;
        put_f32(lin + 1, 0.0f);
        lin[5] = 10;
        put_f32(lin + 6, 100.0f);
        fill_desc(&desc, lin, 10, 12, HSD_A_FRAC_FLOAT, HSD_A_FRAC_FLOAT,
                  NULL);
        fobj = HSD_FObjLoadDesc(&desc);
        CHECK(fobj != NULL);
        CHECK(fobj->obj_type == 12);
        CHECK(fobj->length == 10);
        CHECK(fobj->ad_head == lin);
        CHECK(HSD_FObjGetState(fobj) == 0);
        HSD_FObjReqAnimAll(fobj, 0.0f);
        CHECK(HSD_FObjGetState(fobj) == 1);
        CHECK_NEAR(frame_value(fobj, &log, 0.0f), 0.0, EPS);
        for (k = 1; k <= 10; k++) {
            CHECK_NEAR(frame_value(fobj, &log, 1.0f), 10.0 * k, 1e-3);
        }
        CHECK(log.last_type == 12);
        HSD_FObjRemoveAll(fobj);
        CHECK(HSD_ObjAllocGetUsing(HSD_FObjGetAllocData()) == 0);
    }

    /* constant, signed 16-bit fixed point with 8 fractional bits. */
    {
        u8 con[] = { 0x11, 0x80, 0x01, 5, 0x00, 0xFF, 3 };
        fill_desc(&desc, con, sizeof(con), 1, (u8) (HSD_A_FRAC_S16 | 8),
                  HSD_A_FRAC_FLOAT, NULL);
        fobj = HSD_FObjLoadDesc(&desc);
        HSD_FObjReqAnimAll(fobj, 0.0f);
        CHECK_NEAR(frame_value(fobj, &log, 0.0f), 1.5, EPS);
        for (k = 1; k < 5; k++) {
            CHECK_NEAR(frame_value(fobj, &log, 1.0f), 1.5, EPS);
        }
        CHECK_NEAR(frame_value(fobj, &log, 1.0f), -1.0, EPS);
        HSD_FObjRemoveAll(fobj);
    }

    /* every value encoding, chained through desc->next. */
    {
        u8 s8s[] = { 0x11, 0xFB, 0x00, 0xFB, 0x00 };
        u8 u8s[] = { 0x11, 0x28, 0x00, 0x28, 0x00 };
        u8 u16s[] = { 0x11, 0x34, 0x12, 0x00, 0x34, 0x12, 0x00 };
        u8 s16s[] = { 0x11, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00 };
        u8 flt[11];
        HSD_FObjDesc d4, d3, d2, d1, d0;
        HSD_FObj* head;
        flt[0] = 0x11;
        put_f32(flt + 1, -3.25f);
        flt[5] = 0;
        put_f32(flt + 6, -3.25f);
        flt[10] = 0;
        fill_desc(&d4, flt, sizeof(flt), 4, HSD_A_FRAC_FLOAT, 0, NULL);
        fill_desc(&d3, s16s, sizeof(s16s), 3, (u8) (HSD_A_FRAC_S16 | 15), 0,
                  &d4);
        fill_desc(&d2, u16s, sizeof(u16s), 2, (u8) (HSD_A_FRAC_U16 | 0), 0,
                  &d3);
        fill_desc(&d1, u8s, sizeof(u8s), 1, (u8) (HSD_A_FRAC_U8 | 4), 0, &d2);
        fill_desc(&d0, s8s, sizeof(s8s), 0, (u8) (HSD_A_FRAC_S8 | 1), 0, &d1);
        head = HSD_FObjLoadDesc(&d0);
        CHECK(head != NULL && head->next != NULL &&
              head->next->next != NULL && head->next->next->next != NULL &&
              head->next->next->next->next != NULL &&
              head->next->next->next->next->next == NULL);
        HSD_FObjReqAnimAll(head, 0.0f);
        memset(&log, 0, sizeof(log));
        HSD_FObjInterpretAnimAll(head, &log, on_update, 0.0f);
        CHECK(log.calls == 5);
        CHECK_NEAR(log.by_type[0], -2.5, EPS);
        CHECK_NEAR(log.by_type[1], 2.5, EPS);
        CHECK_NEAR(log.by_type[2], 4660.0, EPS);
        CHECK_NEAR(log.by_type[3], -1.0, EPS);
        CHECK_NEAR(log.by_type[4], -3.25, EPS);
        HSD_FObjRemoveAll(head);
        CHECK(HSD_ObjAllocGetUsing(HSD_FObjGetAllocData()) == 0);
    }

    /* pack count with a continuation byte: 8 + (1 << 3) = 16 keys. */
    {
        u8 pk[34];
        u32 pos = 0;
        pk[pos++] = 0xF1;
        pk[pos++] = 0x01;
        for (k = 0; k < 16; k++) {
            pk[pos++] = (u8) k;
            pk[pos++] = 1;
        }
        fill_desc(&desc, pk, pos, 5, (u8) (HSD_A_FRAC_U8 | 0),
                  HSD_A_FRAC_FLOAT, NULL);
        fobj = HSD_FObjLoadDesc(&desc);
        HSD_FObjReqAnimAll(fobj, 0.0f);
        CHECK_NEAR(frame_value(fobj, &log, 0.0f), 0.0, EPS);
        for (k = 1; k <= 15; k++) {
            CHECK_NEAR(frame_value(fobj, &log, 1.0f), (double) k, EPS);
        }
        HSD_FObjRemoveAll(fobj);
    }

    /* hermite op: float value + float slope through splGetHelmite. */
    {
        u8 sp[19];
        sp[0] = 0x14;
        put_f32(sp + 1, 0.0f);
        put_f32(sp + 5, 2.0f);
        sp[9] = 10;
        put_f32(sp + 10, 10.0f);
        put_f32(sp + 14, -1.0f);
        fill_desc(&desc, sp, 18, 7, HSD_A_FRAC_FLOAT, HSD_A_FRAC_FLOAT, NULL);
        fobj = HSD_FObjLoadDesc(&desc);
        HSD_FObjReqAnimAll(fobj, 0.0f);
        CHECK_NEAR(frame_value(fobj, &log, 3.0f),
                   hermite_ref(10.0, 3.0, 0.0, 10.0, 2.0, -1.0), 1e-4);
        CHECK_NEAR(frame_value(fobj, &log, 4.0f),
                   hermite_ref(10.0, 7.0, 0.0, 10.0, 2.0, -1.0), 1e-4);
        HSD_FObjRemoveAll(fobj);
    }

    /* start frame offset: time = startframe + requested start. */
    {
        u8 st[] = { 0x01, 0x05, 0x00 };
        fill_desc(&desc, st, sizeof(st), 2, HSD_A_FRAC_U8, 0, NULL);
        desc.startframe = 12.0f;
        fobj = HSD_FObjLoadDesc(&desc);
        CHECK(fobj->startframe == 12);
        HSD_FObjReqAnimAll(fobj, 3.0f);
        CHECK_NEAR(fobj->time, 15.0, 0.0);
        HSD_FObjStopAnimAll(fobj, &log, on_update, 0.0f);
        CHECK(HSD_FObjGetState(fobj) == 0);
        HSD_FObjRemoveAll(fobj);
    }
}

static void test_random(void)
{
    u32 ref = 1;
    u32 local_seed = 12345;
    u32* saved = HSD_RandSeedPtr;
    int i;

    *HSD_RandSeedPtr = 1;
    for (i = 0; i < 64; i++) {
        ref = ref * 214013u + 2531011u;
        CHECK(HSD_Rand() == (s32) (ref >> 16));
    }

    *HSD_RandSeedPtr = 1;
    CHECK(HSD_Rand() == 41);
    CHECK(HSD_Rand() == 51235);
    CHECK(HSD_Rand() == 6334);

    ref = 7;
    *HSD_RandSeedPtr = 7;
    for (i = 0; i < 64; i++) {
        s32 expect;
        ref = ref * 214013u + 2531011u;
        expect = (s32) (10 * (s32) (ref >> 16) / 65536);
        CHECK(HSD_Randi(10) == expect);
    }

    *HSD_RandSeedPtr = 99;
    for (i = 0; i < 256; i++) {
        f32 f = HSD_Randf();
        s32 n = HSD_Randi(6);
        CHECK(f >= 0.0f && f < 1.0f);
        CHECK(n >= 0 && n < 6);
    }
    CHECK(randi(0) == 0);

    HSD_RandSeedPtr = &local_seed;
    CHECK(HSD_Rand() == (s32) ((12345u * 214013u + 2531011u) >> 16));
    _HSD_RandForgetMemory(&local_seed, &local_seed + 1);
    CHECK(HSD_RandSeedPtr == saved);
    CHECK(HSD_RandSeedPtr != &local_seed);
    _HSD_RandForgetMemory(&local_seed, &local_seed + 1);
    CHECK(HSD_RandSeedPtr == saved);
}

int main(void)
{
    M360_HSD_HeapInit();
    HSD_LogInit();

    printf("[M360][MATH] host byte order: %s (fobj stream is byte-assembled, "
           "endian-independent)\n",
           host_is_little_endian() ? "little" : "big");

    test_primitives();
    test_mtx();
    test_quat();
    test_spline();
    test_fobj();
    test_random();

    if (fail_count) {
        fprintf(stderr, "[M360][MATH] %d check(s) failed\n", fail_count);
        return 1;
    }

    printf("[M360][MATH] mtx/quatlib/spline/fobj/random host validation "
           "passed\n");
    return 0;
}
