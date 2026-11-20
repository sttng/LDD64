// LEGO Digital Designer — restored source
// Windows VFS disk backend (Mac: MacOSXDirectory): Win32Directory (absolute path), Win32File
// (HANDLE), Win32FileProtocolDirectory (root "file": "c" -> "c:"), Win32NetworkProtocolDirectory
// (root "network": "x" -> "\\x"), Win32StorageFactory (mounts both). Names inferred except RTTI
// class names. Win 0x43c9a0-0x43e9ef and 0x42e860-0x42ee7f. The original's A/W wrappers (Win
// 0x415140-0x416bb0) are direct ...W calls here (see Win32Directory.cpp).
#pragma once
#if defined(_WIN32)

#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "ObjectFactory.h"
#include "Directory.h"
#include "File.h"

#include <windows.h>

namespace LEGO {

// A directory on disk. Children are addressed as m_path + "\\" + name.
// Win sizeof 0x78 (Directory 0x74 + path). Vtable 0xa202e4 (30 slots):
//   0 sdd 0x43cae0 (dtor 0x43ca80)   1-3 Directory          4 DoSetAttributes 0x43cc10
//   5 DoGetAttributes 0x43cc40       6 DoSetTimes 0x44b280   7 DoGetTimes 0x43cc80
//   8 DoSetInfo 0x425830 (base)      9 DoGetDirectory 0x43cca0
//  10 DoCreateDirectory 0x43cd60    11 DoOpenFile 0x43cdf0  12 DoDelete 0x43cf30
//  13-15 DoRename/DoMove/DoCopy (base: rename = copy + delete)
//  16 DoGetChildAttributes 0x43d450 17 DoSetChildAttributes 0x43d530
//  18 DoGetChildTimes 0x43d5d0      19 DoSetChildTimes 0x43d680  20 DoIsEmpty 0x43d7b0
//  21 DoDirectoryExists 0x43d9e0    22 DoFileExists 0x43dad0     23 DoEnumerate 0x43db60
//  24-29 enumeration (base)
class Win32Directory : public Directory {
public:
    // `path`: absolute Win32 path of this directory without trailing separator.
    Win32Directory(const String& name, Directory* parent, const String& path); // Win 0x43ca10
    virtual ~Win32Directory();                                                // Win 0x43ca80 / sdd 0x43cae0

    virtual bool       DoSetAttributes(const Attributes& attributes);         // Win 0x43cc10
    virtual Attributes DoGetAttributes() const;                               // Win 0x43cc40
    // NOTE: ignored; always succeeds (Win 0x44b280 is a shared `mov al,1; ret 4`).
    virtual bool       DoSetTimes(const Times&) { return true; }              // Win 0x44b280
    virtual Times      DoGetTimes() const;                                    // Win 0x43cc80

    virtual Directory* DoGetDirectory(const String& name);                    // Win 0x43cca0
    virtual bool       DoCreateDirectory(const String& name);                 // Win 0x43cd60
    virtual File*      DoOpenFile(const String& name, const OpenMode& mode, bool create,
                                  const UInt64* size);                        // Win 0x43cdf0
    virtual bool       DoDelete(const String& name, bool recursive);          // Win 0x43cf30
    virtual bool       DoGetChildAttributes(const String& name, Attributes* attributes); // Win 0x43d450
    virtual bool       DoSetChildAttributes(const String& name, const Attributes& attributes); // Win 0x43d530
    virtual bool       DoGetChildTimes(const String& name, Times* times);     // Win 0x43d5d0
    virtual bool       DoSetChildTimes(const String& name, const Times& times); // Win 0x43d680
    virtual bool       DoIsEmpty();                                           // Win 0x43d7b0
    virtual bool       DoDirectoryExists(const BasicArray<String>& path);     // Win 0x43d9e0
    virtual bool       DoFileExists(const String& name);                      // Win 0x43dad0
    virtual void       DoEnumerate(const String& pattern, DescriptorSet& entries); // Win 0x43db60
    // (not in the original) FlushFileBuffers, then MoveFileEx over `to` (Directory.h).
    virtual bool       DoSupportsReplaceFile() const { return true; }
    virtual bool       DoReplaceFile(const String& from, const String& to);

