#include <math.h>
#include <string.h>

#pragma warning(push, 3)
#pragma warning(disable : 4244)
#include <melee/gr/types.h>
#include <melee/mp/types.h>
#include <melee/lb/lbspdisplay.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/displayfunc.h>
#include <sysdolphin/baselib/fog.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/wobj.h>
#pragma warning(pop)

#include "match_xdk.h"

typedef union AnimFn {
    void (*fn)(void);
    void* ptr;
} AnimFn;

static void* AnimCallback(void (*fn)(void))
{
    AnimFn u;
    u.fn = fn;
    return u.ptr;
}

void HSD_GObj_JObjCallback(HSD_GObj* gobj, int arg1);
void HSD_GObj_LObjCallback(HSD_GObj* gobj, int unused);
void HSD_GObj_80390ED0(HSD_GObj* gobj, u32 mask);
void HSD_GObj_80390FC0(void);
void lb_8000B1CC(HSD_JObj* jobj, Vec3* offset, Vec3* out);
void HSD_GObj_RunProcs(void);
int M360_InputScriptHolding(void);

enum {
    kLinkLight = 0,
    kLinkStage = 1,
    kLinkFighter = 2,
    kMaxMapGObjs = 8,
    kMaxFighters = 2,
    kRespawnFrames = 60,
    kStartingStocks = 4,
    kGameModeClassic = 3,
    kGameModeAdventure = 4,
    kCampaignRounds = 5
};

typedef struct CamGlobals {
    float x0, x4, x8, xC, x10, x14, x18, x1C, x20, x24, x28, x2C, x30, x34, x38, x3C, x40, x44;
} CamGlobals;

/* First 18 values of cm_803BCCA0 (melee/cm/camera.c). */
static const CamGlobals kCam = {
    83.0f, 1000.0f, -30.0f, 5.0f, -7.0f, 17.5f, -17.5f, 0.0f, 0.0682f,
    60.0f, 120.0f, 0.05f, 0.1f, 120.0f, 900.0f, 0.15f, 38.0f, 0.1f
};
/* cm_803BCB64 (melee/cm/camera.c). */
static const float kCamNear = 0.1f;
static const float kCamFar = 16384.0f;
static const float kCamFov = 30.0f;
static const float kCamAspect = 1.2173333f;
/* cm_803BCB9C (melee/cm/camera.c). */
static const float kTrackWeight[5] = { 0.0f, 1.5f, 1.32f, 1.16f, 1.0f };

typedef struct CamTransform {
    Vec3 interest, targetInterest, position, targetPosition;
    float fov, targetFov;
} CamTransform;

typedef struct CamBounds {
    float xMin, xMax, yMin, yMax, zPos;
    int subjects;
} CamBounds;

static void* s_archive;
static UnkStageDat* s_mapHead;
static MapCollData* s_coll;
static GroundParam* s_param;
static DiscU32* s_plit;
static HSD_JObj* s_points[256];
static HSD_JObj* s_models[kMaxMapGObjs];
static unsigned s_modelCount;
static HSD_GObj* s_cameraGObj;
static HSD_CObj* s_cobj;
static HSD_LObj* s_lobj;
static void* s_fighters[kMaxFighters];
static unsigned s_fighterCount;
static unsigned s_respawn[kMaxFighters];
static unsigned s_stocksLost[kMaxFighters];
static unsigned s_stocksRemaining[kMaxFighters];
static int s_human[kMaxFighters];
static int s_matchOver;
static unsigned s_winner;
static M360MatchStage s_stage;
static CamTransform s_cam;
static int s_loaded;
static int s_active;
static int s_paused;
static int s_holdTraced;
static unsigned s_frame;
static unsigned s_gameMode = 2;
static unsigned s_campaignRound;
static float s_scale = 1.0f;
static float s_tilt, s_pan, s_camX20, s_camX24, s_zoomRate, s_maxDepth;
static float s_trackRatio, s_fixedZoom, s_trackSmooth;
static HSD_WObjDesc s_eyeDesc, s_interestDesc;
static HSD_CameraDescPerspective s_camDesc;

