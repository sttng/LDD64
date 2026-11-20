// LEGO Digital Designer — restored source
// Storage factories registered at start-up (RegisterStorageFactories): LiffStorageFactory mounts
// a .lif archive met while resolving a path (Directory::OpenArgs -> root LiffStorageDirectory);
// MemoryStorageFactory owns the "memory" directory and wraps memory blocks (File::WrapArgs).
// TU Mac 0x18bc0-0x1a24f, Win 0x42e7c0-0x430fdf; header-only factories (weak vtables Mac 0x75fabc
// / 0x75fb24) first emitted there. Names inferred. A path through "Name.lif" (or "Name" when not
// strict) mounts implicitly; explicitly: ObjectFactory::GetInstance()->Create<Directory>(
// Directory::OpenArgs(parent, path, true)).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "ObjectFactory.h"
#include "Directory.h"
#include "File.h"
#include "MemoryDirectory.h"

namespace LEGO {

// Registers the native (MacOSX / Win32), LIFF, Zip and memory storage factories with the shared
// ObjectFactory. Called once from the application start-up (Mac 0x13ad50). (name inferred)
bool RegisterStorageFactories();                                       // Mac 0x18cb0, Win 0x430090

// Mac sizeof 0x10. Mac vtable 0x75fabc {D1 0x19090, D0 0x190f0, Object slots, CreateObject
// 0x19160, Initialize 0x19070, Shutdown 0x19080}, Win 0xa1fe50 {sdd 0x42f700, .., 0x42f410}.
class LiffStorageFactory : public SpecificObjectFactory {
public:
    LiffStorageFactory() {}                                            // Mac inline, Win 0x42f360
    virtual ~LiffStorageFactory() {}                                   // Mac 0x19090/0x190f0, Win 0x42f3c0/0x42f700

    // Directory::OpenArgs: resolves args.m_path below args.m_directory to an archive file
    // (appending ".lif" when the file does not exist and args are not strict), opens it
    // read/write — falling back to read-only when not strict — and mounts it. Returns the new
    // root LiffStorageDirectory (not AddRef'd) or null.
    virtual Object* CreateObject(const ObjectFactoryArgs& args);       // Mac 0x19160, Win 0x42f410
};

// Mac sizeof 0x14, Win 0x14. Mac vtable 0x75fb24 {D1 0x197c0, D0 0x19820, Object slots,
// CreateObject 0x198e0, Initialize 0x19930, Shutdown 0x19ad0}, Win 0xa1ffb0
// {sdd 0x42ff90, .., 0x42ff50, 0x42fd70, 0x42ff30}.
class MemoryStorageFactory : public SpecificObjectFactory {
public:
    MemoryStorageFactory() {}                                          // Mac inline, Win 0x42fca0
    virtual ~MemoryStorageFactory() {}                                 // Mac 0x197c0/0x19820, Win 0x42fd00/0x42ff90

    // File::WrapArgs -> MemoryFile over the caller's block (not AddRef'd), else null.
    virtual Object* CreateObject(const ObjectFactoryArgs& args)        // Mac 0x198e0, Win 0x42ff50
    {
        const File::WrapArgs* wrap = dynamic_cast<const File::WrapArgs*>(&args);
        if (!wrap)
            return 0;
        return m_directory->WrapMemory(wrap->m_data, wrap->m_size);
    }

    // Mounts "memory" below the VFS root; fails if a directory of that name exists.
    virtual bool Initialize()                                          // Mac 0x19930, Win 0x42fd70
    {
        RefPointer<Directory> existing = Directory::GetRoot()->GetDirectory(String("memory"), false);
        if (!existing.IsNull())
            return false;
        m_directory = new MemoryProtocolDirectory(String("memory"), Directory::GetRoot());
        return true;
    }

    virtual void Shutdown()                                            // Mac 0x19ad0, Win 0x42ff30
    {
        m_directory = 0;
    }

    MemoryProtocolDirectory* GetDirectory() const { return m_directory.Get(); } // (name inferred; not seen)

private:
    RefPointer<MemoryProtocolDirectory> m_directory;   // +0x10
};

} // namespace LEGO
