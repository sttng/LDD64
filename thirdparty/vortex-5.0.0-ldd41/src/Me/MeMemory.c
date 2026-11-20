/* Vortex 5.0.0 build 800 "vortexldd41" - default memory functions and the MeMemoryAPI table
 * (restored). Mac __text 0x489c67-0x489d63 (TU 530 Me block), Win 0x72cab0. */
#include "Me/MeMemory.h"
#include "Me/MeMessage.h"
#include <stdlib.h>

/* Mac 0x489c67: NOTE: a size of 0 returns 0 without freeing the block (original behaviour). */
void* MEAPI MallocResize(void* block, size_t bytes)
{
    void* p = 0;
    if (bytes != 0)
        p = realloc(block, bytes);
    return p;
}

/* Mac 0x489c8a */
void MEAPI MallocDestroy(void* block)
{
    free(block);
}

/* Mac 0x489c9d: the offset to the malloc'd block is stored in the word before the pointer */
void MEAPI MallocDestroyAligned(void* block)
{
    if (block)
        MallocDestroy((char*)block - ((unsigned int*)block)[-1]);
}

/* Mac 0x489cb7 */
void* MEAPI MallocCreateZeroed(size_t bytes)
{
    return calloc(1, bytes);
}

/* Mac 0x489cd2 */
void* MEAPI MallocCreate(size_t bytes)
{
    return malloc(bytes);
}

/* Mac 0x489ce5, Win 0x72cab0 (Win calls malloc directly; same result).
 * NOTE: the alignment check runs after the allocation and only warns; an alignment of 0
 * divides by zero in the original too. */
void* MEAPI MallocCreateAligned(size_t bytes, unsigned int alignment)
{
    char* p;
    unsigned int pad;
#if defined(_WIN32)
    p = (char*)malloc(bytes + 4 + alignment);
#else
    p = (char*)MallocCreate(bytes + 4 + alignment);
#endif
    if (p == 0)
        return 0;
    pad = alignment - (unsigned int)((size_t)p % alignment);
    if (pad == alignment)
        pad = 0;
    if ((alignment & 3) != 0)
        MeWarning(0, "MallocCreateAligned: Alignment must be a multiple of sizeof(unsigned int).\n");
    if (pad < 4)
        pad += alignment;
    p += pad;
    ((unsigned int*)p)[-1] = pad;
    return p;
}

/* Mac 0x779488 */
MeMemoryAPIStruct MeMemoryAPI = {
    MallocCreate,
    MallocCreateZeroed,
    MallocCreateAligned,
    MallocDestroy,
    MallocDestroyAligned,
    MallocResize
};