static float DegToRad(float d) { return d * 0.017453292f; }

static void CollectPoints(HSD_JObj* root, HSD_Joint* joint)
{
    struct MapJointRemapEntry* entry = s_mapHead->unk0;
    HSD_JObj* order[128];
    unsigned count = 0;
    HSD_JObj* j = root;
    int i;
    while (j && count < 128) {
        order[count++] = j;
        if (j->child)
            j = j->child;
        else {
            while (j && !j->next && j != root)
                j = j->parent;
            if (!j || j == root)
                break;
            j = j->next;
        }
    }
    for (i = 0; i < s_mapHead->unk4; ++i, ++entry) {
        DiscS16* pairs;
        int k;
        if (entry->joint != joint)
            continue;
        pairs = entry->pairs;
        for (k = 0; k < entry->pair_count; ++k) {
            const int index = pairs[k * 2].v;
            const int slot = pairs[k * 2 + 1].v;
            if (index >= 0 && (unsigned) index < count && slot >= 0 && slot < 256)
                s_points[slot] = order[index];
        }
    }
}

static int PointPosition(int slot, Vec3* out)
{
    if (!s_points[slot])
        return 0;
    lb_8000B1CC(s_points[slot], NULL, out);
    return 1;
}

static HSD_JObj* CreateMapGObj(int id)
{
    struct UnkStageDat_x8_t* desc;
    HSD_Joint* joint;
    HSD_Joint scaleJoint;
    HSD_JObj* model;
    HSD_JObj* root;
    HSD_GObj* gobj;
    DiscU32* aj;
    DiscU32* ma;
    DiscU32* sa;
    u8* loop;
    if (id >= s_mapHead->unkC || s_modelCount >= kMaxMapGObjs)
        return NULL;
    desc = MAP_GOBJ_DESC(s_mapHead, id);
    joint = desc->unk0;
    model = HSD_JObjLoadJoint(joint);
    CollectPoints(model, joint);
    memset(&scaleJoint, 0, sizeof(scaleJoint));
    scaleJoint.scale.x = scaleJoint.scale.y = scaleJoint.scale.z = s_scale;
    root = HSD_JObjLoadJoint(&scaleJoint);
    HSD_JObjAddNext(model, root);
    aj = desc->unk4;
    ma = desc->unk8;
    sa = desc->unkC;
    HSD_JObjAddAnimAll(model, aj ? (HSD_AnimJoint*) (uintptr_t) aj[0].v : NULL,
                       ma ? (HSD_MatAnimJoint*) (uintptr_t) ma[0].v : NULL,
                       sa ? (HSD_ShapeAnimJoint*) (uintptr_t) sa[0].v : NULL);
    HSD_JObjReqAnimAll(model, 0.0f);
    loop = (u8*) desc->x28;
    if (loop && loop[0])
        HSD_ForeachAnim(model, JOBJ_TYPE, 0x77A4, AnimCallback((void (*)(void)) HSD_AObjSetFlags), AOBJ_ARG_AU, AOBJ_LOOP);
    HSD_JObjAnimAll(model);
    gobj = GObj_Create(HSD_GOBJ_CLASS_STAGE, 5, 0);
    HSD_GObjObject_80390A70(gobj, HSD_GObj_JObjKind, root);
    GObj_SetupGXLink(gobj, HSD_GObj_JObjCallback, kLinkStage, 0);
    s_models[s_modelCount++] = model;
    return root;
}

