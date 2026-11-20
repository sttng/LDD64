/* Vortex 5.0.0 build 800 "vortexldd41" - MeChunk: a growable, aligned scratch buffer
 * (restored; src/Me/MeChunk.c). 0x20 bytes on both platforms. */
#ifndef ME_MECHUNK_H
#define ME_MECHUNK_H

#include "Me/MeCall.h"

#ifdef __cplusplus
extern "C" {
#endif

struct MeChunk;
/* user resize callback for non-automatic chunks: (chunk, old buffer, old size, wanted size + 16)
 * (name inferred) */
typedef void (*MeChunkResizeFn)(struct MeChunk* chunk, void* buffer, int size, int wanted);

typedef struct MeChunk {
    void* buffer;            /* +0x00 raw block */
    void* alignedBuffer;     /* +0x04 */
    int size;                /* +0x08 raw size */
    int alignedSize;         /* +0x0c usable bytes from alignedBuffer */
    int maxUsed;             /* +0x10 largest request seen */
    int alignment;           /* +0x14 */
    int automatic;           /* +0x18 buffer owned and grown through MeMemoryAPI */
    MeChunkResizeFn resize;  /* +0x1c */
} MeChunk;

void MEAPI MeChunkInit(MeChunk* chunk);                          /* Mac 0x487cdc (name inferred) */
void MEAPI MeChunkTerm(MeChunk* chunk);                          /* Mac 0x487d1b (name inferred) */
void* MEAPI MeChunkGetMem(MeChunk* chunk, int bytes);            /* Mac 0x487d56, Win 0x790b60 */
void MEAPI MeChunkSetAuto(MeChunk* chunk, int size, int alignment); /* Mac 0x487e12, Win 0x790ab0 */

#ifdef __cplusplus
}
#endif

#endif
