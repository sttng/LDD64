// LEGO Digital Designer — restored source
// LEGO::BasicString<C> — copy-on-write buffer shared by Win String (UTF-16) and AString.
//   [Int32 shares][UInt32 capacity][UInt32 length][C data[capacity+1]]  (m_data -> data)
//   shares == 0: one owner; release frees at 0. Growth: (max(2*old, n) + 1) | 0x20, minus 1.
// Reserve: Mac AString instance 0x1cb20, Win String instance 0x401350.
#pragma once
#include "LEGO/Types.h"
#include "RangeException.h"
#include <string.h>
#if defined(_WIN32)
#include <windows.h>
#else
#include <libkern/OSAtomic.h>
#endif

namespace LEGO {

template <class C>
class BasicString {
public:
    enum { npos = 0xFFFFFFFF };

    BasicString() : m_data(0) {}
    BasicString(const C* s) : m_data(0) { Assign(s, Length(s)); }
    BasicString(const C* s, UInt32 n) : m_data(0) { Assign(s, n); }
    // NOTE: the original counts the shares with plain ++ / -- (here, operator=, Reserve and
    // ReleaseBuffer), so a buffer shared by strings of two threads can be freed twice or leak;
    // fixed: atomic increment / decrement (AddShare / DropShare).
    BasicString(const BasicString& o) : m_data(o.m_data) { if (m_data) AddShare(Head()); }
    ~BasicString() { ReleaseBuffer(); }

    BasicString& operator=(const BasicString& o)
    {
        if (o.m_data != m_data) {
            if (o.m_data)
                AddShare(o.Head());
            ReleaseBuffer();
            m_data = o.m_data;
        }
        return *this;
    }

    UInt32   GetLength() const   { return m_data ? Head()->length : 0; }
    UInt32   GetCapacity() const { return m_data ? Head()->capacity : 0; }
    bool     IsEmpty() const     { return GetLength() == 0; }
    const C* GetBuffer() const   { static const C empty = 0; return m_data ? m_data : &empty; }
    const C* c_str() const       { return GetBuffer(); }
    C        operator[](UInt32 i) const { return m_data[i]; }

    // Make the buffer unique and at least n characters large.
    // discard == true: old contents are not needed (fresh, empty buffer).
    // Mac 0x1cb20 (C = char).
    bool Reserve(UInt32 n, bool discard)
    {
        if (n == 0) {
            ReleaseBuffer();
            return false;
        }
        UInt32 cap;
        if (!m_data) {
            cap = 0;
        } else if (Head()->shares != 0) {
            // shared: always copy into a private block
            UInt32 keep;
            if (!discard) {
                keep = Head()->length;
            } else {
                ReleaseBuffer();    // frees the block if the other sharer let go meanwhile
                keep = 0;
            }
            UInt32 c = (n + 1) | 0x20;
            Header* h = Alloc(c);
            UInt32 copy = keep < n ? keep : n;
            if (keep)
                memcpy(h + 1, m_data, copy * sizeof(C));
            if (!discard)
                ReleaseBuffer();
            Install(h, c - 1, copy);
            return true;
        } else {
            cap = Head()->capacity;
        }
        if (cap < n) {
            if (!discard) {
                UInt32 want = cap * 2 > n ? cap * 2 : n;
                UInt32 c = (want + 1) | 0x20;
                UInt32 len = GetLength();
                Header* h = Alloc(c);
                UInt32 copy = len < want ? len : want;
                if (len)
                    memcpy(h + 1, m_data, copy * sizeof(C));
                ReleaseBuffer();
                Install(h, c - 1, copy);
            } else {
                ReleaseBuffer();
                UInt32 c = (n + 1) | 0x20;
                Install(Alloc(c), c - 1, 0);
            }
        }
        return true;
    }

    BasicString& Assign(const C* s, UInt32 n)
    {
        if (!s || n == 0) {
            ReleaseBuffer();
            return *this;
        }
        if (Reserve(n, true)) {
            memcpy(m_data, s, n * sizeof(C));
            SetLength(n);
        }
        return *this;
    }

