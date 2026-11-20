// LEGO Digital Designer — restored source
// In-memory VFS backend: MemoryDirectoryNode (tree node; file data as malloc'ed / borrowed
// segments), MemoryDirectory (Directory view of a node), MemoryFile (open file on a node),
// MemoryProtocolDirectory ("memory" below the root, owns the root node). Names inferred.
// TU Mac 0x1aa1e0-0x1ac5df, Win 0x4393f0..; vtables MemoryDirectory Mac 0x71b8e8 / Win 0xa1feb4,
// MemoryFile Mac 0x71b898 / Win 0xa2028c. Node and protocol directory are header-only, first
// emitted in TU 014 (Mac 0x19b00-0x1a1ff).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Directory.h"
#include "File.h"

#include <stdlib.h>

namespace LEGO {

class MemoryFile;

// Mac sizeof 0x7c, Win 0x80. Mac vtable 0x75fbf4 {D1 0x19f70, D0 0x19f80 (body 0x1a100), Object
// slots}, Win 0xa1fe90 {sdd 0x42f960 (body 0x42f8e0)}.
class MemoryDirectoryNode : public Object {
public:
    // A run of file bytes [m_offset, m_offset + m_size) stored at m_data. Segments are sorted by
    // offset and contiguous. Copying transfers ownership of the block (auto_ptr-like; the
    // BasicArray grow path Mac 0x1ab9c0 clears the source flag after each element copy).
    // sizeof 0x10.
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
        ~FileDataSegment()                                            // Mac 0x1aa1e0, Win 0x439760 / 0x430fb0 (array)
        {
            if (m_owned)
                free(m_data);
        }

        UInt32       m_offset;   // +0x00
        UInt32       m_size;     // +0x04
        void*        m_data;     // +0x08
        mutable bool m_owned;    // +0x0c  m_data was malloc'ed by AllocateSegments
    };

    typedef BasicArray<FileDataSegment> SegmentArray;                 // vtable Mac 0x75fc1c, Win sdd 0x430f80
    typedef BasicMap<String, RefPointer<MemoryDirectoryNode>, StringCaseInsensitiveLess<String> > ChildMap; // vtable Mac 0x75fc38, Win sdd 0x430540

    // New node; the root (no parent) is not removable. Inline: Mac 0x19d70, Win 0x42f7f0.
    MemoryDirectoryNode(const String& name, bool isDirectory, MemoryDirectoryNode* parent)
        : m_openCount(0), m_readers(0), m_writers(0), m_denyRead(0), m_denyWrite(0),
          m_size(0), m_name(name), m_parent(parent),
          m_attributes(isDirectory, true, true, parent != 0)
    {
    }
    virtual ~MemoryDirectoryNode() {}                                 // Mac 0x19f70/0x19f80 (0x1a100), Win 0x42f8e0/0x42f960

    // The child called `name` (case-insensitive) or null. Inlined on Mac, Win 0x439650.
    MemoryDirectoryNode* FindChild(const String& name)
    {
        ChildMap::Iterator it = m_children.Find(name);
        if (it == m_children.End())
            return 0;
        return it->second.Get();
    }

    // Returns the existing child `name` (of any kind) or adds a new one. Never fails.
    MemoryDirectoryNode* CreateChild(const String& name, bool isDirectory);   // Mac 0x1aa450, Win 0x439540
    // Deletes this node from its parent (children first). Fails while a file is open on it, and
    // unless `recursive` or the node is removable. NOTE: a non-recursive delete of a removable
    // directory still deletes its (removable) children one by one.
    bool Delete(bool recursive);                                       // Mac 0x1aa200, Win 0x439780 (find+call) / 0x439800
    // Adds a Descriptor for every child whose name matches `pattern`.
    void Enumerate(const String& pattern, DescriptorSet& entries);      // Mac 0x1aae90, Win 0x4393f0

    // Layout (Mac / Win): vptr +0x00, refcount +0x04
    UInt32              m_openCount;   // +0x08 MemoryFile objects open on this node
    UInt32              m_readers;     // +0x0c opened with OpenMode::m_read
    UInt32              m_writers;     // +0x10 opened with OpenMode::m_write
    UInt32              m_denyRead;    // +0x14 opened without m_shareRead
    UInt32              m_denyWrite;   // +0x18 opened without m_shareWrite
    SegmentArray        m_segments;    // +0x1c file contents
    UInt32              m_size;        // +0x2c file size (<= end of the last segment)
    String              m_name;        // +0x30 (Mac 0x10 bytes, Win 4)
    MemoryDirectoryNode* m_parent;     // Mac +0x40, Win +0x34 (not owned)
    ChildMap            m_children;    // Mac +0x44, Win +0x38 (owning references)
    Times               m_times;       // +0x60
    Attributes          m_attributes;  // +0x78

private:
    MemoryDirectoryNode(const MemoryDirectoryNode&);
    MemoryDirectoryNode& operator=(const MemoryDirectoryNode&);
};

