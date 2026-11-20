// LEGO Digital Designer — restored source
// LEGO::LiffStorageDirectory / LiffStorageFile — Directory and File of a mounted LIFF archive
// (LiffStorageVolume), wrapping a MonolithTreeDirectory / MonolithTreeFile and its chunk; the
// n-th child entry lives in the n-th child chunk. TU Mac 0xedd0-0x1252f, Win 0x4362c0-0x4392ff.
// Writable only when the volume's File was opened with OpenMode::m_write (DoGetAttributes masks).
// Vtables: directory Mac 0x703d08 (31 slots; 14-16 inherited: 0x11920, 0x15df0, 0x15e60), Win
// 0xa201bc; file Mac 0x703d9c (18 slots), Win 0xa2023c. Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "Directory.h"
#include "File.h"
#include "MonolithTree.h"
#include "LiffStorageVolume.h"

namespace LEGO {

class LiffChunk;

// Mac sizeof 0x78, Win 0x80.
class LiffStorageDirectory : public Directory {
public:
    // `alias` (optional) is registered with AddAlias (the archive root is reachable both as
    // "Name" and "Name.lif").
    LiffStorageDirectory(const String& name, Directory* parent, LiffStorageVolume* volume,
                         MonolithTreeDirectory* entry, LiffChunk* chunk,
                         const String* alias = 0);                     // Mac 0x119b0, Win 0x4367e0
    virtual ~LiffStorageDirectory();                                   // Mac 0x118d0/0x118e0 (body 0x120c0), Win 0x4368a0/0x436930

    // --- DirectoryElement -------------------------------------------------------------------
    // The directory bit cannot be changed.
    virtual bool       DoSetAttributes(const Attributes& attributes);  // Mac 0x10bd0, Win 0x438530
    virtual Attributes DoGetAttributes() const;                        // Mac 0x10c00, Win 0x438560
    virtual bool       DoSetTimes(const Times& times);                 // Mac 0x10c30, Win 0x4385a0
    virtual Times      DoGetTimes() const;                             // Mac 0x10c70, Win 0x4385e0
    virtual bool       DoSetInfo(const Attributes& attributes, const Times& times); // Mac 0x10ca0, Win 0x438610

    // --- Directory --------------------------------------------------------------------------
    virtual Directory* DoGetDirectory(const String& name);             // Mac 0x10cf0, Win 0x438660
    virtual bool       DoCreateDirectory(const String& name);          // Mac 0x10eb0, Win 0x438780
    virtual File*      DoOpenFile(const String& name, const OpenMode& mode, bool create,
                                  const UInt64* size);                 // Mac 0x10ef0, Win 0x4387b0
    // `recursive` doubles as "force": it allows deleting a non-writable entry.
    virtual bool       DoDelete(const String& name, bool recursive);   // Mac 0x10fe0, Win 0x438840
    virtual bool       DoGetChildAttributes(const String& name, Attributes* attributes); // Mac 0x11030, Win 0x438870
    virtual bool       DoSetChildAttributes(const String& name, const Attributes& attributes); // Mac 0x110b0, Win 0x4388d0
    virtual bool       DoGetChildTimes(const String& name, Times* times); // Mac 0x11120, Win 0x438920
    virtual bool       DoSetChildTimes(const String& name, const Times& times); // Mac 0x111a0, Win 0x438980
    virtual bool       DoIsEmpty();                                    // Mac 0x11220, Win 0x4389e0
    virtual bool       DoDirectoryExists(const BasicArray<String>& path); // Mac 0x11240, Win 0x4389f0
    virtual bool       DoFileExists(const String& name);               // Mac 0x11300, Win 0x438a60
    virtual void       DoEnumerate(const String& pattern, DescriptorSet& entries); // Mac 0x11390, Win 0x438a90