    // Win String instance 0x401f80 (append raw characters). A length that would reach
    // 0xFFFFFFFF throws RangeException* (both builds; Mac e.g. 0x1331f0).
    BasicString& Append(const C* s, UInt32 n)
    {
        UInt32 len = GetLength();
        if (n >= ~len)
            ThrowRangeException();
        if (n == 0)
            return *this;
        UInt32 total = len + n;
        if (!m_data || Head()->shares != 0 || Head()->capacity < total)
            Reserve(total, false);
        memcpy(m_data + len, s, n * sizeof(C));
        SetLength(total);
        return *this;
    }

    // Win String instance 0x401e90
    BasicString& Append(const BasicString& o)
    {
        if (&o == this) {
            BasicString tmp(o);
            return Append(tmp.GetBuffer(), tmp.GetLength());
        }
        return Append(o.GetBuffer(), o.GetLength());
    }

    BasicString& Append(const C* s) { return Append(s, Length(s)); }

    // Remove `count` characters at `start` (count npos = to the end).
    BasicString& Erase(UInt32 start, UInt32 count)
    {
        UInt32 len = GetLength();
        if (start >= len)
            return *this;
        if (count > len - start)
            count = len - start;
        Reserve(len, false);
        memmove(m_data + start, m_data + start + count, (len - start - count) * sizeof(C));
        SetLength(len - count);
        return *this;
    }

    // The checks of Win 0x402d10 (Replace with count 0): pos > length or a result of 0xFFFFFFFF
    // characters or more throws RangeException*.
    BasicString& Insert(UInt32 pos, const C* s, UInt32 n)
    {
        UInt32 len = GetLength();
        if (pos > len || len >= ~n)
            ThrowRangeException();
        if (pos == len)
            return Append(s, n);
        BasicString tmp(s, n);                    // s may alias our buffer
        Reserve(len + n, false);
        memmove(m_data + pos + n, m_data + pos, (len - pos) * sizeof(C));
        memcpy(m_data + pos, tmp.GetBuffer(), n * sizeof(C));
        SetLength(len + n);
        return *this;
    }

    // Replaces `count` characters at `pos` (clipped to the end) by `n` characters of `s`.
    // Mac 0x1ac3b0 (C = char, emitted in TU 097), Win 0x402d10 (C = UniChar16): pos > length or
    // a result of 0xFFFFFFFF characters or more throws RangeException* (Mac typeinfo 0x764410).
    BasicString& Replace(UInt32 pos, UInt32 count, const C* s, UInt32 n)
    {
        UInt32 len = GetLength();
        if (pos > len)
            ThrowRangeException();
        if (count > len - pos)
            count = len - pos;
        if (len - count >= ~n)
            ThrowRangeException();
        BasicString tmp(s, n);                    // s may alias our buffer
        UInt32 tail = len - count - pos;
        UInt32 total = len - count + n;
        if (total == 0) {
            ReleaseBuffer();
            return *this;
        }
        Reserve(total > len ? total : len, false);   // unique buffer keeping all old characters
        memmove(m_data + pos + n, m_data + pos + count, tail * sizeof(C));
        if (n)
            memcpy(m_data + pos, tmp.GetBuffer(), n * sizeof(C));
        SetLength(total);
        return *this;
    }

    // Replaces every occurrence of `from` (scanning left to right, continuing after each
    // inserted replacement). Inlined on Mac (MemoryDirectory 0x1aafd0), Win 0x43a840 (C = char).
    BasicString& ReplaceAll(const C* from, UInt32 fromLength, const C* to, UInt32 toLength)
    {
        if (fromLength == 0)
            return *this;
        UInt32 pos = 0;
        while (m_data && pos + fromLength <= GetLength()) {
            if (memcmp(m_data + pos, from, fromLength * sizeof(C)) == 0) {
                Replace(pos, fromLength, to, toLength);
                pos += toLength;
            } else {
                ++pos;
            }
        }
        return *this;
    }

