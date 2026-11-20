// Vortex 5.0.0 build 800 "vortexldd41" - Kea scratch memory (restored;
// src/Kea/MemoryPool.cpp, Mac 0x4ab11a-0x4ab54a, Win 0x7b97d0-0x7b9c40).
// A bump allocator over the work slot's MeChunk (sized numRows * 0x80 by MdtWorld); requests that
// do not fit go to MeMemoryAPI and are freed by reset() ("overflow"). push / pop marks let the LCP
// release its temporaries. RTTI: MemoryPool : MemoryHandler (empty base). No virtual destructor.
#ifndef KEA_MEMORYPOOL_H
#define KEA_MEMORYPOOL_H

#include <stddef.h>

class MemoryHandler {
};

class MemoryPool : public MemoryHandler {
public:
    MemoryPool();                                   // Mac C1 0x4ab194 -> C2 0x4ab11a, Win 0x7b9aa0
    ~MemoryPool();                                  // Mac 0x4ab46a -> 0x4ab418, Win 0x7b9980
    // Win 0x7b9ae0 (no caller): the constructor that starts on a buffer (name inferred)
    MemoryPool(char* buffer, int size);

    // slot 0, Mac 0x4ab47e, Win 0x7b9bf0 ("PoolAlloc"): size must be a multiple of 16
    virtual void* poolAlloc(unsigned int size, const char* name);
    // slot 1, Mac 0x4ab31c, Win 0x7b9b60: size + 4 + align rounded to 16, then aligned
    virtual void* alloc(int size, unsigned int align, const char* name);
    // slot 2, Mac 0x4ab4f2, Win 0x7b97f0
    virtual void* allocZeroed(size_t size, unsigned int align, const char* name);
    virtual void slot3();                           // slot 3, Mac 0x4ab1c6, Win 0x7b9830 (empty)
    virtual void free(void* p);                     // slot 4, Mac 0x4ab1cc, Win 0x7b9840 (empty)
    virtual int takeOverflowBytes();                // slot 5, Mac 0x4ab54a, Win 0x7b9c40
    virtual void reset();                           // slot 6, Mac 0x4ab3b0, Win 0x7b9930

    void setBuffer(char* buffer, int size);         // Mac 0x4ab1a8, Win 0x7b97d0
    void push();                                    // Mac 0x4ab1ee, Win 0x7b9870
    void pop();                                     // Mac 0x4ab1d2, Win 0x7b9850
    void* overflowAlloc(int size, int align);       // Mac 0x4ab24a, Win 0x7b98b0
    void* overflowAlloc16(int size);                // Mac 0x4ab2fa, Win 0x7b9bd0 (not Win 0x752c60, a V0.1 mispairing)

    char** mMarks;      // +0x04 (10 entries at first, MeMemoryAPI)
    char* mCurrent;     // +0x08
    char* mEnd;         // +0x0c
    int mNumMarks;      // +0x10
    int mMarkCapacity;  // +0x14 (10)
    void** mOverflow;   // +0x18 blocks from MeMemoryAPI.createAligned
    int mOverflowBytes; // +0x1c requested since the last takeOverflowBytes()
    int mNumOverflow;   // +0x20
    int mOverflowCapacity; // +0x24
    int mOverflowing;   // +0x28 set once a request did not fit; everything goes to the heap then
};

// Win only, no callers (dead in LDD; Mac dead-stripped): the plain allocator functions of a
// "StdMemoryAllocator" (strings in the Win image). Win 0x7b99b0 / 0x7b99e0 / 0x7b9a20 /
// 0x7b9a60 / 0x7b9a80 (names from the strings / inferred).
void* StdMemoryAllocatorCreate(size_t size, const char* name);
void* StdMemoryAllocatorCreateAligned(size_t size, unsigned int align, const char* name);
void* StdMemoryAllocatorCreateAlignedZeroed(size_t size, unsigned int align, const char* name);
void StdMemoryAllocatorDestroy(void* p);
void StdMemoryAllocatorDestroyAligned(void* p);

#endif
