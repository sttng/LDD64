// LEGO Digital Designer — restored source
// LEGO::Directory — VFS directory: per-name virtual interface of every storage backend plus the
// path API ('/' separated, "." and "x/.." normalised, storages mounted via the ObjectFactory with
// OpenArgs); Descriptor — one enumerated entry. TU "Directory.cpp" (Mac 0x12530-0x18bbf); key
// function OnFinalRelease (Mac 0x16b80), vtable Mac 0x703e60 / Win 0xa1f78c; inline dtor (body
// Mac 0x11a80 in TU 012; D1 0x17ed0 / D0 0x17ee0, Win sdd 0x424f10).
//
// Slots (Mac index; Win = Mac - 1): base body, then [LiffStorageDirectory, MacOSXDirectory]:
//   0-9  see DirectoryElement.h; 2/3/4 = 0x16b80 OnFinalRelease, 0x16be0 AddRef,
//        0x16bf0 Release (takes the storage lock), 6 = 0x17f20 {dir, readable}
//   10 DoGetDirectory(name) -> new Directory*   0x17f30 0     [0x10cf0, 0x329f30]
//   11 DoCreateDirectory(name)                   0x17f40 false [0x10eb0, 0x329fb0]
//   12 DoOpenFile(name, mode, create, size*)     0x17f50 0     [0x10ef0, 0x32a020]
//   13 DoDelete(name, recursive)                 0x17f60 false [0x10fe0, 0x32a1c0]
//   14 DoRename(from, to)                        0x11920 -> DoMove(from, this, to, 0, 0) [-, 0x32a5d0]
//   15 DoMove(name, dest, destName, ovw, force)  0x15df0
//   16 DoCopy(name, dest, destName, ovw, force)  0x15e60
//   17 DoGetChildAttributes(name, Attributes*)   0x17f70 false [0x11030, 0x32a670]
//   18 DoSetChildAttributes(name, attributes)    0x17f80 false [0x110b0, 0x32a750]
//   19 DoGetChildTimes(name, Times*)             0x17f90 false [0x11120, 0x32a7a0]
//   20 DoSetChildTimes(name, times)              0x17fa0 false [0x111a0, 0x32a7f0]
//   21 DoIsEmpty()                               0x17fb0 true  [0x11220, 0x32a840]
//   22 DoDirectoryExists(path components)        0x17fc0 false [0x11240, 0x32a890]
//   23 DoFileExists(name)                        0x17fd0 false [0x11300, 0x32a990]
//   24 DoEnumerate(pattern, BasicSet<Descriptor>&) 0x17fe0 {} [0x11390, 0x32aa10]
//   25 DoBeginEnumeration()                      0x167b0       [0x113d0, -]
//   26 DoOpenCurrent(mode, size*) -> RefPointer<File> 0x16850  [0x11420, -]
//   27 DoMoveNext()                              0x168a0       [0x11480, -]
//   28 DoIsEnumerationDone()                     0x168e0       [0x114d0, -]
//   29 DoIsEnumerating()                         0x16900       [0x114f0, -]
//   30 DoEndEnumeration()                        0x16910       [0x11500, -]
// RootDirectory (Directory.cpp) overrides 14 and adds 31, 32, 33.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "ObjectFactory.h"
#include "DirectoryElement.h"
#include "File.h"

namespace LEGO {

// File.h may still be in progress when it is the first header a TU includes.
class File;
struct OpenMode;

// One entry of an enumeration (RTTI name LEGO::Descriptor via BasicSet<Descriptor>).
// Mac sizeof 0x34 (UInt64 4-aligned), Win 0x28. Ordered case-insensitively by name.
struct Descriptor {
    Descriptor(const String& name, const Attributes& attributes, const Times& times, UInt64 size) // Win 0x424c10
        : m_name(name), m_attributes(attributes), m_times(times), m_size(size)
    {
    }

    bool operator<(const Descriptor& other) const { return m_name.LessNoCase(other.m_name); }

