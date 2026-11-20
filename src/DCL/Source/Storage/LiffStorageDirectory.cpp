// LEGO Digital Designer — restored source
// TU "LiffStorageDirectory.cpp" Mac 0xedd0-0x1252f (95 functions), Win 0x4362c0-0x4392ff: LIFF
// archive storage — MonolithTree (MonolithTree.h, serialised with Stream.h),
// LiffStorageVolume::Mount, LiffStorageDirectory / LiffStorageFile. Names inferred.
//
// Library / inline code emitted in this TU (not written out here):
//   Mac 0xfe20/0x12440 BasicList<MonolithTreeEntry*> D1/D0 (Win 0x439060/0x4390e0; Win list
//   helpers 0x439000 ctor, 0x4390b0 erase, 0x439100 clear, 0x439170 push_back), 0x11a80
//   Directory dtor body, 0x11c30/0x11c90 + 0x11ce0 DescriptorSet dtors / node eraser
//   (Win 0x42a050/0x42a9c0), 0x11c60/0x11d40 + 0x11d90 Directory::ChildMap dtors / node eraser
//   (Win 0x429fe0/0x429e00), 0x122b0/0x12360 DescriptorSet insert_unique/_M_insert,
//   0x11f40/0x11fc0 File D1/D0, 0x12050-0x12080 DirectoryElement/File default slots,
//   0x11920 Directory::DoRename (all from Directory.h/File.h/DirectoryElement.h);
//   Win 0x439290 dynamic_cast<MonolithTreeDirectory*> thunk, 0x438fc0 RefPointer assignment.
// Mac static initialiser 0x12490: <iostream> Init and the five chunk ids below
// (Mac 0x77d02a..0x77d03a).
//
// Win-only out-of-line copies of Stream members: see Stream.h. Win 0x436350 / 0x436370 are
// one-line forwarders to Stream::Write(UInt16) / Write(UInt32) (callers not located).
#include "LiffStorageDirectory.h"
#include "LiffStorageVolume.h"
#include "LiffChunk.h"
#include "MonolithTree.h"
#include "Stream.h"

#include <iostream>

