#include <assert.h>
#include <stdio.h>
struct HSD_DObj { HSD_DObj* next; };
struct HSD_JObj { struct { HSD_DObj* dobj; } u; };
struct Bone { HSD_JObj* joint; HSD_JObj* x4_jobj2; bool flags_b1; bool flags_b2; };
struct Fighter_GObj;
struct Fighter { Fighter_GObj* gobj; Bone* parts; struct { unsigned count; } dobj_list; };
struct M360Fighter { Fighter fighter; unsigned dobjCount; HSD_DObj* dobjs[8]; };
struct Fighter_GObj { M360Fighter* user_data; };
struct Fighter_804D6540_x0_t { unsigned char x0, x1, x2, x3; };
enum { kMaxJoints = 128 };
M360Fighter* Owner(Fighter_GObj* object) { return object->user_data; }
unsigned removed;
void HSD_JObjRemove(HSD_JObj* joint) { assert(joint); ++removed; }
#include "reserved_cleanup_original.h"
int main() {
    M360Fighter actor = {};
    Fighter_GObj object = { &actor };
    Bone bones[kMaxJoints] = {};
    HSD_DObj original = {}, first = {}, second = {}, other = {};
    first.next = &second;
    HSD_JObj joint = { { &first } };
    actor.fighter.gobj = &object; actor.fighter.parts = bones;
    actor.dobjCount = actor.fighter.dobj_list.count = 4;
    actor.dobjs[0] = &original; actor.dobjs[1] = &first;
    actor.dobjs[2] = &other; actor.dobjs[3] = &second;
    bones[68].joint = bones[68].x4_jobj2 = &joint;
    bones[68].flags_b1 = bones[68].flags_b2 = true;
    Fighter_804D6540_x0_t entry = { 68, 0, 0, 0 };
    ftParts_800755E8(&actor.fighter, &entry);
    assert(removed == 1 && actor.dobjCount == 2 && actor.fighter.dobj_list.count == 2);
    assert(actor.dobjs[0] == &original && actor.dobjs[1] == &other);
    assert(!bones[68].joint && !bones[68].x4_jobj2);
    assert(!bones[68].flags_b1 && !bones[68].flags_b2);
    ftParts_800755E8(&actor.fighter, &entry);
    assert(removed == 1 && actor.dobjCount == 2);
    ftParts_800755E8(&actor.fighter, 0);
    entry.x0 = 255; ftParts_800755E8(&actor.fighter, &entry);
    assert(removed == 1);
    puts("PASS: reserved-part removal clears display references, preserves other parts and avoids double removal");
}
