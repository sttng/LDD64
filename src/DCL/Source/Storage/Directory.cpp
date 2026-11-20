// LEGO Digital Designer — restored source
// TU "Directory.cpp" Mac 0x12530-0x18bbf (88 functions), Win 0x424d30-0x42a30f (+ COMDATs): VFS
// base layer (DirectoryElement, File wrappers, Directory, RootDirectory + instantiator, OpenArgs,
// the Path members on directories). Names inferred.
// Templates emitted here (not written out): Mac 0x18360 BasicArray<String>::operator=, 0x18520
//   Insert, 0x187f0/0x188a0 ChildMap insert, 0x18960 ChildMap::Erase (Win 0x42a2b0), 0x15420/0x182e0
//   OpenArgs D1/D0. Static init Mac 0x18aa0: the common 24-byte header static, s_wildcardAll ("*",
//   Mac 0x77d060 / Win 0xb42e98) and the storage mutex (Mac 0x77d070 / Win 0xb42e70).
#include "Directory.h"
#include "Path.h"
#include "ObjectFactory.h"
#include "ObjectRegistry.h"

#include <iostream>
#include <typeinfo>

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace LEGO {

namespace {

// Process-wide recursive lock of the storage layer (name inferred). Plain static object:
// Mac pthread_mutex_t at 0x77d070 (recursive attr, destroyed at exit by 0x12930),
// Win CRITICAL_SECTION at 0xb42e70. Lock/Unlock are inlined everywhere.
class StorageMutex {
public:
    StorageMutex()
    {
#if defined(_WIN32)
        InitializeCriticalSection(&m_lock);
#else
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&m_lock, &attr);
        pthread_mutexattr_destroy(&attr);
#endif
    }
    ~StorageMutex()                                                   // Mac 0x12930
    {
#if defined(_WIN32)
        DeleteCriticalSection(&m_lock);
#else
        pthread_mutex_destroy(&m_lock);
#endif
    }
    void Lock()
    {
#if defined(_WIN32)
        EnterCriticalSection(&m_lock);
#else
        pthread_mutex_lock(&m_lock);
#endif
    }
    void Unlock()
    {
#if defined(_WIN32)
        LeaveCriticalSection(&m_lock);
#else
        pthread_mutex_unlock(&m_lock);
#endif
    }

private:
#if defined(_WIN32)
    CRITICAL_SECTION m_lock;
#else
    pthread_mutex_t  m_lock;
#endif
};

// Mac 0x77d060 / Win 0xb42e98
const String s_wildcardAll("*");
// Mac 0x77d070 / Win 0xb42e70
StorageMutex s_storageMutex;

class StorageLocker {                                                 // (name inferred)
public:
    StorageLocker()  { s_storageMutex.Lock(); }
    ~StorageLocker() { s_storageMutex.Unlock(); }
private:
    StorageLocker(const StorageLocker&);
    StorageLocker& operator=(const StorageLocker&);
};

} // namespace

// ---------------------------------------------------------------------------
// RootDirectory / RootDirectoryInstantiator
// ---------------------------------------------------------------------------
// Both classes are fully inline (their vtables and typeinfos are weak: Mac RootDirectory
// vtable 0x75f9b4 / ti 0x75fa40, instantiator vtable 0x75f988 / ti 0x75f9a0) and only used here,
// so they are kept in this file. Other modules reach the root through Directory::GetRoot().

// The unnamed root of the virtual file system; storages register themselves as its children.
// Mac sizeof 0x6c, Win 0x74. Win vtable 0xa1f83c.
class RootDirectory : public Directory {
public:
    RootDirectory() : Directory(String(), 0) {}                       // Mac inlined in 0x18000, Win 0x424f90
    virtual ~RootDirectory() {}                                       // Mac 0x180a0/0x180b0, Win 0x425010/0x425210

    virtual bool DoRename(const String&, const String&) { return false; } // Mac 0x180f0, Win 0x4a9530

    // Slots 31 and 32: always false in every build; purpose and parameter types unknown
    // (Win pops four dwords: `ret 0x10`). (names inferred)
    virtual bool Unknown31(const void*, const void*, const void*, const void*) { return false; } // Mac 0x18100, Win 0x42e9e0
    virtual bool Unknown32(const void*, const void*, const void*, const void*) { return false; } // Mac 0x18110, Win 0x42e9e0

