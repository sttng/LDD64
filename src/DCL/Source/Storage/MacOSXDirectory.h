// LEGO Digital Designer — restored source
// LEGO::MacOSXDirectory, MacOSXFile — Mac disk backend of the VFS (NSFileManager / NSFileHandle);
// MacOSXStorageFactory mounts "/" as the root's child "file" ("file/Users/x" = /Users/x). TU Mac
// 0x329570-0x32bdb9 (52 functions). Names inferred, except the category +[NSFileHandle
// (MacOSXStorage) fileHandleAtPath:AccessStyle:] (encoding "@13@0:4@8{AccessStyle=b1b1b1b1}12":
// OpenMode was "AccessStyle"). Vtables (slots in Directory.h / File.h):
//   MacOSXDirectory Mac 0x74c158 (ti 0x74c260), sizeof 0x70
//   MacOSXFile      Mac 0x74c1dc (ti 0x74c270), sizeof 0x28
//   MacOSXStorageFactory Mac 0x74c22c (ti 0x74c250), sizeof 0x14
#pragma once

#if !defined(_WIN32)

#include "LEGO/Types.h"
#include "Directory.h"
#include "File.h"
#include "ObjectFactory.h"

#if defined(__OBJC__)
@class NSFileHandle;
#else
// Opaque in plain C++ translation units (and in HOST-ONLY builds, see MacOSXDirectory.mm).
typedef struct objc_object NSFileHandle;
#endif

namespace LEGO {

// A directory on disk. Mac sizeof 0x70.
class MacOSXDirectory : public Directory {
public:
    // `path` is the absolute disk path (copied).
    MacOSXDirectory(const String& name, Directory* parent, CFStringRef path); // Mac 0x329c60
    virtual ~MacOSXDirectory();                                       // Mac 0x329e50 (body), D1 0x329ee0, D0 0x329ef0

    virtual bool       DoSetAttributes(const Attributes& attributes);  // Mac 0x329d10
    virtual Attributes DoGetAttributes() const;                       // Mac 0x329d30
    virtual bool       DoSetTimes(const Times& times);                // Mac 0x329de0
    virtual Times      DoGetTimes() const;                            // Mac 0x329e00

    virtual Directory* DoGetDirectory(const String& name);            // Mac 0x329f30
    virtual bool       DoCreateDirectory(const String& name);         // Mac 0x329fb0
    virtual File*      DoOpenFile(const String& name, const OpenMode& mode, bool create,
                                  const UInt64* size);                // Mac 0x32a020
    virtual bool       DoDelete(const String& name, bool recursive);  // Mac 0x32a1c0
    virtual bool       DoRename(const String& from, const String& to); // Mac 0x32a5d0
    virtual bool       DoGetChildAttributes(const String& name, Attributes* attributes); // Mac 0x32a670
    virtual bool       DoSetChildAttributes(const String& name, const Attributes& attributes); // Mac 0x32a750
    virtual bool       DoGetChildTimes(const String& name, Times* times); // Mac 0x32a7a0
    virtual bool       DoSetChildTimes(const String& name, const Times& times); // Mac 0x32a7f0
    virtual bool       DoIsEmpty();                                   // Mac 0x32a840
    virtual bool       DoDirectoryExists(const BasicArray<String>& path); // Mac 0x32a890
    virtual bool       DoFileExists(const String& name);              // Mac 0x32a990
    virtual void       DoEnumerate(const String& pattern, DescriptorSet& entries); // Mac 0x32aa10
    // (not in the original) fsync, then rename(2) over `to` (Directory.h).
    virtual bool       DoSupportsReplaceFile() const { return true; }
    virtual bool       DoReplaceFile(const String& from, const String& to);

private:
    MacOSXDirectory(const MacOSXDirectory&);
    MacOSXDirectory& operator=(const MacOSXDirectory&);

    CFStringRef m_path;   // +0x6c (owned copy; an NSString)
};

// An open file on disk. Mac sizeof 0x28.
class MacOSXFile : public File {
public:
    // Retains `handle`; `path` is copied.
    MacOSXFile(const String& name, Directory* parent, const OpenMode& mode, CFStringRef path,
               NSFileHandle* handle);                                 // Mac 0x32ad80
    // Closes and releases the handle.
    virtual ~MacOSXFile();                                            // Mac 0x32b550 (body), D1 0x32b6b0, D0 0x32b6c0

    virtual bool       DoSetAttributes(const Attributes& attributes);  // Mac 0x32aeb0
    virtual Attributes DoGetAttributes() const;                       // Mac 0x32aed0
    virtual bool       DoSetTimes(const Times& times);                // Mac 0x32af80
    virtual Times      DoGetTimes() const;                            // Mac 0x32afa0

    virtual bool   DoRead(UInt64 position, UInt64 size, void* buffer, UInt64* read);           // Mac 0x32aff0
    virtual bool   DoWrite(UInt64 position, UInt64 size, const void* buffer, UInt64* written); // Mac 0x32b030
    virtual bool   DoRead(UInt64 size, void* buffer, UInt64* read);                            // Mac 0x32b070
    virtual bool   DoWrite(UInt64 size, const void* buffer, UInt64* written);                  // Mac 0x32b230
    virtual UInt64 DoSize();                                                                // Mac 0x32b370
    virtual bool   DoSize(UInt64 size);                                                     // Mac 0x32b3b0
    virtual UInt64 DoPosition();                                                            // Mac 0x32b4d0
    virtual bool   DoPosition(UInt64 position);                                                    // Mac 0x32b500

private:
    MacOSXFile(const MacOSXFile&);
    MacOSXFile& operator=(const MacOSXFile&);

    CFStringRef   m_path;     // +0x20 (owned copy)
    NSFileHandle* m_handle;   // +0x24 (retained)
};

// Mounts the disk. Never creates objects itself. Constructed (inline) and registered by the
// application; sizeof 0x14.
class MacOSXStorageFactory : public SpecificObjectFactory {
public:
    MacOSXStorageFactory() {}                                         // inline (not in this TU)
    virtual ~MacOSXStorageFactory() {}                                // Mac D1 0x32b920, D0 0x32b980

    virtual Object* CreateObject(const ObjectFactoryArgs&) { return 0; } // Mac 0x32ba40
    // Fails when the root already has a child "file"; otherwise creates it for "/".
    virtual bool Initialize();                                        // Mac 0x32b700
    virtual void Shutdown();                                          // Mac 0x32b8f0

private:
    RefPointer<Directory> m_disk;   // +0x10 the "file" directory (name inferred)
};

} // namespace LEGO

#endif // !_WIN32