    // File enumeration; the state lives in the MonolithTreeDirectory.
    virtual void             DoBeginEnumeration();                     // Mac 0x113d0, Win 0x438ac0
    virtual RefPointer<File> DoOpenCurrent(const OpenMode& mode, const UInt64* size); // Mac 0x11420, Win 0x438ad0
    virtual void             DoMoveNext();                             // Mac 0x11480, Win 0x438ba0
    virtual bool             DoIsEnumerationDone();                    // Mac 0x114d0, Win 0x438bb0
    virtual bool             DoIsEnumerating();                        // Mac 0x114f0, Win 0x438be0
    virtual void             DoEndEnumeration();                       // Mac 0x11500, Win 0x438bf0

protected:
    // Opens the `index`-th child chunk as a LiffStorageFile after checking the share modes
    // against the files already open on `entry`; `size` (optional) resizes it.
    File* OpenFile(MonolithTreeFile* entry, UInt32 index, const OpenMode& mode,
                   const UInt64* size);                                // Mac 0x10a60, Win 0x4383c0
    // Case-insensitive lookup of a child entry; `index` receives its position (or the child
    // count when not found). Static on Win (3 stack arguments), inlined everywhere on Mac.
    static MonolithTreeEntry* FindEntry(MonolithTreeDirectory* directory, const String& name,
                                        UInt32& index);                // Mac inlined, Win 0x436720
    // The archive file was opened for writing (volume -> LiffFile -> File -> OpenMode::m_write).
    bool IsWritable() const;                                           // (inlined)

    RefPointer<LiffStorageVolume> m_volume;   // Mac +0x6c, Win +0x74
    MonolithTreeDirectory*        m_entry;    // Mac +0x70, Win +0x78 (owned by the volume's tree)
    RefPointer<LiffChunk>         m_chunk;    // Mac +0x74, Win +0x7c
};

// Mac sizeof 0x34, Win 0x28.
class LiffStorageFile : public File {
public:
    // Registers the open mode with `entry`'s share counters.
    LiffStorageFile(Directory* parent, const OpenMode& mode, LiffStorageVolume* volume,
                    MonolithTreeFile* entry, LiffChunk* chunk);        // Mac 0x11df0, Win 0x436950
    virtual ~LiffStorageFile();                                        // Mac 0x11950/0x11960 (body 0x12150), Win 0x436a40/0x436b00

    // --- DirectoryElement -------------------------------------------------------------------
    virtual bool       DoSetAttributes(const Attributes& attributes);  // Mac 0x11510, Win 0x438c00
    virtual Attributes DoGetAttributes() const;                        // Mac 0x11540, Win 0x438c30
    virtual bool       DoSetTimes(const Times& times);                 // Mac 0x11570, Win 0x438c70
    virtual Times      DoGetTimes() const;                             // Mac 0x115b0, Win 0x438cb0
    virtual bool       DoSetInfo(const Attributes& attributes, const Times& times); // Mac 0x115e0, Win 0x438ce0

    // --- File (Win slot order differs for the overloads, see File.h) -------------------------
    virtual bool   DoRead(UInt64 position, UInt64 size, void* buffer, UInt64* read);         // Mac 0x11630, Win 0x438d30
    virtual bool   DoWrite(UInt64 position, UInt64 size, const void* buffer, UInt64* written); // Mac 0x116c0, Win 0x438dd0
    virtual bool   DoRead(UInt64 size, void* buffer, UInt64* read);                          // Mac 0x11790, Win 0x438ec0
    virtual bool   DoWrite(UInt64 size, const void* buffer, UInt64* written);                // Mac 0x117c0, Win 0x4425e0 (ICF)
    virtual UInt64 DoSize();                                        // Mac 0x117f0, Win 0x438ef0
    // NOTE: ignores the result of resizing the chunk.
    virtual bool   DoSize(UInt64 size);                             // Mac 0x11810, Win 0x438f00
    virtual UInt64 DoPosition();                                    // Mac 0x11880, Win 0x438f80
    virtual bool   DoPosition(UInt64 position);                            // Mac 0x11890, Win 0x438f90

protected:
    bool IsWritable() const;                                           // (inlined)

    RefPointer<LiffStorageVolume> m_volume;     // Mac +0x20, Win +0x14
    MonolithTreeFile*             m_entry;      // Mac +0x24, Win +0x18
    RefPointer<LiffChunk>         m_chunk;      // Mac +0x28, Win +0x1c
    UInt64                        m_position;   // Mac +0x2c, Win +0x20
};

} // namespace LEGO