static void LoadCollision(void)
{
    MapLine* lines = s_coll->lines;
    DiscVec2* verts = s_coll->verts;
    int i;
    s_stage.lineCount = 0;
    for (i = 0; i < s_coll->line_count && s_stage.lineCount < 128; ++i) {
        M360StageLine* out = &s_stage.lines[s_stage.lineCount++];
        out->x0 = verts[lines[i].v0_idx].x * s_scale;
        out->y0 = verts[lines[i].v0_idx].y * s_scale;
        out->x1 = verts[lines[i].v1_idx].x * s_scale;
        out->y1 = verts[lines[i].v1_idx].y * s_scale;
        out->kind = lines[i].hi_flags;
        out->flags = lines[i].lo_flags;
    }
}

static void LoadBounds(void)
{
    Vec3 a, b, c;
    int i;
    s_stage.camLeft = -170.0f; s_stage.camRight = 170.0f;
    s_stage.camTop = 120.0f; s_stage.camBottom = -60.0f;
    s_stage.camX = s_stage.camY = 0.0f;
    if (PointPosition(0x95, &a) && PointPosition(0x96, &b) && PointPosition(0x94, &c)) {
        s_stage.camLeft = (a.x < b.x ? a.x : b.x) - c.x;
        s_stage.camRight = (a.x < b.x ? b.x : a.x) - c.x;
        s_stage.camBottom = (a.y < b.y ? a.y : b.y) - c.y;
        s_stage.camTop = (a.y < b.y ? b.y : a.y) - c.y;
        s_stage.camX = c.x;
        s_stage.camY = c.y;
    }
    s_stage.blastLeft = -250.0f; s_stage.blastRight = 250.0f;
    s_stage.blastTop = 200.0f; s_stage.blastBottom = -100.0f;
    if (PointPosition(0x97, &a) && PointPosition(0x98, &b)) {
        s_stage.blastLeft = (a.x < b.x ? a.x : b.x) - s_stage.camX;
        s_stage.blastRight = (a.x < b.x ? b.x : a.x) - s_stage.camX;
        s_stage.blastBottom = (a.y < b.y ? a.y : b.y) - s_stage.camY;
        s_stage.blastTop = (a.y < b.y ? b.y : a.y) - s_stage.camY;
    }
    for (i = 0; i < 4; ++i) {
        if (PointPosition(i, &a)) {
            s_stage.spawnX[i] = a.x;
            s_stage.spawnY[i] = a.y;
        } else {
            s_stage.spawnX[i] = (i & 1 ? 1.0f : -1.0f) * 30.0f;
            s_stage.spawnY[i] = 20.0f;
        }
    }
}

static float CamLeft(void) { return s_stage.camLeft + s_stage.camX; }
static float CamRight(void) { return s_stage.camRight + s_stage.camX; }
static float CamTop(void) { return s_stage.camTop + s_stage.camY; }
static float CamBottom(void) { return s_stage.camBottom + s_stage.camY; }

static void ClampToCamBounds(Vec3* p)
{
    if (p->x < CamLeft()) p->x = CamLeft();
    if (p->x > CamRight()) p->x = CamRight();
    if (p->y > CamTop()) p->y = CamTop();
    if (p->y < CamBottom()) p->y = CamBottom();
}