    const String& GetNativePath() const { return m_path; }                    // (name inferred; not seen)

protected:
    String m_path;   // Win +0x74
};

// An open disk file. Win sizeof 0x1c (File 0x14 + path + handle). Vtable 0xa20364 (17 slots):
//   0 sdd 0x43cb80 (dtor 0x43e300)   1-3 Object   4 DoSetAttributes 0x43df40
//   5 DoGetAttributes 0x43df70       6 DoSetTimes 0x43dfb0   7 DoGetTimes 0x43e010
//   8 DoSetInfo 0x425830             9 DoRead(size) 0x43e080 10 DoRead(pos) 0x439e10 (ICF)
//  11 DoWrite(size) 0x43e110        12 DoWrite(pos) 0x43e030 (ICF)
//  13 DoSize 0x43e1d0  14 DoSize 0x43e1a0  15 DoPosition 0x43e290  16 DoPosition 0x43e250
// (13-16: set-before-get in every File subclass, i.e. overload pairs; see File.h.)
class Win32File : public File {
public:
    // Takes ownership of `handle` (closed by the destructor).
    Win32File(const String& name, Directory* parent, const OpenMode& mode, const String& path,
              HANDLE handle);                                                 // Win 0x43cb00
    virtual ~Win32File();                                                     // Win 0x43e300 / sdd 0x43cb80

    virtual bool       DoSetAttributes(const Attributes& attributes);         // Win 0x43df40
    virtual Attributes DoGetAttributes() const;                               // Win 0x43df70
    virtual bool       DoSetTimes(const Times& times);                        // Win 0x43dfb0
    virtual Times      DoGetTimes() const;                                    // Win 0x43e010

    virtual bool   DoRead(UInt64 position, UInt64 size, void* buffer, UInt64* read);         // Win 0x439e10 (ICF)
    virtual bool   DoWrite(UInt64 position, UInt64 size, const void* buffer, UInt64* written); // Win 0x43e030 (ICF)
    virtual bool   DoRead(UInt64 size, void* buffer, UInt64* read);           // Win 0x43e080
    virtual bool   DoWrite(UInt64 size, const void* buffer, UInt64* written); // Win 0x43e110
    virtual UInt64 DoSize();                                               // Win 0x43e1a0
    virtual bool   DoSize(UInt64 size);                                    // Win 0x43e1d0
    virtual UInt64 DoPosition();                                           // Win 0x43e250
    virtual bool   DoPosition(UInt64 position);                                   // Win 0x43e290

protected:
    String m_path;     // Win +0x14
    HANDLE m_handle;   // Win +0x18
};

// Root directory "file": its children are the drive letters; "c" maps to the Win32 path "c:".
// Win sizeof 0x74 (no own members). Vtable 0xa1fc7c (33 slots): sdd 0x42e910 (dtor 0x42e8c0),
// DoGetDirectory 0x43e370, DoIsEmpty 0x43e430, DoDirectoryExists 0x43e440, DoEnumerate 0x43e5a0,
// Unknown31/32 0x42e9e0, DoRootExists 0x43e550; all other slots are the Directory defaults.
class Win32FileProtocolDirectory : public Directory {
public:
    Win32FileProtocolDirectory(const String& name, Directory* parent)       // Win 0x42e860
        : Directory(name, parent)
    {
    }
    virtual ~Win32FileProtocolDirectory() {}                                  // Win 0x42e8c0 / sdd 0x42e910

    virtual Directory* DoGetDirectory(const String& name);                    // Win 0x43e370
    // True when no drive exists at all.
    virtual bool       DoIsEmpty();                                           // Win 0x43e430
    virtual bool       DoDirectoryExists(const BasicArray<String>& path);     // Win 0x43e440
    // One directory entry per drive letter (upper case) matching `pattern`.
    virtual void       DoEnumerate(const String& pattern, DescriptorSet& entries); // Win 0x43e5a0

