#include "gcm.h"

#include <stdlib.h>
#include <string.h>

#define GCM_FST_OFFSET 0x424u
#define GCM_FST_SIZE 0x428u
#define GCM_ENTRY_SIZE 12u
#define GCM_MAX_FST_SIZE (32u * 1024u * 1024u)

static uint32_t read_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static const uint8_t *entry_at(const struct m360_gcm *gcm, uint32_t index)
{
    if (index >= gcm->entry_count)
        return NULL;
    return gcm->fst + index * GCM_ENTRY_SIZE;
}

static const char *entry_name(const struct m360_gcm *gcm,
                              const uint8_t *entry)
{
    uint32_t table_offset = gcm->entry_count * GCM_ENTRY_SIZE;
    uint32_t name_offset = read_be32(entry) & 0x00FFFFFFu;
    const char *name;
    size_t remaining;

    if (table_offset > gcm->fst_size ||
        name_offset >= gcm->fst_size - table_offset)
        return NULL;
    name = (const char *)(gcm->fst + table_offset + name_offset);
    remaining = gcm->fst_size - table_offset - name_offset;
    return memchr(name, '\0', remaining) ? name : NULL;
}

int m360_gcm_mount(struct m360_gcm *gcm, FILE *image)
{
    uint8_t fields[8];
    uint32_t fst_offset;
    const uint8_t *root;

    if (!gcm || !image)
        return -1;
    memset(gcm, 0, sizeof(*gcm));
    if (fseek(image, GCM_FST_OFFSET, SEEK_SET) != 0 ||
        fread(fields, 1, sizeof(fields), image) != sizeof(fields))
        return -1;

    fst_offset = read_be32(fields);
    gcm->fst_size = read_be32(fields + 4);
    if (gcm->fst_size < GCM_ENTRY_SIZE ||
        gcm->fst_size > GCM_MAX_FST_SIZE)
        return -1;

    gcm->fst = malloc(gcm->fst_size);
    if (!gcm->fst)
        return -1;
    if (fseek(image, (long)fst_offset, SEEK_SET) != 0 ||
        fread(gcm->fst, 1, gcm->fst_size, image) != gcm->fst_size) {
        m360_gcm_unmount(gcm);
        return -1;
    }

    root = gcm->fst;
    gcm->entry_count = read_be32(root + 8);
    if ((read_be32(root) >> 24) == 0 || gcm->entry_count == 0 ||
        gcm->entry_count > gcm->fst_size / GCM_ENTRY_SIZE) {
        m360_gcm_unmount(gcm);
        return -1;
    }
    gcm->image = image;
    return 0;
}

void m360_gcm_unmount(struct m360_gcm *gcm)
{
    if (!gcm)
        return;
    free(gcm->fst);
    memset(gcm, 0, sizeof(*gcm));
}

int m360_gcm_find(const struct m360_gcm *gcm, const char *path,
                  struct m360_gcm_file *file)
{
    uint32_t directory = 0;
    const char *part = path;

    if (!gcm || !gcm->fst || !path || !file)
        return 0;
    while (*part == '/')
        ++part;
    while (*part) {
        const char *slash = strchr(part, '/');
        size_t part_len = slash ? (size_t)(slash - part) : strlen(part);
        const uint8_t *dir_entry = entry_at(gcm, directory);
        uint32_t end = read_be32(dir_entry + 8);
        uint32_t index = directory + 1;
        int found = 0;

        while (index < end) {
            const uint8_t *entry = entry_at(gcm, index);
            uint32_t word0 = read_be32(entry);
            int is_directory = (word0 >> 24) != 0;
            const char *name = entry_name(gcm, entry);

            if (!name)
                return 0;
            if (strlen(name) == part_len && memcmp(name, part, part_len) == 0) {
                if (slash) {
                    if (!is_directory)
                        return 0;
                    directory = index;
                    part = slash + 1;
                    while (*part == '/')
                        ++part;
                    found = 1;
                    break;
                }
                if (is_directory)
                    return 0;
                file->offset = read_be32(entry + 4);
                file->size = read_be32(entry + 8);
                return 1;
            }
            index = is_directory ? read_be32(entry + 8) : index + 1;
        }
        if (!found)
            return 0;
    }
    return 0;
}

size_t m360_gcm_read(const struct m360_gcm *gcm,
                     const struct m360_gcm_file *file, uint32_t offset,
                     void *buffer, size_t size)
{
    uint32_t available;

    if (!gcm || !gcm->image || !file || !buffer || offset > file->size)
        return 0;
    available = file->size - offset;
    if (size > available)
        size = available;
    if (fseek(gcm->image, (long)(file->offset + offset), SEEK_SET) != 0)
        return 0;
    return fread(buffer, 1, size, gcm->image);
}