    // Position of the first occurrence of s[0..n) at or after `start`, npos if none (also for
    // n == 0, start >= length or a pattern longer than the rest). memchr for the first
    // character, then memcmp. Mac 0xf2730 (weak copy in TU 033), Win 0x43a9f0 (C = char).
    // (name inferred)
    UInt32 Find(const C* s, UInt32 start, UInt32 n) const
    {
        UInt32 len = GetLength();
        if (!m_data || n == 0 || start >= len || n > len - start)
            return npos;
        const C* p = m_data + start;
        const C* last = m_data + (len - n);
        for (; p <= last; ++p)
            if (*p == s[0] && memcmp(p, s, n * sizeof(C)) == 0)
                return (UInt32)(p - m_data);
        return npos;
    }

    // Writable access after Reserve(); caller sets the length afterwards.
    C*   GetWritableBuffer() { return m_data; }
    void SetLength(UInt32 n)
    {
        if (m_data) {
            Head()->length = n;
            m_data[n] = 0;
        }
    }

    void Clear() { ReleaseBuffer(); }

    static UInt32 Length(const C* s)
    {
        if (!s)
            return 0;
        const C* p = s;
        while (*p)
            ++p;
        return (UInt32)(p - s);
    }

private:
    struct Header {
        Int32  shares;
        UInt32 capacity;
        UInt32 length;
    };

    Header* Head() const { return (Header*)m_data - 1; }

    // shares + 1 owners; DropShare returns the new count, -1 = the last owner let go.
    static void AddShare(Header* h)
    {
#if defined(_WIN32)
        InterlockedIncrement((LONG*)&h->shares);
#else
        OSAtomicAdd32(1, (int32_t*)&h->shares);
#endif
    }
    static Int32 DropShare(Header* h)
    {
#if defined(_WIN32)
        return (Int32)InterlockedDecrement((LONG*)&h->shares);
#else
        return (Int32)OSAtomicAdd32(-1, (int32_t*)&h->shares);
#endif
    }

    static Header* Alloc(UInt32 chars)
    {
        return (Header*)new char[sizeof(Header) + chars * sizeof(C)];
    }

    void Install(Header* h, UInt32 capacity, UInt32 length)
    {
        h->shares = 0;
        h->capacity = capacity;
        h->length = length;
        m_data = (C*)(h + 1);
        m_data[length] = 0;
    }

    // Win String instance 0x4017c0 (dtor body)
    void ReleaseBuffer()
    {
        if (m_data) {
            if (DropShare(Head()) < 0)                       // was 0: the only owner
                delete[] (char*)Head();
            m_data = 0;
        }
    }

    C* m_data;
};

// 8-bit string (ASCII / UTF-8 payloads, XML text …). A named class in both builds'
// RTTI (".?AVAString@LEGO@@", BasicArray<AString>), so not a plain typedef.
class AString : public BasicString<char> {
public:
    AString() {}
    AString(const char* s) : BasicString<char>(s) {}
    AString(const char* s, UInt32 n) : BasicString<char>(s, n) {}
    AString(const AString& o) : BasicString<char>(o) {}
    AString& operator=(const AString& o) { BasicString<char>::operator=(o); return *this; }
};

// Byte-wise ordering of Less<AString> (inlined into every BasicMap<AString, …>, e.g. the
// uniform-location cache of OpenGLShader: Mac 0x24fed0 / 0x250020): memcmp over the shorter
// length, then the shorter string first (name of the inline not recovered).
inline bool operator<(const AString& a, const AString& b)
{
    UInt32 la = a.GetLength(), lb = b.GetLength();
    int r = memcmp(a.GetBuffer(), b.GetBuffer(), la < lb ? la : lb);
    return r != 0 ? r < 0 : la < lb;
}

} // namespace LEGO
