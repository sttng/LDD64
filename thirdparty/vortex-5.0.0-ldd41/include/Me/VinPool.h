/* Vortex 5.0.0 build 800 "vortexldd41" - VinPool: fixed-size struct pool with a free list,
 * growing by blocks (restored; src/Me/VinPool.c; "VinPoolInit(%s)" string). */
#ifndef ME_VINPOOL_H
#define ME_VINPOOL_H

#include "Me/MeCall.h"

#ifdef __cplusplus
extern "C" {
#endif

/* block header (0xc bytes), the structs follow at begin (aligned) */
typedef struct VinPoolBlock {
    struct VinPoolBlock* next; /* +0x00 older block */
    char* begin;               /* +0x04 */
    char* end;                 /* +0x08 */
} VinPoolBlock;

typedef struct VinPool {
    int structSize;        /* +0x00 (min 4) */
    int stride;            /* +0x04 structSize rounded up to alignment */
    int blockCount;        /* +0x08 structs in the next block */
    const char* name;      /* +0x0c */
    VinPoolBlock* blocks;  /* +0x10 newest block */
    char* next;            /* +0x14 first never-used struct in the newest block */
    void* freeList;        /* +0x18 */
    float growFactor;      /* +0x1c next block = blockCount * growFactor + 0.5 + growIncrement */
    int growIncrement;     /* +0x20 */
    int alignment;         /* +0x24 */
    int used;              /* +0x28 structs handed out */
    int m2c;               /* +0x2c (not touched by the pool functions) */
    int m30;               /* +0x30 (zeroed by Init/InitSimple/Destroy) */
} VinPool;

/* Mac 0x48a281, Win 0x758e80 */
void MEAPI VinPoolInit(VinPool* pool, int count, int structSize, int growIncrement, float growFactor,
                 int alignment);
/* Mac 0x48a367, Win 0x758f20 (name inferred): VinPoolInit(pool, count, size, 0, 2.0f, alignment) */
void MEAPI VinPoolInitSimple(VinPool* pool, int count, int structSize, int alignment);
void MEAPI VinPoolDestroy(VinPool* pool);                 /* Mac 0x48a0f0, Win 0x758cd0 (name inferred) */
void* MEAPI VinPoolGetStruct(VinPool* pool);              /* Mac 0x48a222, Win 0x758f60 (name inferred) */
void MEAPI VinPoolPutStruct(VinPool* pool, void* p);      /* Mac 0x48a136, Win 0x758d60 (name inferred) */
void MEAPI VinPoolSetName(VinPool* pool, const char* n);  /* Mac 0x48a14c, Win 0x758de0 (name inferred) */

#ifdef __cplusplus
}
#endif

#endif
