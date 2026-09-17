#include "dvd_compat.h"
#include "gcm.h"

#include <dolphin/dvd.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static struct m360_gcm mounted_gcm;
static FILE *mounted_image;
static DVDDiskID mounted_disk_id;

int m360_dvd_mount_image(const char *path)
{
    uint8_t header[sizeof(DVDDiskID)];

    m360_dvd_unmount_image();
    if (!path)
        return -1;
    mounted_image = fopen(path, "rb");
    if (!mounted_image)
        return -1;
    if (fread(header, 1, sizeof(header), mounted_image) != sizeof(header) ||
        m360_gcm_mount(&mounted_gcm, mounted_image) != 0) {
        m360_dvd_unmount_image();
        return -1;
    }
    memcpy(&mounted_disk_id, header, sizeof(mounted_disk_id));
    return 0;
}

void m360_dvd_unmount_image(void)
{
    m360_gcm_unmount(&mounted_gcm);
    if (mounted_image)
        fclose(mounted_image);
    mounted_image = NULL;
    memset(&mounted_disk_id, 0, sizeof(mounted_disk_id));
}

void DVDInit(void)
{
}

s32 DVDConvertPathToEntrynum(const char *path)
{
    struct m360_gcm_file file;

    if (!path || !m360_gcm_find(&mounted_gcm, path, &file))
        return -1;
    return (s32)file.entry_index;
}

BOOL DVDFastOpen(s32 entrynum, DVDFileInfo *file_info)
{
    struct m360_gcm_file file;

    if (entrynum < 0 || !file_info ||
        !m360_gcm_file_by_index(&mounted_gcm, (uint32_t)entrynum, &file))
        return FALSE;
    memset(file_info, 0, sizeof(*file_info));
    file_info->startAddr = file.offset;
    file_info->length = file.size;
    file_info->cb.state = DVD_STATE_END;
    return TRUE;
}

BOOL DVDOpen(const char *file_name, DVDFileInfo *file_info)
{
    s32 entrynum = DVDConvertPathToEntrynum(file_name);
    return entrynum < 0 ? FALSE : DVDFastOpen(entrynum, file_info);
}

BOOL DVDClose(DVDFileInfo *file_info)
{
    if (!file_info)
        return FALSE;
    file_info->cb.state = DVD_STATE_END;
    file_info->cb.userData = NULL;
    return TRUE;
}

s32 DVDReadPrio(DVDFileInfo *file_info, void *addr, s32 length, s32 offset,
                s32 prio)
{
    size_t transferred;
    uint64_t end;

    (void)prio;
    if (!mounted_image || !file_info || !addr || length < 0 || offset < 0)
        return DVD_RESULT_FATAL_ERROR;
    end = (uint64_t)(uint32_t)offset + (uint64_t)(uint32_t)length;
    if ((uint32_t)offset > file_info->length ||
        end >= (uint64_t)file_info->length + DVD_MIN_TRANSFER_SIZE)
        return DVD_RESULT_FATAL_ERROR;

    file_info->cb.command = DVD_COMMAND_READ;
    file_info->cb.state = DVD_STATE_BUSY;
    file_info->cb.offset = (uint32_t)offset;
    file_info->cb.length = (uint32_t)length;
    file_info->cb.addr = addr;
    file_info->cb.transferredSize = 0;
    if (fseek(mounted_image,
              (long)((uint64_t)file_info->startAddr + (uint32_t)offset),
              SEEK_SET) != 0) {
        file_info->cb.state = DVD_STATE_FATAL_ERROR;
        return DVD_RESULT_FATAL_ERROR;
    }
    transferred = fread(addr, 1, (size_t)length, mounted_image);
    file_info->cb.transferredSize = (u32)transferred;
    file_info->cb.currTransferSize = (u32)transferred;
    file_info->cb.state = transferred == (size_t)length
                              ? DVD_STATE_END
                              : DVD_STATE_FATAL_ERROR;
    return transferred == (size_t)length ? (s32)transferred
                                         : DVD_RESULT_FATAL_ERROR;
}

BOOL DVDReadAsyncPrio(DVDFileInfo *file_info, void *addr, s32 length,
                      s32 offset, DVDCallback callback, s32 prio)
{
    s32 result;

    if (!file_info)
        return FALSE;
    result = DVDReadPrio(file_info, addr, length, offset, prio);
    file_info->callback = callback;
    if (callback)
        callback(result, file_info);
    return result < 0 ? FALSE : TRUE;
}

s32 DVDGetFileInfoStatus(const DVDFileInfo *file_info)
{
    return file_info ? file_info->cb.state : DVD_STATE_FATAL_ERROR;
}

s32 DVDGetTransferredSize(DVDFileInfo *file_info)
{
    return file_info ? (s32)file_info->cb.transferredSize : 0;
}

DVDDiskID *DVDGetCurrentDiskID(void)
{
    return mounted_image ? &mounted_disk_id : NULL;
}

BOOL DVDCheckDisk(void)
{
    return mounted_image ? TRUE : FALSE;
}