    String     m_name;         // +0x00
    Attributes m_attributes;   // Mac +0x10, Win +0x04
    Times      m_times;        // Mac +0x14, Win +0x08
    UInt64     m_size;         // Mac +0x2c, Win +0x20 (0 for live sub-directories)
};

typedef BasicSet<Descriptor, Less<Descriptor> > DescriptorSet;          // (name inferred)

// Mac sizeof 0x6c, Win 0x74.
class Directory : public DirectoryElement {
public:
    // Arguments passed to ObjectFactory::CreateObject(typeid(Directory), ...) to mount a
    // storage (archive file) found at `m_path` below `m_directory`.
    // Mac vtable 0x769208 {D1 0x15420, D0 0x182e0}, Win 0x9cdbcc {sdd 0x424ef0}.
    struct OpenArgs : public ObjectFactoryArgs {
        OpenArgs(Directory* directory, const BasicArray<String>& path, bool strict) // Win 0x424d90 (inlined on Mac)
            : m_directory(directory), m_path(path), m_strict(strict)
        {
        }
        virtual ~OpenArgs() {}                                        // Mac 0x15420/0x182e0, Win 0x424f30/0x424ef0

        Directory*         m_directory;   // +0x04 (not referenced)
        BasicArray<String> m_path;        // +0x08
        // LiffStorageFactory (0x19160): when false a missing file gets the default extension
        // appended and a read/write open falls back to read-only. (name inferred)
        bool               m_strict;      // Mac +0x18, Win +0x18
    };

    typedef BasicMap<String, Directory*, StringCaseInsensitiveLess<String> > ChildMap; // (name inferred)

    Directory(const String& name, Directory* parent);                 // Mac 0x16950, Win 0x428c30
    virtual ~Directory() {}                                           // Mac 0x11a80 (TU 012), Win 0x424f10

    // --- Object / DirectoryElement --------------------------------------------------------
    // Removes this directory (and its aliases) from the parent's child map.
    virtual void OnFinalRelease();                                    // Mac 0x16b80, Win 0x428d20
    virtual void AddRef() const;                                      // Mac 0x16be0, Win 0x428d60
    virtual void Release() const;                                     // Mac 0x16bf0, Win 0x428d70
    virtual Attributes DoGetAttributes() const                        // Mac 0x17f20, Win 0x424ed0
    {
        return Attributes(true, true);
    }

    // --- per-name virtual interface (unlocked; `name` is a single path component) ------------
    virtual Directory* DoGetDirectory(const String&) { return 0; }                        // Mac 0x17f30, Win 0x4f9670
    virtual bool       DoCreateDirectory(const String&) { return false; }                 // Mac 0x17f40, Win 0x424d30
    // `size` (optional) truncates/extends the file to that size after opening.
    virtual File*      DoOpenFile(const String&, const OpenMode&, bool /*create*/, const UInt64* /*size*/) { return 0; } // Mac 0x17f50, Win 0x424e90
    virtual bool       DoDelete(const String&, bool /*recursive*/) { return false; }      // Mac 0x17f60, Win 0x4a9530
    virtual bool       DoRename(const String& from, const String& to)                     // Mac 0x11920 (TU 012), Win 0x424ea0
    {
        return DoMove(from, this, to, false, false);
    }
    // Copy, then delete the source (recursively).
    virtual bool       DoMove(const String& name, Directory* destination, const String& destinationName,
                              bool overwrite, bool force);                                // Mac 0x15df0, Win 0x428290
    // Copies a file (contents, attributes, times) or recursively a directory. `overwrite`
    // allows replacing an existing file, `force` makes a read-only target writable first.
    virtual bool       DoCopy(const String& name, Directory* destination, const String& destinationName,
                              bool overwrite, bool force);                                // Mac 0x15e60, Win 0x4282d0
    virtual bool       DoGetChildAttributes(const String&, Attributes*) { return false; } // Mac 0x17f70, Win 0x4a9530
    virtual bool       DoSetChildAttributes(const String&, const Attributes&) { return false; } // Mac 0x17f80, Win 0x4a9530
    virtual bool       DoGetChildTimes(const String&, Times*) { return false; }           // Mac 0x17f90, Win 0x4a9530
    virtual bool       DoSetChildTimes(const String&, const Times&) { return false; }     // Mac 0x17fa0, Win 0x4a9530
    virtual bool       DoIsEmpty() { return true; }                                       // Mac 0x17fb0, Win 0x4f5b70
    // True when every component of `path` names an existing sub-directory (chain).
    virtual bool       DoDirectoryExists(const BasicArray<String>&) { return false; }     // Mac 0x17fc0, Win 0x424d30
    virtual bool       DoFileExists(const String&) { return false; }                      // Mac 0x17fd0, Win 0x424d30
    // Adds the entries matching the wildcard `pattern` (see MatchWildcard) to `entries`.
    virtual void       DoEnumerate(const String& /*pattern*/, DescriptorSet& /*entries*/) {} // Mac 0x17fe0, Win 0x424ec0

