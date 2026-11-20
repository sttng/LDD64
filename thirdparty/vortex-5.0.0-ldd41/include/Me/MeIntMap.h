/* Vortex 5.0.0 build 800 "vortexldd41" - sorted int -> int table searched with bsearch
 * (restored; src/Me/MeIntMap.c). All names inferred. */
#ifndef ME_MEINTMAP_H
#define ME_MEINTMAP_H

#include "Me/MeCall.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PORT64: the keys are body addresses, stored as int by the 32-bit originals (compared by
 * subtraction: safe below 2 GB, true for LDD.exe but not guaranteed on Mac i386; see the NOTE in
 * MeIntMap.c). 64-bit builds keep the whole address: truncated keys would break the bsearch order
 * and could collide. On 32-bit MeIntMapKey is int, as in the original. */
#if defined(_WIN64) || defined(__LP64__)
typedef long long MeIntMapKey;
#else
typedef int MeIntMapKey;
#endif

typedef struct MeIntMapEntry {
    MeIntMapKey key; /* +0x00 */
    int value;       /* +0x04 */
} MeIntMapEntry;

typedef struct MeIntMap {
    int count;               /* +0x00 */
    MeIntMapEntry* entries;  /* +0x04 */
} MeIntMap;

int MeIntMapCompare(const void* a, const void* b);  /* Mac 0x487bee: key difference */
int MEAPI MeIntMapLookup(const MeIntMap* map, MeIntMapKey key);   /* Mac 0x487bfd: value or -1 */
void MEAPI MeIntMapSort(MeIntMap* map);                   /* Mac 0x487c51 */

#ifdef __cplusplus
}
#endif

#endif
