#include "hsdanim_xdk_compat.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
#include "aobj.h"
#include "fobj.h"
#include "tobj.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gcm.h"
#include "hsd_texture_xdk.h"

extern void M360_HSD_HeapInit(void);

static int g_failures;

#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            ++g_failures;                                                      \
            printf("[M360][MENU][FAIL] %s (%s:%d)\n", #cond, __FILE__,         \
                   __LINE__);                                                  \
        }                                                                      \
    } while (0)

static unsigned char* g_file;
static unsigned g_fileSize;
static unsigned g_dataSize;
static unsigned g_publicCount;
static unsigned g_publicTable;
static unsigned g_stringTable;

static unsigned Be32(unsigned offset)
{
    const unsigned char* p = g_file + offset;
    return ((unsigned) p[0] << 24) | ((unsigned) p[1] << 16) |
           ((unsigned) p[2] << 8) | p[3];
}

static unsigned Be16(unsigned offset)
{
    const unsigned char* p = g_file + offset;
    return ((unsigned) p[0] << 8) | p[1];
}

static float BeF32(unsigned offset)
{
    union {
        unsigned u;
        float f;
    } v;
    v.u = Be32(offset);
    return v.f;
}

static unsigned Data(unsigned offset) { return 0x20 + offset; }
static unsigned U32(unsigned dataOffset) { return Be32(Data(dataOffset)); }

static unsigned Symbol(const char* name)
{
    unsigned i;
    for (i = 0; i < g_publicCount; ++i) {
        const unsigned entry = g_publicTable + i * 8;
        const char* s = (const char*) g_file + g_stringTable + Be32(entry + 4);
        if (strcmp(s, name) == 0)
            return Be32(entry);
    }
    return 0xFFFFFFFFu;
}

