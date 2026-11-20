// LEGO Digital Designer — restored source
// LEGO::LiffChunk, LiffFile — see LiffChunk.h. TU Mac 0x6bb0-0x8c6f; also emitted here: chunk map
// (dtors 0x86a0/0x86d0, _M_erase 0x8720, erase 0x88d0, insert_unique 0x8a00), header array
// (dtors 0x8770/0x87a0), chunk list (dtors 0x7bd0/0x8b30, node 0x8bb0), thunks 0x8480/0x84e0
// (LiffChunk) and 0x8640/0x8660 (LiffFile), static init 0x8c30.
#include "LiffChunk.h"
#include "BasicList.h"
#include "Path.h"

namespace LEGO {

namespace {

// Big-endian (de)serialisation. Win does this with a LEGO::Stream over the stack buffer.
inline void PutBE16(UInt8* p, UInt16 v)
{
    p[0] = (UInt8)(v >> 8);
    p[1] = (UInt8)v;
}

inline void PutBE32(UInt8* p, UInt32 v)
{
    p[0] = (UInt8)(v >> 24);
    p[1] = (UInt8)(v >> 16);
    p[2] = (UInt8)(v >> 8);
    p[3] = (UInt8)v;
}

inline void PutBE64(UInt8* p, UInt64 v)
{
    PutBE32(p, (UInt32)(v >> 32));
    PutBE32(p + 4, (UInt32)v);
}

inline UInt16 GetBE16(const UInt8* p)
{
    return (UInt16)((p[0] << 8) | p[1]);
}

inline UInt32 GetBE32(const UInt8* p)
{
    return ((UInt32)p[0] << 24) | ((UInt32)p[1] << 16) | ((UInt32)p[2] << 8) | (UInt32)p[3];
}

inline UInt64 GetBE64(const UInt8* p)
{
    return ((UInt64)GetBE32(p) << 32) | GetBE32(p + 4);
}

const UInt32 kLiffMagic   = 0x4c494646;   // "LIFF"
const UInt16 kLiffVersion = 1;

} // namespace

// ============================================================================================
// LiffChunk
// ============================================================================================

// Mac 0x6bb0, Win 0x43ab30
RefPointer<LiffChunk> LiffChunk::CreateChunk(const ChunkId& id, UInt32 flags, const UInt64* dataSize)
{
    if (IsLeaf())
        return RefPointer<LiffChunk>();

    UInt64 offset = GetDataSize();
    UInt64 size = ChunkHeader::kSize + (dataSize ? *dataSize : 0);
    if (!DoSetSize(offset + size))
        return RefPointer<LiffChunk>();

    RefPointer<LiffChunk> chunk = new LiffChunk(m_root, this, offset, ChunkHeader(id, size, flags, 0));
    if (!chunk->WriteHeader())
        return RefPointer<LiffChunk>();
    m_children.Insert(offset, chunk.Get());
    return chunk;
}

// Mac 0x6dd0, Win 0x43ad10
RefPointer<LiffChunk> LiffChunk::OpenChunk(UInt32 index, const ChunkId& id)
{
    UInt64 offset;
    ChunkHeader header;
    if (!FindChunk(index, id, offset, header))
        return RefPointer<LiffChunk>();
    return OpenChunkAt(offset, header);
}

// Mac 0x6e30, Win 0x43b440
bool LiffChunk::FindChunk(UInt32 index, const ChunkId& id, UInt64& offset, ChunkHeader& header)
{
    if (IsLeaf() || GetDataSize() == 0)
        return false;

    bool any = id.IsAny();
    UInt32 found = 0;
    offset = 0;

    // headers already scanned
    for (UInt32 i = 0; offset < GetDataSize() && i < m_headers.GetSize(); ++i) {
        header = m_headers[i].second;
        if (any || header.m_id == id)
            ++found;
        offset = m_headers[i].first + header.m_size;
        if (found > index) {
            offset -= header.m_size;
            return true;
        }
    }

    // continue scanning the data area
    while (found <= index) {
        if (offset >= GetDataSize())
            return false;
        if (!ReadChunkHeader(offset, header))
            return false;
        // NOTE: the declared chunk size is not checked (both builds): a size below the chunk
        // header makes the chunk's data size wrap, a size past the parent's data reaches
        // outside it, and a size of 0 never advances (endless loop); fixed: such a chunk ends
        // the search (H3, H4).
        if (header.m_size < ChunkHeader::kSize || header.m_size > GetDataSize() - offset)
            return false;
        if (any || header.m_id == id)
            ++found;
        m_headers.PushBack(MakePair(offset, header));
        offset += header.m_size;
    }
    offset -= header.m_size;
    return true;
}

// Mac inlined into 0x6e30, Win 0x43afc0
bool LiffChunk::ReadChunkHeader(UInt64 offset, ChunkHeader& header)
{
    UInt8 buffer[ChunkHeader::kSize];
    UInt64 read;
    if (!ReadAt(offset, sizeof(buffer), buffer, &read))
        return false;
    header.m_id.m_major = GetBE16(buffer);
    header.m_id.m_minor = GetBE16(buffer + 2);
    header.m_size       = GetBE64(buffer + 4);
    header.m_flags      = GetBE32(buffer + 12);
    header.m_reserved   = GetBE32(buffer + 16);
    return true;
}

// Mac 0x72e0, Win 0x43b290
RefPointer<LiffChunk> LiffChunk::OpenChunkAt(UInt64 offset, const ChunkHeader& header)
{
    ChildMap::Iterator it = m_children.Find(offset);
    if (it != m_children.End())
        return it->second;

    RefPointer<LiffChunk> chunk = new LiffChunk(m_root, this, offset, header);
    m_children.Insert(offset, chunk.Get());
    return chunk;
}

// Mac 0x7500, Win 0x43ade0
bool LiffChunk::DeleteChunk(UInt32 index, const ChunkId& id)
{
    UInt64 offset;
    ChunkHeader header;
    if (!FindChunk(index, id, offset, header))
        return false;
    if (m_children.Find(offset) != m_children.End())
        return false;   // still open

    UInt64 dataSize = GetDataSize();
    UInt64 end = offset + header.m_size;
    if (!CopyRange(end, offset, dataSize - end))
        return false;
    ShiftChunks(offset, -(Int64)header.m_size);
    return DoSetSize(dataSize - header.m_size);
}

// Mac 0x7680 (Ghidra attributed it to BasicList<RefPointer<LiffChunk>>), Win 0x43b660
void LiffChunk::ShiftChunks(UInt64 after, Int64 delta)
{
    BasicList<RefPointer<LiffChunk> > moved;
    ChildMap::Iterator first = m_children.UpperBound(after);
    for (ChildMap::Iterator it = first; it != m_children.End(); ++it)
        moved.PushBack(it->second);
    m_children.Erase(first, m_children.End());

    for (BasicList<RefPointer<LiffChunk> >::Iterator it = moved.Begin(); it != moved.End(); ++it) {
        LiffChunk* chunk = *it;
        chunk->m_position += delta;
        m_children.Insert(chunk->m_position, chunk);
    }
    m_headers.Clear();
}

// Mac 0x78f0, Win inlined
bool LiffChunk::SetSize(UInt64 dataSize)
{
    if (!IsLeaf())
        return false;
    return DoSetSize(dataSize);
}

// Mac 0x7910, Win 0x43af00
bool LiffChunk::ReadAt(UInt64 position, UInt64 size, void* buffer, UInt64* read)
{
    Int64 count = (Int64)(GetDataSize() - position);
    if ((Int64)size < count)
        count = (Int64)size;
    if (count <= 0) {
        if (read)
            *read = 0;
        return size == 0;
    }

    UInt64 done;
    bool ok = DoReadAt(m_headerLength + position + m_position, (UInt64)count, buffer, &done);
    if (read)
        *read = done;
    // NOTE: compared with the requested size, so reading past the end fails (after reading).
    return ok & (done == size);
}

// Mac 0x79e0, Win 0x43bb80
bool LiffChunk::WriteAt(UInt64 position, UInt64 size, const void* buffer, UInt64* written)
{
    if (position > GetDataSize() ||
        (position + size > GetDataSize() && !DoSetSize(position + size))) {
        if (written)
            *written = 0;
        return false;
    }
    return DoWriteAt(m_headerLength + position + m_position, size, buffer, written);
}

// Mac 0x7ab0, Win 0x43b110
bool LiffChunk::WriteHeader()
{
    UInt8 buffer[ChunkHeader::kSize];
    PutBE16(buffer, m_header.m_id.m_major);
    PutBE16(buffer + 2, m_header.m_id.m_minor);
    PutBE64(buffer + 4, m_header.m_size);
    PutBE32(buffer + 12, m_header.m_flags);
    PutBE32(buffer + 16, m_header.m_reserved);
    return m_parent->WriteAt(m_position, sizeof(buffer), buffer, 0);
}

// Mac 0x7c30, Win 0x43b930
bool LiffChunk::DoSetSize(UInt64 dataSize)
{
    UInt64 oldSize = GetDataSize();
    UInt64 parentSize = m_parent->GetDataSize();
    m_headers.Clear();
    Int64 delta = (Int64)(dataSize - oldSize);
    // NOTE: uses the chunk header size, not m_headerLength (equal for every non-file chunk).
    UInt64 start = m_position + ChunkHeader::kSize;
    UInt64 end = start + oldSize;

    bool ok;
    if (delta <= 0) {
        // shrinking: move what follows first, then cut the parent
        if (!m_parent->CopyRange(end, start + dataSize, parentSize - end))
            return false;
        ok = m_parent->DoSetSize(parentSize + delta);
    } else {
        // growing: enlarge the parent first, then move what follows
        if (!m_parent->DoSetSize(parentSize + delta))
            return false;
        ok = m_parent->CopyRange(end, start + dataSize, parentSize - end);
    }
    if (!ok)
        return false;

    m_header.m_size += delta;
    if (!WriteHeader())
        return false;
    m_parent->ShiftChunks(m_position, delta);
    return true;
}

// Mac inlined (0x7500, 0x7c30, 0x7e20), Win 0x43b8e0
bool LiffChunk::CopyRange(UInt64 from, UInt64 to, UInt64 size)
{
    m_headers.Clear();
    UInt64 base = m_headerLength + m_position;
    return DoCopyRange(base + from, base + to, size);
}

// Mac 0x7e20, Win 0x43bac0
bool LiffChunk::DoCopyRange(UInt64 from, UInt64 to, UInt64 size)
{
    return m_parent->CopyRange(from, to, size);
}

// Mac 0x7e70, Win 0x43bb10 (-> 0x43af00)
bool LiffChunk::DoReadAt(UInt64 position, UInt64 size, void* buffer, UInt64* read)
{
    return m_parent->ReadAt(position, size, buffer, read);
}

// Mac 0x7f40, Win 0x43bb40 (-> 0x43bb80)
bool LiffChunk::DoWriteAt(UInt64 position, UInt64 size, const void* buffer, UInt64* written)
{
    return m_parent->WriteAt(position, size, buffer, written);
}

// Mac D1 0x83b0, D0 0x8410, D2 (VTT) 0x87d0; thunks 0x8480/0x84e0. Win 0x42ef90, sdd 0x42f050.
LiffChunk::~LiffChunk()
{
    if (m_parent)
        m_parent->m_children.Erase(m_position);
}

// ============================================================================================
// LiffFile
// ============================================================================================

// Mac 0x8000, Win 0x43bc20
bool LiffFile::Open(const Path& path, const OpenMode& mode, bool create)
{
    if (m_file)
        return false;
    RefPointer<File> file = path.OpenFile(mode, create);
    if (!file)
        return false;
    return Open(file, create);
}

// Mac 0x80a0, Win 0x43bcd0
bool LiffFile::Open(File* file, bool create)
{
    if (m_file)
        return false;
    m_file = file;

    if (create && m_file->GetSize() == 0) {
        m_header.m_size = ChunkHeader::kFileSize;
        m_header.m_reserved = 0;
        if (m_file->SetSize(ChunkHeader::kFileSize) && WriteHeader())
            return true;
        m_file = 0;
        return false;
    }

    UInt8 buffer[ChunkHeader::kFileSize];
    if (!m_file->ReadAt(0, sizeof(buffer), buffer, 0))
        return false;   // NOTE: the file stays attached
    UInt32 magic = GetBE32(buffer);
    m_header.m_size = GetBE64(buffer + 4);
    UInt16 version = GetBE16(buffer + 12);
    m_header.m_reserved = GetBE32(buffer + 14);
    if (magic == kLiffMagic && m_file->GetSize() == m_header.m_size && version == kLiffVersion)
        return true;
    m_file = 0;
    return false;
}

// Mac 0x8230, Win 0x43be80
bool LiffFile::WriteHeader()
{
    UInt8 buffer[ChunkHeader::kFileSize];
    PutBE32(buffer, kLiffMagic);
    PutBE64(buffer + 4, m_header.m_size);
    PutBE16(buffer + 12, kLiffVersion);
    PutBE32(buffer + 14, m_header.m_reserved);
    return m_file->WriteAt(m_position, sizeof(buffer), buffer, 0);
}

// Mac 0x82d0, Win 0x43bf90
bool LiffFile::DoSetSize(UInt64 dataSize)
{
    if (!m_file->SetSize(m_headerLength + dataSize))
        return false;
    m_header.m_size = m_file->GetSize();
    return WriteHeader();
}

// Mac 0x8320, Win 0x43bfe0
bool LiffFile::DoCopyRange(UInt64 from, UInt64 to, UInt64 size)
{
    return m_file->CopyRange(from, to, size);
}

// Mac 0x8350, Win 0x43c010
bool LiffFile::DoReadAt(UInt64 position, UInt64 size, void* buffer, UInt64* read)
{
    return m_file->ReadAt(position, size, buffer, read);
}

// Mac 0x8380, Win 0x43c040
bool LiffFile::DoWriteAt(UInt64 position, UInt64 size, const void* buffer, UInt64* written)
{
    return m_file->WriteAt(position, size, buffer, written);
}

// Mac D1 0x8560, D0 0x8600, thunks 0x8640/0x8660. Win 0x42f160, sdd 0x42f1e0.
LiffFile::~LiffFile()
{
}

} // namespace LEGO
