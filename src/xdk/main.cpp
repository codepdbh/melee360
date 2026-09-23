#include <xtl.h>
#include <stdio.h>

#include "melee_pad_xdk.h"
#include "melee_boot_xdk.h"
#include "melee_audio_xdk.h"
#include "melee_flow_xdk.h"
#include "melee_movie_xdk.h"
#include "hsd_render_xdk.h"
#include "menu_scene_xdk.h"
#include "match_xdk.h"
#include "melee_title_scene_xdk.h"
#include "sprite_renderer.h"

extern "C" unsigned int lbTime_8000AEC8(unsigned int a, unsigned int b);
extern "C" unsigned int lbTime_8000AEE4(unsigned int a, int b);
extern "C" unsigned int lbTime_8000AF74(unsigned int a, int b);
extern "C" int powi(int base, int exponent);
extern "C" int lb_8000D148(float point0X, float point0Y,
                             float point1X, float point1Y,
                             float point2X, float point2Y,
                             float threshold);
extern "C" void M360_HSD_HeapInit(void);
extern "C" void* HSD_MemAlloc(int size);
extern "C" void HSD_Free(void* ptr);
extern "C" int M360_HsdAnimSelfTest(void);
extern "C" int M360_HsdJObjSelfTest(void);
extern "C" int M360_HsdSynthSelfTest(void);
extern "C" int M360_GameplayLayoutProbe(void);
extern "C" unsigned M360_MenuInputSelfTest(void);
extern "C" void gm_EvaluateAllControllerInputs(void);
extern "C" unsigned __int64 gm_GetButtonsTriggered(unsigned char index);

#include "hsd_class_xdk_compat.h"
extern "C" {
#include "class.h"
#include "id.h"
#include "objalloc.h"
#include "object.h"
}

#include "gobj_xdk_compat.h"
extern "C" {
#include "list.h"
#include "gobj.h"
#include "gobjplink.h"
#include "gobjproc.h"
#include "gobjuserdata.h"
}

#include "hsdmath_xdk_compat.h"
extern "C" {
#include "fobj.h"
#include "mtx.h"
#include "quatlib.h"
#include "random.h"
#include "spline.h"
}

namespace {

void SelfTestUserDataRemoveFunc(void* data)
{
    *static_cast<int*>(data) = 1;
}

struct FObjProbe {
    int calls;
    float value;
};

void SelfTestFObjUpdate(void* obj, enum_t type, HSD_ObjData* fval)
{
    FObjProbe* probe = static_cast<FObjProbe*>(obj);
    (void) type;
    probe->calls++;
    probe->value = fval->fv;
}

bool SelfTestNear(float a, float b, float eps)
{
    const float d = a - b;
    return d <= eps && d >= -eps;
}

bool SelfTestMtxNear(Mtx a, Mtx b, float eps)
{
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) {
            if (!SelfTestNear(a[r][c], b[r][c], eps))
                return false;
        }
    }
    return true;
}

bool SelfTestHsdMath()
{
    Vec3 scale = { 2.0f, 3.0f, 0.5f };
    Vec3 rot = { 0.3f, -0.4f, 0.5f };
    Vec3 trans = { 10.0f, -20.0f, 30.0f };
    Mtx m, inv, prod, ident;
    HSD_MtxSRT(m, &scale, &rot, &trans, NULL);
    HSD_MtxInverse(m, inv);
    PSMTXConcat(m, inv, prod);
    PSMTXIdentity(ident);
    const bool mtxOk = SelfTestMtxNear(prod, ident, 1e-3f);

    Vec3 back;
    HSD_MtxGetRotation(m, &back);
    const bool rotOk = SelfTestNear(back.x, rot.x, 1e-4f) &&
                       SelfTestNear(back.y, rot.y, 1e-4f) &&
                       SelfTestNear(back.z, rot.z, 1e-4f);

    Quaternion q, qm;
    Mtx viaQuat, viaEuler;
    EulerToQuat(&rot, &q);
    HSD_MtxQuat(viaQuat, &q);
    HSD_MkRotationMtx(viaEuler, &rot);
    MatToQuat(viaQuat, &qm);
    const bool quatOk = SelfTestMtxNear(viaQuat, viaEuler, 1e-4f) &&
                        SelfTestNear(qm.w, q.w, 1e-4f) &&
                        SelfTestNear(qm.x, q.x, 1e-4f);

    Quaternion identQ = { 0.0f, 0.0f, 0.0f, 1.0f };
    Quaternion quarterQ = { 0.0f, 0.0f, 0.70710678f, 0.70710678f };
    Quaternion mid;
    HSD_QuatLib_8037EF28(&identQ, &quarterQ, &mid, 0.5f);
    const bool slerpOk = SelfTestNear(mid.z, 0.38268343f, 1e-4f) &&
                         SelfTestNear(mid.w, 0.92387953f, 1e-4f);

    static DiscVec3 cv[4];
    cv[0].x = 0.0f; cv[0].y = 0.0f; cv[0].z = 0.0f;
    cv[1].x = 2.0f; cv[1].y = 4.0f; cv[1].z = 6.0f;
    cv[2].x = 10.0f; cv[2].y = 4.0f; cv[2].z = -6.0f;
    cv[3].x = 10.0f; cv[3].y = 4.0f; cv[3].z = -6.0f;
    HSD_Spline spline;
    spline.type = 0;
    spline.numcv = 3;
    spline.tension = 0.0f;
    spline.cv = cv;
    spline.totalLength = 0.0f;
    spline.segLength = NULL;
    spline.segPoly = NULL;
    Vec3 point;
    splGetSplinePoint(&point, &spline, 0.25f);
    const bool splineOk = SelfTestNear(point.x, 1.0f, 1e-4f) &&
                          SelfTestNear(point.y, 2.0f, 1e-4f) &&
                          SelfTestNear(point.z, 3.0f, 1e-4f) &&
                          SelfTestNear(splGetHelmite(0.1f, 5.0f, 0.0f, 10.0f,
                                                     0.0f, 0.0f),
                                       5.0f, 1e-4f);

    static unsigned char stream[10] = { 0x12, 0x00, 0x00, 0x00, 0x00, 10,
                                        0x00, 0x00, 0xC8, 0x42 };
    static HSD_FObjDesc desc;
    desc.next = NULL;
    desc.length = sizeof(stream);
    desc.startframe = 0.0f;
    desc.type = 12;
    desc.frac_value = HSD_A_FRAC_FLOAT;
    desc.frac_slope = HSD_A_FRAC_FLOAT;
    desc.dummy0 = 0;
    desc.ad = stream;
    HSD_FObjInitAllocData();
    HSD_FObj* fobj = HSD_FObjLoadDesc(&desc);
    bool fobjOk = false;
    if (fobj != NULL) {
        FObjProbe probe = { 0, 0.0f };
        HSD_FObjReqAnimAll(fobj, 0.0f);
        HSD_FObjInterpretAnim(fobj, &probe, SelfTestFObjUpdate, 0.0f);
        const bool startOk = probe.calls == 1 && SelfTestNear(probe.value, 0.0f, 1e-4f);
        for (int i = 0; i < 5; ++i)
            HSD_FObjInterpretAnim(fobj, &probe, SelfTestFObjUpdate, 1.0f);
        fobjOk = startOk && SelfTestNear(probe.value, 50.0f, 1e-3f);
        HSD_FObjRemoveAll(fobj);
    }

    *HSD_RandSeedPtr = 1;
    const int rand0 = HSD_Rand();
    const int rand1 = HSD_Rand();
    const bool randOk = rand0 == 41 && rand1 == 51235;

    return mtxOk && rotOk && quatOk && slerpOk && splineOk && fobjOk && randOk;
}

