#include <xtl.h>

#include "melee_archive_xdk.h"

/* archive.c is original melee-pc code.  Xenon is 32-bit big-endian like the
 * GameCube, so its on-disc scalar and pointer representation can be used
 * directly; only the unavailable Dolphin headers are replaced here. */
#define _archive_h_
#define _DOLPHIN_OS_H_

typedef unsigned char u8;
typedef unsigned int u32;
typedef signed int s32;

typedef struct DiscU32 {
    u32 v;
} DiscU32;

typedef struct HSD_ArchiveHeader {
    u32 file_size;
    u32 data_size;
    u32 nb_reloc;
    u32 nb_public;
    u32 nb_extern;
    u8 version[4];
    u32 pad[2];
} HSD_ArchiveHeader;

typedef struct HSD_ArchiveRelocationInfo {
    u32 offset;
} HSD_ArchiveRelocationInfo;

typedef struct HSD_ArchivePublicInfo {
    u32 offset;
    u32 symbol;
} HSD_ArchivePublicInfo;

typedef struct HSD_ArchiveExternInfo {
    u32 offset;
    u32 symbol;
} HSD_ArchiveExternInfo;

typedef struct HSD_Archive HSD_Archive;
struct HSD_Archive {
    HSD_ArchiveHeader header;
    u8* data;
    HSD_ArchiveRelocationInfo* reloc_info;
    HSD_ArchivePublicInfo* public_info;
    HSD_ArchiveExternInfo* extern_info;
    char* symbols;
    HSD_Archive* next;
    char* name;
    u32 flags;
    void* top_ptr;
};

#define DP_SET(slot, pointer) \
    ((slot) = static_cast<u32>(reinterpret_cast<size_t>(pointer)))

extern "C" void OSReport(char* format, ...);

extern "C" {
#include "../../upstream/melee-pc/src/sysdolphin/baselib/archive.c"
}

namespace {

HSD_Archive s_titleArchive;
HSD_Archive s_menuArchive;

}

bool M360_ParseHsdArchive(unsigned char* image, unsigned imageSize,
                          char* firstSymbol, size_t firstSymbolCapacity,
                          void** firstRoot)
{
    if (!image || !firstSymbol || firstSymbolCapacity == 0 || !firstRoot)
        return false;

    firstSymbol[0] = '\0';
    *firstRoot = 0;
    if (HSD_ArchiveParse(&s_titleArchive, image, imageSize) != 0 ||
        s_titleArchive.header.nb_public == 0 || !s_titleArchive.public_info ||
        !s_titleArchive.symbols) {
        return false;
    }

    const char* symbol =
        s_titleArchive.symbols + s_titleArchive.public_info[0].symbol;
    strncpy(firstSymbol, symbol, firstSymbolCapacity - 1);
    firstSymbol[firstSymbolCapacity - 1] = '\0';
    *firstRoot = HSD_ArchiveGetPublicAddress(&s_titleArchive, firstSymbol);
    return *firstRoot != 0;
}

void* M360_GetHsdPublic(const char* symbol)
{
    if (!symbol || !s_titleArchive.data || !s_titleArchive.symbols)
        return 0;
    return HSD_ArchiveGetPublicAddress(&s_titleArchive, symbol);
}

bool M360_ParseMenuHsdArchive(unsigned char* image, unsigned imageSize,
                              unsigned* resolvedSymbols)
{
    if (!image || !resolvedSymbols)
        return false;
    *resolvedSymbols = 0;
    if (HSD_ArchiveParse(&s_menuArchive, image, imageSize) != 0 ||
        !s_menuArchive.data || !s_menuArchive.symbols)
        return false;
    static const char* const modelNames[] = {
        "MenMainBack", "MenMainPanel", "MenMainConTop", "MenMainCursor",
        "MenMainConRl", "MenMainCursorRl", "MenMainNmRl",
        "MenMainCursorTr01", "MenMainCursorTr02", "MenMainCursorTr03",
        "MenMainCursorTr04", "MenMainCursorRl01", "MenMainCursorRl02",
        "MenMainCursorRl03", "MenMainCursorRl04", "MenMainCursorRl05",
        "MenMainConIs", "MenMainCursorIs", "MenMainConSs", "MenMainCursorSs"
    };
    static const char* const suffixes[] = {
        "_Top_joint", "_Top_animjoint", "_Top_matanim_joint",
        "_Top_shapeanim_joint"
    };
    char name[80];
    for (unsigned model = 0; model < 20; ++model) {
        for (unsigned part = 0; part < 4; ++part) {
            strcpy(name, modelNames[model]);
            strcat(name, suffixes[part]);
            if (HSD_ArchiveGetPublicAddress(&s_menuArchive, name))
                ++*resolvedSymbols;
        }
    }
    static const char* const sceneSymbols[] = {
        "ScMenMain_cam_int1_camera", "ScMenMain_scene_lights", "ScMenMain_fog"
    };
    for (unsigned i = 0; i < 3; ++i)
        if (HSD_ArchiveGetPublicAddress(&s_menuArchive, sceneSymbols[i]))
            ++*resolvedSymbols;
    return *resolvedSymbols == 83;
}

void* M360_GetMenuHsdPublic(const char* symbol)
{
    if (!symbol || !s_menuArchive.data || !s_menuArchive.symbols)
        return 0;
    return HSD_ArchiveGetPublicAddress(&s_menuArchive, symbol);
}
