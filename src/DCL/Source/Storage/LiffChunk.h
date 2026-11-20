// LEGO Digital Designer — restored source
// LEGO::ChunkId, ChunkHeader, LiffChunk, LiffFile — the chunked "LIFF" container behind
// LiffStorage (Assets.lif, Sound.lif). TU "LiffChunk.cpp": Mac 0x6bb0-0x8c6f (41 functions), Win
// 0x42ee80-0x430f7f / 0x43ab30-0x43c3ff.
// Format (big-endian): file header (18 bytes) "LIFF", UInt64 file size, UInt16 version 1, UInt32
//   reserved; chunk header (20 bytes) UInt16 id major/minor, UInt64 size incl. header, UInt32
//   flags (bit 0 = leaf), UInt32 reserved. A non-leaf data area is a sequence of child chunks;
//   the LiffFile is the root. Assets.lif: one (1,1) with (1,2) version, (1,3) folders, (1,4) file
//   data, (1,5) directory tree.
// A chunk's Do...At / DoCopyRange slots take positions in its parent's data area (own data at
//   m_position + m_headerLength) and forward up; LiffFile uses absolute File positions. Open
//   children are cached weakly by offset (they hold their parent and unregister in the dtor);
//   scanned headers are cached in m_headers until the data area changes.
//
// Win uses a LEGO::Stream over a stack buffer to (de)serialise the headers (Win 0x43afc0,
// 0x43b110, 0x43bcd0, 0x43be80); Mac inlines it to byte swaps, and so does the
// .cpp (local big-endian helpers).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicMap.h"
#include "BasicArray.h"
#include "File.h"

namespace LEGO {

class Path;
class LiffFile;

// Chunk type, two UInt16 (name inferred). (0,0) means "any type" in lookups.
// Evidence: TU 012 has 2-byte aligned static instances (1,1) .. (1,5) with a dynamic initialiser
// (Mac 0x12490), so the type has a constructor.
struct ChunkId {
    ChunkId(UInt16 major = 0, UInt16 minor = 0) : m_major(major), m_minor(minor) {}

    bool operator==(const ChunkId& o) const { return m_major == o.m_major && m_minor == o.m_minor; }
    bool operator!=(const ChunkId& o) const { return !(*this == o); }
    bool IsAny() const { return m_major == 0 && m_minor == 0; }   // (name inferred)

    UInt16 m_major;   // +0x00 (1 for every chunk in the shipped files)
    UInt16 m_minor;   // +0x02
};

// In-memory copy of a chunk header. RTTI: class LEGO::ChunkHeader (Pair<UInt64, ChunkHeader>).
// Mac sizeof 0x14, Win 0x18 (UInt64 is 8-aligned there).
class ChunkHeader {
public:
    enum {
        kFlagLeaf = 0x1   // chunk holds raw data, no child chunks (name inferred)
    };
    enum {
        kSize     = 20,   // serialised chunk header
        kFileSize = 18    // serialised LIFF file header
    };

    ChunkHeader() : m_id(), m_size(0), m_flags(0), m_reserved(0) {}
    ChunkHeader(const ChunkId& id, UInt64 size, UInt32 flags, UInt32 reserved = 0)
        : m_id(id), m_size(size), m_flags(flags), m_reserved(reserved)
    {
    }

    ChunkId m_id;         // +0x00
    UInt64  m_size;       // Mac +0x04, Win +0x08: whole chunk including its header
    UInt32  m_flags;      // Mac +0x0c, Win +0x10
    UInt32  m_reserved;   // Mac +0x10, Win +0x14 (0 in all shipped files; name inferred)
};

// Mac sizeof 100 (0x5c + virtual Object at +0x5c), Win 0x80 (Object at +0x78).
// Mac vtable 0x703a9c (10 slots, D1/D0 last), Win 0xa1fdac (8 slots; the scalar deleting dtor
// sits in the Object sub-vtable 0xa1fdd0: 0x42f050).
//   slot  Mac     Win       member
//    0    0x6bb0  0x43ab30  CreateChunk
//    1    0x6dd0  0x43ad10  OpenChunk
//    2    0x7500  0x43ade0  DeleteChunk
//    3    0x7ab0  0x43b110  WriteHeader
//    4    0x7c30  0x43b930  DoSetSize
//    5    0x7e20  0x43bac0  DoCopyRange
//    6    0x7e70  0x43bb10  DoReadAt
//    7    0x7f40  0x43bb40  DoWriteAt
//    8/9  0x83b0/0x8410     ~LiffChunk (D1/D0)
class LiffChunk : public virtual Object {
public:
    typedef BasicMap<UInt64, LiffChunk*>        ChildMap;     // RTTI BasicMap<UInt64, LiffChunk*, Less<UInt64>>
    typedef BasicArray<Pair<UInt64, ChunkHeader> > HeaderArray;

