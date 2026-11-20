// LEGO Digital Designer — restored source
// LEGO::LiffStorageVolume — one mounted .lif: owns the LiffFile, its root chunk and the
// MonolithTree; creates the root LiffStorageDirectory. Made by LiffStorageFactory::CreateObject
// (TU 014, Mac 0x19160, Win 0x42f410) without keeping a reference: lives as long as its
// directories / files. Vtable Mac 0x75faf8 {D1 0x196a0, D0 0x196b0, ..}, Win 0xa1fe3c {sdd
// 0x42f340}; sizeof 0x20. Dtor body Mac 0x196f0, Win 0x42f2a0; ctor inline on Mac, Win 0x42f210;
// Mount Mac 0x10300 (TU 012), Win 0x437df0. Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"

namespace LEGO {

class Directory;
class LiffChunk;
class LiffFile;
class LiffStorageDirectory;
class MonolithTree;

class LiffStorageVolume : public Object {
public:
    // Takes a reference on `file` (already opened by the caller).
    explicit LiffStorageVolume(LiffFile* file);                       // Mac inline in 0x19160, Win 0x42f210
    virtual ~LiffStorageVolume();                                     // Mac 0x196a0/0x196b0 (body 0x196f0), Win 0x42f2a0/0x42f340

    // Reads (or, with `create`, initialises) the archive header and directory chunks, builds the
    // MonolithTree and creates the root LiffStorageDirectory named `name` below `parent`.
    // Fails if already mounted. Implemented in LiffStorageDirectory.cpp (TU 012).
    bool Mount(bool create, const String& name, Directory* parent);   // Mac 0x10300, Win 0x437df0 (name inferred)

    // The root directory of the archive after a successful Mount (not AddRef'd).
    LiffStorageDirectory* GetRoot() const      { return m_root; }      // (name inferred; inlined)
    LiffFile*             GetFile() const      { return m_file.Get(); } // (name inferred; inlined)
    LiffChunk*            GetRootChunk() const { return m_rootChunk.Get(); } // (name inferred; inlined)
    MonolithTree*         GetTree() const      { return m_tree; }      // (name inferred; inlined)
    UInt16                GetVersion() const   { return m_version; }   // (name inferred; inlined)
    UInt32                GetFlags() const     { return m_flags; }     // (name inferred; inlined)

protected:
    friend class LiffStorageDirectory;
    friend class LiffStorageFile;

    // Layout (Mac and Win): vptr +0x00, refcount +0x04
    UInt16                m_version;     // +0x08 archive format version, 1 (stored big-endian in the header chunk)
    UInt32                m_flags;       // +0x0c header flags; bit 0 is copied to the MonolithTree (+0x08)
    LiffStorageDirectory* m_root;        // +0x10 created by Mount (not owned)
    MonolithTree*         m_tree;        // +0x14 owned, deleted first in the destructor
    RefPointer<LiffChunk> m_rootChunk;   // +0x18
    RefPointer<LiffFile>  m_file;        // +0x1c

private:
    LiffStorageVolume(const LiffStorageVolume&);
    LiffStorageVolume& operator=(const LiffStorageVolume&);
};

} // namespace LEGO
