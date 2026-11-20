// LEGO Digital Designer — restored source
// ZIP archive storage of the VFS (LXF files are ZIPs): ZipStorageFactory (mounts a .zip met while
// resolving a path), ZipStorageVolume (open File, end record, entry headers, tree), ZipFileHeader
// (one entry), ZipTreeNode (tree from the entry names), ZipStorageDirectory / ZipStorageFile.
// TU Mac 0x10b7a0-0x11127f (83 functions), Win 0x43eec0-0x4446af. Stored entries opened read-only
// are read in place; others are inflated into FileDataSegments and written back (deflate level 9)
// when the last file object of a modified entry goes. New entries are inserted in place. zlib
// raw deflate (windowBits -15): Mac /usr/lib/libz.1.dylib, Win static zlib (original
// 1.2.7, LDD64 1.4).
//
// Vtables (Mac / Win):
//   ZipStorageDirectory 0x70ac94 (31 slots) / 0xa20414. Slots 9 DoSetInfo (0x12940), 14-16
//                       (DoRename 0x11920, DoMove 0x15df0, DoCopy 0x15e60) inherited.
//   ZipStorageFile      0x70ac34 (18 slots) / 0xa20494. Slot 2 OnFinalRelease overridden.
//   ZipStorageFactory   0x70ad24 / 0xa1fe70 {.., CreateObject, Initialize/Shutdown inherited}
//   ZipStorageVolume    0x70ac18 / 0xa203c4; ZipTreeNode 0x7640f0 / 0xa203f4;
//   ZipFileHeader       0x764138 / 0xa203d8 (Object slots only).
// Member names inferred; class names and FileDataSegment from RTTI.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "ObjectFactory.h"
#include "Directory.h"
#include "File.h"

#include <stdlib.h>

namespace LEGO {

class ZipStorageDirectory;
class ZipStorageFile;
class ZipStorageVolume;
class ZipTreeNode;

// ===========================================================================================
// ZipFileHeader — one entry of the archive (Mac sizeof 0x7c, Win 0x80).
// Constructor inline on Mac (in Mount 0x10c410 and CreateEntry 0x10b7a0), Win 0x43ed80.
// Destructor Mac D1 0x110430 / D0 0x1104b0, Win body 0x43ee40 / sdd 0x43eec0.
// ===========================================================================================
class ZipFileHeader : public Object {
public:
    // A run of the uncompressed entry data [m_offset, m_offset + m_size) at m_data (malloc'ed).
    // Copying transfers ownership of the block (BasicArray grow path Mac 0x110d50 clears the
    // source flag after each element copy). sizeof 0x10.
    struct FileDataSegment {
        FileDataSegment(UInt32 offset, UInt32 size, void* data, bool owned)
            : m_offset(offset), m_size(size), m_data(data), m_owned(owned)
        {
        }
        FileDataSegment(const FileDataSegment& o)
            : m_offset(o.m_offset), m_size(o.m_size), m_data(o.m_data), m_owned(o.m_owned)
        {
            o.m_owned = false;
        }
        FileDataSegment& operator=(const FileDataSegment& o)
        {
            if (&o != this) {
                if (m_owned)
                    free(m_data);
                m_offset = o.m_offset;
                m_size = o.m_size;
                m_data = o.m_data;
                m_owned = o.m_owned;
                o.m_owned = false;
            }
            return *this;
        }
        ~FileDataSegment()                                            // Win 0x442760
        {
            if (m_owned)
                free(m_data);
        }

        UInt32       m_offset;   // +0x00
        UInt32       m_size;     // +0x04
        void*        m_data;     // +0x08
        mutable bool m_owned;    // +0x0c
    };

    // vtable Mac 0x764164 {D1 0x110540, D0 0x1105a0}, Win 0xa203ec {sdd 0x444670};
    // PushBack Mac 0x110d50, Win 0x4445d0.
    typedef BasicArray<FileDataSegment> SegmentArray;

