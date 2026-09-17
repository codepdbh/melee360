#ifndef MELEE360_GCM_H
#define MELEE360_GCM_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

struct m360_gcm {
    FILE *image;
    uint8_t *fst;
    uint32_t fst_size;
    uint32_t entry_count;
};

struct m360_gcm_file {
    uint32_t entry_index;
    uint32_t offset;
    uint32_t size;
};

int m360_gcm_mount(struct m360_gcm *gcm, FILE *image);
void m360_gcm_unmount(struct m360_gcm *gcm);
int m360_gcm_find(const struct m360_gcm *gcm, const char *path,
                  struct m360_gcm_file *file);
int m360_gcm_file_by_index(const struct m360_gcm *gcm, uint32_t entry_index,
                           struct m360_gcm_file *file);
size_t m360_gcm_read(const struct m360_gcm *gcm,
                     const struct m360_gcm_file *file, uint32_t offset,
                     void *buffer, size_t size);

#endif