    // Inline: Mac inlines it into 0x6bb0, 0x72e0 and LiffFile's ctor (TU 014); Win 0x42ee80.
    LiffChunk(LiffFile* root, LiffChunk* parent, UInt64 position, const ChunkHeader& header)
        : m_header(header), m_headerLength(ChunkHeader::kSize), m_parent(parent), m_root(root),
          m_position(position)
    {
    }

    // --- virtual interface ------------------------------------------------------------------
    // Appends a new child chunk at the end of the data area, with `*dataSize` (or 0) bytes of
    // zero/garbage data, and writes its header. Fails on leaf chunks.
    virtual RefPointer<LiffChunk> CreateChunk(const ChunkId& id, UInt32 flags,
                                              const UInt64* dataSize);                // Mac 0x6bb0, Win 0x43ab30
    // Opens the index-th child of type `id` ((0,0) = any type; counting only matching children).
    virtual RefPointer<LiffChunk> OpenChunk(UInt32 index, const ChunkId& id);         // Mac 0x6dd0, Win 0x43ad10
    // Removes the index-th child of type `id` and closes the gap. Fails if that child is open.
    virtual bool DeleteChunk(UInt32 index, const ChunkId& id);                        // Mac 0x7500, Win 0x43ade0
    // Serialises m_header at m_position in the parent (LiffFile: the 18-byte file header).
    virtual bool WriteHeader();                                                       // Mac 0x7ab0, Win 0x43b110
    // Resizes the data area to `dataSize` bytes, moving everything behind this chunk in all
    // ancestors and shifting the open siblings. (name inferred)
    virtual bool DoSetSize(UInt64 dataSize);                                          // Mac 0x7c30, Win 0x43b930
    // Raw accessors in the parent's data area (see "Coordinates" above). (names inferred)
    virtual bool DoCopyRange(UInt64 from, UInt64 to, UInt64 size);                    // Mac 0x7e20, Win 0x43bac0
    virtual bool DoReadAt(UInt64 position, UInt64 size, void* buffer, UInt64* read);  // Mac 0x7e70, Win 0x43bb10
    virtual bool DoWriteAt(UInt64 position, UInt64 size, const void* buffer,
                           UInt64* written);                                          // Mac 0x7f40, Win 0x43bb40
    virtual ~LiffChunk();                                                             // Mac 0x83b0/0x8410 (D2 0x87d0), Win 0x42ef90

    // --- data access (positions relative to this chunk's data area) --------------------------
    // Reads min(size, available) bytes; returns true only when all `size` bytes were read.
    // NOTE: a short read returns false but still delivers the data and sets *read.
    bool ReadAt(UInt64 position, UInt64 size, void* buffer, UInt64* read);            // Mac 0x7910, Win 0x43af00
    // Writes, growing the data area when needed; `position` must be <= GetDataSize().
    bool WriteAt(UInt64 position, UInt64 size, const void* buffer, UInt64* written);  // Mac 0x79e0, Win 0x43bb80
    // DoSetSize for leaf chunks only (container chunks grow through CreateChunk). (name inferred)
    bool SetSize(UInt64 dataSize);                                                    // Mac 0x78f0, Win inlined