    ZipFileHeader()
        : m_centralOffset(0), m_localPrev(0), m_localNext(0), m_centralPrev(0), m_centralNext(0),
          m_versionMadeBy(0), m_versionNeeded(0), m_flags(0), m_method(0), m_crc(0),
          m_compressedSize(0), m_uncompressedSize(0), m_localOffset(0), m_times(), m_attributes(),
          m_dataOffset(0), m_segments(), m_dirty(false), m_openCount(0), m_readers(0),
          m_writers(0), m_denyRead(0), m_denyWrite(0)
    {
    }
    virtual ~ZipFileHeader() {}                                       // Mac 0x110430/0x1104b0, Win 0x43ee40/0x43eec0

    // Appends malloc'ed blocks for `size` more bytes starting at `offset` (advanced past the new
    // blocks). When the allocation fails, blocks of 16 KB and more are split into two halves
    // (rounded up to 1 KB) and retried. Mac 0x10f670 (register arguments), Win 0x442630.
    static bool AllocateSegments(SegmentArray& segments, UInt32& offset, UInt32 size);

    // Layout (Mac / Win): vptr +0x00, refcount +0x04
    UInt32         m_centralOffset;     // +0x08 central directory record (archive relative)
    ZipFileHeader* m_localPrev;         // +0x0c volume list in local-header order (not owned)
    ZipFileHeader* m_localNext;         // +0x10
    ZipFileHeader* m_centralPrev;       // +0x14 volume list in central-directory order (not owned)
    ZipFileHeader* m_centralNext;       // +0x18
    UInt16         m_versionMadeBy;     // +0x1c (high byte = host system)
    UInt16         m_versionNeeded;     // +0x1e
    UInt16         m_flags;             // +0x20 general purpose flags (1 encrypted, 8 data descriptor)
    UInt16         m_method;            // +0x22 0 stored, 8 deflated
    UInt32         m_crc;               // +0x24
    UInt32         m_compressedSize;    // +0x28
    UInt32         m_uncompressedSize;  // +0x2c
    UInt32         m_localOffset;       // +0x30 local header (archive relative)
    Times          m_times;             // Mac +0x34, Win +0x38 (the DOS time stamp is not converted)
    Attributes     m_attributes;        // Mac +0x4c, Win +0x50
    UInt32         m_dataOffset;        // Mac +0x50, Win +0x54 entry data; 0 = local header not validated yet
    SegmentArray   m_segments;          // Mac +0x54, Win +0x58 uncompressed data while open (may be empty)
    bool           m_dirty;             // Mac +0x64, Win +0x68 m_segments differ from the archive
    UInt32         m_openCount;         // Mac +0x68, Win +0x6c open ZipStorageFile objects
    UInt32         m_readers;           // Mac +0x6c, Win +0x70 opened with OpenMode::m_read
    UInt32         m_writers;           // Mac +0x70, Win +0x74 opened with OpenMode::m_write
    UInt32         m_denyRead;          // Mac +0x74, Win +0x78 opened without m_shareRead
    UInt32         m_denyWrite;         // Mac +0x78, Win +0x7c opened without m_shareWrite

private:
    ZipFileHeader(const ZipFileHeader&);
    ZipFileHeader& operator=(const ZipFileHeader&);
};

// ===========================================================================================
// ZipTreeNode — one directory or file name of the archive (sizeof 0x44 on both builds).
// A node has a header when the archive has an entry for it (directories may be implicit).
// ===========================================================================================
class ZipTreeNode : public Object {
public:
    // vtable Mac 0x76411c {D1 0x110240, D0 0x1102c0}, Win 0xa20408 {sdd 0x443720};
    // node eraser Mac 0x110310, Win 0x443540; operator[] Mac 0x110610 (+ 0x110740 / 0x110a30 /
    // 0x110b10), Win 0x443800; erase(range) Mac 0x110c70.
    typedef BasicMap<String, RefPointer<ZipTreeNode>, StringCaseInsensitiveLess<String> > ChildMap;

    ZipTreeNode(const String& name, ZipFileHeader* header, ZipTreeNode* parent); // Mac 0x110180, Win 0x43eee0
    virtual ~ZipTreeNode();                                           // Mac 0x110270/0x110280 (body 0x110390), Win 0x43f3d0/0x43f450

