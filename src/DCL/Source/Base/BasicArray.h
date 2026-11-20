// LEGO Digital Designer — restored source
// LEGO::BasicArray<T> — polymorphic dynamic array (partial). Layout: { vptr; m_begin; m_end;
// m_capacityEnd; } sizeof 16; operator new[] storage with placement construction.
// PushBack growth: max(size + 4, capacity * 3 / 2).
// Instances: BasicArray<String> PushBack Mac 0x8f80, Reserve 0x28cbe0, dtor 0x9070/0x90d0.
#pragma once
#include "LEGO/Types.h"
#include <new>
#include <stddef.h>

namespace LEGO {

template <class T>
class BasicArray {
public:
    typedef T*       Iterator;
    typedef const T* ConstIterator;

    BasicArray() : m_begin(0), m_end(0), m_capacityEnd(0) {}
    BasicArray(const BasicArray& o) : m_begin(0), m_end(0), m_capacityEnd(0)
    {
        Reserve(o.GetSize());
        for (const T* p = o.m_begin; p != o.m_end; ++p)
            new (m_end++) T(*p);
    }
    virtual ~BasicArray()
    {
        Destroy(m_begin, m_end);
        if (m_begin)
            operator delete[](m_begin);
    }

    BasicArray& operator=(const BasicArray& o)
    {
        if (&o != this) {
            Clear();
            Reserve(o.GetSize());
            for (const T* p = o.m_begin; p != o.m_end; ++p)
                new (m_end++) T(*p);
        }
        return *this;
    }

    UInt32   GetSize() const     { return (UInt32)(m_end - m_begin); }
    UInt32   GetCapacity() const { return (UInt32)(m_capacityEnd - m_begin); }
    bool     IsEmpty() const     { return m_begin == m_end; }
    T&       operator[](UInt32 i)       { return m_begin[i]; }
    const T& operator[](UInt32 i) const { return m_begin[i]; }
    Iterator      Begin()       { return m_begin; }
    Iterator      End()         { return m_end; }
    ConstIterator Begin() const { return m_begin; }
    ConstIterator End() const   { return m_end; }
    T&       Back()             { return m_end[-1]; }

    // Mac 0x8f80 (T = String)
    void PushBack(const T& v)
    {
        if (m_end == m_capacityEnd) {
            UInt32 cap = GetCapacity();
            UInt32 grow = cap + (cap >> 1);
            UInt32 n = GetSize() + 4;
            if (n <= grow)
                n = grow;
            T* nb = (T*)operator new[](ByteSize(n));
            T* d = nb;
            for (T* p = m_begin; p != m_end; ++p, ++d)
                new (d) T(*p);
            new (d) T(v);
            Destroy(m_begin, m_end);
            if (m_begin)
                operator delete[](m_begin);
            m_begin = nb;
            m_end = d + 1;
            m_capacityEnd = nb + n;
        } else {
            new (m_end) T(v);
            ++m_end;
        }
    }

    // Mac 0x28cbe0 (T = String)
    void Reserve(UInt32 n)
    {
        if (GetCapacity() >= n)
            return;
        T* nb = (T*)operator new[](ByteSize(n));
        T* d = nb;
        if (m_begin) {
            for (T* p = m_begin; p != m_end; ++p, ++d)
                new (d) T(*p);
            Destroy(m_begin, m_end);
            operator delete[](m_begin);
        }
        m_begin = nb;
        m_end = d;
        m_capacityEnd = nb + n;
    }

    void Clear()
    {
        Destroy(m_begin, m_end);
        m_end = m_begin;
    }

    void PopBack()
    {
        --m_end;
        m_end->~T();
    }

    // Exchanges the element storage (std::vector::swap shape; the vptrs stay). Inlined in
    // ModelLoaderStorageScene (Mac 0x2efd40: cameras / metas of StorageScene and StorageScene2).
    void Swap(BasicArray& o)
    {
        T* b = m_begin;
        T* e = m_end;
        T* c = m_capacityEnd;
        m_begin = o.m_begin;
        m_end = o.m_end;
        m_capacityEnd = o.m_capacityEnd;
        o.m_begin = b;
        o.m_end = e;
        o.m_capacityEnd = c;
    }

    void Erase(UInt32 index)
    {
        for (T* p = m_begin + index; p + 1 < m_end; ++p)
            *p = p[1];
        PopBack();
    }

