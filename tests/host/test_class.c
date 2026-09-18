#include "hsd_class_xdk_compat.h"

#include "class.h"
#include "hash.h"
#include "id.h"
#include "memory.h"
#include "objalloc.h"
#include "object.h"

#include <stdio.h>
#include <string.h>

extern void M360_HSD_HeapInit(void);

static int fail_count = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            fprintf(stderr, "[M360][CLASS][FAIL] %s (%s:%d)\n", #cond,       \
                    __FILE__, __LINE__);                                     \
            fail_count++;                                                    \
        }                                                                    \
    } while (0)

static void test_class_hierarchy(void)
{
    void* obj = hsdNew(&hsdObj);
    CHECK(obj != NULL);
    CHECK(HSD_CLASS_METHOD(obj) == &hsdObj);
    CHECK(hsdIsDescendantOf(&hsdObj, &hsdClass));
    CHECK(hsdObjIsDescendantOf((HSD_Obj*) obj, &hsdClass));
    CHECK(hsdObj.head.nb_exist >= 1);
    hsdDelete(obj);
}

static int GetFixedIdx(HSD_Hash* hash)
{
    (void) hash;
    return 0;
}

static bool KeyCheckStr(HSD_Hash* hash, void* table_key, void* key)
{
    /* hash.c treats a keycheck() return of 0 as a match, mirroring
     * strcmp()'s convention, despite the `bool` return type. */
    (void) hash;
    return (bool) strcmp((const char*) table_key, (const char*) key);
}

static void test_hash_search(void)
{
    HSD_HashClassInfo hashInfo;
    HSD_HashEntry entryA;
    HSD_HashEntry entryB;
    HSD_HashEntry* table[1];
    HSD_Hash hash;
    int success = -1;

    memset(&hashInfo, 0, sizeof(hashInfo));
    hashInfo.getidx = GetFixedIdx;
    hashInfo.keycheck = KeyCheckStr;

    entryA.next = &entryB;
    entryA.key = (void*) "alpha";
    entryA.value = (void*) 0x1001;
    entryB.next = NULL;
    entryB.key = (void*) "beta";
    entryB.value = (void*) 0x1002;
    table[0] = &entryA;

    hash.parent.class_info = &hashInfo;
    hash.table = table;
    hash.table_size = 1;

    HSD_HashClassInfo* found =
        HSD_HashSearch(&hash, (void*) "beta", &success);
    CHECK(found == (HSD_HashClassInfo*) 0x1002);
    CHECK(success == 1);

    found = HSD_HashSearch(&hash, (void*) "missing", &success);
    CHECK(found == NULL);
    CHECK(success == 0);
}

static void test_objalloc(void)
{
    /* HSD_ObjAllocInit() threads every pool onto a process-lifetime global
     * list (alloc_datas) for later bulk teardown, so the HSD_ObjAllocData
     * itself must outlive the function, not live on the stack. */
    static HSD_ObjAllocData data;
    void* slots[8];
    int i;

    HSD_ObjAllocInit(&data, 24, 4);
    CHECK(HSD_ObjAllocGetUsing(&data) == 0);

    for (i = 0; i < 8; i++) {
        slots[i] = HSD_ObjAlloc(&data);
        CHECK(slots[i] != NULL);
    }
    CHECK(HSD_ObjAllocGetUsing(&data) == 8);
    CHECK(HSD_ObjAllocGetPeak(&data) == 8);

    for (i = 0; i < 8; i++) {
        HSD_ObjFree(&data, slots[i]);
    }
    CHECK(HSD_ObjAllocGetUsing(&data) == 0);
    CHECK(HSD_ObjAllocGetFreed(&data) == 8);
}

static void test_id_table(void)
{
    int a = 1, b = 2, c = 3;
    void* out;
    s32 success = -1;

    HSD_IDSetup();
    HSD_IDInitAllocData();

    HSD_IDInsertToTable(NULL, 101, &a);
    HSD_IDInsertToTable(NULL, 202, &b);
    HSD_IDInsertToTable(NULL, 303, &c);

    out = HSD_IDGetData(202, &success);
    CHECK(out == &b);
    CHECK(success == 1);

    HSD_IDRemoveByIDFromTable(NULL, 202);
    out = HSD_IDGetData(202, &success);
    CHECK(out == NULL);
    CHECK(success == 0);

    out = HSD_IDGetData(303, &success);
    CHECK(out == &c);
    CHECK(success == 1);
}

int main(void)
{
    M360_HSD_HeapInit();
    HSD_LogInit();

    test_class_hierarchy();
    test_hash_search();
    test_objalloc();
    test_id_table();

    if (fail_count) {
        fprintf(stderr, "[M360][CLASS] %d check(s) failed\n", fail_count);
        return 1;
    }

    printf("[M360][CLASS] hash/debug/class/object/objalloc/id host "
           "validation passed\n");
    return 0;
}