    // Slot 33: "<child>/<rest>" -> the live child directory <child>, then rest resolved in it.
    // (name inferred)
    virtual RefPointer<Directory> GetMountedDirectory(const String& path, bool create) // Mac 0x18120, Win 0x425060
    {
        UInt32 slash = path.Find((UniChar16)'/');
        String volume = path.Substring(0, slash);
        ChildMap::Iterator it = m_children.Find(volume);
        if (it == m_children.End())
            return RefPointer<Directory>();
        RefPointer<Directory> directory = it->second;
        if (slash == (UInt32)String::npos)
            return directory;
        return directory->GetDirectory(path.Substring(slash + 1, String::npos), create);
    }
};

// Creates the root for ObjectRegistry id 'STOR'. Mac vtable 0x75f988 {D1 0x12a70, D0 0x17ff0,
// CreateObject 0x18000}; Win 0xa1f8c4 {sdd 0x425960, CreateObject 0x4258c0}.
class RootDirectoryInstantiator : public ObjectRegistryInstantiator {
public:
    RootDirectoryInstantiator() {}
    virtual ~RootDirectoryInstantiator() {}                           // Mac 0x12a70 (D1) / 0x17ff0 (D0), Win 0x425870/0x425960
    virtual RefPointer<Object> CreateObject() const                   // Mac 0x18000, Win 0x4258c0
    {
        return new RootDirectory;
    }
};

// ---------------------------------------------------------------------------
// DirectoryElement
// ---------------------------------------------------------------------------

// Mac 0x128a0, Win 0x4257a0
DirectoryElement::DirectoryElement(const String& name, Directory* parent)
    : m_name(name), m_parent(parent)
{
}

// Mac 0x12940 (tail-calls DoSetAttributes), Win 0x425830
bool DirectoryElement::DoSetInfo(const Attributes& attributes, const Times& times)
{
    if (!DoSetTimes(times))
        return false;
    return DoSetAttributes(attributes);
}

// Mac 0x126c0, Win 0x425470
Attributes DirectoryElement::GetAttributes() const
{
    StorageLocker lock;
    return DoGetAttributes();
}

// Mac: inlined only (out-of-line copy dead-stripped), Win 0x4254f0
Times DirectoryElement::GetTimes() const
{
    StorageLocker lock;
    return DoGetTimes();
}

// Mac: inlined only, Win 0x425560
bool DirectoryElement::SetInfo(const Attributes& attributes, const Times& times)
{
    StorageLocker lock;
    return DoSetInfo(attributes, times);
}

// Mac 0x12720, Win 0x4255e0
String DirectoryElement::GetPath(bool full) const
{
    StorageLocker lock;
    if (!full)
        return m_name;
    String path;
    if (!m_parent.IsNull()) {
        path = m_parent->GetPath(true);
        if (!path.IsEmpty())
            path << "/";
        path << m_name;
    }
    return path;
}

// Mac 0x12830, Win 0x425700
RefPointer<Directory> DirectoryElement::GetParent() const
{
    StorageLocker lock;
    return m_parent;
}

// ---------------------------------------------------------------------------
// File — locked wrappers
// ---------------------------------------------------------------------------

// Mac 0x16690, Win 0x428e00
bool File::ReadAt(UInt64 position, UInt64 size, void* buffer, UInt64* read)
{
    StorageLocker lock;
    return DoRead(position, size, buffer, read);
}

// Mac 0x16720, Win 0x428ea0
bool File::WriteAt(UInt64 position, UInt64 size, const void* buffer, UInt64* written)
{
    StorageLocker lock;
    return DoWrite(position, size, buffer, written);
}

// Mac 0x16c50, Win 0x428f30
bool File::Read(UInt64 size, void* buffer, UInt64* read)
{
    StorageLocker lock;
    return DoRead(size, buffer, read);
}

// Mac 0x16cd0, Win 0x428fc0
bool File::Write(UInt64 size, const void* buffer, UInt64* written)
{
    StorageLocker lock;
    return DoWrite(size, buffer, written);
}

// Mac 0x16620, Win 0x4292d0
UInt64 File::GetSize()
{
    StorageLocker lock;
    return DoSize();
}

// Mac 0x17140, Win 0x429350
bool File::SetSize(UInt64 size)
{
    StorageLocker lock;
    return DoSize(size);
}

// Mac 0x171b0, Win 0x4293e0
UInt64 File::GetPosition()
{
    StorageLocker lock;
    return DoPosition();
}

// Mac 0x17220, Win 0x429460
bool File::Seek(UInt64 position)
{
    StorageLocker lock;
    return DoPosition(position);
}