    // File enumeration over m_enumeration (directories are skipped).
    virtual void             DoBeginEnumeration();                                        // Mac 0x167b0, Win 0x4289d0
    virtual RefPointer<File> DoOpenCurrent(const OpenMode& mode, const UInt64* size);     // Mac 0x16850, Win 0x428ab0
    virtual void             DoMoveNext();                                                // Mac 0x168a0, Win 0x428b60
    virtual bool             DoIsEnumerationDone();                                       // Mac 0x168e0, Win 0x428bd0
    virtual bool             DoIsEnumerating();                                           // Mac 0x16900, Win 0x4df3c0
    virtual void             DoEndEnumeration();                                          // Mac 0x16910, Win 0x428c00
    // (not in the original) Only the native disk storage (Win32Directory, MacOSXDirectory)
    // can replace a file in one step: DoReplaceFile flushes the file `from` to the disk and
    // renames it over `to` (MoveFileEx with MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH /
    // rename(2)), so `to` stays the complete old file until it is the complete new one. The
    // other storages have no such operation (see SafeFileWrite.h).
    virtual bool             DoSupportsReplaceFile() const { return false; }
    virtual bool             DoReplaceFile(const String& /*from*/, const String& /*to*/) { return false; }

    // --- path-based public API (takes the storage lock) ---------------------------------------
    using DirectoryElement::GetAttributes;

    RefPointer<Directory> GetDirectory(const String& path, bool create);                  // Mac 0x12c80, Win 0x425d50
    RefPointer<Directory> GetDirectory(const BasicArray<String>& path, bool create);      // Mac 0x12d80, Win 0x425df0
    // Resolves `path` (normalised in place) as far as possible, through live child directories,
    // DoDirectoryExists/DoGetDirectory and mounted storages. On return `path` holds the
    // unresolved rest and *directory (if given) the deepest directory reached.
    bool DirectoryExists(BasicArray<String>& path, RefPointer<Directory>* directory);     // Mac 0x12fd0, Win 0x427220
    bool DirectoryExists(const String& path, RefPointer<Directory>* directory);           // Mac 0x15190, Win 0x427190
    RefPointer<File> OpenFile(const String& path, const OpenMode& mode, bool create, const UInt64* size); // Mac 0x137a0, Win 0x425ff0
    RefPointer<File> OpenFile(const BasicArray<String>& path, const OpenMode& mode, bool create, const UInt64* size); // Mac 0x138c0, Win 0x4260a0
    bool Exists(const String& path);                                                      // Mac 0x13bf0, Win 0x427920
    bool Exists(const BasicArray<String>& path);                                          // Mac 0x15740, Win 0x4279b0
    bool FileExists(const String& path);                                                  // Mac 0x15490, Win 0x427700
    bool FileExists(const BasicArray<String>& path);                                      // Mac 0x15580, Win 0x427790
    bool Delete(const String& path, bool recursive);                                      // Mac 0x13d90, Win 0x4262c0
    bool Delete(const BasicArray<String>& path, bool recursive);                          // Mac 0x13e90, Win 0x426350
    // (not in the original) DoReplaceFile under the storage lock. Refused when `to` exists
    // and is read-only, as the in-place write it replaces would be (rename(2) itself would
    // replace a read-only file).
    bool ReplaceFile(const String& from, const String& to);
    // Copies `sourcePath` (below this) to `destinationPath` (below `destination`). A source
    // leaf "*" copies every entry of the source directory.
    bool Copy(const String& sourcePath, Directory& destination, const String& destinationPath,
              bool overwrite, bool force);                                                // Mac 0x140e0, Win 0x426530
    bool Copy(const BasicArray<String>& sourcePath, Directory& destination,
              const BasicArray<String>& destinationPath, bool overwrite, bool force);      // Mac 0x142a0, Win 0x426610
    bool SetAttributes(const String& path, const Attributes& attributes);                 // Mac 0x14c10, Win 0x426d70
    bool SetAttributes(const BasicArray<String>& path, const Attributes& attributes);     // Mac 0x14d00, Win 0x426e00
    bool GetAttributes(const String& path, Attributes* attributes);                       // Mac 0x14ed0, Win 0x426f80
    bool GetAttributes(const BasicArray<String>& path, Attributes* attributes);           // Mac 0x14fc0, Win 0x427010
    // Live child directories matching the pattern plus DoEnumerate of the parent directory.
    void Enumerate(const String& pattern, DescriptorSet& entries);                        // Mac 0x159a0, Win 0x427b80
    // Registers this directory under an additional name in its parent's child map.
    bool AddAlias(const String& alias);                                                   // Mac 0x15d00, Win 0x428150