/* Camera_8002958C: subject framing bounds. */
static void CamComputeBounds(CamBounds* bounds)
{
    float minX = 3.4e38f, minY = 3.4e38f, maxX = -3.4e38f, maxY = -3.4e38f;
    float mult, z, zf;
    unsigned i;
    int n = 0;
    for (i = 0; i < s_fighterCount; ++i)
        if (s_fighters[i] && !s_respawn[i])
            ++n;
    mult = (n < 5 ? kTrackWeight[n] : 1.0f) * s_trackRatio;
    for (i = 0; i < s_fighterCount; ++i) {
        Vec3 base, test;
        float left, right, up, down;
        if (!s_fighters[i] || s_respawn[i])
            continue;
        M360_FighterCameraBox(s_fighters[i], &base.x, &base.y, &left, &right, &up, &down);
        base.z = 0.0f;
        ClampToCamBounds(&base);
        test = base;
        test.x = left * mult + base.x;
        ClampToCamBounds(&test);
        if (test.x < minX) minX = test.x;
        if (test.x > maxX) maxX = test.x;
        test.x = right * mult + base.x;
        ClampToCamBounds(&test);
        if (test.x < minX) minX = test.x;
        if (test.x > maxX) maxX = test.x;
        test.y = down * mult + base.y;
        ClampToCamBounds(&test);
        if (test.y < minY) minY = test.y;
        if (test.y > maxY) maxY = test.y;
        test.y = up * mult + base.y;
        ClampToCamBounds(&test);
        if (test.y < minY) minY = test.y;
        if (test.y > maxY) maxY = test.y;
    }
    if (!n) {
        minX = s_stage.camX - 40.0f; maxX = s_stage.camX + 40.0f;
        minY = s_stage.camY - 40.0f; maxY = s_stage.camY + 40.0f;
    }
    z = fabsf(s_cam.position.z);
    zf = z < 80.0f ? 0.0f : (z > 5000.0f ? 1.0f : (z - 80.0f) / 4920.0f);
    bounds->xMin = minX;
    bounds->yMin = minY - (390.0f * zf + 10.0f);
    bounds->xMax = maxX;
    bounds->yMax = maxY;
    bounds->subjects = n;
    bounds->zPos = z;
}

/* Camera_80029CF8: target interest/position from the framing bounds. */
static void CamTarget(const CamBounds* b)
{
    const float dx = b->xMax - b->xMin, dy = b->yMax - b->yMin;
    const float spread = dx > dy ? dx : dy;
    float t, base, angle, yAngle, tanU, tanD, distY, yOff, xCenter, tanR, tanL;
    float distX, xOff, dist;
    if (spread > kCam.x28) t = kCam.x20;
    else if (spread < kCam.x24) t = kCam.x1C;
    else t = (kCam.x20 - kCam.x1C) * ((spread - kCam.x24) / (kCam.x28 - kCam.x24)) + kCam.x1C;
    base = ((b->yMin - s_stage.camY) + (b->yMax - s_stage.camY)) * (0.5f - t) + s_stage.camY;
    angle = -DegToRad((base + kCam.x8) * s_camX24);
    if (angle > DegToRad(kCam.xC)) angle = DegToRad(kCam.xC);
    if (angle < DegToRad(kCam.x10)) angle = DegToRad(kCam.x10);
    angle += DegToRad(s_pan);
    yAngle = angle;
    tanU = tanf(0.5f * DegToRad(s_cam.fov) + angle);
    tanD = tanf(0.5f * DegToRad(s_cam.fov) - angle);
    distY = dy / (tanU + tanD);
    yOff = distY * tanf(yAngle);
    s_cam.targetInterest.y = yOff + (b->yMax - distY * tanU);
    xCenter = 0.5f * (b->xMin + b->xMax);
    angle = -DegToRad((xCenter - s_stage.camX) * s_camX20);
    if (angle > DegToRad(kCam.x14)) angle = DegToRad(kCam.x14);
    if (angle < DegToRad(kCam.x18)) angle = DegToRad(kCam.x18);
    tanR = kCamAspect * tanf(0.5f * DegToRad(s_cam.fov) - angle);
    tanL = kCamAspect * tanf(0.5f * DegToRad(s_cam.fov) + angle);
    distX = dx / (tanR + tanL);
    xOff = kCamAspect * (distX * tanf(angle));
    s_cam.targetInterest.x = (b->xMax - distX * tanR) - xOff;
    s_cam.targetInterest.z = 0.0f;
    dist = distY > distX ? distY : distX;
    if (dist < s_zoomRate) dist = s_zoomRate;
    if (dist > s_maxDepth) dist = s_maxDepth;
    s_cam.targetPosition.x = s_cam.targetInterest.x + xOff;
    s_cam.targetPosition.y = s_cam.targetInterest.y - yOff;
    s_cam.targetPosition.z = s_cam.targetInterest.z + dist;
}

