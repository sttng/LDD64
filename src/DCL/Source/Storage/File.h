// LEGO Digital Designer — restored source
// LEGO::File — abstract random-access VFS byte stream; OpenMode. Header-only except the locked
// wrappers in Directory.cpp (weak Mac vtable 0x75f918, typeinfo 0x75f8e8; D1 0x11f40 / D0 0x11fc0
// in TU 012). Win vtable 0xa20114.
// Slots (Mac; Win = Mac - 1, overload pairs reversed: Win 9 DoRead(size,..), 10 DoRead(pos,..),
// 11/12 DoWrite, 13 DoSize(UInt64), 14 DoSize(), 15 DoPosition(UInt64), 16 DoPosition(); seen on
// LiffStorageFile Win 0x438ec0 / 0x438d30 / 0x4425e0 / 0x438dd0 / 0x438f00 / 0x438ef0 and
// Win32File; overload names inferred):
//   0-9  see DirectoryElement.h (File overrides 6 DoGetAttributes -> 0x12060 "readable")
//   10   DoRead(position, size, buffer, read)        pure   Liff 0x11630  MacOSX 0x32aff0
//   11   DoWrite(position, size, buffer, written)    pure   Liff 0x116c0  MacOSX 0x32b030
//   12   DoRead(size, buffer, read)                  pure   Liff 0x11790  MacOSX 0x32b070
//   13   DoWrite(size, buffer, written)              pure   Liff 0x117c0  MacOSX 0x32b230
//   14   DoSize()                                 pure   Liff 0x117f0  MacOSX 0x32b370
//   15   DoSize(size)                             pure   Liff 0x11810  MacOSX 0x32b3b0
//   16   DoPosition()                             pure   Liff 0x11880  MacOSX 0x32b4d0
//   17   DoPosition(position)                            pure   Liff 0x11890  MacOSX 0x32b500
// Every slot has a public wrapper that holds the global storage lock (Directory.cpp).
#pragma once
#include "LEGO/Types.h"
#include "DirectoryElement.h"
#include "ObjectFactory.h"

namespace LEGO {

// Open flags (name inferred). One byte of bit-fields; callers assign the four fields
// (Win: `m = m & 0xf5 | 5`, `m & 0xf7 | 7`). MacOSX (0x329570) opens for reading when
// bit 0 or 2 is set and for writing when bit 1 or 3 is set. Bits 2/3 are presumably share
// flags (names inferred).
struct OpenMode {
    OpenMode(bool read = false, bool write = false, bool shareRead = false, bool shareWrite = false)
        : m_read(read), m_write(write), m_shareRead(shareRead), m_shareWrite(shareWrite)
    {
    }

    bool m_read       : 1;   // 0x01
    bool m_write      : 1;   // 0x02 (required when a truncation size is passed to OpenFile)
    bool m_shareRead  : 1;   // 0x04
    bool m_shareWrite : 1;   // 0x08
};

// Mac sizeof 0x20, Win 0x14.
class File : public DirectoryElement {
public:
    // Arguments for ObjectFactory::CreateObject(typeid(File), ...): wrap a caller-owned memory
    // block as a (read-only) File. Served by MemoryStorageFactory (LiffStorageFactory.h), which
    // reads m_data at +0x04 and m_size at +0x08 (Mac 0x198e0, Win 0x42ff50).
    // Mac vtable 0x76e19c {D1 0x1b2900, D0 0x1b45b0} (emitted in TU 099), Win 0x9dc52c {sdd 0x5c807d}.
    // (member names inferred)
    struct WrapArgs : public ObjectFactoryArgs {
        WrapArgs(void* data, UInt64 size) : m_data(data), m_size(size) {}
        virtual ~WrapArgs() {}

        void*  m_data;   // +0x04 not copied, not freed
        UInt64 m_size;   // +0x08
    };

    // Inline (MacOSXFile ctor 0x32ad80 calls DirectoryElement ctor 0x128a0 and then stores
    // the File vtable and the mode byte itself).
    File(const String& name, Directory* parent, const OpenMode& mode)
        : DirectoryElement(name, parent), m_mode(mode)
    {
    }
    virtual ~File() {}                                                // Mac 0x11f40/0x11fc0, Win 0x4362e0

    // NOTE: the original File keeps Object::Release (Mac vtable slot 4 = 0x9280, Win slot 3 =
    // 0x412520): the final release (ZipStorageFile flushing into its volume, the share counters
    // of an entry, a LIFF chunk) runs without the storage lock that every other storage access
    // holds; fixed: Release takes the lock like Directory::Release. Directory.cpp.
    virtual void Release() const;

    virtual Attributes DoGetAttributes() const                       // Mac 0x12060, Win 0x4362c0
    {
        return Attributes(false, true);
    }

    virtual bool   DoRead(UInt64 position, UInt64 size, void* buffer, UInt64* read) = 0;
    virtual bool   DoWrite(UInt64 position, UInt64 size, const void* buffer, UInt64* written) = 0;
    virtual bool   DoRead(UInt64 size, void* buffer, UInt64* read) = 0;
    virtual bool   DoWrite(UInt64 size, const void* buffer, UInt64* written) = 0;
    virtual UInt64 DoSize() = 0;
    virtual bool   DoSize(UInt64 size) = 0;
    virtual UInt64 DoPosition() = 0;
    virtual bool   DoPosition(UInt64 position) = 0;

    // --- locked public interface (Directory.cpp) ------------------------------------------
    // `read`/`written` may be null.
    bool   ReadAt(UInt64 position, UInt64 size, void* buffer, UInt64* read);           // Mac 0x16690, Win 0x428e00
    bool   WriteAt(UInt64 position, UInt64 size, const void* buffer, UInt64* written); // Mac 0x16720, Win 0x428ea0
    bool   Read(UInt64 size, void* buffer, UInt64* read);                              // Mac 0x16c50, Win 0x428f30
    bool   Write(UInt64 size, const void* buffer, UInt64* written);                     // Mac 0x16cd0, Win 0x428fc0
    UInt64 GetSize();                                                                  // Mac 0x16620, Win 0x4292d0
    bool   SetSize(UInt64 size);                                                       // Mac 0x17140, Win 0x429350
    UInt64 GetPosition();                                                              // Mac 0x171b0, Win 0x4293e0
    bool   Seek(UInt64 position);                                                      // Mac 0x17220, Win 0x429460
    // Copies `size` bytes from offset `from` to offset `to` inside this file (overlap safe,
    // memmove-like; grows the file when needed). Used by LiffChunk to shift chunk data.
    bool   CopyRange(UInt64 from, UInt64 to, UInt64 size);                             // Mac 0x16d50, Win 0x429050

    const OpenMode& GetMode() const { return m_mode; }                // (name inferred; not seen)

protected:
    OpenMode m_mode;   // Mac +0x1c, Win +0x10
};

} // namespace LEGO
