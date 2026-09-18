#include "gobj_xdk_compat.h"

#ifdef _MSC_VER
#pragma warning(disable : 4201)
#endif

#include "class.h"
#include "gobj.h"
#include "gobjplink.h"
#include "gobjproc.h"
#include "gobjuserdata.h"
#include "gobjobject.h"
#include "list.h"

#include <stdio.h>
#include <string.h>

extern void M360_HSD_HeapInit(void);
extern int M360_HsdJObjStubDispCount(void);
extern void HSD_FObjInitAllocData(void);
extern void HSD_AObjInitAllocData(void);
extern void HSD_RObjInitAllocData(void);
extern void HSD_IDSetup(void);
extern void HSD_IDInitAllocData(void);
extern void HSD_VecInitAllocData(void);
extern void HSD_MtxInitAllocData(void);
extern void* HSD_JObjAlloc(void);
extern void HSD_JObjAddChild(void* jobj, void* child);
extern void HSD_JObjSetFlags(void* jobj, u32 flags);
extern HSD_ClassInfo hsdJObj;

static int fail_count = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            fprintf(stderr, "[M360][GOBJ][FAIL] %s (%s:%d)\n", #cond,        \
                    __FILE__, __LINE__);                                     \
            fail_count++;                                                    \
        }                                                                    \
    } while (0)

static int plink_count(u8 link)
{
    int n = 0;
    HSD_GObj* cur;
    for (cur = HSD_GObjPLinkHead[link]; cur != NULL; cur = cur->next) {
        n++;
    }
    return n;
}

static void test_list(void)
{
    HSD_SList* n1;
    HSD_SList* n2;
    HSD_SList* n3;
    HSD_SList* rest;
    HSD_SList* appended;
    int a = 1, b = 2, c = 3;
    int count;
    HSD_SList* cur;

    CHECK(HSD_SListGetAllocData() != NULL);
    CHECK(HSD_DListGetAllocData() != NULL);

    n1 = HSD_SListAlloc();
    n1->data = &a;
    n1->next = NULL;

    n2 = HSD_SListAllocAndPrepend(n1, &b);
    n3 = HSD_SListAllocAndPrepend(n2, &c);

    /* Prepend threads the newest node in front: n3(c) -> n2(b) -> n1(a). */
    CHECK(n3->data == &c);
    CHECK(n3->next == n2);
    CHECK(n2->data == &b);
    CHECK(n2->next == n1);
    CHECK(n1->data == &a);
    CHECK(n1->next == NULL);

    count = 0;
    for (cur = n3; cur != NULL; cur = cur->next) {
        count++;
    }
    CHECK(count == 3);

    /* Append inserts a new node directly after the reference node, and
     * returns that same reference node (not the new node). */
    appended = HSD_SListAllocAndAppend(n1, &b);
    CHECK(appended == n1);
    CHECK(n1->next != NULL);
    CHECK(n1->next->data == &b);
    CHECK(n1->next->next == NULL);
    HSD_SListRemove(n1->next);
    n1->next = NULL;

    rest = HSD_SListRemove(n3);
    CHECK(rest == n2);
    rest = HSD_SListRemove(rest);
    CHECK(rest == n1);
    rest = HSD_SListRemove(rest);
    CHECK(rest == NULL);
}

static void test_gobj_create_and_plink_order(void)
{
    HSD_GObj* a = GObj_Create(0x10, 0, 10);
    HSD_GObj* b = GObj_Create(0x11, 0, 5);
    HSD_GObj* c = GObj_Create(0x12, 0, 20);
    HSD_GObj* d = GObj_Create(0x13, 0, 7);
    HSD_GObj* cur;

    CHECK(a != NULL && b != NULL && c != NULL && d != NULL);
    CHECK(plink_count(0) == 4);

    /* GObj_Create inserts each new gobj so HSD_GObjPLinkHead[link] stays
     * sorted ascending by p_priority (numerically lower runs first):
     * b(5) -> d(7) -> a(10) -> c(20). */
    cur = HSD_GObjPLinkHead[0];
    CHECK(cur == b);
    cur = cur->next;
    CHECK(cur == d);
    cur = cur->next;
    CHECK(cur == a);
    cur = cur->next;
    CHECK(cur == c);
    cur = cur->next;
    CHECK(cur == NULL);
    CHECK(plinklow_gobjs[0] == c);

    /* Walking backwards from the tail must reproduce the reverse order. */
    cur = plinklow_gobjs[0];
    CHECK(cur == c);
    cur = cur->prev;
    CHECK(cur == a);
    cur = cur->prev;
    CHECK(cur == d);
    cur = cur->prev;
    CHECK(cur == b);
    cur = cur->prev;
    CHECK(cur == NULL);

    CHECK(HSD_GObjGetClassifier(a) == 0x10);
    CHECK(HSD_GObjGetNext(b) == d);

    HSD_GObjFree(a);
    HSD_GObjFree(b);
    HSD_GObjFree(c);
    HSD_GObjFree(d);
    CHECK(plink_count(0) == 0);
    CHECK(HSD_GObjPLinkHead[0] == NULL);
    CHECK(plinklow_gobjs[0] == NULL);
}

static int g_invoke_count;
static HSD_GObj* g_invoke_gobj;

static void OnProcInvoke(HSD_GObj* gobj)
{
    g_invoke_count++;
    g_invoke_gobj = gobj;
}

