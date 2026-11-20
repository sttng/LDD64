// LEGO Digital Designer — restored source
// LEGO::MonolithTree — a LIFF archive's directory tree, one blob in chunk (1,5) below (1,1);
// MonolithTreeEntry / File / Directory are its nodes. In LiffStorageDirectory.cpp (TU 012, Mac
// 0xedd0-0x1252f). Serialised form (big-endian on x86, see Load/Save):
//   tree      := UInt16 1, directory-body                  (the root directory)
//   entry     := UInt32 flags, UTF-16 name, UInt16 0, UInt64 size + 0x14
//   directory := entry, UInt32 count, count x (UInt16 1, directory | UInt16 2, file)
//   file      := entry, UInt64 created, UInt64 modified, UInt64 accessed
// flags: bit0 readable, bit1 writable, bit2 removable, bit4 hidden (Attributes bits 1-4).
// size = the entry chunk's data size + the 20-byte chunk header. The n-th child entry belongs
// to the n-th child chunk.
//
// Vtables (Mac slot index; Win has one scalar-deleting dtor at slot 4):
//   slot                        Entry            File             Directory
//   0 Read(Stream&)             0xedd0/436b20    0xf180/436d40    0xf460/436f40
//   1 Write(Stream&)            0xeed0/436b90    0xf280/436d80    0xf670/437080
//   2 GetSerializedSize()       0xefe0/436c00    0xf380/436e00    0xf710/437170
//   3 Save()                    0xf000/436c20    (inherited)      (inherited)
//   4,5 ~ (D1, D0 / sdd)        0xf100,0xf130 / 436cd0  0xf3e0,0xf410 / 436ed0  0xff70,0xff80 / 437940
//   6 SetDataSize(size)                          0xf3b0/436e30
//   7 GetDataSize()                              0xf3d0/436e50
// Mac vtables: Entry 0x703c90, File 0x703cb0, Directory 0x703cd8, MonolithTree 0x703cf8.
// Win vtables: Entry 0xa2017c, File 0xa20194, Directory 0xa20164, MonolithTree 0xa2015c.
// All names inferred (RTTI gives only the class names).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicList.h"
#include "DirectoryElement.h"
#include "Stream.h"

namespace LEGO {

class LiffChunk;
class LiffStorageDirectory;
class LiffStorageFile;
class LiffStorageVolume;
class MonolithTree;
class MonolithTreeDirectory;

// Mac sizeof 0x40, Win 0x38 (MSVC pads the vptr to the 8-byte alignment of the UInt64s).
class MonolithTreeEntry {
public:
    // Appends the new entry to `parent`'s children (when given).
    MonolithTreeEntry(MonolithTree* tree, MonolithTreeDirectory* parent); // Win 0x436c30 (inlined on Mac)

    virtual bool   Read(Stream& stream);                               // Mac 0xedd0, Win 0x436b20
    virtual void   Write(Stream& stream);                              // Mac 0xeed0, Win 0x436b90
    virtual UInt64 GetSerializedSize() const;                          // Mac 0xefe0, Win 0x436c00
    // Writes the whole tree back to its chunk.
    virtual bool   Save();                                             // Mac 0xf000, Win 0x436c20
    virtual ~MonolithTreeEntry() {}                                    // Mac 0xf100/0xf130, Win 0x436cf0/0x436cd0

    const String&     GetName() const       { return m_name; }         // (not seen)
    const Attributes& GetAttributes() const { return m_attributes; }   // (not seen)
    const Times&      GetTimes() const      { return m_times; }        // (not seen)
    UInt64            GetSize() const       { return m_size; }         // (not seen)

protected:
    friend class MonolithTreeDirectory;
    friend class LiffStorageDirectory;
    friend class LiffStorageFile;

    Attributes             m_attributes;   // Mac +0x04, Win +0x08 (m_isDirectory is never serialised)
    String                 m_name;         // Mac +0x08, Win +0x0c
    UInt64                 m_size;         // Mac +0x18, Win +0x10 data size of the entry's chunk
    Times                  m_times;        // Mac +0x20, Win +0x18 (serialised for files only)
    MonolithTree*          m_tree;         // Mac +0x38, Win +0x30
    MonolithTreeDirectory* m_parent;       // Mac +0x3c, Win +0x34 (not used after construction)

private:
    MonolithTreeEntry(const MonolithTreeEntry&);
    MonolithTreeEntry& operator=(const MonolithTreeEntry&);
};

// Mac sizeof 0x54, Win 0x4c. The counters implement the share modes of LiffStorageFile
// (incremented by its constructor, decremented by its destructor).
class MonolithTreeFile : public MonolithTreeEntry {
public:
    MonolithTreeFile(MonolithTree* tree, MonolithTreeDirectory* parent) // Win 0x436e60 (inlined on Mac)
        : MonolithTreeEntry(tree, parent), m_openCount(0), m_readers(0), m_writers(0),
          m_denyRead(0), m_denyWrite(0)
    {
    }