/* Camera_80029AAC and Camera_80029C88: smoothing toward the targets. */
static void CamFollow(const CamBounds* b)
{
    float spread, follow, lerp, scale;
    if (b->subjects) {
        const float dx = b->xMax - b->xMin, dy = b->yMax - b->yMin;
        spread = dx > dy ? dx : dy;
    } else {
        spread = 99999.0f;
    }
    if (spread > kCam.x38) follow = kCam.x30;
    else if (spread < kCam.x34) follow = kCam.x2C;
    else follow = ((spread - kCam.x34) / (kCam.x38 - kCam.x34)) * (kCam.x30 - kCam.x2C) + kCam.x2C;
    lerp = follow * s_trackSmooth;
    if (lerp > 1.0f) lerp = 1.0f;
    if (lerp < 0.0001f) lerp = 0.0001f;
    s_cam.interest.x += (s_cam.targetInterest.x - s_cam.interest.x) * lerp;
    s_cam.interest.y += (s_cam.targetInterest.y - s_cam.interest.y) * lerp;
    scale = kCam.x3C * s_trackSmooth;
    if (scale > 1.0f) scale = 1.0f;
    s_cam.position.x += (s_cam.targetPosition.x - s_cam.position.x) * scale;
    s_cam.position.y += (s_cam.targetPosition.y - s_cam.position.y) * scale;
    s_cam.position.z += (s_cam.targetPosition.z - s_cam.position.z) * scale;
}

static void CamApply(void)
{
    HSD_CObjSetFov(s_cobj, s_cam.fov);
    HSD_CObjSetInterest(s_cobj, &s_cam.interest);
    HSD_CObjSetEyePosition(s_cobj, &s_cam.position);
}

static void CamUpdate(int snap)
{
    CamBounds bounds;
    CamComputeBounds(&bounds);
    s_cam.targetFov = kCam.x40;
    s_cam.fov += (s_cam.targetFov - s_cam.fov) * kCam.x44;
    if (snap)
        s_cam.fov = s_cam.targetFov;
    CamTarget(&bounds);
    if (snap) {
        s_cam.interest = s_cam.targetInterest;
        s_cam.position = s_cam.targetPosition;
    } else {
        CamFollow(&bounds);
    }
    CamApply();
}

static void CameraRender(HSD_GObj* gobj, int pass)
{
    (void) pass;
    if (HSD_CObjSetCurrent(gobj->hsd_obj)) {
        HSD_SetEraseColor(0, 0, 0, 255);
        HSD_CObjEraseScreen(gobj->hsd_obj, 1, 0, 1);
        HSD_FogSet(NULL);
        HSD_GObj_80390ED0(gobj, 7);
        HSD_CObjEndCurrent();
    }
}

static void CreateCamera(void)
{
    memset(&s_camDesc, 0, sizeof(s_camDesc));
    memset(&s_eyeDesc, 0, sizeof(s_eyeDesc));
    memset(&s_interestDesc, 0, sizeof(s_interestDesc));
    s_eyeDesc.pos.z = 100.0f;
    s_camDesc.projection_type = 1;
    s_camDesc.viewport.xmax = 640;
    s_camDesc.viewport.ymax = 480;
    s_camDesc.scissor.right = 640;
    s_camDesc.scissor.bottom = 480;
    s_camDesc.eyepos = &s_eyeDesc;
    s_camDesc.interest = &s_interestDesc;
    s_camDesc.nnear = kCamNear;
    s_camDesc.ffar = kCamFar;
    s_camDesc.fov = kCamFov;
    s_camDesc.aspect = kCamAspect;
    s_cobj = HSD_CObjLoadDesc((HSD_CObjDesc*) &s_camDesc);
    s_cameraGObj = GObj_Create(19, 20, 0);
    HSD_GObjObject_80390A70(s_cameraGObj, HSD_GObj_CameraKind, s_cobj);
    GObj_SetupGXLinkMax(s_cameraGObj, CameraRender, 0);
    s_cameraGObj->gxlink_prios = (1 << kLinkLight) | (1 << kLinkStage) | (1 << kLinkFighter);
    memset(&s_cam, 0, sizeof(s_cam));
    s_cam.fov = kCamFov;
}

