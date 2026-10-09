#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
using u32=unsigned; using s8=signed char; using u8=unsigned char; using Fighter_Part=int;
struct Vec3 { float x,y,z; };
struct HSD_JObj { unsigned flags,visits; Vec3 translate,sample; HSD_JObj *child,*next,*parent; };
struct FighterBone { HSD_JObj* joint; };
struct ftData_x8 { unsigned x10; }; struct FighterData { ftData_x8* x8; };
struct Fighter {
    unsigned kind; bool x594_b6,x594_b5,x2226_b2,x2221_b2;
    float size; struct { float model_scaling; } co_attrs;
    Vec3 x68C_transNPos,x698,x6A4_transNOffset,x6B0,x6C0,x6CC,x6D8,x6E4;
    FighterBone* parts; FighterData* ft_data;
};
struct FighterPartsTable { unsigned parts_num; s8* part_to_joint; };
struct Slot { uintptr_t v; }; static Slot ftPartsTable[1];
#define DP(type,value) ((type*)(value))
static unsigned begins,ends;
float ftCommon_GetModelScale(Fighter* fp) { return fp->size*fp->co_attrs.model_scaling; }
void HSD_AObjInitEndCallBack() { ++begins; }
void HSD_AObjInvokeCallBacks() { ++ends; }
void HSD_JObjAnim(HSD_JObj* j) { ++j->visits; j->translate=j->sample; }
void HSD_JObjGetTranslation(HSD_JObj* j,Vec3* out) { *out=j->translate; }
void HSD_JObjSetTranslate(HSD_JObj* j,Vec3* value) { j->translate=*value; }
unsigned HSD_JObjGetFlags(HSD_JObj* j) { return j->flags; }
HSD_JObj* HSD_JObjGetChild(HSD_JObj* j) { return j->child; }
HSD_JObj* HSD_JObjGetNext(HSD_JObj* j) { return j->next; }
HSD_JObj* HSD_JObjGetParent(HSD_JObj* j) { return j->parent; }
void lbVector_Diff(Vec3* a,Vec3* b,Vec3* out) { *out={a->x-b->x,a->y-b->y,a->z-b->z}; }
void lbVector_Sub(Vec3* a,Vec3* b) { a->x-=b->x; a->y-=b->y; a->z-=b->z; }
#pragma warning(push)
#pragma warning(disable:4101)
#include "root_motion_original.h"
#pragma warning(pop)
static void Near(Vec3 a,Vec3 b) { assert(std::fabs(a.x-b.x)<0.0001f && std::fabs(a.y-b.y)<0.0001f && std::fabs(a.z-b.z)<0.0001f); }
int main() {
    s8 remap[56]; for(auto& part:remap) part=-1; remap[1]=3; remap[0x35]=7;
    FighterPartsTable table={10,remap}; ftPartsTable[0].v=(uintptr_t)&table;
    HSD_JObj nodes[6]={}; FighterBone parts[10]={};
    nodes[0].child=&nodes[1]; nodes[1].parent=&nodes[0];
    nodes[1].child=&nodes[2]; nodes[2].parent=&nodes[1]; nodes[2].next=&nodes[3]; nodes[3].parent=&nodes[1];
    nodes[1].next=&nodes[4]; nodes[4].parent=&nodes[0]; nodes[4].flags=0x1000;
    nodes[4].child=&nodes[5]; nodes[5].parent=&nodes[4]; // Instance child must be skipped.
    nodes[2].sample={1,2,3}; nodes[3].sample={4,5,6}; nodes[1].sample={20,30,40};
    parts[3].joint=&nodes[2]; parts[7].joint=&nodes[3]; parts[9].joint=&nodes[1];
    ftData_x8 data8={9}; FighterData data={&data8};
    Fighter fp={}; fp.parts=parts; fp.ft_data=&data; fp.size=3; fp.co_attrs.model_scaling=2;
    assert(ftParts_GetBoneIndex(&fp,1)==3 && ftParts_GetBone(&fp,1)==&parts[3]);
    assert(ftParts_GetBone(&fp,0x35)==&parts[7]);
    assert(ftParts_GetBoneIndex(&fp,56)==-1 && !ftParts_GetBone(&fp,55));
    remap[55]=16; assert(!ftParts_GetBone(&fp,55)); remap[55]=-1;
    fp.x68C_transNPos={10,20,30}; fp.x6A4_transNOffset={7,8,9};
    ftAnim_8006E054(&fp,&nodes[0],parts[3].joint,parts[7].joint);
    Near(fp.x698,{10,20,30}); Near(fp.x68C_transNPos,{6,12,18});
    Near(fp.x6A4_transNOffset,{-4,-8,-12}); Near(fp.x6B0,{7,8,9}); Near(nodes[2].translate,{0,0,0});
    assert(nodes[5].visits==0 && begins==1 && ends==1);
    // Secondary displacement becomes the authoritative position and delta.
    fp.x594_b5=true; fp.x6C0={2,3,4}; fp.x6D8={9,8,7};
    ftAnim_8006E054(&fp,&nodes[0],parts[3].joint,parts[7].joint);
    Near(fp.x68C_transNPos,{24,30,36}); Near(fp.x698,{2,3,4});
    Near(fp.x6A4_transNOffset,{22,27,32}); Near(fp.x6B0,{9,8,7});
    Near(nodes[2].translate,{-18,-18,-18}); Near(nodes[3].translate,{0,0,0});
    // Flag b6 uses the authored model multiplier without encounter size.
    fp.x594_b6=true; ftAnim_8006E054(&fp,&nodes[0],parts[3].joint,parts[7].joint);
    Near(fp.x68C_transNPos,{8,10,12}); Near(nodes[2].translate,{-6,-6,-6});
    // Captured-object joint compensation divides by the same chosen scale.
    fp.x594_b5=false; fp.x2221_b2=true;
    ftAnim_8006E054(&fp,&nodes[0],parts[3].joint,nullptr);
    Near(nodes[1].translate,{19,28,37});
    assert(begins==4 && ends==4 && nodes[5].visits==0);
    puts("PASS: original remapped bone bounds, root/secondary animation displacement, scale flags, instance traversal and captured-object compensation (animation sampling mocked)");
}
