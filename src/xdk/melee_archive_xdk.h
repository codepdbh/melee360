#ifndef MELEE360_XDK_ARCHIVE_H
#define MELEE360_XDK_ARCHIVE_H

#include <stddef.h>

/* Parses and relocates one writable HAL DAT image with the original
 * melee-pc HSD_Archive implementation.  The image must remain alive while
 * the returned root is used. */
bool M360_ParseHsdArchive(unsigned char* image, unsigned imageSize,
                          char* firstSymbol, size_t firstSymbolCapacity,
                          void** firstRoot);
void* M360_GetHsdPublic(const char* symbol);
bool M360_ParseMenuHsdArchive(unsigned char* image, unsigned imageSize,
                              unsigned* resolvedSymbols);
void* M360_GetMenuHsdPublic(const char* symbol);

#endif
