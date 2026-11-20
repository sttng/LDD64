// Vortex 5.0.0 build 800 "vortexldd41" - Kea scratch memory pool (restored).
// Mac 0x4ab11a-0x4ab54a (part of TU 556), Win 0x7b97d0-0x7b9c40.
#include "Kea/MemoryPool.h"
#include "Me/MeMemory.h"
#include "Me/MeMessage.h"

#include <string.h>

// Mac C2 0x4ab11a (C1 0x4ab194), Win 0x7b9aa0
MemoryPool::MemoryPool()
{
    mEnd = 0;
    mCurrent = 0;
    mNumMarks = 0;
    mMarkCapacity = 10;
    mOverflowing = 0;
    mMarks = (char**)MeMemoryAPI.create(10 * sizeof(char*));
    mOverflowBytes = 0;
    mNumOverflow = 0;
    mOverflowCapacity = 0;
    mOverflow = 0;
}

// Win 0x7b9ae0 (no caller in LDD). NOTE: the original builds and destroys a temporary pool and
// then only sets the buffer: the other members stay uninitialised (kept as is).
MemoryPool::MemoryPool(char* buffer, int size)
{
    {
        MemoryPool unused;
    }
    mCurrent = buffer;
    mNumMarks = 0;
    mEnd = buffer + size;
}

// Mac 0x4ab418 (0x4ab46a), Win 0x7b9980
MemoryPool::~MemoryPool()
{
    reset();
    if (mOverflow != 0)
        MeMemoryAPI.destroy(mOverflow);
    MeMemoryAPI.destroy(mMarks);
}

// Mac 0x4ab1a8, Win 0x7b97d0
void MemoryPool::setBuffer(char* buffer, int size)
{
    mCurrent = buffer;
    mNumMarks = 0;
    mEnd = buffer + size;
}

// Mac 0x4ab1d2, Win 0x7b9850
void MemoryPool::pop()
{
    if (mNumMarks > 0) {
        --mNumMarks;
        mCurrent = mMarks[mNumMarks];
    }
}

// Mac 0x4ab1ee, Win 0x7b9870
void MemoryPool::push()
{
    const int cap = mMarkCapacity;
    if (cap - 1 <= mNumMarks) {
        mMarkCapacity = cap * 2;
        mMarks = (char**)MeMemoryAPI.resize(mMarks, cap * 2 * sizeof(char*));
    }
    mMarks[mNumMarks] = mCurrent;
    ++mNumMarks;
}

// Mac 0x4ab24a, Win 0x7b98b0
void* MemoryPool::overflowAlloc(int size, int align)
{
    void* p = 0;
    if (size != 0) {
        mOverflowBytes += size + align;
        if (mOverflowCapacity <= mNumOverflow) {
            int cap = mOverflowCapacity * 2;
            if (cap < 10)
                cap = 10;
            mOverflowCapacity = cap;
            if (mOverflow == 0)
                mOverflow = (void**)MeMemoryAPI.create(cap * sizeof(void*));
            else
                mOverflow = (void**)MeMemoryAPI.resize(mOverflow, cap * sizeof(void*));
        }
        p = MeMemoryAPI.createAligned(size, align);
        mOverflow[mNumOverflow] = p;
        ++mNumOverflow;
    }
    return p;
}

// Mac 0x4ab2fa, Win 0x7b9bd0
void* MemoryPool::overflowAlloc16(int size)
{
    return overflowAlloc(size, 0x10);
}

// Mac 0x4ab31c, Win 0x7b9b60
void* MemoryPool::alloc(int size, unsigned int align, const char* name)
{
    if (size == 0)
        return 0;
    unsigned int n = size + 4 + align;
    if ((n & 0xf) != 0)
        n = (n & ~0xfu) + 0x10;
    if (mOverflowing == 0 && n < (unsigned int)(mEnd - mCurrent)) {
        const size_t p = (size_t)poolAlloc(n, name);
        unsigned int pad = align - (unsigned int)(p % align);
        if (pad == align)
            pad = 0;
        return (void*)(p + pad);
    }
    mOverflowing = 1;
    return overflowAlloc(size, align);
}

// Mac 0x4ab3b0, Win 0x7b9930
void MemoryPool::reset()
{
    if (mOverflowing != 0 || mNumOverflow > 0) {
        for (int i = 0; i < mNumOverflow; ++i)
            MeMemoryAPI.destroyAligned(mOverflow[i]);
        mNumOverflow = 0;
        mOverflowing = 0;
    }
    mNumMarks = 0;
    mCurrent = mMarks[0];
}

// Mac 0x4ab47e, Win 0x7b9bf0
void* MemoryPool::poolAlloc(unsigned int size, const char* name)
{
    if ((size & 0xf) != 0)
        MeFatalError(3, "PoolAlloc: allocating %s of size %d. Size must be a multiple of 16 bytes (remainder = %d).\n",
                     name, size, size & 0xf);
    char* p = mCurrent;
    if (mOverflowing == 0 && p + size < mEnd) {
        mCurrent = p + size;
        return p;
    }
    return overflowAlloc16(size);
}

// Mac 0x4ab4f2, Win 0x7b97f0
void* MemoryPool::allocZeroed(size_t size, unsigned int align, const char* name)
{
    void* p = 0;
    if (size != 0) {
        p = alloc((int)size, align, name);
        memset(p, 0, size);
    }
    return p;
}

void MemoryPool::slot3() {}             // Mac 0x4ab1c6, Win 0x7b9830
void MemoryPool::free(void*) {}         // Mac 0x4ab1cc, Win 0x7b9840

// Mac 0x4ab54a, Win 0x7b9c40
int MemoryPool::takeOverflowBytes()
{
    const int n = mOverflowBytes;
    mOverflowBytes = 0;
    return n;
}

// ---- StdMemoryAllocator (Win only, no callers) ------------------------------------------------
// Win 0x7b99b0
void* StdMemoryAllocatorCreate(size_t size, const char* name)
{
    void* p = MeMemoryAPI.create(size);
    if (p == 0)
        MeFatalError(0x14, "StdMemoryAllocator::creaate::cannot allocate memory for buffer named %s\n", name);
    return p;
}

// Win 0x7b99e0
void* StdMemoryAllocatorCreateAligned(size_t size, unsigned int align, const char* name)
{
    void* p = MeMemoryAPI.createAligned(size, align);
    if (p == 0)
        MeFatalError(0x14, "StdMemoryAllocator::creaate_aligned::cannot allocate memory for buffer named %s\n",
                     name);
    return p;
}

// Win 0x7b9a20: NOTE clears the block even when the allocation failed
void* StdMemoryAllocatorCreateAlignedZeroed(size_t size, unsigned int align, const char* name)
{
    void* p = MeMemoryAPI.createAligned(size, align);
    if (p == 0)
        MeFatalError(0x14,
                     "StdMemoryAllocator::creaate_aligned_zeroed::cannot allocate memory for buffer named %s\n",
                     name);
    memset(p, 0, size);
    return p;
}

// Win 0x7b9a60
void StdMemoryAllocatorDestroy(void* p)
{
    if (p != 0)
        MeMemoryAPI.destroy(p);
}

// Win 0x7b9a80
void StdMemoryAllocatorDestroyAligned(void* p)
{
    if (p != 0)
        MeMemoryAPI.destroyAligned(p);
}
