// LEGO Digital Designer — restored source
// TU "ZipStorageDirectory.cpp" Mac 0x10b7a0-0x11127f (83 functions), Win 0x43eec0-0x4446af: ZIP
// archive storage (see ZipStorageDirectory.h). Names inferred. Records are parsed with
// LEGO::Stream as on Win (Mac inlines direct struct reads); Win out-of-line helpers: see the header.
//
// zlib: Mac imports inflateInit2_/inflate/inflateEnd/deflateInit2_/deflate/deflateEnd from
// /usr/lib/libz.1.dylib (version string "1.2.11"); Win links zlib 1.2.7 statically (LDD64: 1.4)
// (inflateInit2_ 0x6a6650, inflate 0x6a6770, inflateEnd 0x6a89a0, deflateInit2_ 0x6a2dc0,
// deflate 0x6a3230, deflateEnd 0x6a4230; "inflate 1.2.7 Copyright 1995-2012 Mark Adler").
//
// Library / inline code emitted in this TU (not written out here):
//   Mac 0x10d350/0x110ee0 + 0x110f30 BasicMap<UInt32, ZipFileHeader*> D1/D0 + node eraser
//   (Win 0x443790/0x443680), 0x110f80/0x111170 its insert-with-hint / insert_unique (Mount's
//   operator[]); 0x110240/0x1102c0 + 0x110310 ZipTreeNode::ChildMap D1/D0 + node eraser
//   (Win 0x443720/0x443540), 0x110610 ChildMap operator[] (+ 0x110740 insert hint, 0x110a30
//   insert_unique, 0x110b10 create node; Win 0x443800), 0x110c70 ChildMap erase(range);
//   0x110540/0x1105a0 BasicArray<FileDataSegment> D1/D0 (Win 0x444670), 0x110d50 its PushBack
//   (Win 0x4445d0); 0x1100b0/0x110110 ZipStorageFactory D1/D0 (Win 0x42f780/0x42f7d0).
// Mac 0x111240: static initialiser (<iostream> only).
#include "ZipStorageDirectory.h"
#include "Path.h"
#include "Stream.h"
#include "ObjectFactory.h"

#include <string.h>
#include <zlib.h>

#include <iostream>

// The byte buffers of this file come from new[]; the original frees them with scalar delete
// (undefined behaviour, harmless with its allocator; low).

