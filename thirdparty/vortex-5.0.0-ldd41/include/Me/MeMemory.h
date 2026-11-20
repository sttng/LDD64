// Vortex 5.0.0 build 800 "vortexldd41" - Me memory API (restored header).
// All Vortex heap memory goes through one global table of function pointers (Karma's
// "MeMemoryAPI"): Mac non-lazy pointer 0x7025c8 -> table 0x779488. vxAlloc (Mac 0x463c0a) calls
// slot 0, vxFree (Mac 0x463c2c) slot 3; qhull uses it too (thirdparty/qhull-2010.1-vortex/NOTES.md).
#ifndef ME_MEMEMORY_H
#define ME_MEMEMORY_H

#include "Me/MeCall.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* (MEAPI *MeMemoryCreateFn)(size_t bytes);
typedef void* (MEAPI *MeMemoryCreateAlignedFn)(size_t bytes, unsigned int alignment);
typedef void (MEAPI *MeMemoryDestroyFn)(void* block);
typedef void* (MEAPI *MeMemoryResizeFn)(void* block, size_t bytes);

// Mac 0x779488 (6 slots, 24 bytes on both platforms).
typedef struct MeMemoryAPIStruct {
    MeMemoryCreateFn create;                // +0x00 Mac 0x489cd2: malloc(n)
    MeMemoryCreateFn createZeroed;          // +0x04 Mac 0x489cb7: calloc(1, n)
    MeMemoryCreateAlignedFn createAligned;  // +0x08 Mac 0x489ce5
    MeMemoryDestroyFn destroy;              // +0x0c Mac 0x489c8a: free(p)
    MeMemoryDestroyFn destroyAligned;       // +0x10 Mac 0x489c9d: free(p - ((int*)p)[-1])
    MeMemoryResizeFn resize;                // +0x14 Mac 0x489c67: p ? realloc(p, n) : 0
} MeMemoryAPIStruct;

// The global table (Mac 0x779488; reached through the non-lazy pointer 0x7025c8).
extern MeMemoryAPIStruct MeMemoryAPI;

// The default implementations (src/Me/MeMemory.c, V1.1; names: "MallocCreateAligned" from its
// message, the others inferred).
void* MEAPI MallocCreate(size_t bytes);                               // Mac 0x489cd2
void* MEAPI MallocCreateZeroed(size_t bytes);                         // Mac 0x489cb7
void* MEAPI MallocCreateAligned(size_t bytes, unsigned int alignment);// Mac 0x489ce5, Win 0x72cab0
void MEAPI MallocDestroy(void* block);                                // Mac 0x489c8a
void MEAPI MallocDestroyAligned(void* block);                         // Mac 0x489c9d
void* MEAPI MallocResize(void* block, size_t bytes);                  // Mac 0x489c67

#ifdef __cplusplus
}
#endif

#endif