    // The child `name` (case-insensitive) or null. Inlined on Mac. (Win 0x43f250)
    ZipTreeNode* GetChild(const String& name);
    // Returns the existing child `name`, else a new child with `header`. NOTE: an existing
    // child keeps its header (a duplicate entry name is ignored).
    ZipTreeNode* AddChild(const String& name, ZipFileHeader* header);   // Mac 0x10d250, Win 0x43f2c0
    // Appends the '/' separated path of this node below the archive root to `path`.
    void         GetPath(String& path) const;                         // Mac 0x10d380, Win 0x43f210
    // Attributes / times of the entry; a node without header is a directory without times.
    const Attributes& GetAttributes() const;                          // inline on Mac, Win 0x43f020
    const Times&      GetTimes() const;                               // inline on Mac, Win 0x43ef90
    // Adds the children matching `pattern` to `entries`. NOTE: `writable` is not used.
    void Enumerate(const String& pattern, DescriptorSet& entries, bool writable); // Mac 0x10e440, Win 0x43f0b0
    // File enumeration over m_children (directories are skipped).
    void BeginEnumeration();                                          // inline on Mac, Win 0x43f830
    void MoveNext();                                                  // inline on Mac, Win 0x43f8b0
    // Removes every child entry from the archive (recursively). Fails for an entry that is not
    // removable unless `force`. NOTE: the children are removed even without `force`.
    bool DeleteChildren(bool force, ZipStorageVolume* volume);        // Mac 0x10bf90, Win 0x43f9b0
    // Removes the child `name` (and its children) unless one of its files is open.
    bool DeleteChild(const String& name, bool force, ZipStorageVolume* volume); // inline on Mac, Win 0x43f910

    String                    m_name;             // +0x08
    RefPointer<ZipFileHeader> m_header;           // Mac +0x18, Win +0x0c
    ZipTreeNode*              m_parent;           // Mac +0x1c, Win +0x10 (not owned)
    ChildMap                  m_children;         // Mac +0x20, Win +0x14
    ChildMap::Iterator        m_enumIterator;     // Mac +0x3c, Win +0x38
    bool                      m_enumerating;      // +0x40

private:
    ZipTreeNode(const ZipTreeNode&);
    ZipTreeNode& operator=(const ZipTreeNode&);
};

// ===========================================================================================
// ZipStorageVolume — one mounted archive (sizeof 0x40 on both builds).
// Created by ZipStorageFactory::CreateObject, which keeps no reference: the volume lives as long
// as the ZipStorageDirectory/ZipStorageFile objects referencing it. Offsets are relative to the
// start of the archive inside the file (m_baseOffset, always 0: Mount rejects anything else).
// ===========================================================================================
class ZipStorageVolume : public Object {
public:
    explicit ZipStorageVolume(File* file);                            // Mac inline in 0x10fad0, Win 0x43f470
    virtual ~ZipStorageVolume();                                      // Mac 0x10d710/0x10d720 (body 0x10d660), Win 0x4417a0/0x43f5b0

    // Reads the end of central directory record and every central directory record (an empty
    // file first gets an empty record), builds the tree and creates the root directory named
    // `name` (".zip" stripped, alias with ".zip") below `parent`. Fails if already mounted, for
    // multi-disk archives and for data in front of the archive. NOTE: `create` is not used.
    bool Mount(bool create, const String& name, Directory* parent);   // Mac 0x10c410, Win 0x43fbd0

    // Adds an empty entry `name` (a directory when `directory`) below `parent`: inserts the local
    // header after the last entry's data and the central record at the end of the central
    // directory. `localSize`/`centralSize` (optional) receive the sizes of the new records.
    ZipTreeNode* CreateEntry(const String& name, bool directory, ZipTreeNode* parent,
                             UInt32* localSize, UInt32* centralSize); // Mac 0x10b7a0, Win 0x440770
    // Removes the entry of `node` (local header + data and central record) from the archive and
    // the node from its parent. NOTE: crashes for a node without header (implicit directory).
    void RemoveEntry(ZipTreeNode* node);                              // Mac 0x10c000, Win 0x4413e0
    // Resizes the stored data of `header` to `size` bytes (moving everything behind it) and
    // clears the data-descriptor flag.
    bool ResizeEntry(ZipFileHeader* header, UInt64 size);             // Mac 0x10d3e0, Win 0x441610