// Mac 0x16d50, Win 0x429050. 128 KiB buffer; copies back to front when moving data up.
bool File::CopyRange(UInt64 from, UInt64 to, UInt64 size)
{
    StorageLocker lock;
    const UInt64 kChunk = 0x20000;
    UInt64 chunk = size < kChunk ? size : kChunk;
    UInt8* buffer = new UInt8[(UInt32)chunk];
    UInt64 source = from;
    UInt64 target = to;
    if (from < to) {
        source = from + size - chunk;
        target = to + size - chunk;
    }
    if (GetSize() < to + size && !SetSize(to + size)) {
        delete[] buffer;
        return false;
    }
    if (chunk != 0) {
        if (from < to) {
            do {
                if (!ReadAt(source, chunk, buffer, 0) || !WriteAt(target, chunk, buffer, 0)) {
                    delete[] buffer;
                    return false;
                }
                size -= chunk;
                chunk = size < kChunk ? size : kChunk;
                source -= chunk;
                target -= chunk;
            } while (chunk != 0);
        } else {
            do {
                if (!ReadAt(source, chunk, buffer, 0) || !WriteAt(target, chunk, buffer, 0)) {
                    delete[] buffer;
                    return false;
                }
                size -= chunk;
                source += chunk;
                target += chunk;
                chunk = size < kChunk ? size : kChunk;
            } while (chunk != 0);
        }
    }
    delete[] buffer;
    return true;
}

// ---------------------------------------------------------------------------
// Directory — construction, reference counting
// ---------------------------------------------------------------------------

// Mac 0x16950, Win 0x428c30
Directory::Directory(const String& name, Directory* parent)
    : DirectoryElement(name, parent), m_enumerating(false)
{
    if (!m_parent.IsNull())
        m_parent->m_children.Insert(m_name, this);
}

// Mac 0x16b80, Win 0x428d20
// NOTE: erases by name only — an entry of the same name that belongs to another directory
// object would be removed as well.
void Directory::OnFinalRelease()
{
    if (!m_parent.IsNull()) {
        for (BasicArray<String>::Iterator it = m_aliases.Begin(); it != m_aliases.End(); ++it)
            m_parent->m_children.Erase(*it);
        m_parent->m_children.Erase(m_name);
    }
    DirectoryElement::OnFinalRelease();
}

// Mac 0x16be0 (Win: folded into Object::AddRef 0x428d60)
void Directory::AddRef() const
{
    DirectoryElement::AddRef();
}

// Mac 0x16bf0, Win 0x428d70 — the final release edits the parent's child map under the lock.
void Directory::Release() const
{
    StorageLocker lock;
    DirectoryElement::Release();
}

// (not in the original: File kept Object::Release, see File.h) The final release of a
// file (ZipStorageFile::OnFinalRelease flushes into the archive) runs under the storage lock.
void File::Release() const
{
    StorageLocker lock;
    DirectoryElement::Release();
}

// Mac 0x12970, Win 0x4259d0
RefPointer<Directory> Directory::GetRoot()
{
    StorageLocker lock;
    RefPointer<Object> object = ObjectRegistry::GetInstance()->Get(kRootRegistryId, RootDirectoryInstantiator());
    return dynamic_cast<Directory*>(object.Get());
}

// ---------------------------------------------------------------------------
// Directory — pure helpers
// ---------------------------------------------------------------------------

// Mac 0x12530, Win 0x425230
// NOTE: the text after a '*' is searched with its first case-insensitive occurrence only (no
// backtracking): MatchWildcard("aXbXc", "*X*c") succeeds, but MatchWildcard("ab", "*b*b") and
// MatchWildcard("abab", "*ab") fail. A trailing '*' matches everything that remains.
bool Directory::MatchWildcard(const String& name, const String& pattern)
{
    UInt32 namePos = 0;
    UInt32 patternLength = pattern.GetLength();
    if (patternLength != 0) {
        UInt32 patternPos = 0;
        bool afterStar = false;
        do {
            UInt32 star = pattern.Find((UniChar16)'*', patternPos);
            UInt32 end = star == (UInt32)String::npos ? patternLength : star;
            UInt32 length = end - patternPos;
            if (!afterStar) {
                if (star == (UInt32)String::npos)
                    return name.EqualsNoCase(namePos, String::npos, pattern, patternPos, String::npos);
                if (!name.EqualsNoCase(namePos, length, pattern, patternPos, length))
                    return false;
                afterStar = true;
                patternPos = end + 1;
            } else {
                namePos = name.FindNoCase(pattern.Substring(patternPos, length), namePos);
                if (namePos == (UInt32)String::npos)
                    return false;
                afterStar = false;
                patternPos = end;
            }
            namePos += length;
        } while (patternPos < patternLength);
        if (afterStar)
            return true;
    }
    return namePos == name.GetLength();
}