static void test_gobjproc_attach_detach(void)
{
    HSD_GObj* gobj = GObj_Create(0x20, 1, 0);
    HSD_GObjProc* p1;
    HSD_GObjProc* p2;

    CHECK(gobj != NULL);

    p1 = HSD_GObj_SetupProc(gobj, OnProcInvoke, 0);
    CHECK(p1 != NULL);
    CHECK(gobj->proc == p1);
    CHECK(p1->gobj == gobj);

    p2 = HSD_GObj_SetupProc(gobj, OnProcInvoke, 0);
    CHECK(p2 != NULL);
    /* Newest proc becomes the head of the gobj's own proc chain. */
    CHECK(gobj->proc == p2);
    CHECK(p2->child == p1);

    g_invoke_count = 0;
    g_invoke_gobj = NULL;
    HSD_GObj_RunProcs();
    CHECK(g_invoke_count == 2);
    CHECK(g_invoke_gobj == gobj);

    HSD_GObjProc_RemoveProc(p1);
    CHECK(gobj->proc == p2);
    CHECK(p2->child == NULL);

    g_invoke_count = 0;
    HSD_GObj_RunProcs();
    CHECK(g_invoke_count == 1);

    HSD_GObjProc_RemoveProc(p2);
    CHECK(gobj->proc == NULL);

    g_invoke_count = 0;
    HSD_GObj_RunProcs();
    CHECK(g_invoke_count == 0);

    HSD_GObjFree(gobj);
}

static int g_userdata_removed;
static void* g_userdata_removed_ptr;

static void OnUserDataRemove(void* data)
{
    g_userdata_removed = 1;
    g_userdata_removed_ptr = data;
}

static void test_userdata_attach_and_destroy(void)
{
    HSD_GObj* gobj = GObj_Create(0x30, 2, 0);
    int payload = 0x4142;

    CHECK(gobj != NULL);
    CHECK(HSD_GObjGetUserData(gobj) == NULL);

    GObj_InitUserData(gobj, 5, OnUserDataRemove, &payload);
    CHECK(HSD_GObjGetUserData(gobj) == &payload);

    g_userdata_removed = 0;
    g_userdata_removed_ptr = NULL;

    /* GObj_RemoveUserData() invokes the remove callback directly. */
    GObj_RemoveUserData(gobj);
    CHECK(g_userdata_removed == 1);
    CHECK(g_userdata_removed_ptr == &payload);
    CHECK(HSD_GObjGetUserData(gobj) == NULL);

    /* Re-attach and let full object destruction trigger removal instead. */
    g_userdata_removed = 0;
    g_userdata_removed_ptr = NULL;
    GObj_InitUserData(gobj, 5, OnUserDataRemove, &payload);
    HSD_GObjProc_RemoveAllProcs(gobj);
    CHECK(plink_count(2) == 1);
    HSD_GObjFree(gobj);
    CHECK(g_userdata_removed == 1);
    CHECK(g_userdata_removed_ptr == &payload);
    CHECK(plink_count(2) == 0);
}

static void test_jobj_kind_gobj(void)
{
    s32 base = hsdJObj.head.nb_exist;
    HSD_GObj* gobj = GObj_Create(0x40, 3, 0);
    void* root = HSD_JObjAlloc();
    void* kid = HSD_JObjAlloc();
    int disp0 = M360_HsdJObjStubDispCount();

    CHECK(gobj != NULL && root != NULL && kid != NULL);
    HSD_JObjAddChild(root, kid);
    HSD_JObjSetFlags(root, 0x40000);
    HSD_GObjObject_80390A70(gobj, HSD_GObj_JObjKind, root);
    CHECK(gobj->hsd_obj == root && gobj->obj_kind == HSD_GObj_JObjKind);
    CHECK(hsdJObj.head.nb_exist == (u32) base + 2);

    HSD_GObj_JObjCallback(gobj, 0);
    CHECK(M360_HsdJObjStubDispCount() == disp0 + 1);
    HSD_GObj_JObjCallback(gobj, 2);
    CHECK(M360_HsdJObjStubDispCount() == disp0 + 1);

    HSD_GObjFree(gobj);
    CHECK(hsdJObj.head.nb_exist == (u32) base);
    CHECK(plink_count(3) == 0);
}

int main(void)
{
    M360_HSD_HeapInit();
    HSD_LogInit();
    HSD_ListInitAllocData();
    HSD_FObjInitAllocData();
    HSD_AObjInitAllocData();
    HSD_RObjInitAllocData();
    HSD_IDSetup();
    HSD_IDInitAllocData();
    HSD_VecInitAllocData();
    HSD_MtxInitAllocData();

    {
        HSD_GObjLibInitDataType init;
        HSD_GObjSetInitDefaults(&init);
        CHECK(init.p_link_max == HSD_GOBJ_PLINK_MAX);
        CHECK(init.gx_link_max == HSD_GOBJ_GX_LINK_MAX);
        CHECK(init.gproc_pri_max == HSD_GOBJPROC_PRI_MAX);
        HSD_GObjInit(&init);
    }

    test_list();
    test_gobj_create_and_plink_order();
    test_gobjproc_attach_detach();
    test_userdata_attach_and_destroy();
    test_jobj_kind_gobj();

    if (fail_count) {
        fprintf(stderr, "[M360][GOBJ] %d check(s) failed\n", fail_count);
        return 1;
    }

    printf("[M360][GOBJ] list/gobj/gobjproc/gobjplink/gobjuserdata host "
           "validation passed\n");
    return 0;
}