    ZipStorageDirectory* GetRoot() const     { return m_root; }       // (inlined)
    File*                GetFile() const     { return m_file.Get(); } // (inlined)
    ZipTreeNode*         GetRootNode() const { return m_rootNode.Get(); } // (inlined)
    // The archive file was opened for writing.
    bool                 IsWritable() const  { return m_file->GetMode().m_write; } // (inlined)

    // Archive relative access to the file. Inlined on Mac.
    bool ReadAt(UInt64 position, UInt64 size, void* buffer, UInt64* read)       // Win 0x43f510
    {
        return m_file->ReadAt(m_baseOffset + position, size, buffer, read);
    }
    bool WriteAt(UInt64 position, UInt64 size, const void* buffer)              // Win 0x43f540
    {
        return m_file->WriteAt(m_baseOffset + position, size, buffer, 0);
    }
    bool CopyRange(UInt64 from, UInt64 to, UInt64 size)                         // Win 0x43f570
    {
        return m_file->CopyRange(m_baseOffset + from, m_baseOffset + to, size);
    }

protected:
    friend class ZipStorageDirectory;
    friend class ZipStorageFile;
    friend class ZipTreeNode;

    // Size of the local header + data (+ data descriptor) of `header`, from its local header.
    bool GetLocalEntrySize(const ZipFileHeader* header, UInt32* size);          // inline on Mac, Win 0x441830
    // Moves the local headers of `first` and its local-list successors by `delta` and updates
    // their central records. Stops at the first write error.
    bool AdjustLocalOffsets(ZipFileHeader* first, Int32 delta);                 // inline on Mac, Win 0x4418d0
    // Writes the entry counts, size and offset of the central directory into the end record.
    void WriteEndOfCentralDirectory();                                          // inline on Mac, Win 0x441930

    // Layout (Mac and Win): vptr +0x00, refcount +0x04
    ZipStorageDirectory*      m_root;            // +0x08 created by Mount (not owned)
    ZipFileHeader*            m_localFirst;      // +0x0c entries by local header offset (not owned)
    ZipFileHeader*            m_localLast;       // +0x10
    ZipFileHeader*            m_centralFirst;    // +0x14 entries by central record offset (not owned)
    ZipFileHeader*            m_centralLast;     // +0x18
    RefPointer<ZipTreeNode>   m_rootNode;        // +0x1c
    RefPointer<File>          m_file;            // +0x20
    UInt32                    m_baseOffset;      // +0x24 start of the archive in the file
    UInt32                    m_dataEnd;         // +0x28 end of the last entry's data (0 = not known yet)
    UInt32                    m_centralOffset;   // +0x2c start of the central directory
    UInt32                    m_centralEnd;      // +0x30 end of the central directory records
    UInt32                    m_endRecordOffset; // +0x34 end of central directory record
    UInt32                    m_centralSize;     // +0x38
    UInt16                    m_entryCount;      // +0x3c

private:
    ZipStorageVolume(const ZipStorageVolume&);
    ZipStorageVolume& operator=(const ZipStorageVolume&);
};

// ===========================================================================================
// ZipStorageDirectory — Mac sizeof 0x74, Win 0x7c.
// ===========================================================================================
class ZipStorageDirectory : public Directory {
public:
    // Directory for `node` named like the node. Inline on Mac (in DoGetDirectory), Win 0x43f5d0.
    ZipStorageDirectory(Directory* parent, ZipStorageVolume* volume, ZipTreeNode* node)
        : Directory(node->m_name, parent), m_volume(volume), m_node(node)
    {
    }
    // Archive root: named `name` and registered under `alias` too.
    ZipStorageDirectory(Directory* parent, ZipStorageVolume* volume, ZipTreeNode* node,
                        const String& name, const String& alias);    // Mac 0x110bb0, Win 0x43f660
    virtual ~ZipStorageDirectory() {}                                 // Mac 0x10ffa0/0x110020, Win 0x43f700/0x43f780

