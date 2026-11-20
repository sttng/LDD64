/* Vortex 5.0.0 build 800 "vortexldd41" - MeChunk (restored).
 * Mac __text 0x487c8a-0x487e9f (TU 530 Me block); Win 0x790ab0, 0x790b60. */
#include "Me/MeChunk.h"
#include "Me/MeMemory.h"
#include "Me/MeMessage.h"
#include <stddef.h>

/* Mac 0x487c8a (static, register arguments: chunk in EAX, buffer EDX, size ECX)
 * (name inferred) */
static void MEAPI MeChunkSetBuffer(MeChunk* chunk, void* buffer, int size, int alignment)
{
    unsigned int pad;
    chunk->buffer = buffer;
    chunk->size = size;
    pad = (unsigned int)alignment - (unsigned int)((size_t)buffer % (unsigned int)alignment);
    if (pad == (unsigned int)alignment)
        pad = 0;
    chunk->alignedBuffer = (char*)buffer + pad;
    chunk->alignedSize = chunk->size - (int)pad;
    chunk->alignment = alignment;
}

/* Mac 0x487cdc */
void MEAPI MeChunkInit(MeChunk* chunk)
{
    chunk->alignment = 0;
    chunk->automatic = 0;
    chunk->buffer = 0;
    chunk->alignedBuffer = 0;
    chunk->size = 0;
    chunk->alignedSize = 0;
    chunk->resize = 0;
    chunk->maxUsed = 0;
}

/* Mac 0x487d1b */
void MEAPI MeChunkTerm(MeChunk* chunk)
{
    if (chunk->automatic && chunk->buffer) {
        MeMemoryAPI.destroy(chunk->buffer);
        chunk->buffer = 0;
    }
}

/* Mac 0x487d56, Win 0x790b60.
 * NOTE: when the resize fails the fatal handler is called and (if it returns) the chunk is set
 * up on the null block, as in the original. */
void* MEAPI MeChunkGetMem(MeChunk* chunk, int bytes)
{
    if (chunk->maxUsed < bytes)
        chunk->maxUsed = bytes;
    if (bytes > chunk->alignedSize) {
        if (chunk->automatic) {
            void* p = MeMemoryAPI.resize(chunk->buffer, (size_t)(bytes + chunk->alignment));
            if (p == 0)
                MeFatalError(0, "MeChunkGetMem: Could not allocate memory to resize MeChunk: "
                                "old size = %d new size = %d\n",
                             chunk->size, bytes);
            MeChunkSetBuffer(chunk, p, bytes + chunk->alignment, chunk->alignment);
        } else {
            if (chunk->resize == 0)
                return 0;
            chunk->resize(chunk, chunk->buffer, chunk->size, bytes + 16);
            if (bytes > chunk->alignedSize)
                return 0;
        }
    }
    return chunk->alignedBuffer;
}

/* Mac 0x487e12, Win 0x790ab0 */
void MEAPI MeChunkSetAuto(MeChunk* chunk, int size, int alignment)
{
    void* p;
    if (chunk->automatic && chunk->buffer) {
        MeMemoryAPI.destroy(chunk->buffer);
        chunk->buffer = 0;
    }
    p = MeMemoryAPI.create((size_t)(alignment + size));
    if (p == 0)
        MeFatalError(0, "MeChunkSetAuto: Could not allocate memory for MeChunk.\n");
    MeChunkSetBuffer(chunk, p, size + alignment, alignment);
    chunk->automatic = 1;
}
