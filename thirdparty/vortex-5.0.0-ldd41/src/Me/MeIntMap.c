/* Vortex 5.0.0 build 800 "vortexldd41" - MeIntMap (restored).
 * Mac __text 0x487bee-0x487c8a (TU 530 Me block). Names inferred. */
#include "Me/MeIntMap.h"
#include <stdlib.h>

/* Mac 0x487bee */
int MeIntMapCompare(const void* a, const void* b)
{
#if defined(_WIN64) || defined(__LP64__)
    /* PORT64: a three-way compare (the difference of two 64-bit keys does not fit an int) */
    const MeIntMapKey ka = ((const MeIntMapEntry*)a)->key;
    const MeIntMapKey kb = ((const MeIntMapEntry*)b)->key;
    return ka < kb ? -1 : (ka > kb ? 1 : 0);
#else
    /* NOTE: subtraction as in the original; keys more than 2 GB apart (possible on Mac i386) give
       an inconsistent order (MeIntMap.h) */
    return ((const MeIntMapEntry*)a)->key - ((const MeIntMapEntry*)b)->key;
#endif
}

/* Mac 0x487bfd */
int MEAPI MeIntMapLookup(const MeIntMap* map, MeIntMapKey key)
{
    MeIntMapEntry k;
    const MeIntMapEntry* e;
    k.key = key;
    e = (const MeIntMapEntry*)bsearch(&k, map->entries, (size_t)map->count, sizeof(MeIntMapEntry),
                                      MeIntMapCompare);
    return e ? e->value : -1;
}

/* Mac 0x487c51 */
void MEAPI MeIntMapSort(MeIntMap* map)
{
    qsort(map->entries, (size_t)map->count, sizeof(MeIntMapEntry), MeIntMapCompare);
}
