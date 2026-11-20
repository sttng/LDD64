/* Vortex 5.0.0 build 800 "vortexldd41" - VinPool (restored).
 * Mac __text 0x48a0f0-0x48a3b2 (TU 530 Me block); Win 0x758cd0-0x758f60. */
#include "Me/VinPool.h"
#include "Me/MeMemory.h"
#include "Me/MeMessage.h"
#include <stddef.h>

/* LDD64: a pool that grows by a factor (the model and body pools double) adds blocks of at most
 * this size, so that its unused part stays below one block instead of up to half the pool:
 * 1.3 million models of 248 bytes were held in 325 MB of blocks for 70,000 bricks. */
#define VINPOOL_MAX_GROWN_BLOCK (4 << 20)

/* Mac 0x48a0f0, Win 0x758cd0 */
void MEAPI VinPoolDestroy(VinPool* pool)
{
    VinPoolBlock* b = pool->blocks;
    while (b != 0) {
        VinPoolBlock* next = b->next;
        MeMemoryAPI.destroy(b);
        b = next;
    }
    pool->blocks = 0;
    pool->m30 = 0;
}

/* Mac 0x48a136, Win 0x758d60 */
void MEAPI VinPoolPutStruct(VinPool* pool, void* p)
{
    pool->used--;
    *(void**)p = pool->freeList;
    pool->freeList = p;
}

/* Mac 0x48a14c, Win 0x758de0 */
void MEAPI VinPoolSetName(VinPool* pool, const char* name)
{
    pool->name = name;
}

/* Mac 0x48a15a, Win 0x758df0 (static, pool in EAX) (name inferred): a new block of blockCount
 * structs; from the second block on blockCount grows first. The header and the alignment
 * padding come from the same allocation (+0x13 = header 0xc + 7 slack bytes on i386; the
 * header size is sizeof(VinPoolBlock) here, so that 64-bit host builds align correctly). */
static VinPoolBlock* MEAPI VinPoolNewBlock(VinPool* pool)
{
    VinPoolBlock* b;
    int bytes;
    if (pool->blocks != 0) {
#if defined(_WIN32)
        /* Win: x87 float * int and + 0.5 in double precision, then truncated */
        pool->blockCount = (int)((double)pool->growFactor * (double)pool->blockCount + 0.5)
                           + pool->growIncrement;
#else
        /* Mac: the product in single precision (mulss), then + 0.5 in double */
        pool->blockCount = (int)((double)((float)pool->blockCount * pool->growFactor) + 0.5)
                           + pool->growIncrement;
#endif
        if (pool->growFactor > 1.0f && pool->stride > 0
            && (double)pool->blockCount * (double)pool->stride > (double)VINPOOL_MAX_GROWN_BLOCK) {
            pool->blockCount = VINPOOL_MAX_GROWN_BLOCK / pool->stride;
            if (pool->blockCount < 1)
                pool->blockCount = 1;
        }
    }
    bytes = pool->blockCount * pool->stride;
#if defined(_WIN32)
    /* Win allocates 4 slack bytes more (+0x17) */
    b = (VinPoolBlock*)MeMemoryAPI.create((size_t)pool->alignment + sizeof(VinPoolBlock) + 11 + (size_t)bytes);
#else
    b = (VinPoolBlock*)MeMemoryAPI.create((size_t)pool->alignment + sizeof(VinPoolBlock) + 7 + (size_t)bytes);
#endif
    if (b == 0)
        return 0;
    if (pool->blockCount == 0) {
        MeMemoryAPI.destroy(b);
        return 0;
    }
    b->next = pool->blocks;
    if (pool->alignment == 0) {
        b->begin = (char*)(b + 1);
    } else {
        unsigned int a = (unsigned int)pool->alignment;
        unsigned int pad = a - (unsigned int)((size_t)(b + 1) % a);
        if (pad == a)
            pad = 0;
        b->begin = (char*)(b + 1) + pad;
    }
    b->end = b->begin + bytes;
    return b;
}

/* Mac 0x48a222, Win 0x758f60.
 * NOTE: like the original, a pool whose first block could not be allocated is dereferenced. */
void* MEAPI VinPoolGetStruct(VinPool* pool)
{
    void* p = pool->freeList;
    if (p == 0) {
        p = pool->next;
        if ((char*)p < pool->blocks->end) {
            pool->next = (char*)p + pool->stride;
            pool->used++;
        } else {
            VinPoolBlock* b = VinPoolNewBlock(pool);
            if (b == 0)
                return 0;
            pool->blocks = b;
            pool->next = b->begin + pool->stride;
            pool->used++;
            p = b->begin;
        }
    } else {
        pool->freeList = *(void**)p;
        pool->used++;
    }
    return p;
}

/* Mac 0x48a281, Win 0x758e80 */
void MEAPI VinPoolInit(VinPool* pool, int count, int structSize, int growIncrement, float growFactor,
                 int alignment)
{
    VinPoolBlock* b;
    pool->m30 = 0;
    /* the free list lives in the structs: at least one pointer (4 in both binaries; PORT64) */
    if ((unsigned int)structSize < sizeof(void*))
        structSize = (int)sizeof(void*);
    if ((alignment & 3) != 0)
        MeWarning(0, "VinPoolInit(%s): Alignment must be a multiple of sizeof(unsigned int).\n",
                  pool->name);
    pool->structSize = structSize;
    if (alignment == 0)
        pool->stride = structSize;
    else if (structSize % alignment == 0)
        pool->stride = structSize;
    else
        pool->stride = (structSize + alignment) - structSize % alignment;
    /* PORT64: the free-list pointers stay pointer-aligned (a multiple of 4 is enough on 32-bit,
       where this is compiled out) */
    if (sizeof(void*) > 4 && pool->stride % (int)sizeof(void*) != 0)
        pool->stride += (int)sizeof(void*) - pool->stride % (int)sizeof(void*);
    pool->blockCount = count;
    pool->blocks = 0;
    pool->freeList = 0;
    pool->growIncrement = growIncrement;
    pool->growFactor = growFactor;
    pool->alignment = alignment;
    b = VinPoolNewBlock(pool);
    pool->blocks = b;
    if (b == 0) {
        MeWarning(0, "Failed resizing pool %s\n", pool->name);
    } else {
        pool->next = b->begin;
        pool->used = 0;
    }
}

/* Mac 0x48a367, Win 0x758f20 */
void MEAPI VinPoolInitSimple(VinPool* pool, int count, int structSize, int alignment)
{
    VinPoolInit(pool, count, structSize, 0, 2.0f, alignment);
    pool->m30 = 0;
}