// Directory over a MemoryDirectoryNode. Mac sizeof 0x70, Win 0x78.
// Mac vtable 0x71b8e8 {D1 0x1abc50, D0 0x1abc60, ...}, Win 0xa1feb4 {sdd 0x42faf0, ...}.
class MemoryDirectory : public Directory {
public:
    // Named after `node` (or `name` when there is no node). By-value name: Win 0x42f980 destroys
    // it itself, Mac copies it at the inlined call sites. Inline: Mac in 0x1aa310 / 0x19b00,
    // Win 0x42f980.
    MemoryDirectory(String name, Directory* parent, MemoryDirectoryNode* node)
        : Directory(node ? node->m_name : name, parent), m_node(node)
    {
    }
    virtual ~MemoryDirectory();                                        // Mac 0x1abc50/0x1abc60, Win 0x42fa00/0x42faf0

    // --- DirectoryElement slots (inline; Mac code emitted in TU 014) ------------------------
    virtual bool DoSetAttributes(const Attributes& attributes)         // Mac 0x19ce0, Win 0x42fa50
    {
        m_node->m_attributes = attributes;
        return true;
    }
    virtual Attributes DoGetAttributes() const                         // Mac 0x19d00, Win 0x42fa70
    {
        return m_node->m_attributes;
    }
    virtual bool DoSetTimes(const Times& times)                        // Mac 0x19d10, Win 0x42fa80
    {
        m_node->m_times = times;
        return true;
    }
    virtual Times DoGetTimes() const                                   // Mac 0x19d40, Win 0x42fac0
    {
        return m_node->m_times;
    }

    // --- Directory slots ---------------------------------------------------------------------
    // NOTE: no existence/kind check — an unknown name yields a MemoryDirectory without a node
    // (callers check DoDirectoryExists first).
    virtual Directory* DoGetDirectory(const String& name);                                  // Mac 0x1aa310, Win 0x439880
    // NOTE: true also when `name` already exists as a file.
    virtual bool       DoCreateDirectory(const String& name);                               // Mac 0x1aa420, Win 0x439910
    virtual File*      DoOpenFile(const String& name, const OpenMode& mode, bool create, const UInt64* size); // Mac 0x1aa550, Win 0x439930
    virtual bool       DoDelete(const String& name, bool recursive);                        // Mac 0x1aa710, Win 0x439a50
    // Renames a closed child that is not loaded as a Directory object; `to` must be free.
    virtual bool       DoRename(const String& from, const String& to);                      // Mac 0x1aa7b0, Win 0x439a70
    virtual bool       DoGetChildAttributes(const String& name, Attributes* attributes);    // Mac 0x1aaa30, Win 0x439b00
    virtual bool       DoSetChildAttributes(const String& name, const Attributes& attributes); // Mac 0x1aaad0, Win 0x439b30
    virtual bool       DoGetChildTimes(const String& name, Times* times);                   // Mac 0x1aab70, Win 0x439b60
    virtual bool       DoSetChildTimes(const String& name, const Times& times);             // Mac 0x1aac20, Win 0x439bb0
    virtual bool       DoIsEmpty();                                                         // Mac 0x1aacd0, Win 0x439c00
    virtual bool       DoDirectoryExists(const BasicArray<String>& path);                   // Mac 0x1aacf0, Win 0x439c10
    virtual bool       DoFileExists(const String& name);                                    // Mac 0x1aadd0, Win 0x439c70
    virtual void       DoEnumerate(const String& pattern, DescriptorSet& entries);          // Mac 0x1aae70, Win 0x439ca0

    // Adds a read-only file over the caller's buffer (not copied, not freed) and returns it
    // (not AddRef'd). The file is named after the Base64 of the buffer address ('/' -> '-');
    // null if that name exists. Used by MemoryStorageFactory for File::WrapArgs. (name inferred)
    File* WrapMemory(void* data, UInt64 size);                         // Mac 0x1aafd0, Win 0x439cc0

    MemoryDirectoryNode* GetNode() const { return m_node; }            // (name inferred; not seen)

protected:
    MemoryDirectoryNode* m_node;   // Mac +0x6c, Win +0x74 (not owned)

private:
    MemoryDirectory(const MemoryDirectory&);
    MemoryDirectory& operator=(const MemoryDirectory&);
};