    // --- DirectoryElement -------------------------------------------------------------------
    virtual bool       DoSetAttributes(const Attributes& attributes);  // Mac 0x10d760, Win 0x4419a0
    virtual Attributes DoGetAttributes() const;                        // Mac 0x10d780, Win 0x4419c0
    virtual bool       DoSetTimes(const Times& times);                 // Mac 0x10d7e0, Win 0x4419e0
    virtual Times      DoGetTimes() const;                             // Mac 0x10d820, Win 0x441a20

    // --- Directory --------------------------------------------------------------------------
    // NOTE: does not check that `name` exists (callers ask DoDirectoryExists first).
    virtual Directory* DoGetDirectory(const String& name);             // Mac 0x10d8c0, Win 0x441a60
    virtual bool       DoCreateDirectory(const String& name);          // Mac 0x10d9e0, Win 0x441ae0
    virtual File*      DoOpenFile(const String& name, const OpenMode& mode, bool create,
                                  const UInt64* size);                 // Mac 0x10da20, Win 0x441b10
    // `recursive` doubles as "force" (see ZipTreeNode::DeleteChildren).
    virtual bool       DoDelete(const String& name, bool recursive);   // Mac 0x10dda0, Win 0x441ba0
    virtual bool       DoGetChildAttributes(const String& name, Attributes* attributes); // Mac 0x10de80, Win 0x441bd0
    virtual bool       DoSetChildAttributes(const String& name, const Attributes& attributes); // Mac 0x10df60, Win 0x441c00
    virtual bool       DoGetChildTimes(const String& name, Times* times); // Mac 0x10e000, Win 0x441c30
    virtual bool       DoSetChildTimes(const String& name, const Times& times); // Mac 0x10e130, Win 0x441c80
    virtual bool       DoIsEmpty();                                    // Mac 0x10e1f0, Win 0x441cd0
    virtual bool       DoDirectoryExists(const BasicArray<String>& path); // Mac 0x10e210, Win 0x441ce0
    virtual bool       DoFileExists(const String& name);               // Mac 0x10e330, Win 0x441d50
    virtual void       DoEnumerate(const String& pattern, DescriptorSet& entries); // Mac 0x10e410, Win 0x441d80

    // File enumeration; the state lives in the ZipTreeNode.
    virtual void             DoBeginEnumeration();                     // Mac 0x10e670, Win 0x441da0
    virtual RefPointer<File> DoOpenCurrent(const OpenMode& mode, const UInt64* size); // Mac 0x10e700, Win 0x441db0
    virtual void             DoMoveNext();                             // Mac 0x10e760, Win 0x441e90
    virtual bool             DoIsEnumerationDone();                    // Mac 0x10e7f0, Win 0x441ea0
    virtual bool             DoIsEnumerating();                        // Mac 0x10e810, Win 0x441ed0
    virtual void             DoEndEnumeration();                       // Mac 0x10e820, Win 0x441ee0

protected:
    // Checks the share modes and the access rights, validates the local header of the entry
    // (first open only) and opens it.
    File* OpenFile(ZipTreeNode* node, const OpenMode& mode, const UInt64* size); // Mac 0x10dbd0, Win 0x441fc0
    // Creates the file object; loads the entry data into memory unless it is stored and opened
    // read-only, then applies `size`.
    File* CreateFile(ZipTreeNode* node, const OpenMode& mode, const UInt64* size); // Mac 0x10db20, Win 0x441ef0

