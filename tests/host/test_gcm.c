#include "gcm.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    struct m360_gcm gcm;
    struct m360_gcm_file file;
    FILE *image;
    unsigned char banner[4];

    if (argc != 2) {
        fprintf(stderr, "usage: %s melee.iso\n", argv[0]);
        return 2;
    }
    image = fopen(argv[1], "rb");
    if (!image || m360_gcm_mount(&gcm, image) != 0) {
        fprintf(stderr, "could not mount GCM image\n");
        return 1;
    }
    if (!m360_gcm_find(&gcm, "opening.bnr", &file) || file.size < 4 ||
        m360_gcm_read(&gcm, &file, 0, banner, sizeof(banner)) != sizeof(banner) ||
        memcmp(banner, "BNR1", 4) != 0) {
        fprintf(stderr, "opening.bnr validation failed\n");
        m360_gcm_unmount(&gcm);
        fclose(image);
        return 1;
    }
    printf("GCM OK: %u entries, opening.bnr offset=%u size=%u magic=BNR1\n",
           gcm.entry_count, file.offset, file.size);
    m360_gcm_unmount(&gcm);
    fclose(image);
    return 0;
}
