// LEGO Digital Designer — restored source
// TU "LiffStorageFactory.cpp" Mac 0x18bc0-0x1a24f (34 functions), Win 0x42e7c0-0x430fdf: factory
// registration, LiffStorageFactory::CreateObject, LiffStorageVolume ctor/dtor. Names inferred.
//
// Weak / inline code first emitted in this TU and restored in headers instead:
//   ObjectFactory.h       SpecificObjectFactory D2/D1/D0 0x18f40/0x18fa0/0x19000 (Win 0x42e7c0),
//                         Initialize 0x19070, Shutdown 0x19080
//   LiffStorageFactory.h  LiffStorageFactory D1/D0 0x19090/0x190f0; MemoryStorageFactory
//                         D1/D0 0x197c0/0x19820, CreateObject 0x198e0, Initialize 0x19930,
//                         Shutdown 0x19ad0
//   MemoryDirectory.h     MemoryProtocolDirectory ctor 0x19b00, D1/D0 0x19c10/0x19c70;
//                         MemoryDirectory slots 5-8 0x19ce0-0x19d40; MemoryDirectoryNode ctor
//                         0x19d70, D1/D0 0x19f70/0x19f80 (body 0x1a100); its BasicArray /
//                         BasicMap D1/D0 0x19ee0 0x19f10 0x19fc0 0x1a030, map _M_erase 0x1a080
//   LiffChunk.h           LiffFile constructor (inlined into 0x19160)
// Mac 0x1a210: static initialiser (<iostream> only).
#include "LiffStorageFactory.h"
#include "LiffStorageVolume.h"
#include "LiffStorageDirectory.h"
#include "LiffChunk.h"
#include "MonolithTree.h"
#include "MemoryDirectory.h"
#include "Path.h"
#include "ObjectFactory.h"

#if defined(_WIN32)
#include "Win32Directory.h"
#else
#include "MacOSXDirectory.h"
#endif

#include "ZipStorageDirectory.h"

#include <iostream>
#include <typeinfo>

namespace LEGO {

namespace {

// Registers the platform's own file system storage (mounted below the VFS root by the
// factory's Initialize). Mac 0x18bc0, Win 0x42ffb0. (name inferred)
bool RegisterNativeStorageFactory()
{
#if defined(_WIN32)
    RefPointer<SpecificObjectFactory> factory = new Win32StorageFactory;
    return ObjectFactory::GetInstance()->RegisterFactory(typeid(Win32Directory), factory, typeid(Directory));
#else
    RefPointer<ObjectFactory> objectFactory = ObjectFactory::GetInstance();
    RefPointer<SpecificObjectFactory> factory = new MacOSXStorageFactory;
    return objectFactory->RegisterFactory(typeid(MacOSXDirectory), factory, typeid(Directory));
#endif
}

} // namespace

// Mac 0x18cb0, Win 0x430090
bool RegisterStorageFactories()
{
    if (!RegisterNativeStorageFactory())
        return false;

    RefPointer<SpecificObjectFactory> factory = new LiffStorageFactory;
    if (!ObjectFactory::GetInstance()->RegisterFactory(typeid(LiffStorageDirectory), factory, typeid(Directory)))
        return false;

    factory = new ZipStorageFactory;
    if (!ObjectFactory::GetInstance()->RegisterFactory(typeid(ZipStorageDirectory), factory, typeid(Directory)))
        return false;

    factory = new MemoryStorageFactory;
    return ObjectFactory::GetInstance()->RegisterFactory(typeid(MemoryFile), factory, typeid(File));
}

// ---------------------------------------------------------------------------
// LiffStorageFactory
// ---------------------------------------------------------------------------

// Mac 0x19160, Win 0x42f410
Object* LiffStorageFactory::CreateObject(const ObjectFactoryArgs& args)
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
        path.SetName(String(path.GetName()) << ".lif");

    RefPointer<LiffFile> file = new LiffFile;
    if (!file->Open(path, mode, openArgs->m_strict)) {
        if (openArgs->m_strict)
            return 0;
        // Retry read-only (sharing reads).
        mode.m_write = false;
        mode.m_shareRead = true;
        if (!file->Open(path, mode, false))
            return 0;
    }

    RefPointer<LiffStorageVolume> volume = new LiffStorageVolume(file);
    RefPointer<Directory> parent = file->GetFile()->GetParent();
    if (!volume->Mount(openArgs->m_strict, openArgs->m_path.End()[-1], parent))
        return 0;
    // The root directory keeps the volume alive (LiffStorageDirectory::m_volume).
    return volume->GetRoot();
}

// ---------------------------------------------------------------------------
// LiffStorageVolume
// ---------------------------------------------------------------------------

// Mac inline in 0x19160, Win 0x42f210
LiffStorageVolume::LiffStorageVolume(LiffFile* file)
    : m_version(1), m_flags(0), m_root(0), m_tree(0), m_rootChunk(), m_file(file)
{
}

// Mac 0x196f0 (D1 0x196a0, D0 0x196b0), Win 0x42f2a0 (sdd 0x42f340)
LiffStorageVolume::~LiffStorageVolume()
{
    delete m_tree;
}

} // namespace LEGO
