#include <cassert>
#include <cstdio>
#include <cstdint>
#include "match_xdk.h"

using s16 = int16_t;
using u32 = uint32_t;
struct MapLine {
    uint16_t v0_idx, v1_idx;
    s16 prev_id0, next_id0, prev_id1, next_id1;
    uint16_t hi_flags, lo_flags;
};
static_assert(sizeof(MapLine) == 16, "Original archive layout");
struct MapCollData { MapLine* lines; int line_count; };
struct CollLine { MapLine* x0; u32 flags; };
struct CollVtx { struct { float x, y; } pos; };
static MapLine maps[6];
static MapCollData collision = { maps, 6 };
static MapCollData* s_coll = &collision;
static unsigned s_builtStage;
static M360MatchStage s_stage;
static CollLine groundCollLine[6];
static CollVtx groundCollVtx[12];
const M360MatchStage* M360_MatchStageData() { return &s_stage; }
#define LINE_FLAG_ENABLED (1 << 16)
#define LINE_FLAG_HIDDEN (1 << 18)
#define SQ(x) ((x) * (x))
#include "surface_links_native.h"
#include "surface_links_reference.h"

static void SyncReference(unsigned hidden = 0)
{
    for (unsigned i = 0; i < s_stage.lineCount; ++i) {
        auto& line = s_stage.lines[i];
        maps[i].v0_idx = static_cast<uint16_t>(i * 2);
        maps[i].v1_idx = static_cast<uint16_t>(i * 2 + 1);
        groundCollVtx[i * 2].pos = { line.x0, line.y0 };
        groundCollVtx[i * 2 + 1].pos = { line.x1, line.y1 };
        groundCollLine[i] = { &maps[i], line.kind ? LINE_FLAG_ENABLED : 0u };
        if (hidden & (1u << i)) {
            groundCollLine[i].flags = LINE_FLAG_ENABLED | LINE_FLAG_HIDDEN;
            line.kind = 0; // Native collision toggles collapse disabled/hidden.
        }
    }
}

int main()
{
    s_stage.lineCount = 6;
    for (unsigned i = 0; i < 6; ++i) {
        maps[i].next_id0 = 1; maps[i].next_id1 = 2;
        maps[i].prev_id0 = 3; maps[i].prev_id1 = 4;
    }
    // Compare both directions with the actual upstream bodies, not a rewritten
    // reference. Include exact <4 boundary, moving endpoints, 2D separation,
    // absent alternates and inactive lines. Authored fallback remains valid
    // even when that fallback is disabled, as in the original.
    unsigned comparisons = 0;
    for (unsigned enabled = 0; enabled < 64; ++enabled) {
        for (int x = -10; x <= 10; ++x) {
            for (int y = -10; y <= 10; ++y) {
                for (unsigned i = 0; i < 6; ++i) {
                    auto& line = s_stage.lines[i];
                    line.kind = enabled & (1u << i) ? M360_LINE_FLOOR : 0;
                    line.x0 = i == 2 ? x * 0.5f : 0.0f;
                    line.y0 = i == 2 ? y * 0.5f : 0.0f;
                    line.x1 = i == 4 ? x * 0.5f : 0.0f;
                    line.y1 = i == 4 ? y * 0.5f : 0.0f;
                }
                SyncReference();
                for (int i = 0; i < 6; ++i) {
                    assert(mpLineGetNext(i) == Original_mpLineGetNext(i));
                    assert(mpLineGetPrev(i) == Original_mpLineGetPrev(i));
                    comparisons += 2;
                }
            }
        }
    }
    for (unsigned i = 0; i < 6; ++i) s_stage.lines[i].kind = M360_LINE_FLOOR;
    s_stage.lines[2].x0 = 0; s_stage.lines[2].y0 = 0;
    s_stage.lines[4].x1 = 0; s_stage.lines[4].y1 = 0;
    SyncReference((1u << 2) | (1u << 4));
    assert(mpLineGetNext(0) == Original_mpLineGetNext(0));
    assert(mpLineGetPrev(0) == Original_mpLineGetPrev(0));
    maps[0].next_id1 = maps[0].prev_id1 = -1;
    assert(mpLineGetNext(0) == Original_mpLineGetNext(0));
    assert(mpLineGetPrev(0) == Original_mpLineGetPrev(0));
    maps[0].next_id0 = maps[0].prev_id0 = -1;
    assert(mpLineGetNext(0) == -1 && mpLineGetPrev(0) == -1);
    maps[0].next_id0 = maps[0].prev_id0 = 99;
    maps[0].next_id1 = maps[0].prev_id1 = 99;
    assert(mpLineGetNext(0) == -1 && mpLineGetPrev(0) == -1);
    assert(!M360_MatchMapLine(-1) && !M360_MatchMapLine(6));
    assert(mpLineGetNext(-1) == -1 && mpLineGetPrev(6) == -1);
    collision.line_count = 1;
    assert(!M360_MatchMapLine(1));
    collision.line_count = 6;
    s_stage.lineCount = 3;
    assert(!M360_MatchMapLine(3));
    s_builtStage = ~0u;
    assert(!M360_MatchMapLine(0));
    s_builtStage = 0;
    s_coll = nullptr;
    assert(!M360_MatchMapLine(0));
    printf("PASS: %u adjacency comparisons with upstream, alternate/fallback links, moving/disabled lines and archive/lifecycle guards\n", comparisons);
}
