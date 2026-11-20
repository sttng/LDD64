// LEGO Digital Designer — restored source
// LEGO::DataStreamInterface (read-only bytes, virtual Object base) and DataStream (over a heap
// block, optionally owned). Header-only; weak copies in Mac TU 021d (0x52800-0x52990) and Win
// 0x4f6910-0x4f6b90. Holds encoded image files. DataStreamWithDataRef (CFData): MacImageIO.h.
// All names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"

namespace LEGO {

// No members. Mac: vmi class, virtual base Object, no own vtable emitted (slots: GetData 0,
// GetSize 1, D1 2, D0 3 in the derived vtables). Win: vfptr {GetData, GetSize}, vbptr +4.
class DataStreamInterface : public virtual Object {
public:
    virtual const void* GetData() const = 0;                         // Mac slot 0, Win +0
    virtual UInt32      GetSize() const = 0;                         // Mac slot 1, Win +4
    virtual ~DataStreamInterface() {}                                // Mac slots 2/3 (D1/D0); Win sdd 0x4f6910 (Object vtable)
};

// Mac sizeof 0x18 (vptr, data +4, size +8, owns +0xc, Object +0x10), vtables 0x760488 {GetData
// 0x52800, GetSize 0x52810, D1 0x52820, D0 0x52870} / 0x7604b0 {thunks 0x528e0, 0x52930, ...}.
// Win sizeof 0x1c (vfptr, vbptr, data +8, size +0xc, owns +0x10, Object +0x14), ctor 0x4f6950
// thiscall(data, size, owns, mostDerived), dtor 0x4f6a10, sdd 0x4f6aa0, vtables 0x9cf1dc
// {0x52f4b0, 0x46ee40} / 0x9cf1c8 {sdd 0x4f6aa0, ...}.
// The codecs read and (when encoding) shrink the block through the non-virtual accessors: both
// builds access the fields directly (Mac +4/+8, Win +8/+0xc).
class DataStream : public DataStreamInterface {
public:
    DataStream(void* data, UInt32 size, bool ownsData)               // Win 0x4f6950, inlined on Mac (e.g. 0x226a6d)
        : m_data(static_cast<UInt8*>(data)), m_size(size), m_ownsData(ownsData)
    {
    }
    virtual ~DataStream()                                            // Mac 0x52820 (D1), 0x52870 (D0), Win 0x4f6a10
    {
        // NOTE: Mac tests the pointer as well, Win deletes unconditionally (delete[] 0 is a no-op).
        if (m_ownsData && m_data)
            delete[] m_data;
    }

    virtual const void* GetData() const { return m_data; }           // Mac 0x52800, Win 0x52f4b0 (ICF)
    virtual UInt32      GetSize() const { return m_size; }           // Mac 0x52810, Win 0x46ee40 (ICF)

    UInt8* GetBuffer() const      { return m_data; }                 // inlined field access
    UInt32 GetBufferSize() const  { return m_size; }                 // inlined field access
    // The encoders shrink the stream to the number of bytes written. (Win 0x69bbb0 +0xc)
    void   SetBufferSize(UInt32 size) { m_size = size; }

private:
    DataStream(const DataStream&);
    DataStream& operator=(const DataStream&);

    UInt8* m_data;       // Mac +0x04, Win +0x08
    UInt32 m_size;       // Mac +0x08, Win +0x0c
    bool   m_ownsData;   // Mac +0x0c, Win +0x10 (deleted with delete[])
};

} // namespace LEGO