    // Slots 31/32 (Mac numbering, Win 30/31): the same always-false stub (Win 0x42e9e0, `ret
    // 0x10`) as RootDirectory::Unknown31/32 — this class does not derive from RootDirectory
    // (RTTI), so the three extra virtuals were declared again. (names inferred)
    virtual bool Unknown31(const void*, const void*, const void*, const void*) { return false; } // Win 0x42e9e0
    virtual bool Unknown32(const void*, const void*, const void*, const void*) { return false; } // Win 0x42e9e0
    // Slot 33 (Win 32): true when `name` is a single letter naming an existing drive.
    // (name inferred)
    virtual bool DoRootExists(const String& name);                            // Win 0x43e550
};

// Root directory "network": child "server" maps to the UNC path "\\server".
// Win sizeof 0x74. Vtable 0xa1fd04 (33 slots): sdd 0x42e9f0 (dtor 0x42e990),
// DoGetDirectory 0x43e750, DoIsEmpty 0x4aede0 (shared `return false`), DoDirectoryExists
// 0x43e800, Unknown31/32 0x42e9e0, DoRootExists 0x43e8d0; DoEnumerate is the empty default.
class Win32NetworkProtocolDirectory : public Directory {
public:
    Win32NetworkProtocolDirectory(const String& name, Directory* parent)    // Win 0x42e930
        : Directory(name, parent)
    {
    }
    virtual ~Win32NetworkProtocolDirectory() {}                               // Win 0x42e990 / sdd 0x42e9f0

    virtual Directory* DoGetDirectory(const String& name);                    // Win 0x43e750
    virtual bool       DoIsEmpty() { return false; }                          // Win 0x4aede0 (ICF)
    virtual bool       DoDirectoryExists(const BasicArray<String>& path);     // Win 0x43e800

    virtual bool Unknown31(const void*, const void*, const void*, const void*) { return false; } // Win 0x42e9e0
    virtual bool Unknown32(const void*, const void*, const void*, const void*) { return false; } // Win 0x42e9e0
    // True when WNetGetResourceInformation knows "\\name" (a disk resource). (name inferred)
    virtual bool DoRootExists(const String& name);                            // Win 0x43e8d0
};

// Registers the "file" and "network" protocol directories as children of the VFS root on its
// first registration with the ObjectFactory (registered by the storage set-up code, Win
// 0x42ffb0 / 0x430090, other TU). Creates no objects itself.
// Win sizeof 0x18. Vtable 0xa1fd8c {sdd 0x42ee60 (dtor 0x42ea70), OnFinalRelease 0x459790,
// AddRef 0x428d60, Release 0x412520, CreateObject 0x4f9670, Initialize 0x42eaf0,
// Shutdown 0x42ee30}.
class Win32StorageFactory : public SpecificObjectFactory {
public:
    Win32StorageFactory() {}                                                  // Win 0x42ea10
    virtual ~Win32StorageFactory() {}                                         // Win 0x42ea70 / sdd 0x42ee60

    // NOTE: always null (shared `xor eax,eax; ret 8` stub Win 0x4f9670) — on Windows disk
    // directories are reached through "file"/"network", never mounted through the factory.
    virtual Object* CreateObject(const ObjectFactoryArgs&) { return 0; }      // Win 0x4f9670
    // Fails when the root already has a "file" or "network" child.
    virtual bool Initialize();                                                // Win 0x42eaf0
    virtual void Shutdown();                                                  // Win 0x42ee30

private:
    RefPointer<Directory> m_fileDirectory;      // Win +0x10 (name inferred)
    RefPointer<Directory> m_networkDirectory;   // Win +0x14 (name inferred)
};

} // namespace LEGO

#endif // _WIN32
