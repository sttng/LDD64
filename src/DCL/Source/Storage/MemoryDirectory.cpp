// LEGO Digital Designer — restored source
// TU "MemoryDirectory.cpp"  Mac 0x1aa1e0-0x1ac5df (41 functions), Win 0x4393f0-0x43a5ff,
// 0x43e030 (+ COMDATs 0x430fb0, 0x439760).
// In-memory storage backend (see MemoryDirectory.h). Names inferred.
//
// Template / library code emitted in this TU (not written out here):
//   Mac 0x1aa1e0 FileDataSegment dtor, 0x1ab9c0 BasicArray<FileDataSegment>::PushBack
//   (Win 0x43a6e0), 0x1abd30 ChildMap erase(range), 0x1abe10 ChildMap operator[] (Win 0x443800),
//   0x1abf40 / 0x1ac230 / 0x1ac310 its insert helpers, 0x1ac3b0 AString::Replace
//   (BasicString.h), 0x1ac5a0 static initialiser (<iostream> only).
#include "MemoryDirectory.h"
#include "Base64.h"

#include <string.h>
#include <iostream>

namespace LEGO {

namespace {

// Appends malloc'ed segments for `size` bytes starting at file offset `offset` (advanced).
// When malloc fails for 2047 bytes or more, retries as two halves rounded up to 1 KiB.
// Mac 0x1ab7a0 (static, register arguments), Win 0x43a0b0. (name inferred)
bool AllocateSegments(UInt32 size, UInt32& offset, MemoryDirectoryNode::SegmentArray& segments)
{
    void* data = malloc(size);
    if (data) {
        MemoryDirectoryNode::FileDataSegment segment(offset, size, data, true);
        segments.PushBack(segment);
        offset += size;
        return true;
    }
    if (size >= 0x7ff) {
        UInt32 half = ((size >> 1) + 0x3ff) & ~0x3ffu;
        if (AllocateSegments(half, offset, segments) && AllocateSegments(half, offset, segments))
            return true;
    }
    return false;
}

} // namespace

// ---------------------------------------------------------------------------
// MemoryDirectoryNode
// ---------------------------------------------------------------------------

// Mac 0x1aa200, Win 0x439800 (Win 0x439780 = find + call, the body of DoDelete)
bool MemoryDirectoryNode::Delete(bool recursive)
{
    if (m_openCount != 0)
        return false;
    if (!recursive && !m_attributes.m_removable)
        return false;
    while (!m_children.IsEmpty()) {
        if (!m_children.Begin()->second->Delete(recursive))
            return false;
    }
    // Drops the parent's reference: `this` may be destroyed by the erase.
    ChildMap::Map& siblings = m_parent->m_children.Std();
    std::pair<ChildMap::Iterator, ChildMap::Iterator> range = siblings.equal_range(m_name);
    siblings.erase(range.first, range.second);
    return true;
}

// Mac 0x1aa450, Win 0x439540
MemoryDirectoryNode* MemoryDirectoryNode::CreateChild(const String& name, bool isDirectory)
{
    ChildMap::Iterator it = m_children.Find(name);
    if (it != m_children.End())
        return it->second.Get();
    MemoryDirectoryNode* node = new MemoryDirectoryNode(name, isDirectory, this);
    m_children[name] = node;
    return node;
}

// Mac 0x1aae90, Win 0x4393f0
void MemoryDirectoryNode::Enumerate(const String& pattern, DescriptorSet& entries)
{
    for (ChildMap::Iterator it = m_children.Begin(); it != m_children.End(); ++it) {
        MemoryDirectoryNode* child = it->second.Get();
        if (Directory::MatchWildcard(child->m_name, pattern)) {
            String name(child->m_name);
            entries.Insert(Descriptor(name, child->m_attributes, child->m_times, child->m_size));
        }
    }
}

// ---------------------------------------------------------------------------
// MemoryDirectory
// ---------------------------------------------------------------------------

// Mac 0x1abc50 (D1) / 0x1abc60 (D0), Win 0x42fa00 / 0x42faf0
MemoryDirectory::~MemoryDirectory()
{
}

// Mac 0x1aa310, Win 0x439880
Directory* MemoryDirectory::DoGetDirectory(const String& name)
{
    MemoryDirectoryNode* node = m_node->FindChild(name);
    return new MemoryDirectory(String(), this, node);
}

// Mac 0x1aa420, Win 0x439910
bool MemoryDirectory::DoCreateDirectory(const String& name)
{
    return m_node->CreateChild(name, true) != 0;
}

// Mac 0x1aa550, Win 0x439930
File* MemoryDirectory::DoOpenFile(const String& name, const OpenMode& mode, bool create, const UInt64* size)
{
    MemoryDirectoryNode* node = m_node->FindChild(name);
    if (node) {
        if (node->m_attributes.m_isDirectory)
            return 0;
        if (mode.m_read && node->m_denyRead != 0)
            return 0;
        if (mode.m_write && (node->m_denyWrite != 0 || !node->m_attributes.m_writable))
            return 0;
        if (!mode.m_shareRead && node->m_readers != 0)
            return 0;
        if (!mode.m_shareWrite && node->m_writers != 0)
            return 0;
    } else {
        if (!create)
            return 0;
        node = m_node->CreateChild(name, false);
    }

    MemoryFile* file = new MemoryFile(this, node, mode);
    if (size && !file->SetSize(*size)) {
        file->Release();
        file = 0;
    }
    return file;
}

// Mac 0x1aa710, Win 0x439a50 (-> 0x439780)
bool MemoryDirectory::DoDelete(const String& name, bool recursive)
{
    MemoryDirectoryNode::ChildMap::Iterator it = m_node->m_children.Find(name);
    if (it == m_node->m_children.End())
        return false;
    return it->second->Delete(recursive);
}

// Mac 0x1aa7b0, Win 0x439a70 (tail 0x439390)
bool MemoryDirectory::DoRename(const String& from, const String& to)
{
    if (m_node->FindChild(to))
        return false;
    MemoryDirectoryNode* node = m_node->FindChild(from);
    if (!node || node->m_openCount != 0)
        return false;
    // A child that is currently loaded as a Directory object keeps its name.
    if (m_children.Find(from) != m_children.End())
        return false;

    MemoryDirectoryNode* parent = node->m_parent;
    parent->m_children[to] = node;
    MemoryDirectoryNode::ChildMap::Map& siblings = parent->m_children.Std();
    std::pair<MemoryDirectoryNode::ChildMap::Iterator, MemoryDirectoryNode::ChildMap::Iterator> range =
        siblings.equal_range(node->m_name);
    siblings.erase(range.first, range.second);
    node->m_name = to;
    return true;
}

// Mac 0x1aaa30, Win 0x439b00
bool MemoryDirectory::DoGetChildAttributes(const String& name, Attributes* attributes)
{
    MemoryDirectoryNode* node = m_node->FindChild(name);
    if (!node)
        return false;
    *attributes = node->m_attributes;
    return true;
}

// Mac 0x1aaad0, Win 0x439b30
bool MemoryDirectory::DoSetChildAttributes(const String& name, const Attributes& attributes)
{
    MemoryDirectoryNode* node = m_node->FindChild(name);
    if (!node)
        return false;
    node->m_attributes = attributes;
    return true;
}

// Mac 0x1aab70, Win 0x439b60
bool MemoryDirectory::DoGetChildTimes(const String& name, Times* times)
{
    MemoryDirectoryNode* node = m_node->FindChild(name);
    if (!node)
        return false;
    *times = node->m_times;
    return true;
}

// Mac 0x1aac20, Win 0x439bb0
bool MemoryDirectory::DoSetChildTimes(const String& name, const Times& times)
{
    MemoryDirectoryNode* node = m_node->FindChild(name);
    if (!node)
        return false;
    node->m_times = times;
    return true;
}

// Mac 0x1aacd0, Win 0x439c00
bool MemoryDirectory::DoIsEmpty()
{
    return m_node->m_children.IsEmpty();
}

// Mac 0x1aacf0, Win 0x439c10
bool MemoryDirectory::DoDirectoryExists(const BasicArray<String>& path)
{
    MemoryDirectoryNode* node = m_node;
    for (UInt32 i = 0; i < path.GetSize(); ++i) {
        node = node->FindChild(path[i]);
        if (!node || !node->m_attributes.m_isDirectory)
            return false;
    }
    return true;
}

// Mac 0x1aadd0, Win 0x439c70
bool MemoryDirectory::DoFileExists(const String& name)
{
    MemoryDirectoryNode* node = m_node->FindChild(name);
    if (!node)
        return false;
    return !node->m_attributes.m_isDirectory;
}

// Mac 0x1aae70, Win 0x439ca0
void MemoryDirectory::DoEnumerate(const String& pattern, DescriptorSet& entries)
{
    m_node->Enumerate(pattern, entries);
}

// Mac 0x1aafd0, Win 0x439cc0
File* MemoryDirectory::WrapMemory(void* data, UInt64 size)
{
    AString encoded;
    Base64Encode(&data, sizeof(data), encoded);
    encoded.ReplaceAll("/", 1, "-", 1);                // '/' is the path separator

    if (m_node->FindChild(String(encoded)))
        return 0;
    MemoryDirectoryNode* node = m_node->CreateChild(String(encoded), false);
    return new MemoryFile(this, node, data, size);
}

// ---------------------------------------------------------------------------
// MemoryFile
// ---------------------------------------------------------------------------

// Mac 0x1ab890, Win 0x43a3e0
MemoryFile::MemoryFile(Directory* parent, MemoryDirectoryNode* node, void* data, UInt64 size)
    : File(node->m_name, parent, OpenMode(true)), m_position(0), m_node(node)
{
    AddOpen();
    MemoryDirectoryNode::FileDataSegment segment(0, (UInt32)size, data, false);
    m_node->m_segments.PushBack(segment);
    m_node->m_size = (UInt32)size;
}

// Mac 0x1abb50 (D1 0x1abc00, D0 0x1abc10), Win 0x43a4d0 (sdd 0x43a3c0)
MemoryFile::~MemoryFile()
{
    --m_node->m_openCount;
    if (m_mode.m_read)
        --m_node->m_readers;
    if (m_mode.m_write)
        --m_node->m_writers;
    if (!m_mode.m_shareRead)
        --m_node->m_denyRead;
    if (!m_mode.m_shareWrite)
        --m_node->m_denyWrite;
}

// Mac 0x1ab370, Win 0x439e10 — through the locked public wrappers
bool MemoryFile::DoRead(UInt64 position, UInt64 size, void* buffer, UInt64* read)
{
    if (!Seek(position))
        return false;
    return Read(size, buffer, read);
}

// Mac 0x1ab3b0, Win 0x43e030
bool MemoryFile::DoWrite(UInt64 position, UInt64 size, const void* buffer, UInt64* written)
{
    if (!Seek(position))
        return false;
    return Write(size, buffer, written);
}

// Mac 0x1ab3f0, Win 0x439e60
bool MemoryFile::DoRead(UInt64 size, void* buffer, UInt64* read)
{
    if (read)
        *read = 0;
    if (!m_mode.m_read)
        return false;
    if (GetSize() < m_position)
        return false;
    if (size == 0)
        return true;

    // NOTE: both builds clamp the low dword of `size` only.
    UInt32 count = m_node->m_size - m_position;
    if ((UInt32)size < count)
        count = (UInt32)size;

    if (count != 0) {
        UInt32 position = m_position;
        UInt32 done = 0;
        UInt32 remaining = count;
        MemoryDirectoryNode::SegmentArray& segments = m_node->m_segments;
        for (MemoryDirectoryNode::SegmentArray::Iterator s = segments.Begin(); s != segments.End(); ++s) {
            if (position < s->m_offset + s->m_size) {
                UInt32 offset = position - s->m_offset;
                UInt32 chunk = s->m_size - offset;
                if (remaining <= chunk)
                    chunk = remaining;
                memcpy((char*)buffer + done, (const char*)s->m_data + offset, chunk);
                remaining -= chunk;
                if (remaining == 0)
                    break;
                position += chunk;
                done += chunk;
            }
        }
    }
    m_position += count;
    if (read)
        *read = count;
    return count == size;
}

// Mac 0x1ab530, Win 0x439f70
bool MemoryFile::DoWrite(UInt64 size, const void* buffer, UInt64* written)
{
    if (written)
        *written = 0;
    if (!m_mode.m_write)
        return false;
    if (GetSize() < m_position)
        return false;
    if (size == 0)
        return true;
    if ((size >> 32) != 0 || m_position + (UInt32)size < m_position)
        return false;
    UInt64 end = (UInt64)m_position + size;
    if (GetSize() < end && !SetSize(end))
        return false;

    UInt32 count = (UInt32)size;
    UInt32 position = m_position;
    UInt32 done = 0;
    UInt32 remaining = count;
    MemoryDirectoryNode::SegmentArray& segments = m_node->m_segments;
    for (MemoryDirectoryNode::SegmentArray::Iterator s = segments.Begin(); s != segments.End(); ++s) {
        if (position < s->m_offset + s->m_size) {
            UInt32 offset = position - s->m_offset;
            UInt32 chunk = s->m_size - offset;
            if (remaining <= chunk)
                chunk = remaining;
            memcpy((char*)s->m_data + offset, (const char*)buffer + done, chunk);
            remaining -= chunk;
            if (remaining == 0)
                break;
            done += chunk;
            position += chunk;
        }
    }
    m_position += count;
    if (written)
        *written = count;
    return true;
}

// Mac 0x1ab680, Win 0x43a0a0
UInt64 MemoryFile::DoSize()
{
    return m_node->m_size;
}

// Mac 0x1ab690, Win 0x43a1b0
bool MemoryFile::DoSize(UInt64 size)
{
    if (!m_mode.m_write)
        return false;
    MemoryDirectoryNode* node = m_node;
    if (node->m_size == size)
        return true;
    if ((size >> 32) != 0)
        return false;
    UInt32 newSize = (UInt32)size;
    MemoryDirectoryNode::SegmentArray& segments = node->m_segments;

    if (node->m_size < newSize) {
        UInt32 capacity = 0;
        if (!segments.IsEmpty()) {
            capacity = segments.Back().m_offset + segments.Back().m_size;
            if (newSize <= capacity) {
                node->m_size = newSize;
                return true;
            }
        }
        UInt32 grow = (newSize - node->m_size + 0x1fff) & ~0x1fffu;
        if (!AllocateSegments(grow, capacity, segments))
            return false;
    } else {
        while (!segments.IsEmpty() && segments.Back().m_offset >= newSize)
            segments.PopBack();
        if (newSize < m_position)
            m_position = newSize;
    }
    node->m_size = newSize;
    return true;
}

// Mac 0x1ab850, Win 0x43a2e0
UInt64 MemoryFile::DoPosition()
{
    return m_position;
}

// Mac 0x1ab860, Win 0x43a2f0
bool MemoryFile::DoPosition(UInt64 position)
{
    if (position > m_node->m_size)
        return false;
    m_position = (UInt32)position;
    return true;
}

} // namespace LEGO
