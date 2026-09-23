#include <xtl.h>

#include "melee_boot_xdk.h"
#include "melee_archive_xdk.h"
#include "melee_title_scene_xdk.h"
extern "C" {
#include "../common/gcm.h"
}

namespace {

unsigned char* s_titleArchiveImage = 0;
char s_isoPath[260];

unsigned ReadBE32(const unsigned char* bytes)
{
    return (static_cast<unsigned>(bytes[0]) << 24) |
           (static_cast<unsigned>(bytes[1]) << 16) |
           (static_cast<unsigned>(bytes[2]) << 8) |
           static_cast<unsigned>(bytes[3]);
}

unsigned Expand3(unsigned value)
{
    return (value << 5) | (value << 2) | (value >> 1);
}

unsigned Expand4(unsigned value)
{
    return (value << 4) | value;
}

unsigned Expand5(unsigned value)
{
    return (value << 3) | (value >> 2);
}

void DecodeBanner(const unsigned char* source, unsigned* destination)
{
    for (unsigned y = 0; y < 32; ++y) {
        for (unsigned x = 0; x < 96; ++x) {
            const unsigned tile = (y / 4) * 24 + x / 4;
            const unsigned inside = (y & 3) * 4 + (x & 3);
            const unsigned offset = (tile * 16 + inside) * 2;
            const unsigned pixel =
                (static_cast<unsigned>(source[offset]) << 8) |
                source[offset + 1];
            unsigned alpha;
            unsigned red;
            unsigned green;
            unsigned blue;
            if (pixel & 0x8000) {
                alpha = 255;
                red = Expand5((pixel >> 10) & 31);
                green = Expand5((pixel >> 5) & 31);
                blue = Expand5(pixel & 31);
            } else {
                alpha = Expand3((pixel >> 12) & 7);
                red = Expand4((pixel >> 8) & 15);
                green = Expand4((pixel >> 4) & 15);
                blue = Expand4(pixel & 15);
            }
            destination[y * 96 + x] =
                D3DCOLOR_ARGB(alpha, red, green, blue);
        }
    }
}

void SetError(MeleeBootStatus* status, const char* message)
{
    strncpy(status->error, message, sizeof(status->error) - 1);
    status->error[sizeof(status->error) - 1] = '\0';
    OutputDebugStringA("[M360][BOOT] ");
    OutputDebugStringA(message);
    OutputDebugStringA("\n");
}

} // namespace