// Mac 0x12a80, Win 0x425ad0
void Directory::NormalizePath(BasicArray<String>& path)
{
    if (path.IsEmpty())
        return;
    UInt32 removed = 0;
    for (UInt32 i = 0; i < path.GetSize(); ++i) {
        if (path[i] == String(".")) {
            ++removed;
            continue;
        }
        if (path[i] == String("..") && removed < i && path[i - removed - 1] != String("..")) {
            removed += 2;
            continue;
        }
        if (removed != 0)
            path[i - removed] = path[i];
    }
    for (; removed != 0; --removed)
        path.PopBack();
}

// ---------------------------------------------------------------------------
// Directory — path resolution
// ---------------------------------------------------------------------------

// Mac 0x15280, Win 0x427ed0 (recursive)
RefPointer<Directory> Directory::FindLoadedDirectory(Directory* directory, BasicArray<String>& path)
{
    if (!path.IsEmpty()) {
        Directory* next = 0;
        if (path[0] == String("..")) {
            next = directory->m_parent.Get();
            if (!next)
                return RefPointer<Directory>();
        } else {
            ChildMap::Iterator it = directory->m_children.Find(path[0]);
            if (it != directory->m_children.End())
                next = it->second;
        }
        if (next) {
            path.Erase(0);
            return FindLoadedDirectory(next, path);
        }
    }
    return directory;
}

// Mac 0x12fd0, Win 0x427220
bool Directory::DirectoryExists(BasicArray<String>& path, RefPointer<Directory>* result)
{
    StorageLocker lock;
    NormalizePath(path);
    RefPointer<Directory> directory = FindLoadedDirectory(this, path);
    if (directory.IsNull()) {
        if (result)
            *result = 0;
        return false;
    }
    if (path.IsEmpty()) {
        if (result)
            *result = directory;
        return true;
    }

    BasicArray<String> rest;    // components that could not be resolved yet
    for (;;) {
        if (directory->DoDirectoryExists(path)) {
            for (UInt32 i = 0; i < path.GetSize(); ++i)
                directory = directory->DoGetDirectory(path[i]);
            path = rest;
            rest.Clear();
            if (path.IsEmpty()) {
                if (result)
                    *result = directory;
                return true;
            }
        }

        // Try to mount a storage (archive) found at `path`.
        RefPointer<Directory> mounted;
        {
            OpenArgs args(directory, path, false);
            RefPointer<ObjectFactory> factory = ObjectFactory::GetInstance();
            mounted = dynamic_cast<Directory*>(factory->CreateObject(typeid(Directory), args));
        }
        if (mounted.IsNull()) {
            rest.Insert(rest.Begin(), 1, path.Back());
            path.PopBack();
            if (path.IsEmpty()) {
                if (result)
                    *result = directory;
                path = rest;
                return false;
            }
        } else {
            directory = mounted;
            path = rest;
            rest.Clear();
            if (path.IsEmpty()) {
                if (result)
                    *result = directory;
                return true;
            }
        }
    }
}

// Mac 0x15190, Win 0x427190
bool Directory::DirectoryExists(const String& path, RefPointer<Directory>* directory)
{
    BasicArray<String> components;
    path.Split((UniChar16)'/', components);
    return DirectoryExists(components, directory);
}

// Mac 0x12c80, Win 0x425d50
RefPointer<Directory> Directory::GetDirectory(const String& path, bool create)
{
    BasicArray<String> components;
    path.Split((UniChar16)'/', components);
    return GetDirectory(components, create);
}

// Mac 0x12d80, Win 0x425df0
// NOTE: when the path starts with ".." above the root, DirectoryExists yields no directory and
// a `create` request dereferences null (as in both builds).
RefPointer<Directory> Directory::GetDirectory(const BasicArray<String>& path, bool create)
{
    StorageLocker lock;
    BasicArray<String> rest(path);
    RefPointer<Directory> directory;
    if (DirectoryExists(rest, &directory))
        return directory;
    if (!create)
        return RefPointer<Directory>();
    for (UInt32 i = 0; i < rest.GetSize(); ++i) {
        if (!directory->DoCreateDirectory(rest[i]))
            return RefPointer<Directory>();
        directory = directory->DoGetDirectory(rest[i]);
    }
    return directory;
}