    // The storage lock is taken by BeginEnumeration and released by EndEnumeration.
    void             BeginEnumeration();                                                  // Mac 0x13ce0, Win 0x4261f0
    RefPointer<File> OpenCurrent(const OpenMode& mode, const UInt64* size);               // Mac 0x13d10, Win 0x426210
    void             MoveNext();                                                          // Mac 0x13d40, Win 0x426280
    bool             IsEnumerationDone();                                                 // Mac 0x13d50, Win 0x426290
    void             EndEnumeration();                                                    // Mac 0x13d60, Win 0x4262a0

    // --- helpers ---------------------------------------------------------------------------
    // The root of the virtual file system (ObjectRegistry id 'STOR').
    static RefPointer<Directory> GetRoot();                                               // Mac 0x12970, Win 0x4259d0
    // Case-insensitive match; '*' matches any run of characters (first-fit, no backtracking).
    static bool MatchWildcard(const String& name, const String& pattern);                 // Mac 0x12530, Win 0x425230
    // Drops "." components and "x/.." pairs (leading ".." are kept).
    static void NormalizePath(BasicArray<String>& path);                                  // Mac 0x12a80, Win 0x425ad0

    enum { kRootRegistryId = 0x53544f52 };                            // 'STOR' (name inferred)

protected:
    // Normalises `path`, splits off the last component into `leaf` and resolves the rest into
    // `directory`. Fails for an empty path or a ".." leaf.
    bool ResolveParent(BasicArray<String>& path, String& leaf, RefPointer<Directory>& directory,
                       bool create);                                                      // Mac 0x13ad0, Win 0x428020
    // Follows `path` through live child directories / parents, consuming the components.
    static RefPointer<Directory> FindLoadedDirectory(Directory* directory, BasicArray<String>& path); // Mac 0x15280, Win 0x427ed0

    ChildMap            m_children;      // Mac +0x1c, Win +0x10  live sub-directories (not owned)
    DescriptorSet       m_enumeration;   // Mac +0x38, Win +0x34  entries of the running enumeration
    DescriptorSet::Iterator m_enumerationIterator; // Mac +0x54, Win +0x58
    bool                m_enumerating;   // Mac +0x58, Win +0x60
    BasicArray<String>  m_aliases;       // Mac +0x5c, Win +0x64

private:
    Directory(const Directory&);
    Directory& operator=(const Directory&);
};

// ---------------------------------------------------------------------------
// Inline members of DirectoryElement that need the complete Directory
// ---------------------------------------------------------------------------

// Mac D1 0x17dd0 / D0 0x17e40 (emitted with the vtable in this TU), Win sdd 0x424d70
inline DirectoryElement::~DirectoryElement()
{
}

} // namespace LEGO
