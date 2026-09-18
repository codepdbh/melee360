#ifndef MELEE360_XDK_HSD_CLASS_COMPAT_H
#define MELEE360_XDK_HSD_CLASS_COMPAT_H

/* Keep hash.c/debug.c/class.c/object.c/objalloc.c/id.c untouched while
 * replacing the Dolphin OS/runtime headers they transitively include. */
#define RUNTIME_PLATFORM_H
#define _DOLPHIN_OS_H_
#define _DOLPHIN_OSALLOC_H_
#define _initialize_h_

#include <stddef.h>
#include <stdint.h>

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned long long u64;
typedef signed long long s64;
typedef float f32;
typedef double f64;
typedef signed int ssize_t;
typedef int enum_t;

#ifndef __cplusplus
typedef int bool;
#define true 1
#define false 0
#define inline __inline
/* The XDK's VC-era C front end predates C11 _Alignof; __alignof has been
 * an MSVC extension since long before that. */
#define _Alignof __alignof
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void*) 0)
#endif
#endif

#define ATTRIBUTE_NORETURN __declspec(noreturn)
#define ASSERT_SIZE(expr, size)
#define ASSERT_OFFSET(type, member, offset)

typedef int OSHeapHandle;

typedef struct OSContext {
    unsigned char reserved[16];
} OSContext;

#define OSRoundUp32B(x) (((u32) (x) + 32 - 1) & ~(u32) (32 - 1))
#define OSRoundDown32B(x) (((u32) (x)) & ~(u32) (32 - 1))

#ifdef __cplusplus
extern "C" {
#endif

void OSReport(char* fmt, ...);
ATTRIBUTE_NORETURN void OSPanic(char* file, int line, char* msg, ...);
void OSSaveContext(OSContext* context);

OSHeapHandle HSD_GetHeap(void);
void* OSAllocFromHeap(OSHeapHandle heap, u32 size);
void OSFreeToHeap(OSHeapHandle heap, void* ptr);
long OSCheckHeap(int heap);

#ifdef __cplusplus
}
#endif

#endif