// Mac 0x13ad0, Win 0x428020
bool Directory::ResolveParent(BasicArray<String>& path, String& leaf, RefPointer<Directory>& directory,
                              bool create)
{
    NormalizePath(path);
    if (path.IsEmpty())
        return false;
    leaf = path.Back();
    if (leaf == String(".."))
        return false;
    path.PopBack();
    directory = GetDirectory(path, create);
    return !directory.IsNull();
}

// ---------------------------------------------------------------------------
// Directory — path-based operations
// ---------------------------------------------------------------------------

// Mac 0x137a0, Win 0x425ff0
RefPointer<File> Directory::OpenFile(const String& path, const OpenMode& mode, bool create, const UInt64* size)
{
    BasicArray<String> components;
    path.Split((UniChar16)'/', components);
    return OpenFile(components, mode, create, size);
}

// Mac 0x138c0, Win 0x4260a0
RefPointer<File> Directory::OpenFile(const BasicArray<String>& path, const OpenMode& mode, bool create,
                                     const UInt64* size)
{
    StorageLocker lock;
    if (size && !mode.m_write)
        return RefPointer<File>();
    BasicArray<String> components(path);
    String leaf;
    RefPointer<Directory> directory;
    if (!ResolveParent(components, leaf, directory, create))
        return RefPointer<File>();
    return directory->DoOpenFile(leaf, mode, create, size);
}

// Mac 0x13bf0, Win 0x427920
bool Directory::Exists(const String& path)
{
    BasicArray<String> components;
    path.Split((UniChar16)'/', components);
    return Exists(components);
}

// Mac 0x15740, Win 0x4279b0
bool Directory::Exists(const BasicArray<String>& path)
{
    StorageLocker lock;
    BasicArray<String> components(path);
    String leaf;
    RefPointer<Directory> directory;
    if (!ResolveParent(components, leaf, directory, false))
        return false;
    if (directory->m_children.Find(leaf) != directory->m_children.End())
        return true;
    if (directory->FileExists(leaf))
        return true;
    return directory->DirectoryExists(leaf, 0);
}

// Mac 0x15490, Win 0x427700
bool Directory::FileExists(const String& path)
{
    BasicArray<String> components;
    path.Split((UniChar16)'/', components);
    return FileExists(components);
}

// Mac 0x15580, Win 0x427790
bool Directory::FileExists(const BasicArray<String>& path)
{
    StorageLocker lock;
    BasicArray<String> components(path);
    String leaf;
    RefPointer<Directory> directory;
    if (!ResolveParent(components, leaf, directory, false))
        return false;
    return directory->DoFileExists(leaf);
}

// Mac 0x13d90, Win 0x4262c0
bool Directory::Delete(const String& path, bool recursive)
{
    BasicArray<String> components;
    path.Split((UniChar16)'/', components);
    return Delete(components, recursive);
}

// Mac 0x13e90, Win 0x426350 — a directory that is still alive (in its parent's child map)
// cannot be deleted.
bool Directory::Delete(const BasicArray<String>& path, bool recursive)
{
    StorageLocker lock;
    BasicArray<String> components(path);
    String leaf;
    RefPointer<Directory> directory;
    if (!ResolveParent(components, leaf, directory, false))
        return false;
    if (directory->m_children.Find(leaf) != directory->m_children.End())
        return false;
    return directory->DoDelete(leaf, recursive);
}

// Mac 0x140e0, Win 0x426530
bool Directory::Copy(const String& sourcePath, Directory& destination, const String& destinationPath,
                     bool overwrite, bool force)
{
    BasicArray<String> source;
    BasicArray<String> target;
    sourcePath.Split((UniChar16)'/', source);
    destinationPath.Split((UniChar16)'/', target);
    return Copy(source, destination, target, overwrite, force);
}