const unsigned kMaxRects = 4096;
const float kFloorY = 574.0f;
const float kPlayerWidth = 38.0f;
const float kPlayerHeight = 56.0f;

struct RectBatch {
    D3DRECT rects[kMaxRects];
    unsigned count;
    D3DCOLOR color;
};

struct GameState {
    float playerX;
    float playerY;
    float velocityX;
    float velocityY;
    bool grounded;
    bool facingRight;
    unsigned damage;
    DWORD attackUntil;
    DWORD hitUntil;
};

RectBatch g_green = { {}, 0, D3DCOLOR_XRGB(107, 232, 52) };
RectBatch g_cyan = { {}, 0, D3DCOLOR_XRGB(79, 203, 247) };
RectBatch g_white = { {}, 0, D3DCOLOR_XRGB(238, 244, 252) };
RectBatch g_muted = { {}, 0, D3DCOLOR_XRGB(139, 158, 181) };
RectBatch g_panel = { {}, 0, D3DCOLOR_XRGB(24, 39, 61) };
RectBatch g_dynamic = { {}, 0, D3DCOLOR_XRGB(238, 244, 252) };
SpriteRenderer g_renderer;

void TraceStage(const char* stage, unsigned value)
{
    HANDLE file = CreateFileA("game:\\runtime-trace.txt", GENERIC_WRITE,
        FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE)
        return;
    SetFilePointer(file, 0, 0, FILE_END);
    char line[160];
    const int length = _snprintf(line, sizeof(line), "%s: %u\r\n", stage, value);
    DWORD written;
    if (length > 0 && length < sizeof(line))
        WriteFile(file, line, length, &written, 0);
    CloseHandle(file);
}

void AddRect(RectBatch& batch, LONG x, LONG y, LONG width, LONG height)
{
    if (batch.count >= kMaxRects || width <= 0 || height <= 0)
        return;
    D3DRECT& rect = batch.rects[batch.count++];
    rect.x1 = x;
    rect.y1 = y;
    rect.x2 = x + width;
    rect.y2 = y + height;
}

void AddOutline(RectBatch& batch, LONG x, LONG y, LONG width, LONG height,
                LONG thickness)
{
    AddRect(batch, x, y, width, thickness);
    AddRect(batch, x, y + height - thickness, width, thickness);
    AddRect(batch, x, y, thickness, height);
    AddRect(batch, x + width - thickness, y, thickness, height);
}