// Mac sizeof 0x28, Win 0x1c. Mac vtable 0x71b898 {D1 0x1abc00, D0 0x1abc10 (body 0x1abb50)},
// Win 0xa2028c {sdd 0x43a3c0 (body 0x43a4d0)}.
class MemoryFile : public File {
public:
    // Opens `node` with `mode` and registers the open in the node's counters.
    // Inline: Mac in 0x1aa550, Win 0x43a320.
    MemoryFile(Directory* parent, MemoryDirectoryNode* node, const OpenMode& mode)
        : File(node->m_name, parent, mode), m_position(0), m_node(node)
    {
        AddOpen();
    }
    // Read-only file over a borrowed buffer (see MemoryDirectory::WrapMemory).
    MemoryFile(Directory* parent, MemoryDirectoryNode* node, void* data, UInt64 size); // Mac 0x1ab890, Win 0x43a3e0
    virtual ~MemoryFile();                                             // Mac 0x1abc00/0x1abc10 (0x1abb50), Win 0x43a4d0/0x43a3c0

    // --- DirectoryElement slots (node metadata) -------------------------------------------
    virtual bool DoSetAttributes(const Attributes& attributes)         // Mac 0x1abca0, Win 0x4396c0
    {
        m_node->m_attributes = attributes;
        return true;
    }
    virtual Attributes DoGetAttributes() const                         // Mac 0x1abcc0, Win 0x4396e0
    {
        return m_node->m_attributes;
    }
    virtual bool DoSetTimes(const Times& times)                        // Mac 0x1abcd0, Win 0x4396f0
    {
        m_node->m_times = times;
        return true;
    }
    virtual Times DoGetTimes() const                                   // Mac 0x1abd00, Win 0x439730
    {
        return m_node->m_times;
    }

    // --- File slots ----------------------------------------------------------------------
    virtual bool   DoRead(UInt64 position, UInt64 size, void* buffer, UInt64* read);           // Mac 0x1ab370, Win 0x439e10
    virtual bool   DoWrite(UInt64 position, UInt64 size, const void* buffer, UInt64* written); // Mac 0x1ab3b0, Win 0x43e030
    virtual bool   DoRead(UInt64 size, void* buffer, UInt64* read);                            // Mac 0x1ab3f0, Win 0x439e60
    virtual bool   DoWrite(UInt64 size, const void* buffer, UInt64* written);                   // Mac 0x1ab530, Win 0x439f70
    virtual UInt64 DoSize();                                                                // Mac 0x1ab680, Win 0x43a0a0
    // Grows in 8 KiB-rounded malloc'ed segments; shrinking frees whole trailing segments.
    virtual bool   DoSize(UInt64 size);                                                     // Mac 0x1ab690, Win 0x43a1b0
    virtual UInt64 DoPosition();                                                            // Mac 0x1ab850, Win 0x43a2e0
    virtual bool   DoPosition(UInt64 position);                                                    // Mac 0x1ab860, Win 0x43a2f0

private:
    MemoryFile(const MemoryFile&);
    MemoryFile& operator=(const MemoryFile&);

    // Counter bookkeeping shared by the constructors (inlined). (name inferred)
    void AddOpen()
    {
        ++m_node->m_openCount;
        if (m_mode.m_read)
            ++m_node->m_readers;
        if (m_mode.m_write)
            ++m_node->m_writers;
        if (!m_mode.m_shareRead)
            ++m_node->m_denyRead;
        if (!m_mode.m_shareWrite)
            ++m_node->m_denyWrite;
    }

    UInt32               m_position;   // Mac +0x20, Win +0x14
    MemoryDirectoryNode* m_node;       // Mac +0x24, Win +0x18 (not owned; kept alive by the tree)
};

// The "memory" directory below the VFS root; owns the root node of the in-memory tree.
// Header-only (Mac vtable 0x75fb64 weak, code emitted in TU 014). Mac sizeof 0x74, Win 0x7c.
// Mac vtable {D1 0x19c10, D0 0x19c70, then MemoryDirectory's slots}, Win 0xa1ff34 {sdd 0x42fc80}.
class MemoryProtocolDirectory : public MemoryDirectory {
public:
    MemoryProtocolDirectory(const String& name, Directory* parent)    // Mac 0x19b00, Win 0x42fb10
        : MemoryDirectory(name, parent, 0)
    {
        m_root = new MemoryDirectoryNode(name, true, 0);
        m_node = m_root.Get();
    }
    virtual ~MemoryProtocolDirectory() {}                              // Mac 0x19c10/0x19c70, Win 0x42fc10/0x42fc80

private:
    RefPointer<MemoryDirectoryNode> m_root;   // Mac +0x70, Win +0x78
};

} // namespace LEGO