// Mac 0x142a0, Win 0x426610
// An empty source path copies this directory itself; an empty destination path keeps the
// source name. Copying an element onto itself succeeds without doing anything; copying into a
// descendant of the source fails.
bool Directory::Copy(const BasicArray<String>& sourcePath, Directory& destination,
                     const BasicArray<String>& destinationPath, bool overwrite, bool force)
{
    StorageLocker lock;
    String sourceName;
    String destinationName;
    RefPointer<Directory> sourceDirectory;
    if (sourcePath.IsEmpty()) {
        sourceName = GetPath(false);
        sourceDirectory = GetParent();
    } else {
        BasicArray<String> components(sourcePath);
        sourceName = components.Back();
        components.PopBack();
        if (!DirectoryExists(components, &sourceDirectory))
            return false;
    }
    if (sourceDirectory.IsNull())
        return false;

    RefPointer<Directory> destinationDirectory;
    if (destinationPath.IsEmpty()) {
        destinationName = sourceName;
        destinationDirectory = &destination;
    } else {
        BasicArray<String> components(destinationPath);
        destinationName = components.Back();
        components.PopBack();
        destinationDirectory = destination.GetDirectory(components, true);
        if (destinationDirectory.IsNull())
            return false;
    }

    if (sourceDirectory.Get() == destinationDirectory.Get() && sourceName == destinationName)
        return true;

    // Walk up from the destination; reaching the source directory through an ancestor named
    // like the source means the target lies inside the source.
    String name(destinationName);
    RefPointer<Directory> ancestor = destinationDirectory;
    while (!ancestor.IsNull() && ancestor.Get() != sourceDirectory.Get()) {
        name = ancestor->GetPath(false);
        ancestor = ancestor->GetParent();
    }
    if (ancestor.Get() == sourceDirectory.Get() && sourceName.EqualsNoCase(name))
        return false;

    if (sourceName != s_wildcardAll)
        return sourceDirectory->DoCopy(sourceName, destinationDirectory, destinationName, overwrite, force);

    if (destinationName != s_wildcardAll) {
        destinationDirectory = destinationDirectory->GetDirectory(destinationName, true);
        if (destinationDirectory.IsNull())
            return false;
    }
    DescriptorSet entries;
    sourceDirectory->DoEnumerate(s_wildcardAll, entries);
    for (DescriptorSet::Iterator it = entries.Begin(); it != entries.End(); ++it) {
        if (!sourceDirectory->DoCopy(it->m_name, destinationDirectory, it->m_name, overwrite, force))
            return false;
    }
    return true;
}

// Mac 0x14c10, Win 0x426d70
bool Directory::SetAttributes(const String& path, const Attributes& attributes)
{
    BasicArray<String> components;
    path.Split((UniChar16)'/', components);
    return SetAttributes(components, attributes);
}

// Mac 0x14d00, Win 0x426e00
bool Directory::SetAttributes(const BasicArray<String>& path, const Attributes& attributes)
{
    StorageLocker lock;
    BasicArray<String> components(path);
    String leaf;
    RefPointer<Directory> directory;
    if (!ResolveParent(components, leaf, directory, false))
        return false;
    return directory->DoSetChildAttributes(leaf, attributes);
}

// Mac 0x14ed0, Win 0x426f80
bool Directory::GetAttributes(const String& path, Attributes* attributes)
{
    BasicArray<String> components;
    path.Split((UniChar16)'/', components);
    return GetAttributes(components, attributes);
}

// Mac 0x14fc0, Win 0x427010
bool Directory::GetAttributes(const BasicArray<String>& path, Attributes* attributes)
{
    StorageLocker lock;
    BasicArray<String> components(path);
    String leaf;
    RefPointer<Directory> directory;
    if (!ResolveParent(components, leaf, directory, false))
        return false;
    return directory->DoGetChildAttributes(leaf, attributes);
}

// Mac 0x159a0, Win 0x427b80
void Directory::Enumerate(const String& pattern, DescriptorSet& entries)
{
    StorageLocker lock;
    BasicArray<String> components;
    pattern.Split((UniChar16)'/', components);
    String leaf;
    RefPointer<Directory> directory;
    if (!ResolveParent(components, leaf, directory, false))
        return;
    for (ChildMap::Iterator it = directory->m_children.Begin(); it != directory->m_children.End(); ++it) {
        if (MatchWildcard(it->first, leaf)) {
            String name(it->first);
            Directory* child = it->second;
            // NOTE: argument evaluation order differs (Win calls GetTimes first).
            entries.Insert(Descriptor(name, child->GetAttributes(), child->GetTimes(), 0));
        }
    }
    directory->DoEnumerate(leaf, entries);
}

// Mac 0x15d00, Win 0x428150 (no lock taken)
bool Directory::AddAlias(const String& alias)
{
    if (m_parent.IsNull())
        return false;
    if (m_parent->m_children.Find(alias) != m_parent->m_children.End())
        return false;
    m_aliases.PushBack(alias);
    m_parent->m_children.Insert(alias, this);
    return true;
}