static void CreateLights(void)
{
    HSD_GObj* gobj = GObj_Create(12, 3, 0);
    s_lobj = lb_80011AC4(s_plit);
    HSD_GObjObject_80390A70(gobj, HSD_GObj_LightKind, s_lobj);
    GObj_SetupGXLink(gobj, HSD_GObj_LObjCallback, kLinkLight, 0);
    HSD_LObjReqAnimAll(s_lobj, 0.0f);
}

int M360_MatchLoad(void)
{
    unsigned size = 0;
    unsigned char* image;
    if (s_loaded)
        return 1;
    image = M360_ReadDiscFile("GrNBa.dat", &size);
    M360_MatchTrace("match.stage.bytes", size);
    if (!image)
        return 0;
    s_archive = M360_ArchiveOpen(image, size);
    s_mapHead = M360_ArchiveFind(s_archive, "map_head");
    s_coll = M360_ArchiveFind(s_archive, "coll_data");
    s_param = M360_ArchiveFind(s_archive, "grGroundParam");
    s_plit = M360_ArchiveFind(s_archive, "map_plit");
    M360_MatchTrace("match.stage.map_gobjs", s_mapHead ? (unsigned) s_mapHead->unkC : 0);
    M360_MatchTrace("match.stage.coll_lines", s_coll ? (unsigned) s_coll->line_count : 0);
    if (!s_mapHead || !s_coll || !s_param || !s_plit)
        return 0;
    s_scale = s_param->y;
    s_tilt = s_param->x8;
    s_pan = (float) s_param->x14;
    s_camX20 = s_param->x1C;
    s_camX24 = s_param->x18;
    s_zoomRate = (float) s_param->xC;
    s_maxDepth = (float) s_param->x10;
    s_trackSmooth = s_param->x28;
    s_trackRatio = s_param->x20;
    s_fixedZoom = s_param->x24;
    LoadCollision();
    M360_MatchTrace("match.fighter.loaded", M360_FighterLoad());
    s_loaded = 1;
    return 1;
}

float M360_MatchFixedZoom(void)
{
    return s_fixedZoom;
}

void M360_MatchEnter(void)
{
    unsigned i;
    if (!M360_MatchLoad())
        return;
    M360_FighterResetMatch();
    memset(s_points, 0, sizeof(s_points));
    s_modelCount = 0;
    CreateLights();
    M360_MatchTrace("match.enter.step", 1);
    CreateMapGObj(0);
    HSD_JObjSetFlagsAll(CreateMapGObj(3), JOBJ_HIDDEN);
    CreateMapGObj(1);
    CreateMapGObj(6);
    M360_MatchTrace("match.enter.step", 2);
    LoadBounds();
    CreateCamera();
    M360_MatchTrace("match.enter.step", 3);
    s_fighterCount = 0;
    for (i = 0; i < kMaxFighters; ++i) {
        const int campaign = s_gameMode == kGameModeClassic ||
                             s_gameMode == kGameModeAdventure;
        const int port = i == 0 ? 0 : -1;
        (void) campaign;
        s_fighters[i] = M360_FighterSpawn((int) i, s_stage.spawnX[i], s_stage.spawnY[i],
                                          i ? -1.0f : 1.0f, port);
        s_human[i] = port >= 0;
        s_respawn[i] = 0;
        s_stocksLost[i] = 0;
        s_stocksRemaining[i] = kStartingStocks;
        if (s_fighters[i])
            s_fighterCount = i + 1;
    }
    s_matchOver = 0;
    s_winner = 0;
    s_paused = 0;
    s_frame = 0;
    s_active = 1;
    CamUpdate(1);
    M360_MatchTrace("match.enter.fighters", s_fighterCount);
    M360_MatchTrace("match.p2.human", s_human[1]);
    M360_MatchTrace("match.mode", s_gameMode);
    M360_MatchTrace("match.campaign.round", s_campaignRound + 1);
    M360_MatchTrace("match.enter.blast_left", (unsigned) (int) s_stage.blastLeft);
    M360_MatchTrace("match.enter.blast_bottom", (unsigned) (int) s_stage.blastBottom);
    for (i = 0; i < 4; ++i) {
        M360_MatchTrace("match.enter.spawn_x", (unsigned) (int) s_stage.spawnX[i]);
        M360_MatchTrace("match.enter.spawn_y", (unsigned) (int) s_stage.spawnY[i]);
    }
}