    RefPointer<ZipStorageVolume> m_volume;   // Mac +0x6c, Win +0x74
    RefPointer<ZipTreeNode>      m_node;     // Mac +0x70, Win +0x78
};

// ===========================================================================================
// ZipStorageFile — Mac sizeof 0x30, Win 0x28.
// ===========================================================================================
class ZipStorageFile : public File {
public:
    // Registers the open mode with the entry's share counters.
    ZipStorageFile(Directory* parent, const OpenMode& mode, ZipStorageVolume* volume,
                   ZipTreeNode* node);                                 // Mac 0x10f760, Win 0x442ff0
    // The last file object of an entry drops the in-memory data.
    virtual ~ZipStorageFile();                                         // Mac 0x10fa50/0x10fa60 (body 0x10f8b0), Win 0x4430f0/0x4430d0

    // --- Object -----------------------------------------------------------------------------
    // The last file object of a modified entry writes it back (Flush).
    virtual void OnFinalRelease();                                     // Mac 0x10faa0, Win 0x4431c0

    // --- DirectoryElement -------------------------------------------------------------------
    virtual bool       DoSetAttributes(const Attributes& attributes);  // Mac 0x10eb00, Win 0x442250
    virtual Attributes DoGetAttributes() const;                        // Mac 0x10eb20, Win 0x442270
    virtual bool       DoSetTimes(const Times& times);                 // Mac 0x10eb80, Win 0x442290
    virtual Times      DoGetTimes() const;                             // Mac 0x10ebc0, Win 0x4422d0

    // --- File (Win slot order differs for the overloads, see File.h) -------------------------
    virtual bool   DoRead(UInt64 position, UInt64 size, void* buffer, UInt64* read);         // Mac 0x10ec60, Win 0x442310
    virtual bool   DoWrite(UInt64 position, UInt64 size, const void* buffer, UInt64* written); // Mac 0x10ee20, Win 0x4424a0
    virtual bool   DoRead(UInt64 size, void* buffer, UInt64* read);                          // Mac 0x10ef60, Win 0x438ec0 (ICF)
    virtual bool   DoWrite(UInt64 size, const void* buffer, UInt64* written);                // Mac 0x10ef90, Win 0x4425e0 (ICF)
    virtual UInt64 DoSize();                                           // Mac 0x10efc0, Win 0x442610
    virtual bool   DoSize(UInt64 size);                                // Mac 0x10efe0, Win 0x4427b0
    virtual UInt64 DoPosition();                                       // Mac 0x10f0f0, Win 0x438f80 (ICF)
    virtual bool   DoPosition(UInt64 position);                        // Mac 0x10f100, Win 0x438f90 (ICF)

    // Reads (and inflates) the entry data into memory unless it is empty or already loaded.
    bool LoadData();                                                   // Mac 0x10e830, Win 0x442d80
    // Writes modified data back into the archive (deflated when that is smaller, else stored).
    bool Flush();                                                      // Mac 0x10f140, Win 0x442880

protected:
    // Allocates at least `size` more bytes (rounded up to 8 KB) behind the last segment.
    bool GrowData(UInt32 size);                                        // inline on Mac, Win 0x442d30

    RefPointer<ZipStorageVolume> m_volume;     // Mac +0x20, Win +0x14
    RefPointer<ZipTreeNode>      m_node;       // Mac +0x24, Win +0x18
    UInt64                       m_position;   // Mac +0x28, Win +0x20
};

// ===========================================================================================
// ZipStorageFactory — header-only (weak vtable); D1 Mac 0x1100b0, D0 0x110110, Win sdd 0x42f7d0.
// ===========================================================================================
class ZipStorageFactory : public SpecificObjectFactory {
public:
    ZipStorageFactory() {}
    virtual ~ZipStorageFactory() {}                                    // Mac 0x1100b0/0x110110, Win 0x42f780/0x42f7d0

    // Directory::OpenArgs: resolves args.m_path below args.m_directory to an archive file
    // (appending ".zip" when the file does not exist and args are not strict), opens it
    // read/write — falling back to read-only when not strict — and mounts it. Returns the new
    // root ZipStorageDirectory (not AddRef'd) or null.
    virtual Object* CreateObject(const ObjectFactoryArgs& args);       // Mac 0x10fad0, Win 0x4431e0
};

} // namespace LEGO