// ---------------------------------------------------------------------------
// Directory — enumeration
// ---------------------------------------------------------------------------

// Mac 0x13ce0 (tail-calls the slot with the lock held), Win 0x4261f0
void Directory::BeginEnumeration()
{
    s_storageMutex.Lock();
    DoBeginEnumeration();
}

// Mac 0x13d10, Win 0x426210
RefPointer<File> Directory::OpenCurrent(const OpenMode& mode, const UInt64* size)
{
    return DoOpenCurrent(mode, size);
}

// Mac 0x13d40, Win 0x426280
void Directory::MoveNext()
{
    DoMoveNext();
}

// Mac 0x13d50, Win 0x426290
bool Directory::IsEnumerationDone()
{
    return DoIsEnumerationDone();
}

// Mac 0x13d60, Win 0x4262a0
void Directory::EndEnumeration()
{
    DoEndEnumeration();
    s_storageMutex.Unlock();
}

// Mac 0x167b0, Win 0x4289d0
void Directory::DoBeginEnumeration()
{
    Enumerate(String("*"), m_enumeration);
    m_enumerating = true;
    m_enumerationIterator = m_enumeration.Begin();
    while (!DoIsEnumerationDone() && m_enumerationIterator->m_attributes.m_isDirectory)
        ++m_enumerationIterator;
}

// Mac 0x16850, Win 0x428ab0
RefPointer<File> Directory::DoOpenCurrent(const OpenMode& mode, const UInt64* size)
{
    return DoOpenFile(m_enumerationIterator->m_name, mode, false, size);
}

// Mac 0x168a0, Win 0x428b60
void Directory::DoMoveNext()
{
    do {
        ++m_enumerationIterator;
    } while (!DoIsEnumerationDone() && m_enumerationIterator->m_attributes.m_isDirectory);
}

// Mac 0x168e0, Win 0x428bd0
bool Directory::DoIsEnumerationDone()
{
    return m_enumerationIterator == m_enumeration.End();
}

// Mac 0x16900, Win 0x4df3c0
bool Directory::DoIsEnumerating()
{
    return m_enumerating;
}

// Mac 0x16910, Win 0x428c00
void Directory::DoEndEnumeration()
{
    m_enumerating = false;
    m_enumeration.Clear();
}

// ---------------------------------------------------------------------------
// Directory — copy / move between storages
// ---------------------------------------------------------------------------

// Mac 0x15df0, Win 0x428290
bool Directory::DoMove(const String& name, Directory* destination, const String& destinationName,
                       bool overwrite, bool force)
{
    if (!DoCopy(name, destination, destinationName, overwrite, force))
        return false;
    return DoDelete(name, true);
}

// Mac 0x15e60, Win 0x4282d0
bool Directory::DoCopy(const String& name, Directory* destination, const String& destinationName,
                       bool overwrite, bool force)
{
    RefPointer<Directory> sourceDirectory;
    RefPointer<File> source = OpenFile(name, OpenMode(true, false, true, false), false, 0);
    if (source.IsNull()) {
        // Directory: copy every entry, then the directory's attributes and times.
        if (!DirectoryExists(name, &sourceDirectory))
            return false;
        RefPointer<Directory> target = destination->GetDirectory(destinationName, true);
        if (target.IsNull())
            return false;
        DescriptorSet entries;
        sourceDirectory->Enumerate(String("*"), entries);
        for (DescriptorSet::Iterator it = entries.Begin(); it != entries.End(); ++it) {
            if (!sourceDirectory->DoCopy(it->m_name, target, it->m_name, overwrite, force))
                return false;
        }
        return target->SetInfo(sourceDirectory->GetAttributes(), sourceDirectory->GetTimes());
    }

    if (!overwrite && destination->FileExists(destinationName))
        return false;

    UInt64 size = source->GetSize();
    OpenMode writeMode(true, true, true, false);
    RefPointer<File> target = destination->OpenFile(destinationName, writeMode, true, &size);
    if (target.IsNull()) {
        // Make a read-only target writable and retry.
        if (!force || !destination->FileExists(destinationName))
            return false;
        Attributes attributes;
        if (!destination->GetAttributes(destinationName, &attributes) || attributes.m_writable)
            return false;
        attributes.m_writable = true;
        if (!destination->SetAttributes(destinationName, attributes))
            return false;
        target = destination->OpenFile(destinationName, writeMode, true, &size);
        if (target.IsNull()) {
            attributes.m_writable = false;
            destination->SetAttributes(destinationName, attributes);
            return false;
        }
    }

    if (size != 0) {
        const UInt64 kChunk = 0x100000;
        UInt64 chunk = size < kChunk ? size : kChunk;
        UInt8* buffer = new UInt8[(UInt32)chunk];
        for (UInt64 position = 0; position < size; position += chunk) {
            if (size - position < chunk)
                chunk = size - position;
            if (!source->ReadAt(position, chunk, buffer, 0) || !target->WriteAt(position, chunk, buffer, 0)) {
                delete[] buffer;
                return false;
            }
        }
        delete[] buffer;
    }
    return target->SetInfo(source->GetAttributes(), source->GetTimes());
}