    // Inserts `count` copies of `value` before `pos` (std::vector::_M_fill_insert shape).
    // Growth like PushBack: max(size + count, capacity * 3 / 2).
    // Mac 0x18520 (T = String, emitted in TU 013 Directory.cpp), Win 0x42bad0.
    void Insert(Iterator pos, UInt32 count, const T& value)
    {
        if (count == 0)
            return;
        ByteSize((UInt64)GetSize() + count);   // throws when the new size does not fit
        if (GetSize() + count > GetCapacity()) {
            UInt32 cap = GetCapacity();
            UInt32 grow = cap + (cap >> 1);
            UInt32 n = GetSize() + count;
            if (n <= grow)
                n = grow;
            T* nb = (T*)operator new[](ByteSize(n));
            T* d = nb;
            for (T* p = m_begin; p != pos; ++p, ++d)
                new (d) T(*p);
            for (UInt32 i = 0; i < count; ++i, ++d)
                new (d) T(value);
            for (T* p = pos; p != m_end; ++p, ++d)
                new (d) T(*p);
            Destroy(m_begin, m_end);
            if (m_begin)
                operator delete[](m_begin);
            m_begin = nb;
            m_end = d;
            m_capacityEnd = nb + n;
        } else if ((UInt32)(m_end - pos) < count) {
            T* oldEnd = m_end;
            for (T* p = pos; p != oldEnd; ++p)
                new (p + count) T(*p);
            for (T* p = pos; p != oldEnd; ++p)
                *p = value;
            for (T* p = oldEnd; p != pos + count; ++p)
                new (p) T(value);
            m_end = oldEnd + count;
        } else {
            T copy(value);
            T* oldEnd = m_end;
            for (UInt32 i = 0; i < count; ++i)
                new (oldEnd + i) T(*(oldEnd - count + i));
            for (T* s = oldEnd - count, *d = oldEnd; s != pos;)
                *--d = *--s;
            for (UInt32 i = 0; i < count; ++i)
                pos[i] = copy;
            m_end = oldEnd + count;
        }
    }

    // (additive) Inserts [first, last) before `pos` (std::vector::_M_range_insert
    // shape; the range must not alias this array). T = RefPointer<BIEvent>: Mac 0x210760;
    // T = PrimitiveElement*: Mac 0x1c69f0 (BIBrickSubstep constructor).
    void InsertRange(Iterator pos, ConstIterator first, ConstIterator last)
    {
        UInt32 count = (UInt32)(last - first);
        if (count == 0)
            return;
        UInt32 index = (UInt32)(pos - m_begin);
        ByteSize((UInt64)GetSize() + count);
        if (GetSize() + count > GetCapacity()) {
            UInt32 cap = GetCapacity();
            UInt32 grow = cap + (cap >> 1);
            UInt32 n = GetSize() + count;
            if (n <= grow)
                n = grow;
            T* nb = (T*)operator new[](ByteSize(n));
            T* d = nb;
            for (T* p = m_begin; p != pos; ++p, ++d)
                new (d) T(*p);
            for (ConstIterator p = first; p != last; ++p, ++d)
                new (d) T(*p);
            for (T* p = pos; p != m_end; ++p, ++d)
                new (d) T(*p);
            Destroy(m_begin, m_end);
            if (m_begin)
                operator delete[](m_begin);
            m_begin = nb;
            m_end = d;
            m_capacityEnd = nb + n;
            return;
        }
        // Grow by copies of the last elements, shift, assign the range.
        T* oldEnd = m_end;
        for (UInt32 i = 0; i < count; ++i)
            new (m_end++) T(T());
        for (T* s = oldEnd, *d = m_end; s != m_begin + index;)
            *--d = *--s;
        for (UInt32 i = 0; i < count; ++i)
            m_begin[index + i] = first[i];
    }

    // std::vector::resize shape: grows through Insert(End(), …, value), shrinks by destroying
    // the tail. T = UInt8: Win 0x444ad0, inlined in Mac 0x1e8800 (Insert instance 0x1b7a60).
    void Resize(UInt32 n, const T& value = T())
    {
        UInt32 size = GetSize();
        if (size < n) {
            Insert(m_end, n - size, value);
        } else if (n < size) {
            Destroy(m_begin + n, m_end);
            m_end = m_begin + n;
        }
    }

private:
    // (not in the original; H13) The bytes of n elements; std::bad_alloc when n elements do not
    // fit in 32 bits or their size in a size_t. The original multiplies in 32 bits, so a huge
    // count allocates a wrapped (small) block and the elements are written past it.
    static size_t ByteSize(UInt64 n)
    {
        if (n > 0xFFFFFFFFu || n > (UInt64)((size_t)-1) / sizeof(T))
            throw std::bad_alloc();
        return (size_t)n * sizeof(T);
    }
    static void Destroy(T* b, T* e)
    {
        for (; b != e; ++b)
            b->~T();
    }

    T* m_begin;        // +0x04
    T* m_end;          // +0x08
    T* m_capacityEnd;  // +0x0c
};

} // namespace LEGO