namespace LEGO {

namespace {

// Record signatures ("PK\3\4", "PK\1\2", "PK\5\6") and fixed record sizes.
const UInt32 kLocalHeaderSignature   = 0x04034b50;
const UInt32 kCentralHeaderSignature = 0x02014b50;
const UInt32 kEndRecordSignature     = 0x06054b50;
const UInt32 kLocalHeaderSize        = 0x1e;
const UInt32 kCentralHeaderSize      = 0x2e;
const UInt32 kEndRecordSize          = 0x16;
const UInt32 kDataDescriptorSize     = 0x0c;

// Entries by record offset, used by Mount to keep the volume lists sorted.
// vtable Mac 0x7641a0 {D1 0x10d350, D0 0x110ee0}, Win 0xa204dc {sdd 0x443790}.
typedef BasicMap<UInt32, ZipFileHeader*, Less<UInt32> > HeaderMap;

// (not in the original) A path component of an entry name that stays inside its folder
// when the archive is copied out: not "." or "..", and no '\' or ':' (a Windows path separator
// or drive).
bool IsSafeEntryName(const String& component)
{
    if (component == String(".") || component == String(".."))
        return false;
    return component.Find(UniChar16('\\'), 0) == String::npos &&
           component.Find(UniChar16(':'), 0) == String::npos;
}

// Searches the last 64 KB of `file` backwards (1 KB steps, 4 bytes overlap) for the end of
// central directory record and reads it into `record` (kEndRecordSize bytes). `offset` receives
// the record position (0 when none was found). Inlined in Mount on Mac, Win 0x43fa30.
bool FindEndOfCentralDirectory(File* file, UInt8* record, UInt32& offset)
{
    UInt32 fileSize = (UInt32)file->GetSize();
    UInt32 maxBack = 0xffff;
    offset = 0;
    bool found = false;
    if (fileSize < maxBack)
        maxBack = fileSize;
    // NOTE: both builds allocate with new[] and free with scalar delete; fixed: delete[].
    UInt8* buffer = new UInt8[0x404];
    UInt32 back = 4;
    while (back < maxBack) {
        back += 0x400;
        if (back > maxBack)
            back = maxBack;
        UInt32 readPosition = fileSize - back;
        UInt32 readSize = fileSize - readPosition;
        if (readSize > 0x404)
            readSize = 0x404;
        if (!file->ReadAt(readPosition, readSize, buffer, 0))
            break;
        Int32 i = (Int32)readSize - 3;
        while (--i >= 0) {
            if (buffer[i] == 'P' && buffer[i + 1] == 'K' && buffer[i + 2] == 5 && buffer[i + 3] == 6) {
                offset = readPosition + i;
                found = true;
                if (offset > fileSize - kEndRecordSize) {
                    offset = 0;
                    found = false;
                } else if (!file->ReadAt(offset, kEndRecordSize, record, 0)) {
                    delete[] buffer;
                    return false;
                }
                delete[] buffer;
                return found;
            }
        }
    }
    delete[] buffer;
    return found;
}

// Access attributes of an entry from the "version made by" host system and the external
// attributes of its central record. Inlined in Mount on Mac, Win 0x43f7a0 (register arguments).
void ConvertAttributes(Attributes& attributes, UInt16 versionMadeBy, UInt32 externalAttributes)
{
    switch (versionMadeBy >> 8) {
    case 0:     // MS-DOS: FILE_ATTRIBUTE_READONLY / _HIDDEN / _DIRECTORY
        attributes.m_isDirectory = (externalAttributes & 0x10) != 0;
        attributes.m_readable = true;
        attributes.m_writable = (externalAttributes & 0x01) == 0;
        attributes.m_removable = (externalAttributes & 0x01) == 0;
        attributes.m_hidden = (externalAttributes & 0x02) != 0;
        break;
    case 3:     // Unix
    case 0x13:  // OS X: st_mode in the high word (S_IRUSR / S_IWUSR / S_IXUSR)
        attributes.m_isDirectory = false;
        attributes.m_readable = ((externalAttributes >> 24) & 1) != 0;
        attributes.m_writable = ((externalAttributes >> 23) & 1) != 0;
        attributes.m_removable = ((externalAttributes >> 23) & 1) != 0;
        attributes.m_hidden = false;
        attributes.m_executable = ((externalAttributes >> 22) & 1) != 0;
        break;
    default:
        attributes.m_isDirectory = false;
        attributes.m_readable = true;
        attributes.m_writable = true;
        attributes.m_removable = true;
        attributes.m_hidden = false;
        break;
    }
}

} // namespace

// ===========================================================================================
// ZipFileHeader
// ===========================================================================================

// Mac 0x10f670 (register arguments: segments on the stack, offset in EDX, size in ECX),
// Win 0x442630
bool ZipFileHeader::AllocateSegments(SegmentArray& segments, UInt32& offset, UInt32 size)
{
    void* data = malloc(size);
    if (data) {
        FileDataSegment segment(offset, size, data, true);
        segments.PushBack(segment);
        offset += size;
        return true;
    }
    if (size >= 0x3fff) {
        UInt32 half = ((size >> 1) + 0x3ff) & ~0x3ffu;
        if (AllocateSegments(segments, offset, half) && AllocateSegments(segments, offset, half))
            return true;
    }
    return false;
}

// ===========================================================================================
// ZipTreeNode
// ===========================================================================================

// Mac 0x110180, Win 0x43eee0
ZipTreeNode::ZipTreeNode(const String& name, ZipFileHeader* header, ZipTreeNode* parent)
    : m_name(name), m_header(header), m_parent(parent), m_children(), m_enumIterator(),
      m_enumerating(false)
{
}

// Mac 0x110390 (D1 0x110270, D0 0x110280), Win 0x43f3d0 (sdd 0x43f450)
ZipTreeNode::~ZipTreeNode()
{
}

// Win 0x43f250 (inlined on Mac)
ZipTreeNode* ZipTreeNode::GetChild(const String& name)
{
    ChildMap::Iterator it = m_children.Find(name);
    if (it == m_children.End())
        return 0;
    return it->second;
}

// Mac 0x10d250, Win 0x43f2c0
ZipTreeNode* ZipTreeNode::AddChild(const String& name, ZipFileHeader* header)
{
    ChildMap::Iterator it = m_children.Find(name);
    if (it != m_children.End())
        return it->second;
    ZipTreeNode* node = new ZipTreeNode(name, header, this);
    m_children[name] = node;
    return node;
}

// Mac 0x10d380, Win 0x43f210
void ZipTreeNode::GetPath(String& path) const
{
    if (m_parent)
        m_parent->GetPath(path);
    if (!path.IsEmpty())
        path << "/";
    path << m_name;
}

// Win 0x43f020 (inlined on Mac; guard variable Mac 0x7640e0)
const Attributes& ZipTreeNode::GetAttributes() const
{
    static const Attributes s_implicitDirectory(true);                 // Mac 0x7640dc, Win 0xb44988
    if (m_header)
        return m_header->m_attributes;
    return s_implicitDirectory;
}

// Win 0x43ef90 (inlined on Mac; guard variable Mac 0x764190)
const Times& ZipTreeNode::GetTimes() const
{
    static const Times s_noTimes;                                      // Mac 0x764178, Win 0xb44990
    if (m_header)
        return m_header->m_times;
    return s_noTimes;
}

// Mac 0x10e440, Win 0x43f0b0
void ZipTreeNode::Enumerate(const String& pattern, DescriptorSet& entries, bool /*writable*/)
{
    for (ChildMap::Iterator it = m_children.Begin(); it != m_children.End(); ++it) {
        ZipTreeNode* child = it->second;
        if (!Directory::MatchWildcard(child->m_name, pattern))
            continue;
        String name(child->m_name);
        ZipFileHeader* header = child->m_header;
        entries.Insert(Descriptor(name, child->GetAttributes(), child->GetTimes(),
                                  header ? header->m_uncompressedSize : 0));
    }
}

// Win 0x43f830 (inlined on Mac in ZipStorageDirectory::DoBeginEnumeration)
void ZipTreeNode::BeginEnumeration()
{
    m_enumerating = true;
    for (m_enumIterator = m_children.Begin(); m_enumIterator != m_children.End(); ++m_enumIterator) {
        if (!m_enumIterator->second->GetAttributes().m_isDirectory)
            break;
    }
}

// Win 0x43f8b0 (inlined on Mac in ZipStorageDirectory::DoMoveNext)
void ZipTreeNode::MoveNext()
{
    for (++m_enumIterator; m_enumIterator != m_children.End(); ++m_enumIterator) {
        if (!m_enumIterator->second->GetAttributes().m_isDirectory)
            break;
    }
}

// Mac 0x10bf90, Win 0x43f9b0
// NOTE: a child that RemoveEntry cannot remove stays in the map (endless loop), and a child
// without header (implicit directory) crashes RemoveEntry (both builds).
bool ZipTreeNode::DeleteChildren(bool force, ZipStorageVolume* volume)
{
    if (!force && m_header && !m_header->m_attributes.m_removable)
        return false;
    while (!m_children.IsEmpty()) {
        ZipTreeNode* child = m_children.Begin()->second;
        if (!child->DeleteChildren(force, volume))
            return false;
        volume->RemoveEntry(child);
    }
    return true;
}

// (not in the original) True when `node` or an entry below it has open file objects.
static bool HasOpenFiles(const ZipTreeNode* node)
{
    if (node->m_header && node->m_header->m_openCount != 0)
        return true;
    for (ZipTreeNode::ChildMap::ConstIterator it = node->m_children.Begin(); it != node->m_children.End(); ++it) {
        if (HasOpenFiles(it->second.Get()))
            return true;
    }
    return false;
}

// Win 0x43f910 (inlined on Mac in ZipStorageDirectory::DoDelete)
// NOTE: the original only checks the open count of the child itself: a directory whose files
// are still open is removed, RemoveEntry moves the archive data under the open ZipStorageFile
// objects, and their later Flush writes at stale offsets (corrupt archive). Through the path API
// (Directory::Delete) the open file's live parent directories already block it; the storage
// slot itself (DoDelete, DoMove) does not. Fixed: refused while any file below the child is
// open.
bool ZipTreeNode::DeleteChild(const String& name, bool force, ZipStorageVolume* volume)
{
    ChildMap::Iterator it = m_children.Find(name);
    if (it == m_children.End())
        return false;
    ZipTreeNode* child = it->second;
    if (!HasOpenFiles(child) && child->DeleteChildren(force, volume)) {
        volume->RemoveEntry(child);
        return true;
    }
    return false;
}

// ===========================================================================================
// ZipStorageVolume
// ===========================================================================================

// Mac inline in 0x10fad0, Win 0x43f470
ZipStorageVolume::ZipStorageVolume(File* file)
    : m_root(0), m_localFirst(0), m_localLast(0), m_centralFirst(0), m_centralLast(0),
      m_rootNode(), m_file(file), m_baseOffset(0), m_dataEnd(0), m_centralOffset(0),
      m_centralEnd(0), m_endRecordOffset(0), m_centralSize(0), m_entryCount(0)
{
}

// Mac 0x10d660 (D1 0x10d710, D0 0x10d720), Win 0x4417a0 (sdd 0x43f5b0)
ZipStorageVolume::~ZipStorageVolume()
{
    m_rootNode = 0;
}

// Mac 0x10c410, Win 0x43fbd0
bool ZipStorageVolume::Mount(bool /*create*/, const String& name, Directory* parent)
{
    if (m_rootNode)
        return false;

    UInt8 record[kEndRecordSize];
    if (m_file->GetSize() == 0) {
        // empty file: write an empty archive (end record only)
        Stream stream(record, kEndRecordSize, false);
        stream.Write(kEndRecordSignature);
        stream.Write(UInt16(0));    // number of this disk
        stream.Write(UInt16(0));    // disk with the central directory
        stream.Write(UInt16(0));    // entries on this disk
        stream.Write(UInt16(0));    // entries
        stream.Write(UInt32(0));    // central directory size
        stream.Write(UInt32(0));    // central directory offset
        stream.Write(UInt16(0));    // comment length
        if (!m_file->Write(kEndRecordSize, record, 0))
            return false;
    }
    if (!FindEndOfCentralDirectory(m_file, record, m_endRecordOffset))
        return false;

    {
        Stream stream(record + 4, kEndRecordSize - 4, false);
        UInt16 disk;
        UInt16 centralDisk;
        UInt16 totalEntries;
        stream.Read(disk);
        stream.Read(centralDisk);
        stream.Read(m_entryCount);
        stream.Read(totalEntries);
        stream.Read(m_centralSize);
        stream.Read(m_centralOffset);
        if (centralDisk != disk || centralDisk != 0 || disk != 0 || m_entryCount != totalEntries)
            return false;
        if (m_endRecordOffset != m_centralOffset + m_centralSize)
            return false;
        m_baseOffset = m_endRecordOffset - m_centralOffset - m_centralSize;
        if (m_baseOffset != 0)
            return false;
    }

    m_rootNode = new ZipTreeNode(String(), 0, 0);

    HeaderMap byLocalOffset;
    HeaderMap byCentralOffset;
    bool failed = false;
    UInt32 position = m_centralOffset;
    m_centralEnd = m_endRecordOffset;
    while (position < m_endRecordOffset) {
        UInt8 central[kCentralHeaderSize];
        if (!m_file->ReadAt(position, kCentralHeaderSize, central, 0)) {
            failed = true;
            break;
        }
        if (central[0] != 'P' || central[1] != 'K' || central[2] != 1 || central[3] != 2) {
            m_centralEnd = position;
            break;
        }

        RefPointer<ZipFileHeader> header = new ZipFileHeader;
        header->m_centralOffset = position;
        UInt16 time;
        UInt16 date;
        UInt16 nameLength;
        UInt16 extraLength;
        UInt16 commentLength;
        UInt16 diskStart;
        UInt16 internalAttributes;
        UInt32 externalAttributes;
        {
            Stream stream(central + 4, kCentralHeaderSize - 4, false);
            stream.Read(header->m_versionMadeBy);
            stream.Read(header->m_versionNeeded);
            stream.Read(header->m_flags);
            stream.Read(header->m_method);
            stream.Read(time);
            stream.Read(date);
            stream.Read(header->m_crc);
            stream.Read(header->m_compressedSize);
            stream.Read(header->m_uncompressedSize);
            stream.Read(nameLength);
            stream.Read(extraLength);
            stream.Read(commentLength);
            stream.Read(diskStart);
            stream.Read(internalAttributes);
            stream.Read(externalAttributes);
            stream.Read(header->m_localOffset);
        }
        // NOTE: the declared sizes and the local header offset are not checked against the
        // archive (both builds): a reader trusts them (a size of 0xFFFFFFFE makes ReadFile's
        // buffer wrap); fixed: the entry's local header and data must lie before the central
        // directory, a stored entry's two sizes must agree, a deflated entry cannot expand more
        // than deflate can (1032:1), and no two entries may share a local header.
        if ((UInt64)header->m_localOffset + kLocalHeaderSize + header->m_compressedSize > m_centralOffset ||
            (header->m_method == 0 && header->m_compressedSize != header->m_uncompressedSize) ||
            (header->m_method == 8 &&
             (UInt64)header->m_uncompressedSize > (UInt64)header->m_compressedSize * 1032 + 64) ||
            byLocalOffset.Contains(header->m_localOffset)) {
            failed = true;
            break;
        }
        // NOTE: the DOS time stamp is not converted (both builds): all times stay zero.
        header->m_times.m_created = header->m_times.m_modified;
        header->m_times.m_accessed = 0;
        ConvertAttributes(header->m_attributes, header->m_versionMadeBy, externalAttributes);
        if (!IsWritable())
            header->m_attributes.m_removable = false;

        // Link into the list ordered by local header offset.
        // NOTE: an entry that falls between two others is linked behind the next larger one
        // (and one in front of the last entry dereferences its null successor); archives whose
        // central directory follows the local order only ever append (both builds); fixed: it
        // is linked in front of the next larger one.
        HeaderMap::Iterator it = byLocalOffset.LowerBound(header->m_localOffset);
        if (it == byLocalOffset.Begin()) {
            header->m_localNext = m_localFirst;
            if (m_localFirst)
                m_localFirst->m_localPrev = header;
            else
                m_localLast = header;
            m_localFirst = header;
        } else if (it == byLocalOffset.End()) {
            header->m_localPrev = m_localLast;
            m_localLast->m_localNext = header;
            m_localLast = header;
        } else {
            ZipFileHeader* next = it->second;
            header->m_localNext = next;
            header->m_localPrev = next->m_localPrev;
            next->m_localPrev->m_localNext = header;
            next->m_localPrev = header;
        }
        byLocalOffset[header->m_localOffset] = header;

        // Link into the list ordered by central record offset (same scheme).
        it = byCentralOffset.LowerBound(header->m_centralOffset);
        if (it == byCentralOffset.Begin()) {
            header->m_centralNext = m_centralFirst;
            if (m_centralFirst)
                m_centralFirst->m_centralPrev = header;
            else
                m_centralLast = header;
            m_centralFirst = header;
        } else if (it == byCentralOffset.End()) {
            header->m_centralPrev = m_centralLast;
            m_centralLast->m_centralNext = header;
            m_centralLast = header;
        } else {
            ZipFileHeader* next = it->second;
            header->m_centralNext = next;
            header->m_centralPrev = next->m_centralPrev;
            next->m_centralPrev->m_centralNext = header;
            next->m_centralPrev = header;
        }
        byCentralOffset[header->m_centralOffset] = header;

        // The entry name follows the central record.
        String entryName;
        AString raw;
        if (nameLength != 0)
            raw.Reserve(nameLength, true);
        if (!m_file->ReadAt(position + kCentralHeaderSize, nameLength, raw.GetWritableBuffer(), 0)) {
            failed = true;
        } else {
            if (nameLength != 0)
                raw.SetLength(nameLength);
            entryName = raw;
        }

        // Walk/create the directory nodes; a name ending in '/' is a directory entry.
        // NOTE: a duplicate name leaves the header linked in the lists but owned by nobody: it
        // is freed at the end of this iteration and the next entry linked beside it writes to
        // it (use after free); "." / ".." components and names with '\' or ':' are taken as they
        // are, so copying the archive out (StartMode's database update) writes outside the
        // target folder (both builds); fixed: such an archive is not mounted.
        if (!failed) {
            ZipTreeNode* node = m_rootNode;
            UInt32 start = 0;
            do {
                UInt32 slash = entryName.Find(UniChar16('/'), start);
                if (slash == String::npos) {
                    String leaf = entryName.Substring(start, String::npos);
                    if (leaf.GetLength() == 0) {
                        if (node->m_header) {
                            failed = true;
                            break;
                        }
                        header->m_attributes.m_isDirectory = true;
                        node->m_header = header;
                    } else {
                        if (!IsSafeEntryName(leaf) || node->GetChild(leaf)) {
                            failed = true;
                            break;
                        }
                        node->AddChild(leaf, header);
                        header->m_attributes.m_isDirectory = false;
                    }
                    break;
                }
                String component = entryName.Substring(start, slash - start);
                bool empty = component.GetLength() == 0;
                if (!empty && !IsSafeEntryName(component)) {
                    failed = true;
                    break;
                }
                if (!empty)
                    node = node->AddChild(component, 0);
                if (empty) {
                    failed = true;
                    break;
                }
                if (node->m_header && !node->m_header->m_attributes.m_isDirectory) {
                    failed = true;   // a path through a file entry
                    break;
                }
                start = slash + 1;
            } while (start != String::npos);
        }

        if (failed)
            break;
        position = position + kCentralHeaderSize + nameLength + extraLength + commentLength;
    }
    if (failed)
        return false;

    // The archive "Model.zip" is mounted as "Model" with the alias "Model.zip".
    String rootName(name);
    if (rootName.EqualsNoCase(rootName.GetLength() - 4, 4, String(".zip")))
        rootName.Erase(rootName.GetLength() - 4, String::npos);
    String alias(rootName);
    alias.Append(".zip");
    m_root = new ZipStorageDirectory(parent, this, m_rootNode, rootName, alias);
    return true;
}

// Win 0x441830 (inlined on Mac in CreateEntry and RemoveEntry)
bool ZipStorageVolume::GetLocalEntrySize(const ZipFileHeader* header, UInt32* size)
{
    UInt8 local[kLocalHeaderSize];
    if (!ReadAt(header->m_localOffset, kLocalHeaderSize, local, 0))
        return false;
    Stream stream(local, kLocalHeaderSize, false);
    UInt32 signature;
    UInt16 version;
    UInt16 flags;
    UInt16 method;
    UInt16 time;
    UInt16 date;
    UInt32 crc;
    UInt32 compressedSize;
    UInt32 uncompressedSize;
    UInt16 nameLength;
    UInt16 extraLength;
    stream.Read(signature);
    if (signature != kLocalHeaderSignature)
        return false;
    stream.Read(version);
    stream.Read(flags);
    stream.Read(method);
    stream.Read(time);
    stream.Read(date);
    stream.Read(crc);
    stream.Read(compressedSize);
    stream.Read(uncompressedSize);
    stream.Read(nameLength);
    stream.Read(extraLength);
    if (size) {
        *size = header->m_compressedSize + kLocalHeaderSize + nameLength + extraLength;
        if (flags & 8)
            *size += kDataDescriptorSize;
    }
    return true;
}

// Win 0x4418d0 (inlined on Mac in RemoveEntry and ResizeEntry)
bool ZipStorageVolume::AdjustLocalOffsets(ZipFileHeader* first, Int32 delta)
{
    if (delta != 0) {
        for (ZipFileHeader* header = first; header; header = header->m_localNext) {
            header->m_localOffset += delta;
            UInt32 value = header->m_localOffset;
            if (!WriteAt(header->m_centralOffset + 0x2a, 4, &value))
                return false;
        }
    }
    return true;
}

// Win 0x441930 (inlined on Mac)
void ZipStorageVolume::WriteEndOfCentralDirectory()
{
    UInt8 record[12];
    Stream stream(record, sizeof(record), false);
    stream.Write(m_entryCount);        // entries on this disk
    stream.Write(m_entryCount);        // entries
    stream.Write(m_centralSize);
    stream.Write(m_centralOffset);
    WriteAt(m_endRecordOffset + 8, sizeof(record), record);
}

// Mac 0x10b7a0, Win 0x440770
ZipTreeNode* ZipStorageVolume::CreateEntry(const String& name, bool directory, ZipTreeNode* parent,
                                           UInt32* localSize, UInt32* centralSize)
{
    RefPointer<ZipFileHeader> header = new ZipFileHeader;
    header->m_versionMadeBy = 0x14;
    header->m_versionNeeded = 0x14;
    header->m_attributes = Attributes(directory, true, true, true);

    String path;
    parent->GetPath(path);
    if (!path.IsEmpty())
        path << "/";
    path << name;
    if (directory)
        path << "/";
    AString encoded = path.ToUTF8();
    UInt32 nameLength = encoded.GetLength();

    // The new local header goes behind the data of the last entry.
    UInt32 offset = m_dataEnd;
    if (offset == 0) {
        m_dataEnd = 0;
        if (m_localLast) {
            UInt32 size;
            if (!GetLocalEntrySize(m_localLast, &size))
                return 0;
            offset = m_localLast->m_localOffset + size;
            m_dataEnd = offset;
        }
    }
    header->m_localOffset = offset;

    UInt32 localLength = (UInt16)nameLength + kLocalHeaderSize;
    // NOTE: both builds allocate the record buffers with new[] and free them with scalar delete;
    // fixed: delete[].
    UInt8* local = new UInt8[localLength];
    {
        Stream stream(local, localLength, false);
        stream.Write(kLocalHeaderSignature);
        stream.Write(header->m_versionNeeded);
        stream.Write(header->m_flags);
        stream.Write(header->m_method);
        stream.Write(UInt16(0));                       // time
        stream.Write(UInt16(0));                       // date
        stream.Write(header->m_crc);
        stream.Write(header->m_compressedSize);
        stream.Write(header->m_uncompressedSize);
        stream.Write(UInt16(nameLength));
        stream.Write(UInt16(0));                       // extra field length
        if (nameLength + kLocalHeaderSize <= localLength)
            memcpy(local + kLocalHeaderSize, encoded.GetBuffer(), nameLength);
    }
    UInt32 centralLength = (UInt16)nameLength + kCentralHeaderSize;
    UInt8* central = new UInt8[centralLength];
    {
        Stream stream(central, centralLength, false);
        stream.Write(kCentralHeaderSignature);
        stream.Write(header->m_versionMadeBy);
        stream.Write(header->m_versionNeeded);
        stream.Write(header->m_flags);
        stream.Write(header->m_method);
        stream.Write(UInt16(0));                       // time
        stream.Write(UInt16(0));                       // date
        stream.Write(header->m_crc);
        stream.Write(header->m_compressedSize);
        stream.Write(header->m_uncompressedSize);
        stream.Write(UInt16(nameLength));
        stream.Write(UInt16(0));                       // extra field length
        stream.Write(UInt16(0));                       // comment length
        stream.Write(UInt16(0));                       // disk number start
        stream.Write(UInt16(0));                       // internal attributes
        stream.Write(UInt32(0));                       // external attributes
        stream.Write(header->m_localOffset);
        if (nameLength + kCentralHeaderSize <= centralLength)
            memcpy(central + kCentralHeaderSize, encoded.GetBuffer(), nameLength);
    }

    UInt64 fileSize = m_file->GetSize();
    UInt64 grownSize = fileSize + localLength;
    m_file->SetSize(grownSize + centralLength);        // NOTE: result ignored

    ZipTreeNode* node = 0;
    UInt32 localOffset = header->m_localOffset;
    if (CopyRange(localOffset, localOffset + localLength, fileSize - localOffset) &&
        WriteAt(localOffset, localLength, local)) {
        for (ZipFileHeader* h = m_centralFirst; h; h = h->m_centralNext)
            h->m_centralOffset += localLength;
        m_dataEnd += localLength;
        m_centralOffset += localLength;
        m_centralEnd += localLength;
        m_endRecordOffset += localLength;
        header->m_centralOffset = m_centralEnd;

        if (CopyRange(m_centralEnd, m_centralEnd + centralLength, grownSize - m_centralEnd) &&
            WriteAt(m_centralEnd, centralLength, central)) {
            m_centralEnd += centralLength;
            m_endRecordOffset += centralLength;
            m_centralSize += centralLength;
            ++m_entryCount;
            WriteEndOfCentralDirectory();

            if (!m_localLast) {
                m_localFirst = header;
            } else {
                header->m_localPrev = m_localLast;
                m_localLast->m_localNext = header;
            }
            m_localLast = header;
            if (!m_centralLast) {
                m_centralFirst = header;
            } else {
                header->m_centralPrev = m_centralLast;
                m_centralLast->m_centralNext = header;
            }
            m_centralLast = header;

            if (localSize)
                *localSize = localLength;
            if (centralSize)
                *centralSize = centralLength;
            node = parent->AddChild(name, header);
        }
    }
    delete[] central;
    delete[] local;
    return node;
}

// Mac 0x10c000, Win 0x4413e0
void ZipStorageVolume::RemoveEntry(ZipTreeNode* node)
{
    ZipFileHeader* header = node->m_header;
    UInt64 fileSize = m_file->GetSize();
    UInt32 localSize;
    if (!GetLocalEntrySize(header, &localSize))
        return;

    UInt8 central[kCentralHeaderSize];
    if (!ReadAt(header->m_centralOffset, kCentralHeaderSize, central, 0))
        return;
    UInt32 signature;
    UInt16 nameLength;
    UInt16 extraLength;
    UInt16 commentLength;
    {
        Stream stream(central, kCentralHeaderSize, false);
        stream.Read(signature);
        Stream lengths(central + 28, kCentralHeaderSize - 28, false);   // name/extra/comment lengths
        lengths.Read(nameLength);
        lengths.Read(extraLength);
        lengths.Read(commentLength);
    }
    if (signature != kCentralHeaderSignature)
        return;

    // unlink from both lists
    ZipFileHeader* localNext = header->m_localNext;
    ZipFileHeader* centralNext = header->m_centralNext;
    if (header->m_localPrev)
        header->m_localPrev->m_localNext = header->m_localNext;
    else
        m_localFirst = header->m_localNext;
    if (header->m_localNext)
        header->m_localNext->m_localPrev = header->m_localPrev;
    else
        m_localLast = header->m_localPrev;
    header->m_localPrev = 0;
    header->m_localNext = 0;
    if (header->m_centralPrev)
        header->m_centralPrev->m_centralNext = header->m_centralNext;
    else
        m_centralFirst = header->m_centralNext;
    if (header->m_centralNext)
        header->m_centralNext->m_centralPrev = header->m_centralPrev;
    else
        m_centralLast = header->m_centralPrev;
    header->m_centralPrev = 0;
    header->m_centralNext = 0;

    UInt32 centralRecordSize = kCentralHeaderSize + nameLength + extraLength + commentLength;
    AdjustLocalOffsets(localNext, -(Int32)localSize);

    // drop the central record, then the local header and data
    UInt32 centralEnd = header->m_centralOffset + centralRecordSize;
    if (!CopyRange(centralEnd, header->m_centralOffset, fileSize - centralEnd))
        return;
    UInt64 shrunkSize = fileSize - centralRecordSize;
    UInt32 localEnd = header->m_localOffset + localSize;
    if (!CopyRange(localEnd, header->m_localOffset, shrunkSize - localEnd))
        return;
    m_file->SetSize(shrunkSize - localSize);

    for (ZipFileHeader* h = m_centralFirst; h; h = h->m_centralNext)
        h->m_centralOffset -= localSize;
    for (ZipFileHeader* h = centralNext; h; h = h->m_centralNext)
        h->m_centralOffset -= centralRecordSize;
    m_centralOffset -= localSize;
    if (m_dataEnd != 0)
        m_dataEnd -= localSize;
    m_centralEnd -= localSize + centralRecordSize;
    m_endRecordOffset -= localSize + centralRecordSize;
    m_centralSize -= centralRecordSize;
    --m_entryCount;
    WriteEndOfCentralDirectory();

    node->m_parent->m_children.Erase(node->m_name);
}

// Mac 0x10d3e0, Win 0x441610
bool ZipStorageVolume::ResizeEntry(ZipFileHeader* header, UInt64 size)
{
    UInt16 flags = header->m_flags;
    UInt32 oldSize = header->m_compressedSize;
    if (flags & 8)
        oldSize += kDataDescriptorSize;

    if (oldSize != size) {
        UInt32 dataOffset = header->m_dataOffset;
        UInt64 fileSize = m_file->GetSize();
        Int32 delta = (Int32)((UInt32)size - oldSize);
        UInt64 resized = (UInt64)((Int64)fileSize + delta);
        if (delta > 0 && !m_file->SetSize(resized))
            return false;
        UInt32 oldEnd = dataOffset + oldSize;
        if (!CopyRange(oldEnd, dataOffset + size, fileSize - oldEnd))
            return false;
        if (delta < 0 && !m_file->SetSize(resized))
            return false;

        for (ZipFileHeader* h = m_centralFirst; h; h = h->m_centralNext)
            h->m_centralOffset += delta;
        AdjustLocalOffsets(header->m_localNext, delta);   // NOTE: result ignored
        m_dataEnd += delta;
        m_centralOffset += delta;
        m_centralEnd += delta;
        m_endRecordOffset += delta;
        WriteEndOfCentralDirectory();
        flags = header->m_flags;
    }

    if (flags & 8) {
        // the data no longer ends with a data descriptor
        header->m_flags = flags & ~8;
        UInt16 value = header->m_flags;
        if (!WriteAt(header->m_localOffset + 6, 2, &value) || !WriteAt(header->m_centralOffset + 8, 2, &value))
            return false;
    }
    return true;
}

// ===========================================================================================
// ZipStorageDirectory
// ===========================================================================================

// Mac 0x110bb0, Win 0x43f660
ZipStorageDirectory::ZipStorageDirectory(Directory* parent, ZipStorageVolume* volume, ZipTreeNode* node,
                                         const String& name, const String& alias)
    : Directory(name, parent), m_volume(volume), m_node(node)
{
    AddAlias(alias);
}

// Mac 0x10d760, Win 0x4419a0
bool ZipStorageDirectory::DoSetAttributes(const Attributes& attributes)
{
    ZipFileHeader* header = m_node->m_header;
    if (!header)
        return false;
    header->m_attributes = attributes;
    return true;
}

// Mac 0x10d780, Win 0x4419c0
Attributes ZipStorageDirectory::DoGetAttributes() const
{
    return m_node->GetAttributes();
}

// Mac 0x10d7e0, Win 0x4419e0
bool ZipStorageDirectory::DoSetTimes(const Times& times)
{
    ZipFileHeader* header = m_node->m_header;
    if (!header)
        return false;
    header->m_times = times;
    return true;
}

// Mac 0x10d820, Win 0x441a20
Times ZipStorageDirectory::DoGetTimes() const
{
    return m_node->GetTimes();
}

// Mac 0x10d8c0, Win 0x441a60
Directory* ZipStorageDirectory::DoGetDirectory(const String& name)
{
    ZipTreeNode* node = m_node->GetChild(name);
    // NOTE: a missing child is dereferenced (both builds).
    return new ZipStorageDirectory(this, m_volume, node);
}

// Mac 0x10d9e0, Win 0x441ae0
bool ZipStorageDirectory::DoCreateDirectory(const String& name)
{
    if (!m_volume->IsWritable())
        return false;
    return m_volume->CreateEntry(name, true, m_node, 0, 0) != 0;
}

// Mac 0x10da20, Win 0x441b10
File* ZipStorageDirectory::DoOpenFile(const String& name, const OpenMode& mode, bool create,
                                      const UInt64* size)
{
    ZipTreeNode* node = m_node->GetChild(name);
    if (node)
        return OpenFile(node, mode, size);
    if (!create || !m_volume->IsWritable())
        return 0;
    UInt32 localSize;
    node = m_volume->CreateEntry(name, false, m_node, &localSize, 0);
    if (!node)
        return 0;
    ZipFileHeader* header = node->m_header;
    header->m_dataOffset = localSize + header->m_localOffset;
    return CreateFile(node, mode, size);
}

// Mac 0x10db20, Win 0x441ef0
File* ZipStorageDirectory::CreateFile(ZipTreeNode* node, const OpenMode& mode, const UInt64* size)
{
    ZipStorageFile* file = new ZipStorageFile(this, mode, m_volume, node);
    ZipFileHeader* header = node->m_header;
    if ((!mode.m_write && header->m_method == 0 && (header->m_flags & 1) == 0) || file->LoadData()) {
        if (!size || file->SetSize(*size))
            return file;
    }
    file->Release();   // NOTE: reference count 0 -> -1 destroys it
    return 0;
}

// Mac 0x10dbd0, Win 0x441fc0
File* ZipStorageDirectory::OpenFile(ZipTreeNode* node, const OpenMode& mode, const UInt64* size)
{
    ZipStorageVolume* volume = m_volume;
    const Attributes& attributes = node->GetAttributes();
    ZipFileHeader* header = node->m_header;
    if (attributes.m_isDirectory)
        return 0;
    // share modes against the files already open on this entry
    if (mode.m_read && header->m_denyRead != 0)
        return 0;
    if (mode.m_write &&
        (header->m_denyWrite != 0 || !attributes.m_writable || !volume->IsWritable()))
        return 0;
    if (!mode.m_shareRead && header->m_readers != 0)
        return 0;
    if (!mode.m_shareWrite && header->m_writers != 0)
        return 0;

    if (header->m_dataOffset == 0) {
        // first open: check the local header against the central record
        UInt8 local[kLocalHeaderSize];
        if (!volume->ReadAt(header->m_localOffset, kLocalHeaderSize, local, 0))
            return 0;
        if (local[0] != 'P' || local[1] != 'K' || local[2] != 3 || local[3] != 4)
            return 0;
        Stream stream(local + 4, kLocalHeaderSize - 4, false);
        UInt16 version;
        UInt16 flags;
        UInt16 method;
        UInt16 time;
        UInt16 date;
        UInt32 crc;
        UInt32 compressedSize;
        UInt32 uncompressedSize;
        UInt16 nameLength;
        UInt16 extraLength;
        stream.Read(version);
        stream.Read(flags);
        stream.Read(method);
        stream.Read(time);
        stream.Read(date);
        stream.Read(crc);
        stream.Read(compressedSize);
        stream.Read(uncompressedSize);
        stream.Read(nameLength);
        stream.Read(extraLength);
        if (flags != header->m_flags || (flags & 1) != 0 || method != header->m_method)
            return 0;
        if ((flags & 8) == 0) {
            if (crc != header->m_crc || compressedSize != header->m_compressedSize ||
                uncompressedSize != header->m_uncompressedSize)
                return 0;
        } else if (crc != 0 || compressedSize != 0 || uncompressedSize != 0) {
            return 0;
        }
        if ((method | 8) != 8)      // stored or deflated only
            return 0;
        header->m_dataOffset = header->m_localOffset + kLocalHeaderSize + nameLength + extraLength;
    }
    return CreateFile(node, mode, size);
}

// Mac 0x10dda0, Win 0x441ba0
bool ZipStorageDirectory::DoDelete(const String& name, bool recursive)
{
    ZipStorageVolume* volume = m_volume;
    if (!volume->IsWritable())
        return false;
    return m_node->DeleteChild(name, recursive, volume);
}

// Mac 0x10de80, Win 0x441bd0
bool ZipStorageDirectory::DoGetChildAttributes(const String& name, Attributes* attributes)
{
    ZipTreeNode* node = m_node->GetChild(name);
    if (!node)
        return false;
    *attributes = node->GetAttributes();
    return true;
}

// Mac 0x10df60, Win 0x441c00
bool ZipStorageDirectory::DoSetChildAttributes(const String& name, const Attributes& attributes)
{
    ZipTreeNode* node = m_node->GetChild(name);
    if (!node || !node->m_header)
        return false;
    node->m_header->m_attributes = attributes;
    return true;
}

// Mac 0x10e000, Win 0x441c30
bool ZipStorageDirectory::DoGetChildTimes(const String& name, Times* times)
{
    ZipTreeNode* node = m_node->GetChild(name);
    if (!node)
        return false;
    *times = node->GetTimes();
    return true;
}

// Mac 0x10e130, Win 0x441c80
bool ZipStorageDirectory::DoSetChildTimes(const String& name, const Times& times)
{
    ZipTreeNode* node = m_node->GetChild(name);
    if (!node || !node->m_header)
        return false;
    node->m_header->m_times = times;
    return true;
}

// Mac 0x10e1f0, Win 0x441cd0
bool ZipStorageDirectory::DoIsEmpty()
{
    return m_node->m_children.IsEmpty();
}

// Mac 0x10e210, Win 0x441ce0
bool ZipStorageDirectory::DoDirectoryExists(const BasicArray<String>& path)
{
    ZipTreeNode* node = m_node;
    for (UInt32 i = 0; i < path.GetSize(); ++i) {
        node = node->GetChild(path[i]);
        if (!node || !node->GetAttributes().m_isDirectory)
            return false;
    }
    return true;
}

// Mac 0x10e330, Win 0x441d50
bool ZipStorageDirectory::DoFileExists(const String& name)
{
    ZipTreeNode* node = m_node->GetChild(name);
    if (!node)
        return false;
    return !node->GetAttributes().m_isDirectory;
}

// Mac 0x10e410, Win 0x441d80
void ZipStorageDirectory::DoEnumerate(const String& pattern, DescriptorSet& entries)
{
    m_node->Enumerate(pattern, entries, m_volume->IsWritable());
}

// Mac 0x10e670, Win 0x441da0
void ZipStorageDirectory::DoBeginEnumeration()
{
    m_node->BeginEnumeration();
}

// Mac 0x10e700, Win 0x441db0
RefPointer<File> ZipStorageDirectory::DoOpenCurrent(const OpenMode& mode, const UInt64* size)
{
    ZipTreeNode* node = m_node->m_enumIterator->second;
    if (!node)
        return RefPointer<File>();
    return RefPointer<File>(OpenFile(node, mode, size));
}

// Mac 0x10e760, Win 0x441e90
void ZipStorageDirectory::DoMoveNext()
{
    m_node->MoveNext();
}

// Mac 0x10e7f0, Win 0x441ea0
bool ZipStorageDirectory::DoIsEnumerationDone()
{
    return m_node->m_enumIterator == m_node->m_children.End();
}

// Mac 0x10e810, Win 0x441ed0
bool ZipStorageDirectory::DoIsEnumerating()
{
    return m_node->m_enumerating;
}

// Mac 0x10e820, Win 0x441ee0
void ZipStorageDirectory::DoEndEnumeration()
{
    m_node->m_enumerating = false;
}

// ===========================================================================================
// ZipStorageFile
// ===========================================================================================

// Mac 0x10f760, Win 0x442ff0
ZipStorageFile::ZipStorageFile(Directory* parent, const OpenMode& mode, ZipStorageVolume* volume,
                               ZipTreeNode* node)
    : File(node->m_name, parent, mode), m_volume(volume), m_node(node), m_position(0)
{
    ZipFileHeader* header = m_node->m_header;
    ++header->m_openCount;
    if (m_mode.m_read)
        ++header->m_readers;
    if (m_mode.m_write)
        ++header->m_writers;
    if (!m_mode.m_shareRead)
        ++header->m_denyRead;
    if (!m_mode.m_shareWrite)
        ++header->m_denyWrite;
}

// Mac 0x10f8b0 (D1 0x10fa50, D0 0x10fa60), Win 0x4430f0 (sdd 0x4430d0)
ZipStorageFile::~ZipStorageFile()
{
    ZipFileHeader* header = m_node->m_header;
    --header->m_openCount;
    if (m_mode.m_read)
        --header->m_readers;
    if (m_mode.m_write)
        --header->m_writers;
    if (!m_mode.m_shareRead)
        --header->m_denyRead;
    if (!m_mode.m_shareWrite)
        --header->m_denyWrite;
    if (header->m_openCount == 0) {
        // the next open validates the local header and loads the data again
        header->m_dataOffset = 0;
        header->m_segments.Clear();
    }
}

// Mac 0x10faa0, Win 0x4431c0
void ZipStorageFile::OnFinalRelease()
{
    ZipFileHeader* header = m_node->m_header;
    if (header->m_dirty && header->m_openCount == 1)
        Flush();
    File::OnFinalRelease();
}

// Mac 0x10eb00, Win 0x442250
bool ZipStorageFile::DoSetAttributes(const Attributes& attributes)
{
    ZipFileHeader* header = m_node->m_header;
    if (!header)
        return false;
    header->m_attributes = attributes;
    return true;
}

// Mac 0x10eb20, Win 0x442270
Attributes ZipStorageFile::DoGetAttributes() const
{
    return m_node->GetAttributes();
}

// Mac 0x10eb80, Win 0x442290
bool ZipStorageFile::DoSetTimes(const Times& times)
{
    ZipFileHeader* header = m_node->m_header;
    if (!header)
        return false;
    header->m_times = times;
    return true;
}

// Mac 0x10ebc0, Win 0x4422d0
Times ZipStorageFile::DoGetTimes() const
{
    return m_node->GetTimes();
}

// Mac 0x10ec60, Win 0x442310
bool ZipStorageFile::DoRead(UInt64 position, UInt64 size, void* buffer, UInt64* read)
{
    if (read)
        *read = 0;
    if (!m_mode.m_read)
        return false;
    if (position > GetSize())
        return false;
    if (size == 0)
        return true;

    ZipFileHeader* header = m_node->m_header;
    UInt64 count = (UInt64)header->m_uncompressedSize - position;
    if (size < count)
        count = size;
    if (header->m_segments.IsEmpty()) {
        // stored entry opened read-only: straight from the archive
        UInt64 done = count;
        m_volume->ReadAt((UInt64)header->m_dataOffset + position, count, buffer, &done); // NOTE: result ignored
        count = done;
    } else if (count != 0) {
        UInt32 offset = (UInt32)position;
        UInt32 remaining = (UInt32)count;
        UInt32 copied = 0;
        for (ZipFileHeader::SegmentArray::Iterator s = header->m_segments.Begin();
             s != header->m_segments.End(); ++s) {
            if (offset < s->m_offset + s->m_size) {
                UInt32 skip = offset - s->m_offset;
                UInt32 n = s->m_size - skip;
                if (remaining <= n)
                    n = remaining;
                memcpy(static_cast<UInt8*>(buffer) + copied, static_cast<UInt8*>(s->m_data) + skip, n);
                remaining -= n;
                if (remaining == 0)
                    break;
                offset += n;
                copied += n;
            }
        }
    }
    m_position = position + count;
    if (read)
        *read = count;
    return count == size;
}

// Mac 0x10ee20, Win 0x4424a0
bool ZipStorageFile::DoWrite(UInt64 position, UInt64 size, const void* buffer, UInt64* written)
{
    if (written)
        *written = 0;
    if (!m_mode.m_write || position > GetSize())
        return false;
    if (size == 0)
        return true;

    UInt64 end = position + size;
    if (end > GetSize() && !SetSize(end))
        return false;

    ZipFileHeader* header = m_node->m_header;
    UInt32 remaining = (UInt32)size;
    if (remaining != 0) {
        UInt32 offset = (UInt32)position;
        UInt32 copied = 0;
        for (ZipFileHeader::SegmentArray::Iterator s = header->m_segments.Begin();
             s != header->m_segments.End(); ++s) {
            if (offset < s->m_offset + s->m_size) {
                UInt32 skip = offset - s->m_offset;
                UInt32 n = s->m_size - skip;
                if (remaining <= n)
                    n = remaining;
                memcpy(static_cast<UInt8*>(s->m_data) + skip, static_cast<const UInt8*>(buffer) + copied, n);
                copied += n;
                offset += n;
                remaining -= n;
                if (remaining == 0)
                    break;
            }
        }
    }
    header->m_dirty = true;
    m_position = end;
    if (written)
        *written = size;
    return true;
}

// Mac 0x10ef60, Win 0x438ec0 (shared with LiffStorageFile through identical-code folding)
bool ZipStorageFile::DoRead(UInt64 size, void* buffer, UInt64* read)
{
    return ReadAt(m_position, size, buffer, read);
}

// Mac 0x10ef90, Win 0x4425e0 (shared with LiffStorageFile through identical-code folding)
bool ZipStorageFile::DoWrite(UInt64 size, const void* buffer, UInt64* written)
{
    return WriteAt(m_position, size, buffer, written);
}

// Mac 0x10efc0, Win 0x442610
UInt64 ZipStorageFile::DoSize()
{
    ZipFileHeader* header = m_node->m_header;
    if (header)
        return header->m_uncompressedSize;
    return 0;
}

// Mac 0x10efe0, Win 0x4427b0
bool ZipStorageFile::DoSize(UInt64 size)
{
    if (!m_mode.m_write)
        return false;
    ZipFileHeader* header = m_node->m_header;
    if (header->m_uncompressedSize == size)
        return true;

    ZipFileHeader::SegmentArray& segments = header->m_segments;
    if (size < header->m_uncompressedSize) {
        while (!segments.IsEmpty() && segments.Back().m_offset >= size)
            segments.PopBack();
        if (size < m_position)
            m_position = size;
    } else if (segments.IsEmpty() || size > (UInt64)segments.Back().m_offset + segments.Back().m_size) {
        if (!GrowData((UInt32)size - header->m_uncompressedSize))
            return false;
    }
    header->m_uncompressedSize = (UInt32)size;
    header->m_dirty = true;
    return true;
}

// Mac 0x10f0f0, Win 0x438f80 (shared with LiffStorageFile through identical-code folding)
UInt64 ZipStorageFile::DoPosition()
{
    return m_position;
}

// Mac 0x10f100, Win 0x438f90 (shared with LiffStorageFile through identical-code folding)
bool ZipStorageFile::DoPosition(UInt64 position)
{
    if (position > GetSize())
        return false;
    m_position = position;
    return true;
}

// Win 0x442d30 (inlined on Mac in LoadData and DoSize)
bool ZipStorageFile::GrowData(UInt32 size)
{
    ZipFileHeader* header = m_node->m_header;
    UInt32 rounded = (size + 0x1fff) & ~0x1fffu;
    UInt32 offset = 0;
    if (!header->m_segments.IsEmpty())
        offset = header->m_segments.Back().m_offset + header->m_segments.Back().m_size;
    return ZipFileHeader::AllocateSegments(header->m_segments, offset, rounded);
}

// Mac 0x10e830, Win 0x442d80
bool ZipStorageFile::LoadData()
{
    ZipFileHeader* header = m_node->m_header;
    if (header->m_uncompressedSize == 0 || !header->m_segments.IsEmpty())
        return true;

    if (header->m_method == 0) {
        // stored
        if (!GrowData(header->m_uncompressedSize))
            return false;
        UInt32 remaining = header->m_uncompressedSize;
        UInt32 offset = 0;
        for (ZipFileHeader::SegmentArray::Iterator s = header->m_segments.Begin();
             remaining != 0 && s != header->m_segments.End(); ++s) {
            UInt32 n = s->m_size;
            if (remaining <= n)
                n = remaining;
            if (!m_volume->ReadAt(header->m_dataOffset + offset, n, s->m_data, 0))
                return false;
            offset += n;
            remaining -= n;
        }
        return (header->m_flags & 1) == 0;
    }

    // deflated: read the whole stream, inflate into the segments
    bool failed = true;
    Bytef* compressed = static_cast<Bytef*>(malloc(header->m_compressedSize));
    if (compressed) {
        if (GrowData(header->m_uncompressedSize) &&
            m_volume->ReadAt(header->m_dataOffset, header->m_compressedSize, compressed, 0) &&
            (header->m_flags & 1) == 0) {
            z_stream stream;
            memset(&stream, 0, sizeof(stream));
            inflateInit2(&stream, -MAX_WBITS);
            stream.next_in = compressed;
            stream.avail_in = header->m_compressedSize;
            int result = Z_OK;
            UInt32 remaining = header->m_uncompressedSize;
            for (ZipFileHeader::SegmentArray::Iterator s = header->m_segments.Begin();
                 remaining != 0 && s != header->m_segments.End(); ++s) {
                stream.next_out = static_cast<Bytef*>(s->m_data);
                stream.avail_out = s->m_size;
                if (remaining <= s->m_size)
                    stream.avail_out = remaining;
                remaining -= stream.avail_out;
                result = inflate(&stream, remaining == 0 ? Z_FINISH : Z_NO_FLUSH);
                if (result != Z_OK && result != Z_STREAM_END)
                    break;
            }
            inflateEnd(&stream);
            failed = result != Z_STREAM_END;
        }
        free(compressed);
    }
    return !failed;
}

// Mac 0x10f140, Win 0x442880
// NOTE: when deflating does not pay off part way through, the data is stored but the CRC only
// covers the segments processed until then (both builds).
// NOTE: the original refuses the write-back when this file object was opened read-only; as
// OnFinalRelease flushes through whichever object of a modified entry goes last, a reader
// closed after the writer drops the writer's changes (the destructor then frees the data);
// fixed: the write-back depends on the archive being writable, not on this object's mode.
bool ZipStorageFile::Flush()
{
    if (!m_volume->IsWritable())
        return false;
    ZipFileHeader* header = m_node->m_header;
    if (!header->m_dirty)
        return true;

    UInt8* compressed = 0;
    bool ownsCompressed = false;   // (Win keeps the buffer in an owner-flag pointer, 0x4435a0)
    UInt16 method = 0;
    UInt32 compressedSize = 0;
    UInt32 crc = 0xffffffff;
    if (header->m_uncompressedSize != 0) {
        // NOTE: allocated with new[] and freed with scalar delete in both builds; fixed: delete[].
        compressed = new UInt8[header->m_uncompressedSize];
        z_stream stream;
        memset(&stream, 0, sizeof(stream));
        deflateInit2(&stream, 9, Z_DEFLATED, -MAX_WBITS, 9, Z_DEFAULT_STRATEGY);
        stream.avail_out = header->m_uncompressedSize;
        stream.next_out = compressed;
        compressedSize = 0;
        method = 8;
        ownsCompressed = true;
        // The CRC table is imported through a non-lazy pointer (Mac 0x702134); zlib's table.
        const z_crc_t* table = get_crc_table();
        UInt32 remaining = header->m_uncompressedSize;
        for (ZipFileHeader::SegmentArray::Iterator s = header->m_segments.Begin();
             s != header->m_segments.End(); ++s) {
            stream.next_in = static_cast<Bytef*>(s->m_data);
            stream.avail_in = remaining;
            if (s->m_size < remaining)
                stream.avail_in = s->m_size;
            remaining -= stream.avail_in;
            for (UInt32 i = 0; i < stream.avail_in; ++i)
                crc = (crc >> 8) ^ (UInt32)table[(crc ^ stream.next_in[i]) & 0xff];
            int result = deflate(&stream, remaining == 0 ? Z_FINISH : Z_NO_FLUSH);
            if (result == Z_STREAM_END) {
                compressedSize = (UInt32)stream.total_out;
                break;
            }
            if (result < 0 || stream.avail_out == 0) {
                // not smaller than the data: store it
                delete[] compressed;
                compressedSize = header->m_uncompressedSize;
                compressed = 0;
                ownsCompressed = false;
                method = 0;
                break;
            }
        }
        deflateEnd(&stream);
    }

    bool ok = false;
    if (m_volume->ResizeEntry(header, compressedSize)) {
        bool written = true;
        if (method == 0) {
            UInt64 offset = header->m_dataOffset;
            UInt32 remaining = compressedSize;
            for (ZipFileHeader::SegmentArray::Iterator s = header->m_segments.Begin();
                 s != header->m_segments.End(); ++s) {
                UInt32 n = s->m_size;
                if (remaining <= n)
                    n = remaining;
                if (!m_volume->WriteAt(offset, n, s->m_data)) {
                    written = false;
                    break;
                }
                offset += n;
                remaining -= n;
            }
        } else {
            written = m_volume->WriteAt(header->m_dataOffset, compressedSize, compressed);
        }
        if (written && header->m_method != method) {
            header->m_method = method;
            UInt16 value = method;
            written = m_volume->WriteAt(header->m_localOffset + 8, 2, &value) &&
                      m_volume->WriteAt(header->m_centralOffset + 10, 2, &value);
        }
        if (written) {
            header->m_crc = ~crc;
            header->m_compressedSize = compressedSize;
            UInt32 values[3];
            values[0] = header->m_crc;
            values[1] = header->m_compressedSize;
            values[2] = header->m_uncompressedSize;
            if (m_volume->WriteAt(header->m_localOffset + 0xe, sizeof(values), values) &&
                m_volume->WriteAt(header->m_centralOffset + 0x10, sizeof(values), values)) {
                header->m_dirty = false;
                ok = true;
            }
        }
    }
    if (compressed && ownsCompressed)
        delete[] compressed;
    return ok;
}

// ===========================================================================================
// ZipStorageFactory
// ===========================================================================================

// Mac 0x10fad0, Win 0x4431e0
Object* ZipStorageFactory::CreateObject(const ObjectFactoryArgs& args)
{
    const Directory::OpenArgs* openArgs = dynamic_cast<const Directory::OpenArgs*>(&args);
    if (!openArgs)
        return 0;

    BasicArray<String> components(openArgs->m_path);
    OpenMode mode(true, true, false, false);
    Path path;
    path.SetElement(openArgs->m_directory);
    if (!path.Append(components))
        return 0;

    if (!path.FileExists() && !openArgs->m_strict)
        path.SetName(String(path.GetName()) << ".zip");

    RefPointer<File> file = path.OpenFile(mode, openArgs->m_strict);
    if (!file) {
        if (openArgs->m_strict)
            return 0;
        // Retry read-only (sharing reads).
        mode.m_write = false;
        mode.m_shareRead = true;
        file = path.OpenFile(mode, false);
        if (!file)
            return 0;
    }

    RefPointer<ZipStorageVolume> volume = new ZipStorageVolume(file);
    RefPointer<Directory> parent = file->GetParent();
    bool mounted = volume->Mount(false, openArgs->m_path.End()[-1], parent);
    parent = 0;
    // The root directory keeps the volume alive (ZipStorageDirectory::m_volume).
    return mounted ? volume->GetRoot() : 0;
}

} // namespace LEGO