// ---------------------------------------------------------------------------
// Path — members defined in this TU
// ---------------------------------------------------------------------------

// Mac 0x17290, Win 0x4294f0
RefPointer<File> Path::OpenFile(const OpenMode& mode, bool create) const
{
    if (m_directory.IsNull())
        return RefPointer<File>();
    return m_directory->OpenFile(m_name, mode, create, 0);
}

// Mac 0x172f0, Win 0x429590
RefPointer<Directory> Path::GetDirectory(bool create) const
{
    if (m_directory.IsNull())
        return RefPointer<Directory>();
    return m_directory->GetDirectory(m_name, create);
}

// Mac 0x17340, Win 0x429620
bool Path::Delete(bool recursive) const
{
    if (m_directory.IsNull())
        return false;
    return m_directory->Delete(m_name, recursive);
}

// Mac 0x17370, Win 0x429640
bool Path::FileExists() const
{
    if (m_directory.IsNull())
        return false;
    return m_directory->FileExists(m_name);
}

// Mac 0x173a0, Win 0x429660
bool Path::Exists() const
{
    if (m_directory.IsNull())
        return false;
    return m_directory->Exists(m_name);
}

// Mac 0x173d0, Win 0x429680
bool Path::Append(const String& relativePath)
{
    BasicArray<String> components;
    relativePath.Split((UniChar16)'/', components);
    return Append(components);
}

// Mac 0x174c0, Win 0x429710
bool Path::Append(const BasicArray<String>& path)
{
    BasicArray<String> components(path);
    if (!m_name.IsEmpty())
        components.Insert(components.Begin(), 1, m_name);
    String leaf;
    Directory::NormalizePath(components);
    if (components.IsEmpty()) {
        // Everything cancelled out: the path now names its own directory.
        if (m_directory.IsNull()) {
            m_name.Clear();
            return true;
        }
        m_name = m_directory->GetPath(false);
        m_directory = m_directory->GetParent();
        return true;
    }

    leaf = components.Back();
    if (leaf == String("..")) {
        // Only leading ".." remain: resolve them from the current directory.
        if (!m_directory.IsNull()) {
            RefPointer<Directory> directory = m_directory->GetDirectory(components, false);
            if (!directory.IsNull()) {
                m_name = directory->GetPath(false);
                m_directory = directory->GetParent();
                return true;
            }
        }
        return false;
    }

    components.PopBack();
    if (components.IsEmpty()) {
        m_name = leaf;
        if (m_directory.IsNull())
            m_directory = Directory::GetRoot();
        return true;
    }

    RefPointer<Directory> base = m_directory;
    if (base.IsNull())
        base = Directory::GetRoot();
    RefPointer<Directory> directory = base->GetDirectory(components, false);
    if (directory.IsNull())
        return false;
    m_name = leaf;
    m_directory = directory;
    return true;
}

// Mac 0x17b40
bool Path::ToParent()
{
    if (m_directory.IsNull()) {
        if (m_name.IsEmpty())
            return false;
        m_name.Clear();
        return true;
    }
    m_name = m_directory->GetPath(false);
    m_directory = m_directory->GetParent();
    return true;
}

// Mac 0x17c70
void Path::SetElement(const DirectoryElement* element)
{
    m_name = element->GetPath(false);
    m_directory = element->GetParent();
}

// Mac 0x17d80
void Path::Assign(const Path& other)
{
    m_name = other.m_name;
    m_directory = other.m_directory;
}

// (not in the original) see Directory.h
bool Directory::ReplaceFile(const String& from, const String& to)
{
    StorageLocker lock;
    Attributes attributes(false, true, true);      // kept when the permissions cannot be read
    if (DoGetChildAttributes(to, &attributes) && !attributes.m_writable)
        return false;
    return DoReplaceFile(from, to);
}

} // namespace LEGO