BYTE GlyphRow(char character, unsigned row)
{
    static const BYTE letters[26][7] = {
        {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
        {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30},
        {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
        {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17},
        {31,4,4,4,4,4,31},      {7,2,2,2,18,18,12},
        {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17},
        {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
        {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30},   {31,4,4,4,4,4,4},
        {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
        {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4},     {31,1,2,4,8,16,31}
    };
    static const BYTE digits[10][7] = {
        {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},
        {14,17,1,2,4,8,31},     {30,1,1,14,1,1,30},
        {2,6,10,18,31,2,2},     {31,16,16,30,1,1,30},
        {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8},
        {14,17,17,14,17,17,14}, {14,17,17,15,1,1,14}
    };

    if (row >= 7)
        return 0;
    if (character >= 'a' && character <= 'z')
        character = static_cast<char>(character - 'a' + 'A');
    if (character >= 'A' && character <= 'Z')
        return letters[character - 'A'][row];
    if (character >= '0' && character <= '9')
        return digits[character - '0'][row];
    if (character == '-')
        return row == 3 ? 31 : 0;
    if (character == '_')
        return row == 6 ? 31 : 0;
    if (character == ':')
        return (row == 2 || row == 5) ? 4 : 0;
    if (character == '.')
        return row == 6 ? 4 : 0;
    if (character == '/')
        return static_cast<BYTE>(1u << (row < 5 ? row : 4));
    return 0;
}

void AddText(RectBatch& batch, LONG x, LONG y, const char* text, LONG scale)
{
    const LONG originX = x;
    while (*text) {
        const char character = *text++;
        if (character == '\n') {
            x = originX;
            y += 9 * scale;
            continue;
        }
        for (unsigned row = 0; row < 7; ++row) {
            const BYTE bits = GlyphRow(character, row);
            for (unsigned column = 0; column < 5; ++column) {
                if (bits & (1u << (4u - column)))
                    AddRect(batch, x + static_cast<LONG>(column) * scale,
                            y + static_cast<LONG>(row) * scale, scale, scale);
            }
        }
        x += 6 * scale;
    }
}

void AddNumber(RectBatch& batch, LONG x, LONG y, unsigned value, LONG scale)
{
    char text[4];
    text[3] = '\0';
    text[2] = static_cast<char>('0' + value % 10);
    text[1] = value >= 10 ? static_cast<char>('0' + (value / 10) % 10) : ' ';
    text[0] = value >= 100 ? static_cast<char>('0' + (value / 100) % 10) : ' ';
    AddText(batch, x, y, text, scale);
}

void AddUnsigned(RectBatch& batch, LONG x, LONG y, unsigned value, LONG scale)
{
    char digits[11];
    char text[11];
    unsigned count = 0;
    do {
        digits[count++] = static_cast<char>('0' + value % 10);
        value /= 10;
    } while (value && count < 10);
    for (unsigned i = 0; i < count; ++i)
        text[i] = digits[count - i - 1];
    text[count] = '\0';
    AddText(batch, x, y, text, scale);
}

void BuildScene(bool meleeCodePassed, const MeleeBootStatus& boot)
{
    AddRect(g_green, 0, 86, 1280, 4);
    AddOutline(g_panel, 62, 125, 1156, 500, 3);
    AddRect(g_cyan, 62, 125, 7, 500);
    AddRect(g_panel, 95, 270, 1090, 2);

    AddText(g_green, 62, 25, "MELEE360", 6);
    AddText(g_white, 430, 38, "GALE01 NATIVE BOOT", 3);

    AddText(g_cyan, 96, 146, "DISC", 2);
    AddText(boot.discValid ? g_green : g_white, 220, 146,
            boot.discValid ? boot.gameId : "FAILED", 2);
    AddText(g_cyan, 390, 146, "FST", 2);
    AddText(boot.fstMounted ? g_green : g_white, 480, 146,
            boot.fstMounted ? "MOUNTED" : "FAILED", 2);
    if (boot.fstMounted)
        AddUnsigned(g_muted, 650, 146, boot.entryCount, 2);
    AddText(g_muted, 755, 146, "ENTRIES", 2);

    AddText(g_cyan, 96, 188, "OPENING BANNER", 2);
    AddText(boot.bannerDecoded ? g_green : g_white, 350, 188,
            boot.bannerDecoded ? "DECODED" : "FAILED", 2);
    AddText(g_cyan, 600, 188,
            boot.languageUS ? "GMTTALL.USD" : "GMTTALL.DAT", 2);
    AddText(boot.titleArchiveValid ? g_green : g_white, 835, 188,
            boot.titleArchiveRelocated ? "RELOCATED" : "FAILED", 2);

    AddText(g_cyan, 96, 230, "MELEE MODULES", 2);
    AddText(meleeCodePassed ? g_green : g_white, 350, 230,
            meleeCodePassed ? "LINKED" : "FAILED", 2);
    AddText(g_muted, 600, 230, "PUBLIC ROOT", 2);
    AddText(g_white, 835, 230,
            boot.titleArchiveValid ? boot.firstPublicSymbol : "UNAVAILABLE", 1);

    AddText(g_muted, 96, 528, "TITLE", 2);
    AddText(g_white, 220, 528,
            boot.title[0] ? boot.title : "SUPER SMASH BROS. MELEE", 2);
    AddText(g_muted, 96, 562, "TITLE SYMBOLS", 2);
    AddUnsigned(boot.titleScene.symbolsResolved ? g_green : g_white,
                310, 562, boot.titleScene.resolvedSymbols, 2);
    AddText(g_muted, 350, 562, "/ 12", 2);
    AddText(g_muted, 440, 562, "MODELS", 2);
    AddUnsigned(boot.titleScene.modelsLoaded ? g_green : g_white,
                580, 562, boot.titleScene.modelCount, 2);
    AddText(g_muted, 650, 562, "JOINTS", 2);
    AddUnsigned(boot.titleScene.modelsLoaded ? g_green : g_white,
                790, 562, boot.titleScene.jointCount, 2);
    AddText(g_muted, 96, 592, "MATERIALS", 2);
    AddUnsigned(boot.titleScene.modelsLoaded ? g_green : g_white,
                270, 592, boot.titleScene.materialCount, 2);
    AddText(g_muted, 390, 592, "POBJS", 2);
    AddUnsigned(boot.titleScene.modelsLoaded ? g_green : g_white,
                500, 592, boot.titleScene.polygonObjectCount, 2);
    AddText(g_muted, 650, 592, "TEXTURES", 2);
    AddUnsigned(boot.titleScene.modelsLoaded ? g_green : g_white,
                810, 592, boot.titleScene.textureImageCount, 2);
    AddText(g_muted, 875, 592, "FMT", 1);
    AddUnsigned(g_white, 910, 592, boot.titleScene.firstTextureFormat, 1);
    AddUnsigned(g_white, 950, 592, boot.titleScene.firstTextureWidth, 1);
    AddText(g_muted, 1015, 592, "X", 1);
    AddUnsigned(g_white, 1030, 592, boot.titleScene.firstTextureHeight, 1);
    AddText(g_muted, 1080, 592, "TRIS", 1);
    AddUnsigned(boot.titleScene.meshVertexCount ? g_green : g_white,
                1140, 592, boot.titleScene.meshVertexCount / 3, 1);
    AddText(g_muted, 96, 616,
            boot.titleScene.animationsBound
                ? "REAL JOBJ MOBJ POBJ TOBJ GRAPHS LOADED"
                : (boot.titleArchiveValid ? "HAL ROOT READY" : boot.error),
            1);
    AddText(g_muted, 76, 672,
            "NATIVE POWERPC / GAMECUBE FST / RGB5A3 / HAL ARCHIVE", 2);
    AddText(g_muted, 1010, 616, "BACK HIDES HUD", 1);
}

SpriteColor ToSpriteColor(D3DCOLOR color, float alpha = 1.0f)
{
    SpriteColor result = {
        static_cast<float>((color >> 16) & 255) / 255.0f,
        static_cast<float>((color >> 8) & 255) / 255.0f,
        static_cast<float>(color & 255) / 255.0f,
        alpha
    };
    return result;
}

void RenderBatch(SpriteRenderer& renderer, const RectBatch& batch)
{
    const SpriteColor color = ToSpriteColor(batch.color);
    for (unsigned i = 0; i < batch.count; ++i) {
        const D3DRECT& rect = batch.rects[i];
        renderer.AddQuad(static_cast<float>(rect.x1),
                         static_cast<float>(rect.y1),
                         static_cast<float>(rect.x2 - rect.x1),
                         static_cast<float>(rect.y2 - rect.y1), color);
    }
}

void DrawRect(SpriteRenderer& renderer, LONG x, LONG y, LONG width,
              LONG height, D3DCOLOR color, float alpha = 1.0f)
{
    renderer.AddQuad(static_cast<float>(x), static_cast<float>(y),
                     static_cast<float>(width), static_cast<float>(height),
                     ToSpriteColor(color, alpha));
}

void RenderHudBackdrop(SpriteRenderer& renderer)
{
    const SpriteColor shade = { 0.0f, 0.0f, 0.0f, 0.55f };
    renderer.AddGradientQuad(0.0f, 0.0f, 1280.0f, 100.0f, shade, shade);
    const SpriteColor panelTop = { 0.025f, 0.045f, 0.095f, 0.80f };
    const SpriteColor panelBottom = { 0.008f, 0.018f, 0.045f, 0.85f };
    renderer.AddGradientQuad(62.0f, 125.0f, 1156.0f, 500.0f,
                             panelTop, panelBottom);
    renderer.AddGradientQuad(62.0f, 660.0f, 1156.0f, 40.0f, shade, shade);
}

void BuildHudStatus(const MeleeFlow& flow, const MeleeMovieStatus& movie,
                    const MeleeAudioStatus& audio, unsigned frameUs)
{
    g_dynamic.count = 0;
    AddText(g_dynamic, 96, 300, "FLOW", 2);
    AddText(g_dynamic, 220, 300, M360_FlowStateName(flow.state), 2);
    AddText(g_dynamic, 96, 330, "MOVIE FRAME", 2);
    AddUnsigned(g_dynamic, 270, 330, movie.currentFrame, 2);
    AddText(g_dynamic, 360, 330, "DECODED", 2);
    AddUnsigned(g_dynamic, 470, 330, movie.framesDecoded, 2);
    AddText(g_dynamic, 96, 360, "DECODE US", 2);
    AddUnsigned(g_dynamic, 240, 360, movie.decodeUsAverage, 2);
    AddText(g_dynamic, 360, 360, "FRAME US", 2);
    AddUnsigned(g_dynamic, 470, 360, frameUs, 2);
    AddText(g_dynamic, 96, 390, "AUDIO", 2);
    AddText(g_dynamic, 190, 390, audio.playing ? audio.track : "SILENT", 2);
}

void RenderMenuPlaceholder(SpriteRenderer& renderer, const MeleeFlow& flow)
{
    const SpriteColor top = { 0.02f, 0.03f, 0.10f, 1.0f };
    const SpriteColor bottom = { 0.00f, 0.00f, 0.02f, 1.0f };
    renderer.AddGradientQuad(160.0f, 0.0f, 960.0f, 720.0f, top, bottom);
    g_dynamic.count = 0;
    AddText(g_dynamic, 460, 250, "MAIN MENU", 6);
    AddText(g_dynamic, 322, 360, "NEXT MILESTONE: ORIGINAL MNMAIN SCENE", 2);
    AddText(g_dynamic, 322, 400,
            flow.rulesBgm == 0x36 ? "MUSIC: MENU3.HPS" : "MUSIC: MENU01.HPS", 2);
    AddText(g_dynamic, 322, 480, "B: BACK TO TITLE", 2);
    RenderBatch(renderer, g_dynamic);
}

void RenderMatchHud(SpriteRenderer& renderer, const M360MatchStatus& match)
{
    static const char* const names[2] = { "MARIO P1", "MARIO P2" };
    static const D3DCOLOR colors[2] = { D3DCOLOR_XRGB(240, 70, 60), D3DCOLOR_XRGB(90, 140, 255) };
    DrawRect(renderer, 18, 16, 452, 62, D3DCOLOR_XRGB(0, 0, 0), 0.38f);
    g_dynamic.count = 0;
    g_dynamic.color = D3DCOLOR_XRGB(238, 244, 252);
    AddText(g_dynamic, 30, 22, "MOVE: LEFT STICK / DPAD   A: ATTACK   X/Y: JUMP", 1);
    if (match.campaignRounds) {
        AddText(g_dynamic, 30, 40,
                match.gameMode == 3 ? "CLASSIC" : "ADVENTURE", 1);
        AddText(g_dynamic, 148, 40, "ROUND", 1);
        AddNumber(g_dynamic, 200, 40, match.campaignRound + 1, 1);
        AddText(g_dynamic, 218, 40, "/ 5   A: NEXT   B: MENU", 1);
        AddText(g_dynamic, 30, 56, "START: PAUSE", 1);
    } else {
        AddText(g_dynamic, 30, 40, "START: PAUSE   B: RETURN AFTER MATCH", 1);
    }
    RenderBatch(renderer, g_dynamic);
    for (unsigned i = 0; i < match.fighters && i < 2; ++i) {
        const LONG x = 380 + static_cast<LONG>(i) * 340;
        DrawRect(renderer, x - 12, 606, 250, 96, D3DCOLOR_XRGB(0, 0, 0), 0.45f);
        g_dynamic.count = 0;
        g_dynamic.color = colors[i];
        AddText(g_dynamic, x, 614,
                i == 1 && !match.human[1] ? "MARIO CPU" : names[i], 2);
        RenderBatch(renderer, g_dynamic);
        g_dynamic.count = 0;
        g_dynamic.color = D3DCOLOR_XRGB(238, 244, 252);
        AddUnsigned(g_dynamic, x + 10, 640, match.damage[i], 6);
        AddText(g_dynamic, x + 150, 662, "%", 3);
        AddText(g_dynamic, x + 12, 682, "STOCKS", 1);
        AddUnsigned(g_dynamic, x + 92, 681, match.stocksRemaining[i], 3);
        RenderBatch(renderer, g_dynamic);
    }
    g_dynamic.color = D3DCOLOR_XRGB(238, 244, 252);
    if (match.matchOver) {
        DrawRect(renderer, 160, 0, 960, 720, D3DCOLOR_XRGB(0, 0, 0), 0.48f);
        g_dynamic.count = 0;
        AddText(g_dynamic, 548, 300, match.winner == 0 ? "P1 WINS" : "P2 WINS", 6);
        AddText(g_dynamic, 400, 382,
                match.campaignRounds && match.winner == 0
                    ? "A: NEXT FIGHT   B: MAIN MENU"
                    : "B: RETURN TO MAIN MENU", 2);
        RenderBatch(renderer, g_dynamic);
    }
    if (match.paused) {
        DrawRect(renderer, 160, 0, 960, 720, D3DCOLOR_XRGB(0, 0, 0), 0.35f);
        g_dynamic.count = 0;
        AddText(g_dynamic, 540, 300, "PAUSE", 6);
        AddText(g_dynamic, 420, 380, "START: RESUME   B: MAIN MENU", 2);
        RenderBatch(renderer, g_dynamic);
    }
}

void ResetGame(GameState& game)
{
    game.playerX = 145.0f;
    game.playerY = kFloorY - kPlayerHeight;
    game.velocityX = 0.0f;
    game.velocityY = 0.0f;
    game.grounded = true;
    game.facingRight = true;
    game.damage = 0;
    game.attackUntil = 0;
    game.hitUntil = 0;
}

void UpdateGame(GameState& game, const HSD_PadStatus& input, float elapsed,
                DWORD now)
{
    const M360U32 buttons = input.button;
    const M360U32 pressed = input.trigger;

    if (pressed & HSD_PAD_START)
        ResetGame(game);

    float direction = 0.0f;
    if (buttons & HSD_PAD_DPADLEFT)
        direction = -1.0f;
    else if (buttons & HSD_PAD_DPADRIGHT)
        direction = 1.0f;
    else if (input.nml_stickX < -0.08f || input.nml_stickX > 0.08f)
        direction = input.nml_stickX;

    game.velocityX = direction * 0.36f;
    if (direction > 0.01f)
        game.facingRight = true;
    else if (direction < -0.01f)
        game.facingRight = false;
    if ((pressed & HSD_PAD_A) && game.grounded) {
        game.velocityY = -0.72f;
        game.grounded = false;
    }

    game.velocityY += 0.00175f * elapsed;
    game.playerX += game.velocityX * elapsed;
    game.playerY += game.velocityY * elapsed;

    if (game.playerX < 96.0f)
        game.playerX = 96.0f;
    if (game.playerX > 1135.0f)
        game.playerX = 1135.0f;
    if (game.playerY + kPlayerHeight >= kFloorY) {
        game.playerY = kFloorY - kPlayerHeight;
        game.velocityY = 0.0f;
        game.grounded = true;
    }

    const float attackStartX = game.playerX + kPlayerWidth * 0.5f;
    const float attackY = game.playerY + 30.0f;
    const int attackHit = lb_8000D148(
        attackStartX, attackY, attackStartX + 88.0f, attackY,
        873.0f, 540.0f, 38.0f);
    if (pressed & HSD_PAD_X) {
        game.attackUntil = now + 160;
        if (attackHit) {
            game.damage = lbTime_8000AF74(game.damage, 8);
            game.hitUntil = now + 180;
        }
    }
}

void RenderGame(SpriteRenderer& renderer, const GameState& game, DWORD now)
{
    const LONG x = static_cast<LONG>(game.playerX);
    const LONG y = static_cast<LONG>(game.playerY);
    DrawRect(renderer, x - 8, 568, 54, 7, D3DCOLOR_XRGB(0, 0, 0), 0.40f);
    const SpriteColor white = { 1.0f, 1.0f, 1.0f, 1.0f };
    float frameX = 8.0f;
    float frameY = 0.0f;
    if (now < game.attackUntil) {
        frameX = 176.0f;
        frameY = 64.0f;
    } else if (!game.grounded) {
        frameX = 120.0f;
        frameY = 64.0f;
    } else if (game.velocityX < -0.01f || game.velocityX > 0.01f) {
        frameX = 64.0f;
        frameY = 64.0f;
    }
    float playerU0 = frameX / 256.0f;
    float playerU1 = (frameX + 48.0f) / 256.0f;
    if (!game.facingRight) {
        const float swap = playerU0;
        playerU0 = playerU1;
        playerU1 = swap;
    }
    renderer.AddSprite(static_cast<float>(x - 13), static_cast<float>(y - 8),
                       65.0f, 64.0f, playerU0, frameY / 128.0f,
                       playerU1, (frameY + 64.0f) / 128.0f, white);

    const float damageTint = static_cast<float>(game.damage) / 637.5f;
    const SpriteColor targetTint = {
        1.0f, 1.0f - damageTint, 1.0f - damageTint, 1.0f
    };
    DrawRect(renderer, 840, 568, 66, 7, D3DCOLOR_XRGB(0, 0, 0), 0.40f);
    renderer.AddSprite(849.0f, 510.0f, 48.0f, 64.0f,
                       68.0f / 256.0f, 0.0f,
                       116.0f / 256.0f, 64.0f / 128.0f, targetTint);

    if (now < game.attackUntil)
        DrawRect(renderer, x + 38, y + 22, 65, 18,
                 D3DCOLOR_XRGB(255, 224, 94), 0.82f);

    if (now < game.hitUntil) {
        const float progress = 1.0f -
            static_cast<float>(game.hitUntil - now) / 180.0f;
        static const int directions[8][2] = {
            {-34,-22}, {-14,-42}, {12,-38}, {36,-18},
            {-38,10}, {-18,31}, {16,34}, {40,12}
        };
        for (unsigned i = 0; i < 8; ++i) {
            const LONG sparkX = 873 + static_cast<LONG>(directions[i][0] * progress);
            const LONG sparkY = 535 + static_cast<LONG>(directions[i][1] * progress);
            DrawRect(renderer, sparkX, sparkY, 7, 7,
                     (i & 1) ? D3DCOLOR_XRGB(255, 91, 146)
                             : D3DCOLOR_XRGB(255, 230, 83),
                     1.0f - progress * 0.7f);
        }
    }

    g_dynamic.count = 0;
    AddNumber(g_dynamic, 1004, 326, game.damage, 5);
    RenderBatch(renderer, g_dynamic);
}

} // namespace

void M360_Trace(const char* stage, unsigned value)
{
    TraceStage(stage, value);
}

void __cdecl main()
{
    OutputDebugStringA("[M360][XEX] starting native Melee boot pipeline\n");

    M360_HSD_HeapInit();

    bool memoryPassed = false;
    void* memoryProbe = HSD_MemAlloc(256);
    if (memoryProbe) {
        unsigned char* probeBytes = static_cast<unsigned char*>(memoryProbe);
        for (int i = 0; i < 256; ++i)
            probeBytes[i] = 0x5A;
        memoryPassed = probeBytes[255] == 0x5A;
        HSD_Free(memoryProbe);
    }

    bool hsdClassPassed = false;
    {
        void* obj = hsdNew(&hsdObj);
        if (obj != NULL && HSD_CLASS_METHOD(obj) == &hsdObj &&
            hsdIsDescendantOf(&hsdObj, &hsdClass))
        {
            hsdDelete(obj);

            static HSD_ObjAllocData s_selfTestPool;
            HSD_ObjAllocInit(&s_selfTestPool, sizeof(IDEntry), 4);
            void* slot = HSD_ObjAlloc(&s_selfTestPool);
            if (slot != NULL) {
                HSD_ObjFree(&s_selfTestPool, slot);

                HSD_IDSetup();
                HSD_IDInitAllocData();
                int probeValue = 0x360;
                HSD_IDInsertToTable(NULL, 42, &probeValue);
                s32 idSuccess = 0;
                void* idData = HSD_IDGetDataFromTable(NULL, 42, &idSuccess);
                hsdClassPassed = idSuccess && idData == &probeValue;
                HSD_IDRemoveByIDFromTable(NULL, 42);
            }
        }
    }

    bool gobjPassed = false;
    {
        HSD_ListInitAllocData();

        HSD_GObjLibInitDataType gobjInit;
        HSD_GObjSetInitDefaults(&gobjInit);
        gobjInit.gproc_pri_max = 0x18;
        HSD_GObjInit(&gobjInit);

        HSD_GObj* gobj = GObj_Create(1, 0, 0);
        if (gobj != NULL) {
            int userDataRemoved = 0;
            GObj_InitUserData(gobj, 0, SelfTestUserDataRemoveFunc,
                              &userDataRemoved);

            const bool linkedAtHead = HSD_GObjPLinkHead[0] == gobj;
            const bool userDataVisible =
                HSD_GObjGetUserData(gobj) == &userDataRemoved;

            HSD_GObjFree(gobj);

            gobjPassed = linkedAtHead && userDataVisible &&
                         userDataRemoved == 1 && HSD_GObjPLinkHead[0] == NULL;
        }
    }

    const bool hsdMathPassed = SelfTestHsdMath();
    OutputDebugStringA(hsdMathPassed
                           ? "[M360][MATH] HSD math self-test passed\n"
                           : "[M360][MATH] HSD math self-test FAILED\n");

    const bool hsdAnimPassed = M360_HsdAnimSelfTest() != 0;
    OutputDebugStringA(hsdAnimPassed
                           ? "[M360][ANIM] HSD anim self-test passed\n"
                           : "[M360][ANIM] HSD anim self-test FAILED\n");

    const bool hsdJObjPassed = M360_HsdJObjSelfTest() != 0;
    OutputDebugStringA(hsdJObjPassed
                           ? "[M360][JOBJ] HSD scene graph self-test passed\n"
                           : "[M360][JOBJ] HSD scene graph self-test FAILED\n");

    const bool hsdSynthPassed = M360_HsdSynthSelfTest() != 0;
    OutputDebugStringA(hsdSynthPassed
                           ? "[M360][SYNTH] HSD synth/devcom self-test passed\n"
                           : "[M360][SYNTH] HSD synth/devcom self-test FAILED\n");

    const bool meleeCodePassed =
        lbTime_8000AEC8(0xfffffff0u, 0x20u) == 0xffffffffu &&
        lbTime_8000AEE4(4u, -10) == 0u &&
        lbTime_8000AF74(250u, 8) == 255u &&
        powi(3, 4) == 81 &&
        lb_8000D148(0.0f, 0.0f, 10.0f, 0.0f, 0.0f, 0.0f, 5.0f) == 1 &&
        lb_8000D148(0.0f, 0.0f, 10.0f, 0.0f, 50.0f, 50.0f, 1.0f) == 0 &&
        memoryPassed && hsdClassPassed && gobjPassed && hsdMathPassed &&
        hsdAnimPassed && hsdJObjPassed && hsdSynthPassed;

    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d)
        return;

    D3DPRESENT_PARAMETERS present;
    ZeroMemory(&present, sizeof(present));
    present.BackBufferWidth = 1280;
    present.BackBufferHeight = 720;
    present.BackBufferFormat = D3DFMT_A8R8G8B8;
    present.FrontBufferFormat = D3DFMT_LE_X8R8G8B8;
    present.BackBufferCount = 1;
    present.MultiSampleType = D3DMULTISAMPLE_NONE;
    present.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    present.EnableAutoDepthStencil = TRUE;
    present.AutoDepthStencilFormat = D3DFMT_D24S8;

    IDirect3DDevice9* device = 0;
    HRESULT result = d3d->CreateDevice(0, D3DDEVTYPE_HAL, 0,
        D3DCREATE_HARDWARE_VERTEXPROCESSING, &present, &device);
    if (FAILED(result) || !device) {
        d3d->Release();
        return;
    }

    SpriteRenderer& renderer = g_renderer;
    if (!renderer.Initialize(device)) {
        OutputDebugStringA("[M360][XEX] sprite renderer initialization failed\n");
        device->Release();
        d3d->Release();
        return;
    }

    const bool hsdRenderReady = M360_HsdRenderInit(device);
    TraceStage("hsd.render.ready", hsdRenderReady);

    MeleeBootStatus boot;
    TraceStage("boot.begin", GetTickCount());
    TraceStage("gameplay.layout", M360_GameplayLayoutProbe());
    const bool bootSucceeded = M360_BootMelee("game:\\melee.iso", &boot);
    TraceStage("boot.complete", bootSucceeded);
    if (boot.bannerDecoded)
        renderer.UploadBanner(device, boot.bannerPixels);
    unsigned* titleTexturePixels = 0;
    unsigned titleTextureWidth = 0;
    unsigned titleTextureHeight = 0;
    const bool titleTextureDecoded = M360_DecodeFirstTitleTexture(
        &titleTexturePixels, &titleTextureWidth, &titleTextureHeight);
    if (titleTextureDecoded) {
        renderer.UploadGameTexture(device, titleTexturePixels,
                                   titleTextureWidth, titleTextureHeight);
        M360_FreeDecodedTitleTexture(titleTexturePixels);
    }
    M360HsdRenderStats renderStats;
    M360_HsdRenderBeginFrame();
    M360_HsdRenderAllowErase(false);
    M360_TitleRender();
    M360_HsdRenderGetStats(&renderStats);
    M360_HsdRenderEndFrame();
    device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    boot.titleScene.meshVertexCount = renderStats.triangles * 3;
    unsigned titleMeshVertexCount = boot.titleScene.meshVertexCount;
    TraceStage("mesh.vertices", boot.titleScene.meshVertexCount);
    BuildScene(meleeCodePassed, boot);
    M360_HSDPadInit();
    TraceStage("menu.input.tests", M360_MenuInputSelfTest());
    OutputDebugStringA(bootSucceeded ? "[M360][BOOT] GALE01 data ready\n"
                                     : "[M360][BOOT] GALE01 boot failed\n");

    MeleeAudioStatus audio;
    const bool audioReady = M360_AudioInit("game:\\melee.iso", &audio);
    TraceStage("audio.xaudio2_create.hr", static_cast<unsigned>(audio.createResult));
    TraceStage("audio.mastering_voice.hr", static_cast<unsigned>(audio.masterResult));
    TraceStage("audio.ready", audioReady);
    const bool movieReady = M360_MovieInit(device, "game:\\melee.iso");
    TraceStage("movie.init", movieReady);

    MeleeFlow flow;
    TraceStage("menu.archive.found", boot.menuArchiveFound);
    TraceStage("menu.archive.bytes", boot.menuArchiveSize);
    TraceStage("menu.archive.symbols", boot.menuSymbolsResolved);
    TraceStage("menu.archive.ready", boot.menuArchiveValid);
    M360_FlowSetMenuAvailable(boot.menuArchiveValid && hsdRenderReady);
    M360_FlowStart(&flow, &audio);
    TraceStage("audio.file.found", audio.fileFound);
    TraceStage("audio.file.size", audio.fileSize);
    TraceStage("audio.sample_rate", audio.sampleRate);
    TraceStage("audio.channels", audio.channels);
    TraceStage("audio.source_voice.hr", static_cast<unsigned>(audio.sourceResult));
    TraceStage("audio.start.hr", static_cast<unsigned>(audio.startResult));
    TraceStage("audio.playing", audio.playing);

    bool hudVisible = false;
    WORD previousPadButtons = 0;
    unsigned frameCount = 0;
    unsigned frameUs = 0;
    unsigned frameUsMax = 0;
    unsigned __int64 frameUsTotal = 0;
    unsigned framesOver20ms = 0;
    unsigned __int64 workUsWindow = 0;
    unsigned workUsMax = 0;
    unsigned workFrames = 0;
    LARGE_INTEGER frequency;
    LARGE_INTEGER previousFrame;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&previousFrame);
    for (;;) {
        LARGE_INTEGER workStart;
        QueryPerformanceCounter(&workStart);
        M360_AudioUpdate(&audio);
        HSD_PadRenewStatus();
        gm_EvaluateAllControllerInputs();
        WORD padButtons = 0;
        for (DWORD user = 0; user < XUSER_MAX_COUNT; ++user) {
            XINPUT_STATE pad;
            ZeroMemory(&pad, sizeof(pad));
            if (XInputGetState(user, &pad) == ERROR_SUCCESS)
                padButtons |= pad.Gamepad.wButtons;
        }
        if (padButtons != previousPadButtons) {
            TraceStage("input.xinput.frame", frameCount);
            TraceStage("input.xinput.buttons", padButtons);
        }
        const bool backPressed = (padButtons & XINPUT_GAMEPAD_BACK) &&
                                 !(previousPadButtons & XINPUT_GAMEPAD_BACK);
        previousPadButtons = padButtons;
        if (backPressed) {
            hudVisible = !hudVisible;
            TraceStage("input.hud", hudVisible);
        }
        const unsigned __int64 triggered = gm_GetButtonsTriggered(4);
        if (triggered) {
            TraceStage("input.triggered.frame", frameCount);
            TraceStage("input.triggered.buttons", static_cast<unsigned>(triggered));
            TraceStage("input.triggered.menu", static_cast<unsigned>(triggered >> 32));
        }
        M360_FlowUpdate(&flow, triggered, &audio);

        const bool menuScene = flow.state == kFlowMainMenu && M360_FlowMenuActive();
        const bool matchScene = flow.state == kFlowMatch;
        device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                      D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
        renderer.Begin();
        if (flow.state == kFlowMainMenu && !menuScene)
            RenderMenuPlaceholder(renderer, flow);
        else if (!menuScene && !matchScene)
            renderer.AddQuad(160.0f, 0.0f, 960.0f, 720.0f,
                             ToSpriteColor(M360_TitleClearColor()));
        renderer.End(device);
        if (flow.movieVisible)
            M360_MovieDraw(device);
        M360_HsdRenderBeginFrame();
        if (menuScene) {
            M360_HsdRenderAllowErase(true);
            M360_MenuSceneRender();
        } else if (matchScene) {
            M360_HsdRenderAllowErase(true);
            M360_MatchRender();
        } else if (flow.titleVisible) {
            M360_HsdRenderAllowErase(false);
            M360_TitleRender();
        }
        M360_HsdRenderGetStats(&renderStats);
        M360_HsdRenderEndFrame();
        titleMeshVertexCount = renderStats.triangles * 3;
        MeleeMovieStatus movie;
        M360_MovieGetStatus(&movie);
        M360MatchStatus match;
        M360_MatchGetStatus(&match);
        if (matchScene) {
            renderer.Begin();
            RenderMatchHud(renderer, match);
            renderer.End(device);
        }
        if (hudVisible) {
            renderer.Begin();
            RenderHudBackdrop(renderer);
            RenderBatch(renderer, g_panel);
            RenderBatch(renderer, g_cyan);
            RenderBatch(renderer, g_white);
            RenderBatch(renderer, g_muted);
            RenderBatch(renderer, g_green);
            BuildHudStatus(flow, movie, audio, frameUs);
            RenderBatch(renderer, g_dynamic);
            if (boot.bannerDecoded)
                renderer.AddBanner(720.0f, 330.0f, 384.0f, 128.0f);
            renderer.End(device);
        }
        LARGE_INTEGER workEnd;
        QueryPerformanceCounter(&workEnd);
        const unsigned workUs = static_cast<unsigned>(
            (workEnd.QuadPart - workStart.QuadPart) * 1000000 / frequency.QuadPart);
        workUsWindow += workUs;
        ++workFrames;
        if (workUs > workUsMax)
            workUsMax = workUs;
        const HRESULT presented = device->Present(0, 0, 0, 0);
        ++frameCount;

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        frameUs = static_cast<unsigned>((now.QuadPart - previousFrame.QuadPart) *
                                        1000000 / frequency.QuadPart);
        previousFrame = now;
        if (frameCount > 1) {
            frameUsTotal += frameUs;
            if (frameUs > frameUsMax)
                frameUsMax = frameUs;
            if (frameUs > 20000)
                ++framesOver20ms;
        }
        if (frameCount == 1 || frameCount == 120) {
            if (flow.state == kFlowMainMenu)
                TraceStage("menu.mesh.frame_vertices", titleMeshVertexCount);
            TraceStage("present.frame", frameCount);
            TraceStage("present.result", static_cast<unsigned>(presented));
            TraceStage("present.hud", hudVisible);
            TraceStage("mesh.vertices", titleMeshVertexCount);
            TraceStage("hsd.draw_calls", renderStats.drawCalls);
            TraceStage("hsd.textures", renderStats.textures);
            TraceStage("hsd.tev_stages", renderStats.tevStages);
            TraceStage("hsd.envelope_vertices", renderStats.envelopeVertices);
            TraceStage("hsd.unsupported", renderStats.unsupported);
            TraceStage("hsd.decode_failures", renderStats.decodeFailures);
        }
        if (frameCount % 300 == 0) {
            TraceStage("loop.frame", frameCount);
            TraceStage("loop.flow_state", static_cast<unsigned>(flow.state));
            TraceStage("loop.scene_tick", flow.sceneTick);
            TraceStage("loop.work_us_avg", static_cast<unsigned>(workUsWindow / workFrames));
            TraceStage("loop.work_us_max", workUsMax);
            workUsWindow = 0;
            workUsMax = 0;
            workFrames = 0;
            TraceStage("loop.frame_us_avg",
                       static_cast<unsigned>(frameUsTotal / (frameCount - 1)));
            TraceStage("loop.frame_us_max", frameUsMax);
            TraceStage("loop.frames_over_20ms", framesOver20ms);
            TraceStage("loop.mesh_vertices", titleMeshVertexCount);
            TraceStage("loop.hsd_draw_calls", renderStats.drawCalls);
            TraceStage("loop.audio.sfx_submitted", M360_AudioSfxSubmitted());
            TraceStage("loop.audio.sfx_misses", M360_AudioSfxMisses());
            if (matchScene) {
                TraceStage("loop.match_frame", match.frame);
                TraceStage("loop.match_motion_p1", match.motion[0]);
                TraceStage("loop.match_x_p1", static_cast<unsigned>(static_cast<int>(match.posX[0])));
                TraceStage("loop.match_y_p1", static_cast<unsigned>(static_cast<int>(match.posY[0])));
                TraceStage("loop.match_input_buttons", match.inputButtons);
                TraceStage("loop.match_input_triggered", match.inputTriggered);
                TraceStage("loop.match_stick_x_milli", static_cast<unsigned>(static_cast<int>(match.inputX * 1000.0f)));
                TraceStage("loop.match_stick_y_milli", static_cast<unsigned>(static_cast<int>(match.inputY * 1000.0f)));
                TraceStage("loop.match_motion_p2", match.motion[1]);
                TraceStage("loop.match_x_p2", static_cast<unsigned>(static_cast<int>(match.posX[1])));
                TraceStage("loop.match_y_p2", static_cast<unsigned>(static_cast<int>(match.posY[1])));
                TraceStage("loop.match_damage_p2", match.damage[1]);
                TraceStage("loop.match_hits", match.hits);
            }
            if (flow.state == kFlowMainMenu) {
                TraceStage("loop.menu_kind", flow.menuKind);
                TraceStage("loop.menu_selection", flow.menuSelection);
            }
            if (flow.state == kFlowOpening) {
                TraceStage("movie.frame", movie.currentFrame);
                TraceStage("movie.visible", flow.movieVisible);
                TraceStage("movie.decoded", movie.framesDecoded);
                TraceStage("movie.presented", movie.framesPresented);
                TraceStage("movie.late_updates", movie.framesLate);
                TraceStage("movie.skipped", movie.framesSkipped);
                TraceStage("movie.errors", movie.decodeErrors);
                TraceStage("movie.decode_us_last", movie.decodeUsLast);
                TraceStage("movie.decode_us_avg", movie.decodeUsAverage);
                TraceStage("movie.decode_us_max", movie.decodeUsMax);
                TraceStage("movie.upload_us_last", movie.uploadUsLast);
            }
            TraceStage("audio.playing", audio.playing);
            TraceStage("audio.samples_played", audio.samplesPlayed);
            TraceStage("audio.buffers_submitted", audio.buffersSubmitted);
            TraceStage("audio.history_mismatches", audio.historyMismatches);
        }
    }
}