namespace LEGO {

namespace {

// Chunk ids of the archive layout (dynamic initialiser Mac 0x12490):
//   (1,1) root chunk, child of the LiffFile
//     (1,2) header: UInt16 version (1), UInt32 flags, big-endian
//     (1,3) root directory; a directory chunk holds one child chunk per tree entry:
//           (1,3) for sub-directories, (1,4) for files (leaf chunks with the file data)
//     (1,5) the serialised MonolithTree (leaf)
const ChunkId kRootChunk(1, 1);        // Mac 0x77d02a
const ChunkId kHeaderChunk(1, 2);      // Mac 0x77d02e
const ChunkId kDirectoryChunk(1, 3);   // Mac 0x77d032
const ChunkId kFileChunk(1, 4);        // Mac 0x77d036
const ChunkId kTreeChunk(1, 5);        // Mac 0x77d03a

const UInt32 kMaxTreeDepth = 256;      // (not in the original; H3) folder levels of a tree

// Clears the writable/removable bits of `attributes` unless the archive is writable.
inline Attributes MaskAttributes(Attributes attributes, bool writable)
{
    attributes.m_writable  = attributes.m_writable && writable;
    attributes.m_removable = attributes.m_removable && writable;
    return attributes;
}

inline bool IsVolumeWritable(const LiffStorageVolume* volume)
{
    return volume->GetFile()->GetFile()->GetMode().m_write;
}

} // namespace

// ===========================================================================================
// MonolithTreeEntry
// ===========================================================================================

// Win 0x436c30 (inlined on Mac)
MonolithTreeEntry::MonolithTreeEntry(MonolithTree* tree, MonolithTreeDirectory* parent)
    : m_attributes(), m_name(), m_size(0), m_times(), m_tree(tree), m_parent(parent)
{
    if (parent)
        parent->m_children.PushBack(this);
}

// Mac 0xedd0, Win 0x436b20
bool MonolithTreeEntry::Read(Stream& stream)
{
    UInt32 flags;
    stream.Read(flags);
    stream.Read(m_name);
    stream.Read(m_size);
    m_size -= ChunkHeader::kSize;   // stored including the chunk header
    m_attributes.m_readable  = (flags & 0x01) != 0;
    m_attributes.m_writable  = (flags & 0x02) != 0;
    m_attributes.m_removable = (flags & 0x04) != 0;
    m_attributes.m_hidden    = (flags & 0x10) != 0;
    return true;
}

// Mac 0xeed0, Win 0x436b90
void MonolithTreeEntry::Write(Stream& stream)
{
    UInt32 flags = (m_attributes.m_readable ? 0x01 : 0) | (m_attributes.m_writable ? 0x02 : 0) |
                   (m_attributes.m_removable ? 0x04 : 0) | (m_attributes.m_hidden ? 0x10 : 0);
    stream.Write(flags);
    stream.Write(m_name);
    stream.Write(static_cast<UInt64>(m_size + ChunkHeader::kSize));
}

// Mac 0xefe0, Win 0x436c00
// Type tag (2) + flags (4) + name + terminator (2) + size (8).
UInt64 MonolithTreeEntry::GetSerializedSize() const
{
    return m_name.GetLength() * 2 + 0x10;
}

// Mac 0xf000, Win 0x436c20
bool MonolithTreeEntry::Save()
{
    return m_tree->Save();
}

// ===========================================================================================
// MonolithTreeFile
// ===========================================================================================

// Mac 0xf180, Win 0x436d40
bool MonolithTreeFile::Read(Stream& stream)
{
    bool result = MonolithTreeEntry::Read(stream);
    stream.Read(m_times.m_created);
    stream.Read(m_times.m_modified);
    stream.Read(m_times.m_accessed);
    return result;
}

// Mac 0xf280, Win 0x436d80
void MonolithTreeFile::Write(Stream& stream)
{
    stream.Write(static_cast<UInt16>(2));
    MonolithTreeEntry::Write(stream);
    stream.Write(m_times.m_created);
    stream.Write(m_times.m_modified);
    stream.Write(m_times.m_accessed);
}

// Mac 0xf380, Win 0x436e00
UInt64 MonolithTreeFile::GetSerializedSize() const
{
    return MonolithTreeEntry::GetSerializedSize() + 3 * 8;
}

// Mac 0xf3b0, Win 0x436e30
bool MonolithTreeFile::SetDataSize(UInt64 size)
{
    m_size = size;
    return Save();
}

// Mac 0xf3d0, Win 0x436e50
UInt64 MonolithTreeFile::GetDataSize() const
{
    return m_size;
}

// ===========================================================================================
// MonolithTreeDirectory
// ===========================================================================================

// Mac 0xfd30, Win 0x4378c0
// NOTE: m_enumerationIndex is left uninitialised (both builds); fixed: 0 (low).
MonolithTreeDirectory::MonolithTreeDirectory(MonolithTree* tree, MonolithTreeDirectory* parent)
    : MonolithTreeEntry(tree, parent), m_children(), m_enumerationIterator(), m_enumerating(false)
{
    m_enumerationIndex = 0;
    m_attributes.m_isDirectory = true;
}

// Mac 0xfe70 (D1 0xff70, D0 0xff80), Win 0x437960 (sdd 0x437940)
MonolithTreeDirectory::~MonolithTreeDirectory()
{
    while (!m_children.IsEmpty()) {
        MonolithTreeEntry* child = m_children.Front();
        m_children.PopFront();
        delete child;
    }
}

// Mac 0xf460, Win 0x436f40
bool MonolithTreeDirectory::Read(Stream& stream)
{
    MonolithTreeEntry::Read(stream);
    UInt32 count;
    if (!stream.Read(count))
        return true;   // NOTE: a missing child count is not an error
    for (; count != 0; --count) {
        UInt16 type;
        if (!stream.Read(type))
            return false;
        MonolithTreeEntry* child;
        if (type == 2)
            child = new MonolithTreeFile(m_tree, this);
        else if (type == 1) {
            // NOTE: the nesting is not limited (both builds): a deep chain of folders in the
            // tree chunk overflows the stack here (and in the destructor); fixed: at most
            // kMaxTreeDepth levels (H3).
            UInt32 depth = 0;
            for (const MonolithTreeEntry* e = this; e; e = e->m_parent)
                ++depth;
            if (depth >= kMaxTreeDepth)
                return false;
            child = new MonolithTreeDirectory(m_tree, this);
        } else
            return false;
        if (!child->Read(stream))
            return false;
    }
    return true;
}

// Mac 0xf670, Win 0x437080
void MonolithTreeDirectory::Write(Stream& stream)
{
    stream.Write(static_cast<UInt16>(1));
    MonolithTreeEntry::Write(stream);
    stream.Write(m_children.GetSize());
    for (MonolithTreeEntryList::Iterator it = m_children.Begin(); it != m_children.End(); ++it)
        (*it)->Write(stream);
}

// Mac 0xf710, Win 0x437170
UInt64 MonolithTreeDirectory::GetSerializedSize() const
{
    UInt64 size = MonolithTreeEntry::GetSerializedSize() + 4;
    for (MonolithTreeEntryList::ConstIterator it = m_children.Begin(); it != m_children.End(); ++it)
        size += (*it)->GetSerializedSize();
    return size;
}

// Mac 0xf780, Win 0x437220
// NOTE: fails only when an existing child of that name is non-null; the chunk is appended at
// the end of `chunk` like the entry at the end of the children.
bool MonolithTreeDirectory::CreateChildDirectory(const String& name, LiffChunk* chunk)
{
    for (MonolithTreeEntryList::Iterator it = m_children.Begin(); it != m_children.End(); ++it) {
        if (name.EqualsNoCase((*it)->m_name)) {
            if (*it)
                return false;
            break;
        }
    }
    RefPointer<LiffChunk> child = chunk->CreateChunk(kDirectoryChunk, 0, 0);
    if (!child)
        return false;
    MonolithTreeDirectory* directory = new MonolithTreeDirectory(m_tree, this);
    directory->m_name = name;
    directory->m_attributes.m_readable  = true;
    directory->m_attributes.m_writable  = true;
    directory->m_attributes.m_removable = true;
    return Save();
}

// Mac 0xf8c0, Win 0x437350
// NOTE: when saving fails the new entry stays in the tree but 0 is returned.
MonolithTreeFile* MonolithTreeDirectory::CreateChildFile(const String& name, LiffChunk* chunk,
                                                    const UInt64* size)
{
    for (MonolithTreeEntryList::Iterator it = m_children.Begin(); it != m_children.End(); ++it) {
        if (name.EqualsNoCase((*it)->m_name)) {
            if (*it)
                return 0;
            break;
        }
    }
    RefPointer<LiffChunk> child = chunk->CreateChunk(kFileChunk, ChunkHeader::kFlagLeaf, size);
    if (!child)
        return 0;
    MonolithTreeFile* file = new MonolithTreeFile(m_tree, this);
    file->m_name = name;
    file->m_attributes.m_readable  = true;
    file->m_attributes.m_writable  = true;
    file->m_attributes.m_removable = true;
    file->m_size  = size ? *size : 0;
    file->m_times = Times();
    return Save() ? file : 0;
}

// Mac 0xfaf0, Win 0x4374b0
// NOTE: directories are deleted with their whole subtree, open files below them are not checked.
bool MonolithTreeDirectory::DeleteChild(const String& name, LiffChunk* chunk, bool force)
{
    UInt32 index = 0;
    for (MonolithTreeEntryList::Iterator it = m_children.Begin(); it != m_children.End(); ++it, ++index) {
        MonolithTreeEntry* entry = *it;
        if (!name.EqualsNoCase(entry->m_name))
            continue;
        if (!entry->m_attributes.m_writable && !force)
            return false;
        if (!entry->m_attributes.m_isDirectory &&
            static_cast<MonolithTreeFile*>(entry)->m_openCount != 0)
            return false;
        if (!chunk->DeleteChunk(index, ChunkId()))
            return false;
        m_children.Erase(it);
        delete entry;
        return Save();
    }
    return false;
}

// Mac 0xfbd0, Win 0x437730
void MonolithTreeDirectory::Enumerate(const String& pattern, DescriptorSet& entries, bool writable)
{
    for (MonolithTreeEntryList::Iterator it = m_children.Begin(); it != m_children.End(); ++it) {
        MonolithTreeEntry* entry = *it;
        if (!Directory::MatchWildcard(entry->m_name, pattern))
            continue;
        entries.Insert(Descriptor(entry->m_name, MaskAttributes(entry->m_attributes, writable),
                                  entry->m_times, entry->m_size));
    }
}

// (inlined into BeginEnumeration / MoveNext)
void MonolithTreeDirectory::SkipDirectories()
{
    while (m_enumerationIterator != m_children.End() &&
           (*m_enumerationIterator)->m_attributes.m_isDirectory) {
        ++m_enumerationIterator;
        ++m_enumerationIndex;
    }
}

// Mac inlined in 0x113d0, Win 0x4375d0
void MonolithTreeDirectory::BeginEnumeration()
{
    m_enumerating = true;
    m_enumerationIterator = m_children.Begin();
    m_enumerationIndex = 0;
    SkipDirectories();
}

// Mac inlined in 0x11480, Win 0x437680
void MonolithTreeDirectory::MoveNext()
{
    ++m_enumerationIterator;
    ++m_enumerationIndex;
    SkipDirectories();
}

// ===========================================================================================
// MonolithTree
// ===========================================================================================

// Mac 0x100d0, Win 0x437ca0
MonolithTree::MonolithTree(LiffChunk* chunk)
    : m_root(0), m_littleEndian(false), m_chunk(chunk)
{
    m_root = new MonolithTreeDirectory(this, 0);
}

// Mac 0x101f0 (D0 0x10270), Win 0x437d70 (sdd 0x437d50)
MonolithTree::~MonolithTree()
{
    delete m_root;
}

// Mac 0xffc0, Win 0x437a20
bool MonolithTree::Load()
{
    UInt64 size = m_chunk->GetDataSize();
    // PORT64: as in ReadFile, a 64-bit build refuses a tree of 4 GB and more (allocated with the
    // low 32 bits, read with all 64); compiled out on 32-bit
    if (sizeof(void*) > 4 && (size >> 32) != 0)
        return false;
    UInt8* buffer = new UInt8[static_cast<UInt32>(size)];
    bool result = false;
    if (m_chunk->ReadAt(0, size, buffer, 0)) {
        Stream stream(buffer, static_cast<UInt32>(size), !m_littleEndian);
        UInt16 type;
        if (stream.Read(type) && type == 1)
            result = m_root->Read(stream);
    }
    delete[] buffer;
    return result;
}

// Mac 0xf020, Win 0x437b50
bool MonolithTree::Save()
{
    UInt64 size = m_root->GetSerializedSize();
    if (!m_chunk->SetSize(size))
        return false;
    UInt8* buffer = new UInt8[static_cast<UInt32>(size)];
    Stream stream(buffer, static_cast<UInt32>(size), true);
    m_root->Write(stream);
    bool result = m_chunk->WriteAt(0, size, buffer, 0);
    delete[] buffer;
    return result;
}

// ===========================================================================================
// LiffStorageVolume::Mount (declared in LiffStorageVolume.h, TU 014)
// ===========================================================================================

// Mac 0x10300, Win 0x437df0
bool LiffStorageVolume::Mount(bool create, const String& name, Directory* parent)
{
    if (m_tree)
        return false;

    m_rootChunk = m_file->OpenChunk(0, kRootChunk);
    if (m_rootChunk) {
        // existing archive: header record, then the tree
        RefPointer<LiffChunk> header = m_rootChunk->OpenChunk(0, kHeaderChunk);
        if (!header)
            return false;
        bool failed = true;
        if (header->GetDataSize() == 6) {
            UInt8 record[6];
            bool ok = header->ReadAt(0, 6, record, 0);
            if (ok) {
                Stream stream(record, 6, true);
                stream.Read(m_version);
                stream.Read(m_flags);
            }
            failed = !ok || m_version != 1;
        }
        header = 0;
        if (failed)
            return false;

        RefPointer<LiffChunk> treeChunk = m_rootChunk->OpenChunk(0, kTreeChunk);
        if (!treeChunk)
            return false;
        m_tree = new MonolithTree(treeChunk);
        m_tree->SetLittleEndian((m_flags & 1) != 0);
        bool loaded = m_tree->Load();
        treeChunk = 0;
        if (!loaded)
            return false;
    }
    else {
        // new archive: root chunk with header record, empty root directory and empty tree
        if (!create)
            return false;
        m_rootChunk = m_file->CreateChunk(kRootChunk, 0, 0);
        if (!m_rootChunk)
            return false;
        RefPointer<LiffChunk> chunk = m_rootChunk->CreateChunk(kHeaderChunk, ChunkHeader::kFlagLeaf, 0);
        if (!chunk)
            return false;
        UInt8 record[6];
        Stream stream(record, 6, true);
        stream.Write(m_version);
        stream.Write(m_flags);
        bool failed = true;
        if (chunk->WriteAt(0, 6, record, 0)) {
            chunk = m_rootChunk->CreateChunk(kDirectoryChunk, 0, 0);
            if (chunk) {
                chunk = m_rootChunk->CreateChunk(kTreeChunk, ChunkHeader::kFlagLeaf, 0);
                if (chunk) {
                    m_tree = new MonolithTree(chunk);
                    m_tree->SetLittleEndian((m_flags & 1) != 0);
                    failed = !m_tree->Save();
                }
            }
        }
        chunk = 0;
        if (failed)
            return false;
    }

    RefPointer<LiffChunk> directoryChunk = m_rootChunk->OpenChunk(0, kDirectoryChunk);
    if (!directoryChunk)
        return false;
    // The archive "Assets.lif" is mounted as "Assets" with the alias "Assets.lif".
    String rootName(name);
    if (rootName.EqualsNoCase(rootName.GetLength() - 4, 4, String(".lif")))
        rootName.Erase(rootName.GetLength() - 4, String::npos);
    String alias(rootName);
    alias.Append(".lif");
    m_root = new LiffStorageDirectory(rootName, parent, this, m_tree->GetRoot(), directoryChunk, &alias);
    return true;
}

// ===========================================================================================
// LiffStorageDirectory
// ===========================================================================================

// Mac 0x119b0, Win 0x4367e0
LiffStorageDirectory::LiffStorageDirectory(const String& name, Directory* parent,
                                           LiffStorageVolume* volume, MonolithTreeDirectory* entry,
                                           LiffChunk* chunk, const String* alias)
    : Directory(name, parent), m_volume(volume), m_entry(entry), m_chunk(chunk)
{
    if (alias)
        AddAlias(*alias);
}

// Mac 0x120c0 (D1 0x118d0, D0 0x118e0), Win 0x4368a0 (sdd 0x436930)
LiffStorageDirectory::~LiffStorageDirectory()
{
}

// (inlined)
bool LiffStorageDirectory::IsWritable() const
{
    return IsVolumeWritable(m_volume);
}

// Win 0x436720 (inlined on Mac)
MonolithTreeEntry* LiffStorageDirectory::FindEntry(MonolithTreeDirectory* directory,
                                                   const String& name, UInt32& index)
{
    index = 0;
    for (MonolithTreeEntryList::Iterator it = directory->m_children.Begin();
         it != directory->m_children.End(); ++it, ++index) {
        if (name.EqualsNoCase((*it)->m_name))
            return *it;
    }
    return 0;
}

// Mac 0x10a60, Win 0x4383c0
File* LiffStorageDirectory::OpenFile(MonolithTreeFile* entry, UInt32 index, const OpenMode& mode,
                                     const UInt64* size)
{
    // share modes against the files already open on this entry
    if (mode.m_read && entry->m_denyRead != 0)
        return 0;
    if (mode.m_write &&
        (entry->m_denyWrite != 0 || !entry->m_attributes.m_writable || !IsWritable()))
        return 0;
    if (!mode.m_shareRead && entry->m_readers != 0)
        return 0;
    if (!mode.m_shareWrite && entry->m_writers != 0)
        return 0;

    RefPointer<LiffChunk> chunk = m_chunk->OpenChunk(index, ChunkId());
    if (!chunk)
        return 0;
    // NOTE: the size in the tree is what the file reports, whatever its chunk holds (both
    // builds): readers size their buffers by it; fixed: a file whose chunk disagrees is not
    // opened (H3).
    if (entry->m_size != chunk->GetDataSize())
        return 0;
    LiffStorageFile* file = new LiffStorageFile(this, mode, m_volume, entry, chunk);
    if (size && !file->SetSize(*size)) {
        file->Release();   // NOTE: reference count 0 -> -1 destroys it
        return 0;
    }
    return file;
}

// Mac 0x10bd0, Win 0x438530
bool LiffStorageDirectory::DoSetAttributes(const Attributes& attributes)
{
    if (m_entry->m_attributes.m_isDirectory != attributes.m_isDirectory)
        return false;
    m_entry->m_attributes = attributes;
    return m_entry->Save();
}

// Mac 0x10c00, Win 0x438560
Attributes LiffStorageDirectory::DoGetAttributes() const
{
    return MaskAttributes(m_entry->m_attributes, IsWritable());
}

// Mac 0x10c30, Win 0x4385a0
bool LiffStorageDirectory::DoSetTimes(const Times& times)
{
    m_entry->m_times = times;
    return m_entry->Save();
}

// Mac 0x10c70, Win 0x4385e0
Times LiffStorageDirectory::DoGetTimes() const
{
    return m_entry->m_times;
}

// Mac 0x10ca0, Win 0x438610
bool LiffStorageDirectory::DoSetInfo(const Attributes& attributes, const Times& times)
{
    if (m_entry->m_attributes.m_isDirectory != attributes.m_isDirectory)
        return false;
    m_entry->m_attributes = attributes;
    m_entry->m_times = times;
    return m_entry->Save();
}

// Mac 0x10cf0, Win 0x438660
// NOTE: only the chunk is checked; a missing entry, or a file of that name, yields a
// directory without a tree entry (callers check DoDirectoryExists first).
Directory* LiffStorageDirectory::DoGetDirectory(const String& name)
{
    UInt32 index;
    MonolithTreeDirectory* entry = dynamic_cast<MonolithTreeDirectory*>(FindEntry(m_entry, name, index));
    RefPointer<LiffChunk> chunk = m_chunk->OpenChunk(index, ChunkId());
    if (!chunk)
        return 0;
    return new LiffStorageDirectory(name, this, m_volume, entry, chunk);
}

// Mac 0x10eb0, Win 0x438780
bool LiffStorageDirectory::DoCreateDirectory(const String& name)
{
    if (!IsWritable())
        return false;
    return m_entry->CreateChildDirectory(name, m_chunk);
}

// Mac 0x10ef0, Win 0x4387b0
File* LiffStorageDirectory::DoOpenFile(const String& name, const OpenMode& mode, bool create,
                                       const UInt64* size)
{
    UInt32 index;
    MonolithTreeEntry* entry = FindEntry(m_entry, name, index);
    if (entry) {
        if (dynamic_cast<MonolithTreeDirectory*>(entry))
            return 0;
        if (MonolithTreeFile* file = dynamic_cast<MonolithTreeFile*>(entry))
            return OpenFile(file, index, mode, size);
    }
    if (!create || !IsWritable())
        return 0;
    MonolithTreeFile* file = m_entry->CreateChildFile(name, m_chunk, size);
    if (!file)
        return 0;
    return OpenFile(file, index, mode, size);
}

// Mac 0x10fe0, Win 0x438840
bool LiffStorageDirectory::DoDelete(const String& name, bool recursive)
{
    if (!IsWritable())
        return false;
    return m_entry->DeleteChild(name, m_chunk, recursive);
}

// Mac 0x11030, Win 0x438870
bool LiffStorageDirectory::DoGetChildAttributes(const String& name, Attributes* attributes)
{
    UInt32 index;
    MonolithTreeEntry* entry = FindEntry(m_entry, name, index);
    if (!entry)
        return false;
    *attributes = MaskAttributes(entry->m_attributes, IsWritable());
    return true;
}

// Mac 0x110b0, Win 0x4388d0
bool LiffStorageDirectory::DoSetChildAttributes(const String& name, const Attributes& attributes)
{
    UInt32 index;
    MonolithTreeEntry* entry = FindEntry(m_entry, name, index);
    if (!entry)
        return false;
    if (entry->m_attributes.m_isDirectory != attributes.m_isDirectory)
        return false;
    entry->m_attributes = attributes;
    return m_entry->Save();
}

// Mac 0x11120, Win 0x438920
bool LiffStorageDirectory::DoGetChildTimes(const String& name, Times* times)
{
    UInt32 index;
    MonolithTreeEntry* entry = FindEntry(m_entry, name, index);
    if (!entry)
        return false;
    *times = entry->m_times;
    return true;
}

// Mac 0x111a0, Win 0x438980
bool LiffStorageDirectory::DoSetChildTimes(const String& name, const Times& times)
{
    UInt32 index;
    MonolithTreeEntry* entry = FindEntry(m_entry, name, index);
    if (!entry)
        return false;
    entry->m_times = times;
    return m_entry->Save();
}

// Mac 0x11220, Win 0x4389e0
bool LiffStorageDirectory::DoIsEmpty()
{
    return m_entry->m_children.IsEmpty();
}

// Mac 0x11240, Win 0x4389f0
bool LiffStorageDirectory::DoDirectoryExists(const BasicArray<String>& path)
{
    MonolithTreeDirectory* directory = m_entry;
    for (UInt32 i = 0; i < path.GetSize(); ++i) {
        UInt32 index;
        MonolithTreeEntry* entry = FindEntry(directory, path[i], index);
        if (!entry)
            return false;
        directory = dynamic_cast<MonolithTreeDirectory*>(entry);
        if (!directory)
            return false;
    }
    return true;
}

// Mac 0x11300, Win 0x438a60
bool LiffStorageDirectory::DoFileExists(const String& name)
{
    UInt32 index;
    MonolithTreeEntry* entry = FindEntry(m_entry, name, index);
    if (!entry)
        return false;
    return dynamic_cast<MonolithTreeFile*>(entry) != 0;
}

// Mac 0x11390, Win 0x438a90
void LiffStorageDirectory::DoEnumerate(const String& pattern, DescriptorSet& entries)
{
    m_entry->Enumerate(pattern, entries, IsWritable());
}

// Mac 0x113d0, Win 0x438ac0
void LiffStorageDirectory::DoBeginEnumeration()
{
    m_entry->BeginEnumeration();
}

// Mac 0x11420, Win 0x438ad0
RefPointer<File> LiffStorageDirectory::DoOpenCurrent(const OpenMode& mode, const UInt64* size)
{
    MonolithTreeEntry* entry = *m_entry->m_enumerationIterator;
    if (!entry)
        return RefPointer<File>();
    return RefPointer<File>(OpenFile(static_cast<MonolithTreeFile*>(entry), m_entry->m_enumerationIndex,
                                     mode, size));
}

// Mac 0x11480, Win 0x438ba0
void LiffStorageDirectory::DoMoveNext()
{
    m_entry->MoveNext();
}

// Mac 0x114d0, Win 0x438bb0
bool LiffStorageDirectory::DoIsEnumerationDone()
{
    return m_entry->IsEnumerationDone();
}

// Mac 0x114f0, Win 0x438be0
bool LiffStorageDirectory::DoIsEnumerating()
{
    return m_entry->m_enumerating;
}

// Mac 0x11500, Win 0x438bf0
void LiffStorageDirectory::DoEndEnumeration()
{
    m_entry->m_enumerating = false;
}

// ===========================================================================================
// LiffStorageFile
// ===========================================================================================

// Mac 0x11df0, Win 0x436950
LiffStorageFile::LiffStorageFile(Directory* parent, const OpenMode& mode, LiffStorageVolume* volume,
                                 MonolithTreeFile* entry, LiffChunk* chunk)
    : File(entry->m_name, parent, mode), m_volume(volume), m_entry(entry), m_chunk(chunk), m_position(0)
{
    ++m_entry->m_openCount;
    if (m_mode.m_read)
        ++m_entry->m_readers;
    if (m_mode.m_write)
        ++m_entry->m_writers;
    if (!m_mode.m_shareRead)
        ++m_entry->m_denyRead;
    if (!m_mode.m_shareWrite)
        ++m_entry->m_denyWrite;
}

// Mac 0x12150 (D1 0x11950, D0 0x11960), Win 0x436a40 (sdd 0x436b00)
LiffStorageFile::~LiffStorageFile()
{
    --m_entry->m_openCount;
    if (m_mode.m_read)
        --m_entry->m_readers;
    if (m_mode.m_write)
        --m_entry->m_writers;
    if (!m_mode.m_shareRead)
        --m_entry->m_denyRead;
    if (!m_mode.m_shareWrite)
        --m_entry->m_denyWrite;
}

// (inlined)
bool LiffStorageFile::IsWritable() const
{
    return IsVolumeWritable(m_volume);
}

// Mac 0x11510, Win 0x438c00
bool LiffStorageFile::DoSetAttributes(const Attributes& attributes)
{
    if (m_entry->m_attributes.m_isDirectory != attributes.m_isDirectory)
        return false;
    m_entry->m_attributes = attributes;
    return m_entry->Save();
}

// Mac 0x11540, Win 0x438c30
Attributes LiffStorageFile::DoGetAttributes() const
{
    return MaskAttributes(m_entry->m_attributes, IsWritable());
}

// Mac 0x11570, Win 0x438c70
bool LiffStorageFile::DoSetTimes(const Times& times)
{
    m_entry->m_times = times;
    return m_entry->Save();
}

// Mac 0x115b0, Win 0x438cb0
Times LiffStorageFile::DoGetTimes() const
{
    return m_entry->m_times;
}

// Mac 0x115e0, Win 0x438ce0
bool LiffStorageFile::DoSetInfo(const Attributes& attributes, const Times& times)
{
    if (m_entry->m_attributes.m_isDirectory != attributes.m_isDirectory)
        return false;
    m_entry->m_attributes = attributes;
    m_entry->m_times = times;
    return m_entry->Save();
}

// Mac 0x11630, Win 0x438d30
bool LiffStorageFile::DoRead(UInt64 position, UInt64 size, void* buffer, UInt64* read)
{
    if (!m_mode.m_read)
        return false;
    if (position > GetSize()) {
        if (read)
            *read = 0;
        return false;
    }
    UInt64 done;
    bool result = m_chunk->ReadAt(position, size, buffer, &done);
    m_position = position + done;
    if (read)
        *read = done;
    return result;
}

// Mac 0x116c0, Win 0x438dd0
// Keeps the entry's size in step with the chunk (writing may grow it).
bool LiffStorageFile::DoWrite(UInt64 position, UInt64 size, const void* buffer, UInt64* written)
{
    if (!m_mode.m_write)
        return false;
    if (position > GetSize()) {
        if (written)
            *written = 0;
        return false;
    }
    UInt64 done;
    bool result = m_chunk->WriteAt(position, size, buffer, &done);
    m_position = position + done;
    if (written)
        *written = done;
    UInt64 dataSize = m_chunk->GetDataSize();
    if (m_entry->m_size == dataSize)
        return result;
    m_entry->m_size = dataSize;
    bool saved = m_entry->Save();
    return result && saved;
}

// Mac 0x11790, Win 0x438ec0
bool LiffStorageFile::DoRead(UInt64 size, void* buffer, UInt64* read)
{
    return ReadAt(m_position, size, buffer, read);
}

// Mac 0x117c0, Win 0x4425e0 (shared with ZipStorageFile through identical-code folding)
bool LiffStorageFile::DoWrite(UInt64 size, const void* buffer, UInt64* written)
{
    return WriteAt(m_position, size, buffer, written);
}

// Mac 0x117f0, Win 0x438ef0
UInt64 LiffStorageFile::DoSize()
{
    return m_entry->m_size;
}

// Mac 0x11810, Win 0x438f00
bool LiffStorageFile::DoSize(UInt64 size)
{
    if (size == m_entry->m_size)
        return true;
    if (!m_mode.m_write)
        return false;
    m_chunk->SetSize(size);   // NOTE: result ignored
    m_entry->m_size = size;
    if (size < m_position)
        m_position = size;
    return m_entry->Save();
}

// Mac 0x11880, Win 0x438f80
UInt64 LiffStorageFile::DoPosition()
{
    return m_position;
}

// Mac 0x11890, Win 0x438f90
bool LiffStorageFile::DoPosition(UInt64 position)
{
    if (position > GetSize())
        return false;
    m_position = position;
    return true;
}

} // namespace LEGO
