// Vortex 5.0.0 build 800 "vortexldd41" - VxArray<T> (restored header).
//
// Vortex's own vector: { T* begin; T* end; T* capacity; <4 bytes> } = 16 bytes on both
// platforms. Evidence:
//  * VxConstraint ctor (Mac 0x43ebf6 / Win 0x7225e0) zeroes words 0-2 of five consecutive
//    16-byte members and leaves word 3 untouched (Mac +0x38/+0x48/+0x58/+0x68/+0x78, Win
//    +0x50.../+0x90); the same pattern in VxGeometry (Mac +0x2c, +0x5c), VxEntity (+0x28),
//    VxCollisionSensor (+0x174), VxPart (+0x1c0), VxHeightField (+0x6c).
//  * push_back (inlined, e.g. Mac 0x43ebf6): when end == capacity, new capacity
//    n + 1 + n/2, storage from vxAlloc(cap * sizeof(T)), old block copied and vxFree'd.
//  * operator[] checks the index first: "VxArray::isValidIndex : range error\n" reported
//    through VxFatalError(0, ...) (inlined in LEGO code, Mac 0x3a7010 / Win 0x51e890) - and then
//    still reads the element (the fatal handler LDD installs returns).
// Word 3 is taken to be an empty allocator member (1 byte padded to 4) (name inferred).
#ifndef VX_VXARRAY_H
#define VX_VXARRAY_H

#include "Vx/VxAllocator.h"
#include "Vx/VxMessage.h"
#include <new>
#include <string.h>

namespace Vx {

template <class T>
struct VxArrayAllocator { // (name inferred) empty; occupies the 4th word of VxArray
};

template <class T>
class VxArray {
public:
    typedef T* iterator;
    typedef const T* const_iterator;

    VxArray() : mBegin(0), mEnd(0), mCapacity(0) {}
    explicit VxArray(int n) : mBegin(0), mEnd(0), mCapacity(0) { resize(n); }
    VxArray(int n, const T& fill) : mBegin(0), mEnd(0), mCapacity(0) { resize(n, fill); } // (V1.1)
    VxArray(const VxArray& o) : mBegin(0), mEnd(0), mCapacity(0) { *this = o; }
    ~VxArray()
    {
        clear();
        if (mBegin)
            vxFree(mBegin);
    }
    VxArray& operator=(const VxArray& o)
    {
        if (this != &o) {
            clear();
            reserve(o.size());
            for (const_iterator i = o.mBegin; i != o.mEnd; ++i)
                new (mEnd++) T(*i);
        }
        return *this;
    }

    int size() const { return (int)(mEnd - mBegin); }
    bool empty() const { return mEnd == mBegin; }
    int capacity() const { return (int)(mCapacity - mBegin); }
    iterator begin() { return mBegin; }
    iterator end() { return mEnd; }
    const_iterator begin() const { return mBegin; }
    const_iterator end() const { return mEnd; }

    bool isValidIndex(int i) const
    {
        if ((unsigned)i < (unsigned)size())
            return true;
        VxFatalError(0, "VxArray::isValidIndex : range error\n");
        return false;
    }
    // NOTE: like the original, an invalid index is reported and then still dereferenced.
    T& operator[](int i)
    {
        isValidIndex(i);
        return mBegin[i];
    }
    const T& operator[](int i) const
    {
        isValidIndex(i);
        return mBegin[i];
    }

    void reserve(int n)
    {
        if (n <= capacity())
            return;
        T* p = (T*)vxAlloc(n * sizeof(T));
        int s = size();
        for (int i = 0; i < s; ++i) {
            new (p + i) T(mBegin[i]);
            mBegin[i].~T();
        }
        if (mBegin)
            vxFree(mBegin);
        mBegin = p;
        mEnd = p + s;
        mCapacity = p + n;
    }
    void push_back(const T& x)
    {
        if (mEnd == mCapacity) {
            int n = size();
            T copy(x); // x may live in this array
            reserve(n + 1 + n / 2);
            new (mEnd++) T(copy);
            return;
        }
        new (mEnd++) T(x);
    }
    // inserts [first, last) before pos (inlined in the originals, e.g. the type lists
    // Mac 0x43206a: on overflow the new capacity is size + max(size, n)) (name inferred)
    void insert(iterator pos, const T* first, const T* last)
    {
        int n = (int)(last - first);
        if (n <= 0)
            return;
        int at = (int)(pos - mBegin);
        if (size() + n > capacity()) {
            int s = size();
            int cap = s + (s > n ? s : n);
            T* p = (T*)vxAlloc(cap * sizeof(T));
            for (int i = 0; i < at; ++i)
                new (p + i) T(mBegin[i]);
            for (int i = 0; i < n; ++i)
                new (p + at + i) T(first[i]);
            for (int i = at; i < s; ++i)
                new (p + n + i) T(mBegin[i]);
            clear();
            if (mBegin)
                vxFree(mBegin);
            mBegin = p;
            mEnd = p + s + n;
            mCapacity = p + cap;
            return;
        }
        // room: shift the tail up by n (from the end), then copy the range in
        int s = size();
        for (int i = s - 1; i >= at; --i) {
            if (i + n >= s)
                new (mBegin + i + n) T(mBegin[i]);
            else
                mBegin[i + n] = mBegin[i];
        }
        for (int i = 0; i < n; ++i) {
            if (at + i >= s)
                new (mBegin + at + i) T(first[i]);
            else
                mBegin[at + i] = first[i];
        }
        mEnd += n;
    }
    // inserts x before pos (Mac 0x45d020 / 0x45d144, Win 0x787720 when full: the new
    // capacity is n + 1 + n/2 like push_back, then the insert is repeated); returns the
    // position of x
    iterator insert(iterator pos, const T& x)
    {
        int at = (int)(pos - mBegin);
        if (mEnd == mCapacity) {
            T copy(x); // x may live in this array
            int n = size();
            reserve(n + 1 + n / 2);
            return insert(mBegin + at, copy);
        }
        if (pos == mEnd) {
            new (mEnd++) T(x);
        } else {
            T copy(x);
            new (mEnd) T(mEnd[-1]);
            ++mEnd;
            for (T* p = mEnd - 2; p != pos; --p)
                *p = p[-1];
            *pos = copy;
        }
        return mBegin + at;
    }
    void pop_back() { (--mEnd)->~T(); }
    void resize(int n, const T& fill = T())
    {
        if (n > size()) {
            reserve(n);
            while (size() < n)
                new (mEnd++) T(fill);
        } else {
            while (size() > n)
                pop_back();
        }
    }
    void clear() { resize(0); }
    // appends x unless present; true if appended (Mac 0x420344 for VxControllerBase*, used
    // through a member-function pointer by VxControllerGraph) (name inferred)
    bool pushBackUnique(const T& x)
    {
        for (iterator i = mBegin; i != mEnd; ++i)
            if (*i == x)
                return false;
        push_back(x);
        return true;
    }
    // removes the first element equal to x (order kept); true if found (name inferred)
    bool remove(const T& x)
    {
        for (iterator i = mBegin; i != mEnd; ++i) {
            if (*i == x) {
                for (iterator j = i + 1; j != mEnd; ++j)
                    *(j - 1) = *j;
                pop_back();
                return true;
            }
        }
        return false;
    }

    T* mBegin;                       // +0x00
    T* mEnd;                         // +0x04
    T* mCapacity;                    // +0x08
    VxArrayAllocator<T> mAllocator;  // +0x0c (never written by the originals)
};

} // namespace Vx

#endif