    // --- accessors (inline; LiffStorageFile reads the fields directly) ------------------------
    const ChunkHeader& GetHeader() const       { return m_header; }
    const ChunkId&     GetId() const           { return m_header.m_id; }
    UInt32             GetFlags() const        { return m_header.m_flags; }
    bool               IsLeaf() const          { return (m_header.m_flags & ChunkHeader::kFlagLeaf) != 0; }
    UInt64             GetSize() const         { return m_header.m_size; }       // including header
    UInt64             GetHeaderLength() const { return m_headerLength; }
    UInt64             GetDataSize() const     { return m_header.m_size - m_headerLength; }
    UInt64             GetPosition() const     { return m_position; }           // in parent's data
    LiffChunk*         GetParent() const       { return m_parent; }
    LiffFile*          GetRoot() const         { return m_root; }

protected:
    // Finds the index-th child of type `id`: its offset in the data area and its header.
    // Uses and extends the header cache. (name inferred)
    bool FindChunk(UInt32 index, const ChunkId& id, UInt64& offset, ChunkHeader& header);  // Mac 0x6e30, Win 0x43b440
    // Returns the cached open child at `offset` or opens a new one. (name inferred)
    RefPointer<LiffChunk> OpenChunkAt(UInt64 offset, const ChunkHeader& header);           // Mac 0x72e0, Win 0x43b290
    // Moves the open children behind `after` by `delta` bytes. (name inferred)
    void ShiftChunks(UInt64 after, Int64 delta);                                           // Mac 0x7680, Win 0x43b660
    // DoCopyRange on this chunk's own data area (drops the header cache). (name inferred)
    bool CopyRange(UInt64 from, UInt64 to, UInt64 size);                                   // Mac inlined, Win 0x43b8e0
    // Reads and parses the 20-byte header at `offset` of the data area. (name inferred)
    bool ReadChunkHeader(UInt64 offset, ChunkHeader& header);                              // Mac inlined, Win 0x43afc0

    ChunkHeader           m_header;        // Mac +0x04, Win +0x10
    UInt64                m_headerLength;  // Mac +0x18, Win +0x28: 0x14, LiffFile 0x12
    RefPointer<LiffChunk> m_parent;        // Mac +0x20, Win +0x30 (null for LiffFile)
    LiffFile*             m_root;          // Mac +0x24, Win +0x34 (copied from the parent)
    UInt64                m_position;      // Mac +0x28, Win +0x38: header offset in parent's data
    ChildMap              m_children;      // Mac +0x30, Win +0x40: open children by offset (weak)
    HeaderArray           m_headers;       // Mac +0x4c, Win +0x64: headers scanned so far
};

// The LIFF file: root of the chunk tree, backed by a File.
// Mac sizeof 0x68 (Object at +0x60), Win 0x88 (Object at +0x80).
// Mac vtable 0x703b24 (overrides slots 3-9: 0x8230 0x82d0 0x8320 0x8350 0x8380 0x8560 0x8600),
// Win 0xa1fdfc (0x43be80 0x43bf90 0x43bfe0 0x43c010 0x43c040; sdd 0x42f1e0).
class LiffFile : public LiffChunk {
public:
    // Inline on Mac (TU 014 Mac 0x192b1..0x19392), Win 0x42f080.
    LiffFile()
        : LiffChunk(0, 0, 0, ChunkHeader()), m_file()
    {
        m_root = this;
        m_headerLength = ChunkHeader::kFileSize;
    }
    virtual ~LiffFile();                                                              // Mac 0x8560/0x8600, Win 0x42f160

    // Opens (and with `create`, creates) the file at `path` and attaches it.
    bool Open(const Path& path, const OpenMode& mode, bool create);                    // Mac 0x8000, Win 0x43bc20
    // Attaches `file`: an empty file is initialised when `create` is set, otherwise the LIFF
    // header is read and checked (magic, version 1, size == file size).
    bool Open(File* file, bool create);                                               // Mac 0x80a0, Win 0x43bcd0

    virtual bool WriteHeader();                                                       // Mac 0x8230, Win 0x43be80
    virtual bool DoSetSize(UInt64 dataSize);                                          // Mac 0x82d0, Win 0x43bf90
    virtual bool DoCopyRange(UInt64 from, UInt64 to, UInt64 size);                    // Mac 0x8320, Win 0x43bfe0
    virtual bool DoReadAt(UInt64 position, UInt64 size, void* buffer, UInt64* read);  // Mac 0x8350, Win 0x43c010
    virtual bool DoWriteAt(UInt64 position, UInt64 size, const void* buffer,
                           UInt64* written);                                          // Mac 0x8380, Win 0x43c040

    File* GetFile() const { return m_file; }   // (accessor, not located)

private:
    RefPointer<File> m_file;   // Mac +0x5c, Win +0x78
};

} // namespace LEGO
