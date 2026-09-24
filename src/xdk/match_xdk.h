#ifndef MELEE360_XDK_MATCH_H
#define MELEE360_XDK_MATCH_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
    M360_LINE_FLOOR = 1,
    M360_LINE_CEILING = 2,
    M360_LINE_RIGHT_WALL = 4,
    M360_LINE_LEFT_WALL = 8
};

enum {
    M360_LINE_PLATFORM = 0x100,
    M360_LINE_LEDGE = 0x200
};

enum {
    M360_MATCH_CONTINUE = 0,
    M360_MATCH_TO_MENU = 1,
    M360_MATCH_NEXT_ROUND = 2
};

typedef struct M360StageLine {
    float x0, y0, x1, y1;
    unsigned kind;
    unsigned flags;
} M360StageLine;

typedef struct M360MatchStage {
    M360StageLine lines[128];
    unsigned lineCount;
    float blastLeft, blastRight, blastTop, blastBottom;
    float camLeft, camRight, camTop, camBottom, camX, camY;
    float spawnX[4], spawnY[4];
    float rebirthX[4], rebirthY[4];
} M360MatchStage;

typedef struct M360MatchStatus {
    unsigned loaded;
    unsigned paused;
    unsigned frame;
    unsigned fighters;
    unsigned damage[2];
    unsigned stocksLost[2];
    unsigned stocksRemaining[2];
    unsigned motion[2];
    unsigned human[2];
    unsigned matchOver;
    unsigned winner;
    unsigned gameMode;
    unsigned campaignRound;
    unsigned campaignRounds;
    float posX[2], posY[2];
    unsigned hits;
    unsigned updateUs;
    unsigned inputButtons;
    unsigned inputTriggered;
    float inputX, inputY;
    unsigned selecting;
    unsigned selectKind[2];
    unsigned selectCostume[2];
    unsigned selectReady[2];
    unsigned selectHuman[2];
    unsigned fighterKind[2];
    unsigned stageIndex;
    unsigned stocks;
    unsigned cpuLevel;
} M360MatchStatus;

unsigned char* M360_ReadDiscFile(const char* name, unsigned* size);
void* M360_ArchiveOpen(unsigned char* image, unsigned imageSize);
void* M360_ArchiveFind(void* archive, const char* symbol);
const char* M360_ArchiveFirstSymbol(void* archive);
void M360_MatchTrace(const char* stage, unsigned value);

int M360_MatchLoad(void);
void M360_MatchEnter(void);
int M360_MatchFrame(void);
void M360_MatchRender(void);
void M360_MatchLeave(void);
void M360_MatchSetMode(unsigned gameMode, unsigned round);
void M360_MatchGetStatus(M360MatchStatus* status);
const M360MatchStage* M360_MatchStageData(void);
unsigned M360_MatchStageCount(void);
float M360_MatchStageScale(void);
void M360_MatchCameraVectors(float* interest, float* eye);
const char* M360_MatchStageName(unsigned index);
float M360_MatchFixedZoom(void);
unsigned M360_MatchPadTriggered(void);
unsigned M360_MatchPadTriggeredPort(unsigned port);
void M360_FighterSetPort(void* gobj, int port);
unsigned M360_MatchPadHeld(void);
float M360_MatchPadX(void);
float M360_MatchPadY(void);
float M360_MatchPadStickXPort(unsigned port);
int M360_MatchControllerConnected(unsigned port);
int M360_MatchGroundBelow(float x, float y, float depth, float* groundY,
                          unsigned* line);

int M360_FighterLoad(void);
void M360_FighterBuildIslands(void);
unsigned M360_FighterKindCount(void);
const char* M360_FighterKindName(unsigned index);
void M360_FighterSelect(int slot, unsigned kindIndex, unsigned costume);
unsigned M360_FighterKindIndex(void* gobj);
void M360_FighterResetMatch(void);
void* M360_FighterSpawn(int slot, float x, float y, float facing, int port);
void M360_FighterRespawn(void* gobj, float x, float y);
void M360_FighterRebirth(void* gobj);
void M360_FighterSetDead(void* gobj);
void M360_FighterGetState(void* gobj, float* x, float* y, float* facing,
                          unsigned* motion, unsigned* damage);
void M360_FighterCameraBox(void* gobj, float* x, float* y, float* left,
                           float* right, float* up, float* down);
void M360_FighterSetCpuLevel(unsigned level);
unsigned M360_FighterHitCount(void);

#ifdef __cplusplus
}
#endif

#endif