static int OutsideBlastZone(float x, float y)
{
    return x > s_stage.blastRight + s_stage.camX || x < s_stage.blastLeft + s_stage.camX ||
           y > s_stage.blastTop + s_stage.camY || y < s_stage.blastBottom + s_stage.camY;
}

int M360_MatchFrame(void)
{
    unsigned i;
    const unsigned buttons = M360_MatchPadTriggered();
    const unsigned held = M360_MatchPadHeld();
    if (!s_active)
        return M360_MATCH_TO_MENU;
    if (s_matchOver) {
        if (buttons & 0x200u) {
            M360_MatchTrace("match.result.return_menu", s_winner);
            return M360_MATCH_TO_MENU;
        }
        if ((buttons & 0x100u) && s_winner == 0 &&
            (s_gameMode == kGameModeClassic || s_gameMode == kGameModeAdventure))
            return M360_MATCH_NEXT_ROUND;
        return M360_MATCH_CONTINUE;
    }
    if (buttons & 0x1000u) {
        s_paused = !s_paused;
        M360_MatchTrace("match.pause", s_paused);
    }
    if (M360_InputScriptHolding()) {
        if (!s_holdTraced) {
            M360MatchStatus st;
            s_holdTraced = 1;
            M360_MatchGetStatus(&st);
            M360_MatchTrace("snap.frame", st.frame);
            for (i = 0; i < 2; ++i) {
                M360_MatchTrace(i ? "snap.p2.motion" : "snap.p1.motion", st.motion[i]);
                M360_MatchTrace(i ? "snap.p2.x" : "snap.p1.x", (unsigned) (int) st.posX[i]);
                M360_MatchTrace(i ? "snap.p2.y" : "snap.p1.y", (unsigned) (int) st.posY[i]);
                M360_MatchTrace(i ? "snap.p2.damage" : "snap.p1.damage", st.damage[i]);
                M360_MatchTrace(i ? "snap.p2.stocks" : "snap.p1.stocks", st.stocksRemaining[i]);
                if (s_fighters[i]) {
                    float fx, fy, facing;
                    unsigned fm, fd;
                    M360_FighterGetState(s_fighters[i], &fx, &fy, &facing, &fm, &fd);
                    M360_MatchTrace(i ? "snap.p2.facing_neg" : "snap.p1.facing_neg", facing < 0.0f);
                }
            }
        }
        return M360_MATCH_CONTINUE;
    }
    s_holdTraced = 0;
    if (s_paused) {
        if ((buttons & 0x200u) || ((held & 0x160u) == 0x160u && (buttons & 0x100u))) {
            M360_MatchTrace("match.exit.menu", s_frame);
            return M360_MATCH_TO_MENU;
        }
        return M360_MATCH_CONTINUE;
    }
    ++s_frame;
    for (i = 0; i < s_modelCount; ++i)
        HSD_JObjAnimAll(s_models[i]);
    if (s_lobj)
        HSD_LObjAnimAll(s_lobj);
    if (s_fighterCount > 1 && !s_human[1] && s_gameMode != kGameModeClassic &&
        s_gameMode != kGameModeAdventure && M360_MatchControllerConnected(1) &&
        (M360_MatchPadTriggeredPort(1) & 0x1F00u)) {
        s_human[1] = 1;
        M360_FighterSetPort(s_fighters[1], 1);
        M360_MatchTrace("match.p2.join", s_frame);
    }
    HSD_GObj_RunProcs();
    for (i = 0; i < s_fighterCount; ++i) {
        float x, y, facing;
        unsigned motion, damage;
        if (!s_fighters[i])
            continue;
        if (s_respawn[i]) {
            if (--s_respawn[i] == 0 && s_stocksRemaining[i])
                M360_FighterRespawn(s_fighters[i], s_stage.spawnX[i], s_stage.spawnY[i]);
            continue;
        }
        M360_FighterGetState(s_fighters[i], &x, &y, &facing, &motion, &damage);
        if (OutsideBlastZone(x, y)) {
            ++s_stocksLost[i];
            if (s_stocksRemaining[i])
                --s_stocksRemaining[i];
            s_respawn[i] = s_stocksRemaining[i] ? kRespawnFrames : 0;
            M360_FighterSetDead(s_fighters[i]);
            M360_MatchTrace("match.blast_zone.fighter", i);
            M360_MatchTrace("match.stocks.remaining", s_stocksRemaining[i]);
            if (!s_stocksRemaining[i]) {
                s_matchOver = 1;
                s_winner = 1u - i;
                M360_MatchTrace("match.result.winner", s_winner);
                if (s_winner == 0 &&
                    (s_gameMode == kGameModeClassic || s_gameMode == kGameModeAdventure) &&
                    s_campaignRound + 1 >= kCampaignRounds)
                    M360_MatchTrace("match.campaign.complete", s_gameMode);
            }
        }
    }
    CamUpdate(0);
    return M360_MATCH_CONTINUE;
}