bool M360_BootMelee(const char* isoPath, MeleeBootStatus* status)
{
    if (!status)
        return false;
    ZeroMemory(status, sizeof(*status));
    strncpy(s_isoPath, isoPath, sizeof(s_isoPath) - 1);

    FILE* image = fopen(isoPath, "rb");
    if (!image) {
        SetError(status, "game:\\melee.iso not found");
        return false;
    }
    status->isoOpened = true;

    unsigned char discId[6];
    if (fread(discId, 1, sizeof(discId), image) != sizeof(discId)) {
        SetError(status, "could not read disc ID");
        fclose(image);
        return false;
    }
    memcpy(status->gameId, discId, 6);
    status->gameId[6] = '\0';
    status->discValid = memcmp(status->gameId, "GALE01", 6) == 0;
    if (!status->discValid) {
        SetError(status, "disc is not GALE01");
        fclose(image);
        return false;
    }

    struct m360_gcm gcm;
    if (m360_gcm_mount(&gcm, image) != 0) {
        SetError(status, "GameCube FST mount failed");
        fclose(image);
        return false;
    }
    status->fstMounted = true;
    status->entryCount = gcm.entry_count;

    struct m360_gcm_file bannerFile;
    unsigned char encodedBanner[96 * 32 * 2];
    if (m360_gcm_find(&gcm, "opening.bnr", &bannerFile) &&
        m360_gcm_read(&gcm, &bannerFile, 0x20, encodedBanner,
                      sizeof(encodedBanner)) == sizeof(encodedBanner)) {
        DecodeBanner(encodedBanner, status->bannerPixels);
        status->bannerDecoded = true;
        if (m360_gcm_read(&gcm, &bannerFile, 0x1860, status->title, 64) == 64)
            status->title[64] = '\0';
    }

    struct m360_gcm_file archiveFile;
    struct m360_gcm_file languageFile;
    status->languageUS = m360_gcm_find(&gcm, "usa.ini", &languageFile) != 0;
    if (m360_gcm_find(&gcm, status->languageUS ? "GmTtAll.usd" : "GmTtAll.dat",
                      &archiveFile)) {
        status->titleArchiveFound = true;
        status->titleArchiveSize = archiveFile.size;
        unsigned char header[0x20];
        if (m360_gcm_read(&gcm, &archiveFile, 0, header, sizeof(header)) ==
            sizeof(header)) {
            const unsigned fileSize = ReadBE32(header);
            const unsigned dataSize = ReadBE32(header + 4);
            status->relocationCount = ReadBE32(header + 8);
            status->publicCount = ReadBE32(header + 12);
            status->externalCount = ReadBE32(header + 16);
            const unsigned publicOffset =
                0x20 + dataSize + status->relocationCount * 4;
            const unsigned symbolOffset = publicOffset +
                status->publicCount * 8 + status->externalCount * 8;
            if (fileSize == archiveFile.size && status->publicCount &&
                symbolOffset < archiveFile.size) {
                s_titleArchiveImage = static_cast<unsigned char*>(
                    malloc(archiveFile.size));
                if (s_titleArchiveImage &&
                    m360_gcm_read(&gcm, &archiveFile, 0,
                                  s_titleArchiveImage, archiveFile.size) ==
                        archiveFile.size) {
                    void* firstRoot = 0;
                    status->titleArchiveRelocated = M360_ParseHsdArchive(
                        s_titleArchiveImage, archiveFile.size,
                        status->firstPublicSymbol,
                        sizeof(status->firstPublicSymbol), &firstRoot);
                    status->titlePublicRootResolved = firstRoot != 0;
                    status->titleArchiveValid =
                        status->titleArchiveRelocated &&
                        status->titlePublicRootResolved;
                    if (status->titleArchiveValid)
                        M360_LoadTitleScene(&status->titleScene);
                }
            }
        }
    }

    if (!status->bannerDecoded)
        SetError(status, "opening.bnr decode failed");
    else if (!status->titleArchiveValid)
        SetError(status, "GmTtAll validation failed");

    struct m360_gcm_file menuFile;
    if (m360_gcm_find(&gcm, status->languageUS ? "MnMaAll.usd" : "MnMaAll.dat",
                      &menuFile)) {
        status->menuArchiveFound = true;
        status->menuArchiveSize = menuFile.size;
        if (menuFile.size >= 0x20 && menuFile.size <= 8 * 1024 * 1024) {
            unsigned char* menuImage = static_cast<unsigned char*>(malloc(menuFile.size));
            if (menuImage &&
                m360_gcm_read(&gcm, &menuFile, 0, menuImage, menuFile.size) ==
                    menuFile.size) {
                status->menuArchiveValid = M360_ParseMenuHsdArchive(
                    menuImage, menuFile.size, &status->menuSymbolsResolved);
            }
            if (!status->menuArchiveValid)
                free(menuImage);
        }
    }
    m360_gcm_unmount(&gcm);
    fclose(image);
    return status->discValid && status->fstMounted &&
           status->bannerDecoded && status->titleArchiveValid;
}

extern "C" unsigned char* M360_ReadDiscFile(const char* name, unsigned* size)
{
    *size = 0;
    FILE* image = fopen(s_isoPath, "rb");
    if (!image)
        return 0;
    struct m360_gcm gcm;
    unsigned char* data = 0;
    if (m360_gcm_mount(&gcm, image) == 0) {
        struct m360_gcm_file file;
        if (m360_gcm_find(&gcm, name, &file) && file.size) {
            data = static_cast<unsigned char*>(_aligned_malloc(file.size, 32));
            if (data && m360_gcm_read(&gcm, &file, 0, data, file.size) == file.size) {
                *size = file.size;
            } else {
                _aligned_free(data);
                data = 0;
            }
        }
        m360_gcm_unmount(&gcm);
    }
    fclose(image);
    return data;
}