static unsigned Crc(const unsigned char* p, size_t n, unsigned crc)
{
    size_t i;
    int k;
    crc = ~crc;
    for (i = 0; i < n; ++i) {
        crc ^= p[i];
        for (k = 0; k < 8; ++k)
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

static void Put32(FILE* f, unsigned v)
{
    fputc((int) (v >> 24), f);
    fputc((int) ((v >> 16) & 255), f);
    fputc((int) ((v >> 8) & 255), f);
    fputc((int) (v & 255), f);
}

static void Chunk(FILE* f, const char* type, const unsigned char* data, unsigned n)
{
    unsigned crc = Crc((const unsigned char*) type, 4, 0);
    crc = Crc(data, n, crc);
    Put32(f, n);
    fwrite(type, 1, 4, f);
    fwrite(data, 1, n, f);
    Put32(f, crc);
}

static int WritePng(const char* path, const unsigned* argb, unsigned w, unsigned h)
{
    const unsigned rowBytes = w * 4 + 1;
    const unsigned raw = rowBytes * h;
    const unsigned blocks = (raw + 65534) / 65535;
    unsigned char* z = (unsigned char*) malloc(raw + blocks * 5 + 6);
    unsigned char* rows = (unsigned char*) malloc(raw);
    unsigned char header[13];
    unsigned a = 1, b = 0, pos = 0, i, x, y;
    FILE* f;
    if (!z || !rows)
        return 0;
    for (y = 0; y < h; ++y) {
        rows[y * rowBytes] = 0;
        for (x = 0; x < w; ++x) {
            const unsigned c = argb[y * w + x];
            unsigned char* p = rows + y * rowBytes + 1 + x * 4;
            p[0] = (unsigned char) (c >> 16);
            p[1] = (unsigned char) (c >> 8);
            p[2] = (unsigned char) c;
            p[3] = (unsigned char) (c >> 24);
        }
    }
    for (i = 0; i < raw; ++i) {
        a = (a + rows[i]) % 65521;
        b = (b + a) % 65521;
    }
    z[pos++] = 0x78;
    z[pos++] = 0x01;
    for (i = 0; i < raw; i += 65535) {
        const unsigned n = raw - i < 65535 ? raw - i : 65535;
        z[pos++] = (unsigned char) (i + n >= raw);
        z[pos++] = (unsigned char) (n & 255);
        z[pos++] = (unsigned char) (n >> 8);
        z[pos++] = (unsigned char) (~n & 255);
        z[pos++] = (unsigned char) ((~n >> 8) & 255);
        memcpy(z + pos, rows + i, n);
        pos += n;
    }
    z[pos++] = (unsigned char) (b >> 8);
    z[pos++] = (unsigned char) b;
    z[pos++] = (unsigned char) (a >> 8);
    z[pos++] = (unsigned char) a;
    f = fopen(path, "wb");
    if (!f) {
        free(z);
        free(rows);
        return 0;
    }
    fwrite("\x89PNG\r\n\x1a\n", 1, 8, f);
    header[0] = (unsigned char) (w >> 24); header[1] = (unsigned char) (w >> 16);
    header[2] = (unsigned char) (w >> 8); header[3] = (unsigned char) w;
    header[4] = (unsigned char) (h >> 24); header[5] = (unsigned char) (h >> 16);
    header[6] = (unsigned char) (h >> 8); header[7] = (unsigned char) h;
    header[8] = 8; header[9] = 6; header[10] = 0; header[11] = 0; header[12] = 0;
    Chunk(f, "IHDR", header, 13);
    Chunk(f, "IDAT", z, pos);
    Chunk(f, "IEND", NULL, 0);
    fclose(f);
    free(z);
    free(rows);
    return 1;
}

static int DecodeImage(unsigned image, unsigned tlut, unsigned** out,
                       unsigned* width, unsigned* height)
{
    const unsigned w = Be16(Data(image) + 4), h = Be16(Data(image) + 6);
    const unsigned format = U32(image + 8);
    const unsigned char* palette = tlut ? g_file + Data(U32(tlut)) : NULL;
    const unsigned paletteFormat = tlut ? U32(tlut + 4) : 0;
    const unsigned entries = tlut ? Be16(Data(tlut) + 12) : 0;
    const unsigned size = M360_GxTextureSize(w, h, format);
    unsigned* pixels;
    if (!w || !h || Data(U32(image)) + size > g_fileSize)
        return 0;
    pixels = (unsigned*) calloc(w * h, sizeof(unsigned));
    if (!pixels)
        return 0;
    if (!M360_DecodeGxTexture(g_file + Data(U32(image)), w, h, format, palette,
                              paletteFormat, entries, pixels)) {
        free(pixels);
        return 0;
    }
    *out = pixels;
    *width = w;
    *height = h;
    return 1;
}

static unsigned g_seenImages[1024];
static unsigned g_seenCount;
static unsigned g_formatCounts[16];
static unsigned g_decodeFailures;
static unsigned g_opaqueImages;

static void VisitTObj(unsigned tobj)
{
    for (; tobj; tobj = U32(tobj + 4)) {
        const unsigned image = U32(tobj + 0x4C);
        const unsigned tlut = U32(tobj + 0x50);
        unsigned i, *pixels = NULL, w = 0, h = 0, covered = 0;
        if (!image)
            continue;
        for (i = 0; i < g_seenCount; ++i)
            if (g_seenImages[i] == image)
                break;
        if (i < g_seenCount || g_seenCount == 1024)
            continue;
        g_seenImages[g_seenCount++] = image;
        ++g_formatCounts[U32(image + 8) & 15];
        if (!DecodeImage(image, tlut, &pixels, &w, &h)) {
            ++g_decodeFailures;
            continue;
        }
        for (i = 0; i < w * h; ++i)
            covered += (pixels[i] >> 24) != 0;
        g_opaqueImages += covered > 0;
        free(pixels);
    }
}

static void VisitJoint(unsigned joint, int depth)
{
    for (; joint && depth < 64; joint = U32(joint + 12)) {
        const unsigned flags = U32(joint + 4);
        if (!(flags & ((1u << 14) | (1u << 5)))) {
            unsigned dobj;
            for (dobj = U32(joint + 16); dobj; dobj = U32(dobj + 4)) {
                const unsigned mobj = U32(dobj + 8);
                if (mobj)
                    VisitTObj(U32(mobj + 8));
            }
        }
        if (!(flags & (1u << 12)))
            VisitJoint(U32(joint + 8), depth + 1);
    }
}

static unsigned NodeByIndex(unsigned root, unsigned target)
{
    unsigned stack[64], sp = 0, node = root, index = 0;
    while (node) {
        if (index == target)
            return node;
        ++index;
        if (U32(node)) {
            if (sp < 64)
                stack[sp++] = node;
            node = U32(node);
            continue;
        }
        while (node && !U32(node + 4))
            node = sp ? stack[--sp] : 0;
        node = node ? U32(node + 4) : 0;
    }
    return 0;
}

typedef struct TimgProbe {
    int calls;
    float value;
} TimgProbe;

static void TimgUpdate(void* obj, enum_t type, HSD_ObjData* data)
{
    TimgProbe* probe = (TimgProbe*) obj;
    if (type == HSD_A_T_TIMG) {
        ++probe->calls;
        probe->value = data->fv;
    }
}

static HSD_FObjDesc g_fobjs[32];
static HSD_AObjDesc g_aobj;

static HSD_AObjDesc* BuildAObj(unsigned aobj)
{
    unsigned fobj = U32(aobj + 8), n = 0;
    g_aobj.flags = U32(aobj);
    g_aobj.end_frame = BeF32(Data(aobj + 4));
    g_aobj.obj_id = U32(aobj + 12);
    g_aobj.fobjdesc = fobj ? &g_fobjs[0] : NULL;
    for (; fobj && n < 32; fobj = U32(fobj), ++n) {
        g_fobjs[n].next = U32(fobj) && n + 1 < 32 ? &g_fobjs[n + 1] : NULL;
        g_fobjs[n].length = U32(fobj + 4);
        g_fobjs[n].startframe = BeF32(Data(fobj + 8));
        g_fobjs[n].type = g_file[Data(fobj + 12)];
        g_fobjs[n].frac_value = g_file[Data(fobj + 13)];
        g_fobjs[n].frac_slope = g_file[Data(fobj + 14)];
        g_fobjs[n].dummy0 = 0;
        g_fobjs[n].ad = g_file + Data(U32(fobj + 16));
    }
    return &g_aobj;
}

static void TestLabels(const char* outDir)
{
    static const unsigned kinds[6] = { 0, 1, 2, 3, 4, 5 };
    static const unsigned counts[6] = { 5, 5, 5, 4, 6, 5 };
    static const char* const names[6] = { "main", "1p", "vs", "trophies", "options", "data" };
    const unsigned matRoot = Symbol("MenMainCursor_Top_matanim_joint");
    unsigned labelMat, texanim = 0, matanim, k;
    HSD_AObj* aobj;
    CHECK(matRoot != 0xFFFFFFFFu);
    if (matRoot == 0xFFFFFFFFu)
        return;
    labelMat = NodeByIndex(matRoot, 3);
    CHECK(labelMat != 0);
    for (matanim = labelMat ? U32(labelMat + 8) : 0; matanim && !texanim;
         matanim = U32(matanim))
        if (U32(matanim + 8) && U32(U32(matanim + 8) + 12))
            texanim = U32(matanim + 8);
    CHECK(texanim != 0);
    if (!texanim)
        return;
    CHECK(Be16(Data(texanim) + 0x14) >= 40);
    aobj = HSD_AObjLoadDesc(BuildAObj(U32(texanim + 8)));
    CHECK(aobj != NULL);
    if (!aobj)
        return;
    for (k = 0; k < 6; ++k) {
        unsigned previous = 0xFFFFFFFFu, s;
        for (s = 0; s < counts[k]; ++s) {
            TimgProbe probe = { 0, -1.0f };
            const float frame = (float) (kinds[k] * 20 + s * 2);
            unsigned index, image, *pixels = NULL, w = 0, h = 0;
            HSD_AObjReqAnim(aobj, frame);
            HSD_AObjInterpretAnim(aobj, &probe, TimgUpdate);
            CHECK(probe.calls == 1);
            index = (unsigned) probe.value;
            printf("[M360][MENU] label %s[%u] frame %.0f -> image %u\n",
                   names[k], s, frame, index);
            CHECK(index < Be16(Data(texanim) + 0x14));
            CHECK(index != previous);
            previous = index;
            image = U32(U32(texanim + 12) + index * 4);
            CHECK(image != 0);
            if (image && DecodeImage(image, 0, &pixels, &w, &h)) {
                char path[512];
                sprintf(path, "%s/label_%s_%u.png", outDir, names[k], s);
                CHECK(WritePng(path, pixels, w, h));
                free(pixels);
            } else {
                CHECK(!"label decode");
            }
        }
    }
    HSD_AObjRemove(aobj);
}

int main(int argc, char** argv)
{
    const char* isoPath = argc > 1 ? argv[1] : "dist/melee.iso";
    const char* outDir = argc > 2 ? argv[2] : ".";
    static const char* const models[] = {
        "MenMainBack", "MenMainPanel", "MenMainConTop", "MenMainCursor",
        "MenMainConRl", "MenMainCursorRl", "MenMainNmRl", "MenMainCursorTr01",
        "MenMainCursorTr02", "MenMainCursorTr03", "MenMainCursorTr04",
        "MenMainCursorRl01", "MenMainCursorRl02", "MenMainCursorRl03",
        "MenMainCursorRl04", "MenMainCursorRl05", "MenMainConIs",
        "MenMainCursorIs", "MenMainConSs", "MenMainCursorSs"
    };
    struct m360_gcm gcm;
    struct m360_gcm_file file;
    FILE* iso = fopen(isoPath, "rb");
    unsigned i;
    if (!iso) {
        printf("[M360][MENU][ERROR] ISO not found: %s\n", isoPath);
        return 1;
    }
    CHECK(m360_gcm_mount(&gcm, iso) == 0);
    CHECK(m360_gcm_find(&gcm, "MnMaAll.usd", &file) != 0);
    g_fileSize = file.size;
    g_file = (unsigned char*) malloc(file.size);
    CHECK(g_file && m360_gcm_read(&gcm, &file, 0, g_file, file.size) == file.size);
    if (g_failures) {
        printf("[M360][MENU] %d check(s) failed\n", g_failures);
        return 1;
    }
    g_dataSize = Be32(4);
    g_publicCount = Be32(12);
    g_publicTable = 0x20 + g_dataSize + Be32(8) * 4;
    g_stringTable = g_publicTable + (g_publicCount + Be32(16)) * 8;
    CHECK(g_fileSize == 2155311);

    M360_HSD_HeapInit();
    HSD_FObjInitAllocData();
    HSD_AObjInitAllocData();
    for (i = 0; i < sizeof(models) / sizeof(models[0]); ++i) {
        char name[80];
        unsigned root;
        sprintf(name, "%s_Top_joint", models[i]);
        root = Symbol(name);
        CHECK(root != 0xFFFFFFFFu);
        if (root != 0xFFFFFFFFu)
            VisitJoint(root, 0);
    }
    printf("[M360][MENU] %u distinct images: I4 %u I8 %u IA4 %u CI4 %u CI8 %u CMPR %u\n",
           g_seenCount, g_formatCounts[0], g_formatCounts[1], g_formatCounts[2],
           g_formatCounts[8], g_formatCounts[9], g_formatCounts[14]);
    CHECK(g_seenCount >= 150);
    CHECK(g_decodeFailures == 0);
    CHECK(g_opaqueImages == g_seenCount);
    CHECK(g_formatCounts[8] && g_formatCounts[9] && g_formatCounts[14]);

    TestLabels(outDir);

    m360_gcm_unmount(&gcm);
    fclose(iso);
    free(g_file);
    if (g_failures) {
        printf("[M360][MENU] %d check(s) failed\n", g_failures);
        return 1;
    }
    printf("[M360][MENU] menu texture decode and label TObj animation passed\n");
    return 0;
}
