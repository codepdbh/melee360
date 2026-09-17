#ifndef MELEE360_DISC_COMPAT_H
#define MELEE360_DISC_COMPAT_H

/*
 * melee-pc marks on-disc structures with GCC's scalar_storage_order
 * attribute because desktop CPUs are little-endian.  Xenon's PowerPC CPU is
 * already big-endian and its pointers are 32-bit, matching the original
 * GameCube archive layout.  The old Free60 GCC does not understand that
 * newer attribute, so provide the original-console form of the macros.
 *
 * Defining PC_DISC_H also prevents pc/disc.h from replacing these native
 * definitions if a later upstream header includes it.
 */
#define PC_DISC_H

#include <stdint.h>

#define DISC_STRUCT
#define DISC_PTR(T) T *
#define DP(T, slot) (slot)
#define DP_SET(slot, pointer) \
    ((slot) = (__typeof__(slot))(uintptr_t)(pointer))
#define DISC_ASSERT_SIZE(T, size)
#define PC_IS_ARAM_ADDR(address) ((uint32_t)(uintptr_t)(address) < 0x80000000u)

typedef struct { float v; } DiscF32;
typedef struct { uint32_t v; } DiscU32;
typedef struct { int32_t v; } DiscS32;
typedef struct { uint16_t v; } DiscU16;
typedef struct { int16_t v; } DiscS16;
typedef struct { float x, y; } DiscVec2;
typedef struct { float x, y, z; } DiscVec3;
typedef struct { float x, y, z, w; } DiscVec4;
typedef struct { int16_t x, y, z; } DiscS16Vec3;
typedef struct { float m[3][4]; } DiscMtx;

#define DISC_VEC3_GET(dst, src) \
    ((dst).x = (src).x, (dst).y = (src).y, (dst).z = (src).z)
#define DISC_VEC3_SET(dst, src) DISC_VEC3_GET(dst, src)

#endif
