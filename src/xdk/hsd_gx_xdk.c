#include <math.h>

#include <dolphin/mtx.h>
#include <sysdolphin/baselib/video.h>

HSD_VIInfo HSD_VIData;

void M360_HsdVideoInit(void)
{
    GXRenderModeObj* mode = &HSD_VIData.current.vi.rmode;
    mode->fbWidth = 640;
    mode->efbHeight = 480;
    mode->xfbHeight = 480;
    mode->viWidth = 640;
    mode->viHeight = 480;
    mode->field_rendering = 0;
}

static float Dot(const Vec* a, const Vec* b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

static void Normalize(Vec* v)
{
    const float length = sqrtf(Dot(v, v));
    if (length > 0.0f) {
        v->x /= length;
        v->y /= length;
        v->z /= length;
    }
}

static void Cross(const Vec* a, const Vec* b, Vec* out)
{
    Vec r;
    r.x = a->y * b->z - a->z * b->y;
    r.y = a->z * b->x - a->x * b->z;
    r.z = a->x * b->y - a->y * b->x;
    *out = r;
}

static void Zero44(float m[4][4])
{
    int r, c;
    for (r = 0; r < 4; ++r)
        for (c = 0; c < 4; ++c)
            m[r][c] = 0.0f;
}

void C_MTXLookAt(float m[3][4], Vec* camPos, Vec* camUp, Vec* target)
{
    Vec look, right, up;
    look.x = camPos->x - target->x;
    look.y = camPos->y - target->y;
    look.z = camPos->z - target->z;
    Normalize(&look);
    Cross(camUp, &look, &right);
    Normalize(&right);
    Cross(&look, &right, &up);
    m[0][0] = right.x; m[0][1] = right.y; m[0][2] = right.z;
    m[0][3] = -Dot(camPos, &right);
    m[1][0] = up.x; m[1][1] = up.y; m[1][2] = up.z;
    m[1][3] = -Dot(camPos, &up);
    m[2][0] = look.x; m[2][1] = look.y; m[2][2] = look.z;
    m[2][3] = -Dot(camPos, &look);
}

void MTXFrustum(float m[4][4], float t, float b, float l, float r, float n, float f)
{
    Zero44(m);
    m[0][0] = 2.0f * n / (r - l);
    m[0][2] = (r + l) / (r - l);
    m[1][1] = 2.0f * n / (t - b);
    m[1][2] = (t + b) / (t - b);
    m[2][2] = -n / (f - n);
    m[2][3] = -(f * n) / (f - n);
    m[3][2] = -1.0f;
}

void MTXPerspective(float m[4][4], float fovY, float aspect, float n, float f)
{
    const float cot = 1.0f / tanf(fovY * 0.5f * 3.14159265358979f / 180.0f);
    Zero44(m);
    m[0][0] = cot / aspect;
    m[1][1] = cot;
    m[2][2] = -n / (f - n);
    m[2][3] = -(f * n) / (f - n);
    m[3][2] = -1.0f;
}

void MTXOrtho(float m[4][4], float t, float b, float l, float r, float n, float f)
{
    Zero44(m);
    m[0][0] = 2.0f / (r - l);
    m[0][3] = -(r + l) / (r - l);
    m[1][1] = 2.0f / (t - b);
    m[1][3] = -(t + b) / (t - b);
    m[2][2] = -1.0f / (f - n);
    m[2][3] = -f / (f - n);
    m[3][3] = 1.0f;
}

void PSMTXMultVecSR(float m[3][4], Vec* src, Vec* dst)
{
    Vec r;
    r.x = m[0][0] * src->x + m[0][1] * src->y + m[0][2] * src->z;
    r.y = m[1][0] * src->x + m[1][1] * src->y + m[1][2] * src->z;
    r.z = m[2][0] * src->x + m[2][1] * src->y + m[2][2] * src->z;
    *dst = r;
}

void MTXRotRad(float m[3][4], char axis, float rad)
{
    const float s = sinf(rad), c = cosf(rad);
    int r, col;
    for (r = 0; r < 3; ++r)
        for (col = 0; col < 4; ++col)
            m[r][col] = r == col ? 1.0f : 0.0f;
    switch (axis) {
    case 'x': case 'X':
        m[1][1] = c; m[1][2] = -s; m[2][1] = s; m[2][2] = c;
        break;
    case 'y': case 'Y':
        m[0][0] = c; m[0][2] = s; m[2][0] = -s; m[2][2] = c;
        break;
    default:
        m[0][0] = c; m[0][1] = -s; m[1][0] = s; m[1][1] = c;
        break;
    }
}