void M360_MatchRender(void)
{
    if (s_active)
        HSD_GObj_80390FC0();
}

void M360_MatchLeave(void)
{
    int link;
    for (link = 0; link < 64; ++link) {
        HSD_GObj* gobj = HSD_GObjPLinkHead[link];
        while (gobj) {
            HSD_GObj* next = gobj->next;
            HSD_GObjFree(gobj);
            gobj = next;
        }
    }
    memset(s_fighters, 0, sizeof(s_fighters));
    s_fighterCount = 0;
    s_modelCount = 0;
    s_active = 0;
}

void M360_MatchSetMode(unsigned gameMode, unsigned round)
{
    s_gameMode = gameMode;
    s_campaignRound = round;
}

void M360_MatchGetStatus(M360MatchStatus* status)
{
    unsigned i;
    memset(status, 0, sizeof(*status));
    status->loaded = (unsigned) s_loaded;
    status->paused = (unsigned) s_paused;
    status->frame = s_frame;
    status->fighters = s_fighterCount;
    status->hits = M360_FighterHitCount();
    status->matchOver = (unsigned) s_matchOver;
    status->winner = s_winner;
    status->gameMode = s_gameMode;
    status->campaignRound = s_campaignRound;
    status->campaignRounds = (s_gameMode == kGameModeClassic ||
                              s_gameMode == kGameModeAdventure)
                                 ? kCampaignRounds : 0;
    status->inputButtons = M360_MatchPadHeld();
    status->inputTriggered = M360_MatchPadTriggered();
    status->inputX = M360_MatchPadX();
    status->inputY = M360_MatchPadY();
    for (i = 0; i < s_fighterCount && i < 2; ++i) {
        float facing;
        if (!s_fighters[i])
            continue;
        M360_FighterGetState(s_fighters[i], &status->posX[i], &status->posY[i], &facing,
                             &status->motion[i], &status->damage[i]);
        status->stocksLost[i] = s_stocksLost[i];
        status->stocksRemaining[i] = s_stocksRemaining[i];
        status->human[i] = (unsigned) s_human[i];
    }
}

const M360MatchStage* M360_MatchStageData(void)
{
    return &s_stage;
}