    virtual bool   Read(Stream& stream);                               // Mac 0xf180, Win 0x436d40
    virtual void   Write(Stream& stream);                              // Mac 0xf280, Win 0x436d80
    virtual UInt64 GetSerializedSize() const;                          // Mac 0xf380, Win 0x436e00
    virtual ~MonolithTreeFile() {}                                     // Mac 0xf3e0/0xf410, Win 0x436ef0/0x436ed0
    // Stores the data size and saves the tree.
    virtual bool   SetDataSize(UInt64 size);                           // Mac 0xf3b0, Win 0x436e30
    virtual UInt64 GetDataSize() const;                                // Mac 0xf3d0, Win 0x436e50

protected:
    friend class MonolithTreeDirectory;
    friend class LiffStorageDirectory;
    friend class LiffStorageFile;

    UInt32 m_openCount;          // Mac +0x40, Win +0x38 open LiffStorageFile objects
    UInt32 m_readers;            // Mac +0x44, Win +0x3c opened with OpenMode::m_read
    UInt32 m_writers;            // Mac +0x48, Win +0x40 opened with OpenMode::m_write
    UInt32 m_denyRead;           // Mac +0x4c, Win +0x44 opened without m_shareRead
    UInt32 m_denyWrite;          // Mac +0x50, Win +0x48 opened without m_shareWrite
};

typedef BasicList<MonolithTreeEntry*> MonolithTreeEntryList;           // (name inferred)

// Mac sizeof 0x58, Win 0x68. Owns its children. Also carries the state of the file
// enumeration of the LiffStorageDirectory that wraps it (directories are skipped).
class MonolithTreeDirectory : public MonolithTreeEntry {
public:
    MonolithTreeDirectory(MonolithTree* tree, MonolithTreeDirectory* parent); // Mac 0xfd30, Win 0x4378c0

    // Reads the child count and the tagged children. NOTE: returns true when the count itself
    // cannot be read.
    virtual bool   Read(Stream& stream);                               // Mac 0xf460, Win 0x436f40
    virtual void   Write(Stream& stream);                              // Mac 0xf670, Win 0x437080
    virtual UInt64 GetSerializedSize() const;                          // Mac 0xf710, Win 0x437170
    virtual ~MonolithTreeDirectory();                                  // Mac 0xff70/0xff80 (body 0xfe70), Win 0x437960/0x437940

    // Child management; `chunk` is the chunk of this directory in the archive. Each one saves
    // the tree on success.
    bool CreateChildDirectory(const String& name, LiffChunk* chunk);       // Mac 0xf780, Win 0x437220
    MonolithTreeFile* CreateChildFile(const String& name, LiffChunk* chunk, const UInt64* size); // Mac 0xf8c0, Win 0x437350
    // `force` allows deleting an entry that is not writable.
    bool DeleteChild(const String& name, LiffChunk* chunk, bool force);    // Mac 0xfaf0, Win 0x4374b0
    // Adds a Descriptor for every child matching `pattern`; the writable/removable bits are
    // cleared unless `writable`.
    void Enumerate(const String& pattern, DescriptorSet& entries, bool writable); // Mac 0xfbd0, Win 0x437730

    // File enumeration (inlined on Mac).
    void BeginEnumeration();                                           // Mac inline (0x113d0), Win 0x4375d0
    void MoveNext();                                                   // Mac inline (0x11480), Win 0x437680
    bool IsEnumerationDone() { return m_enumerationIterator == m_children.End(); } // (inlined)

    const MonolithTreeEntryList& GetChildren() const { return m_children; } // (not seen)

protected:
    friend class MonolithTreeEntry;
    friend class LiffStorageDirectory;

    // Skips directories from the current position on.
    void SkipDirectories();                                            // (inlined)

    MonolithTreeEntryList           m_children;              // Mac +0x40, Win +0x38
    UInt32                          m_enumerationIndex;      // Mac +0x4c, Win +0x58 index of the current child
    MonolithTreeEntryList::Iterator m_enumerationIterator;   // Mac +0x50, Win +0x5c
    bool                            m_enumerating;           // Mac +0x54, Win +0x64
};

// Mac sizeof 0x10, Win 0x10.
class MonolithTree {
public:
    // Takes a reference on `chunk` and creates an empty root directory.
    explicit MonolithTree(LiffChunk* chunk);                           // Mac 0x100d0, Win 0x437ca0
    virtual ~MonolithTree();                                           // Mac 0x101f0/0x10270, Win 0x437d70/0x437d50

    // Reads the whole chunk and parses it (first UInt16 must be 1 = directory).
    bool Load();                                                       // Mac 0xffc0, Win 0x437a20
    // Serialises the tree (always byte-swapped, i.e. big-endian) and writes it to the chunk,
    // resizing it first. NOTE: ignores m_littleEndian.
    bool Save();                                                       // Mac 0xf020, Win 0x437b50

    MonolithTreeDirectory* GetRoot() const    { return m_root; }       // (inlined)
    void SetLittleEndian(bool littleEndian)   { m_littleEndian = littleEndian; } // (inlined)

protected:
    MonolithTreeDirectory* m_root;          // +0x04 owned
    bool                   m_littleEndian;  // +0x08 header flag bit 0: data is not byte-swapped
    RefPointer<LiffChunk>  m_chunk;         // +0x0c the tree chunk (1,5)

private:
    MonolithTree(const MonolithTree&);
    MonolithTree& operator=(const MonolithTree&);
};

} // namespace LEGO
