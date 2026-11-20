// LEGO Digital Designer — restored source
// LEGO::Stream — bounds-checked reader/writer over a caller-owned buffer, optional byte swap; used
// for the MonolithTree (TU 012), ZIP records, BitmapMask and the BMP / PCX codecs. RTTI
// "LEGO::Stream", no bases. Mac vtable 0x771d28 {D1 0x100c0, D0 0x119a0}, Win 0x9de36c {sdd
// 0x4363d0}. Inlined on Mac, Win accessors out of line.
// Layout (sizeof 0x14): vptr, data +4, size +8, position +0xc, swap +0x10. Read/Write check
// `position + sizeof(value) <= size`, return the bytes moved (0 on overflow; failed Read stores
// 0). Names inferred. The originals access values through cast pointers at any offset (kept on
// i386); PORT64: 64-bit builds use memcpy (no unaligned-access UB).
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"

#include <string.h>

#if defined(_WIN64) || defined(__LP64__)
#define LEGO_STREAM_UNALIGNED_MEMCPY 1
#else
#define LEGO_STREAM_UNALIGNED_MEMCPY 0
#endif

namespace LEGO {

class Stream {
public:
    Stream(void* data, UInt32 size, bool swap)                         // Win 0x436390 (inlined on Mac)
        : m_data(static_cast<UInt8*>(data)), m_size(size), m_position(0), m_swap(swap)
    {
    }
    virtual ~Stream() {}                                               // Mac 0x100c0/0x119a0, Win 0x4363c0/0x4363d0

    // --- reading ---------------------------------------------------------------------------
    UInt32 Read(UInt16& value)                                         // Win 0x4363f0
    {
        if (m_position + 2 > m_size) {
            value = 0;
            return 0;
        }
#if LEGO_STREAM_UNALIGNED_MEMCPY
        memcpy(&value, m_data + m_position, sizeof(value));
#else
        value = *reinterpret_cast<const UInt16*>(m_data + m_position);
#endif
        m_position += 2;
        if (m_swap)
            value = Swap(value);
        return 2;
    }
    UInt32 Read(UInt32& value)                                         // Win 0x436450
    {
        if (m_position + 4 > m_size) {
            value = 0;
            return 0;
        }
#if LEGO_STREAM_UNALIGNED_MEMCPY
        memcpy(&value, m_data + m_position, sizeof(value));
#else
        value = *reinterpret_cast<const UInt32*>(m_data + m_position);
#endif
        m_position += 4;
        if (m_swap)
            value = Swap(value);
        return 4;
    }
    UInt32 Read(UInt64& value)                                         // Win 0x4364c0
    {
        if (m_position + 8 > m_size) {
            value = 0;
            return 0;
        }
#if LEGO_STREAM_UNALIGNED_MEMCPY
        memcpy(&value, m_data + m_position, sizeof(value));
#else
        value = *reinterpret_cast<const UInt64*>(m_data + m_position);
#endif
        m_position += 8;
        if (m_swap)
            value = Swap(value);
        return 8;
    }
    // Zero-terminated UTF-16 string; returns the encoded size including the terminator.
    UInt32 Read(String& value)                                         // Win 0x436520
    {
        value.Clear();
        UInt16 c;
        Read(c);
        while (c != 0) {
            value.Append(static_cast<UniChar16>(c));
            Read(c);
        }
        return (value.GetLength() + 1) * 2;
    }

    // --- writing ---------------------------------------------------------------------------
    UInt32 Write(UInt16 value)                                         // Win 0x436300
    {
        if (m_position + 2 > m_size)
            return 0;
        if (m_swap)
            value = Swap(value);
#if LEGO_STREAM_UNALIGNED_MEMCPY
        memcpy(m_data + m_position, &value, sizeof(value));
#else
        *reinterpret_cast<UInt16*>(m_data + m_position) = value;
#endif
        m_position += 2;
        return 2;
    }
    UInt32 Write(UInt32 value)                                         // Win 0x4365c0
    {
        if (m_position + 4 > m_size)
            return 0;
        if (m_swap)
            value = Swap(value);
#if LEGO_STREAM_UNALIGNED_MEMCPY
        memcpy(m_data + m_position, &value, sizeof(value));
#else
        *reinterpret_cast<UInt32*>(m_data + m_position) = value;
#endif
        m_position += 4;
        return 4;
    }
    UInt32 Write(UInt64 value)                                         // Win 0x436620
    {
        if (m_position + 8 > m_size)
            return 0;
        if (m_swap)
            value = Swap(value);
#if LEGO_STREAM_UNALIGNED_MEMCPY
        memcpy(m_data + m_position, &value, sizeof(value));
#else
        *reinterpret_cast<UInt64*>(m_data + m_position) = value;
#endif
        m_position += 8;
        return 8;
    }
    // Characters followed by a zero terminator (each one bounds-checked on its own).
    UInt32 Write(const String& value)                                  // Win 0x436670
    {
        for (UInt32 i = 0; i < value.GetLength(); ++i)
            Write(static_cast<UInt16>(value.GetAt(i)));
        Write(static_cast<UInt16>(0));
        return (value.GetLength() + 1) * 2;
    }

    // --- byte / block access (bitmap codecs; inlined on Mac) ---------------------------------
    UInt32 Read(UInt8& value)                                          // Win 0x69cff0 (PCX)
    {
        if (m_position + 1 > m_size) {
            value = 0;
            return 0;
        }
        value = m_data[m_position];
        m_position += 1;
        return 1;
    }
    UInt32 Write(UInt8 value)                                          // Win 0x69b660 (BMP/PCX)
    {
        if (m_position + 1 > m_size)
            return 0;
        m_data[m_position] = value;
        m_position += 1;
        return 1;
    }
    UInt32 Write(const void* data, UInt32 size)                        // Win 0x43ece0
    {
        if (m_position + size > m_size)
            return 0;
        for (UInt32 i = 0; i < size; ++i)
            m_data[m_position + i] = static_cast<const UInt8*>(data)[i];
        m_position += size;
        return size;
    }
    // The BMP codec reads/writes the 'BM' signature byte-swapped and then turns swapping off.
    void   SetSwap(bool swap)  { m_swap = swap; }                      // (inlined field store)

    UInt8* GetData() const     { return m_data; }                      // (not seen)
    UInt32 GetSize() const     { return m_size; }                      // (not seen)
    UInt32 GetPosition() const { return m_position; }                  // (not seen)

    static UInt16 Swap(UInt16 v) { return static_cast<UInt16>((v << 8) | (v >> 8)); }
    static UInt32 Swap(UInt32 v)
    {
        return (v >> 24) | ((v & 0x00ff0000) >> 8) | ((v & 0x0000ff00) << 8) | (v << 24);
    }
    static UInt64 Swap(UInt64 v)                                       // Win 0x439240
    {
        return (static_cast<UInt64>(Swap(static_cast<UInt32>(v))) << 32) |
               Swap(static_cast<UInt32>(v >> 32));
    }

private:
    UInt8* m_data;       // +0x04 (not owned)
    UInt32 m_size;       // +0x08
    UInt32 m_position;   // +0x0c
    bool   m_swap;       // +0x10 swap every value (true = big-endian data on the x86 builds)
};

} // namespace LEGO
