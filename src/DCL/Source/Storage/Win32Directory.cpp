// LEGO Digital Designer — restored source
// TU "Win32Directory.cpp" (name inferred), Windows only (Mac: MacOSXDirectory.mm, TU 345). Win
// 0x43c9a0-0x43e9ef: helpers, directory, file, protocol directories; Win 0x42e860-0x42ee7f:
// protocol directory ctors/dtors and Win32StorageFactory. Names inferred.
// Also emitted (not written out): Win 0x43ea00 / 0x43ea60 / 0x43ebc0 std::list<String> ctor /
// dtor / _Buynode (DoDelete's sub-directory list).
#if defined(_WIN32)

#include "Win32Directory.h"

#include <windows.h>
#include <winnetwk.h>
#include <wctype.h>
#include <list>

#if defined(_MSC_VER) || defined(__clang__)
#pragma comment(lib, "mpr")   // WNetGetResourceInformationW (the original imports MPR.dll)
#endif

namespace LEGO {

namespace {

// ---------------------------------------------------------------------------
// Win32 API wrappers
// ---------------------------------------------------------------------------
// The original calls wrappers in another TU (Win 0x415140-0x416bb0) that take the path as a
// String (in ECX), call the ...W API on Unicode systems and convert to the ANSI code page for
// the ...A API on Windows 9x (flag at +0x14 of the object returned by Win 0x415020). Restored
// as direct ...W calls. (names inferred)

inline LPCWSTR W(const String& s)
{
    return reinterpret_cast<LPCWSTR>(s.GetBuffer());
}

// Win 0x4158a0
inline DWORD Win32GetFileAttributes(const String& path)
{
    return ::GetFileAttributesW(W(path));
}

// Win 0x415a30
inline bool Win32SetFileAttributes(const String& path, DWORD attributes)
{
    return ::SetFileAttributesW(W(path), attributes) != 0;
}

// Win 0x415960
inline bool Win32GetFileAttributesEx(const String& path, WIN32_FILE_ATTRIBUTE_DATA* data)
{
    return ::GetFileAttributesExW(W(path), GetFileExInfoStandard, data) != 0;
}

// Win 0x415210 (registers: access EBX, creation EDI, flags ESI; template always null)
inline HANDLE Win32CreateFile(const String& path, DWORD access, DWORD share, DWORD creation,
                              DWORD flags)
{
    return ::CreateFileW(W(path), access, share, 0, creation, flags, 0);
}

// Win 0x415140
inline bool Win32CreateDirectory(const String& path)
{
    return ::CreateDirectoryW(W(path), 0) != 0;
}

// Win 0x415580
inline bool Win32DeleteFile(const String& path)
{
    return ::DeleteFileW(W(path)) != 0;
}

// Win 0x415e80
inline bool Win32RemoveDirectory(const String& path)
{
    return ::RemoveDirectoryW(W(path)) != 0;
}

// Win 0x415650
inline HANDLE Win32FindFirstFile(const String& pattern, WIN32_FIND_DATAW* data)
{
    return ::FindFirstFileW(W(pattern), data);
}

// Win 0x4157b0
inline bool Win32FindNextFile(HANDLE find, WIN32_FIND_DATAW* data)
{
    return ::FindNextFileW(find, data) != 0;
}

// Win 0x415b00: the drive letters of GetLogicalDriveStrings ("A:\", "C:\", ...) upper-cased,
// one character per drive.
inline void Win32GetLogicalDriveLetters(String& letters)
{
    letters = String();
    WCHAR buffer[0x104];
    ::GetLogicalDriveStringsW(0x104, buffer);
    const WCHAR* p = buffer;
    for (UInt32 i = 4; i <= 0x104; i += 4, p += 4) {
        if (*p == 0)
            break;
        letters.Append(1, (UniChar16)::towupper(*p));
    }
}

// Win 0x416bb0: WNetGetResourceInformation on a NETRESOURCE (the original also copies the
// system part and the provider/remote names of the result into three Strings, unused here).
inline DWORD Win32GetResourceInformation(NETRESOURCEW& resource)
{
    DWORD size = 0x408;
    void* buffer = ::operator new(size);
    LPWSTR system = 0;
    DWORD result = ::WNetGetResourceInformationW(&resource, buffer, &size, &system);
    while (result == ERROR_MORE_DATA) {
        ::operator delete(buffer);
        buffer = ::operator new(size);
        result = ::WNetGetResourceInformationW(&resource, buffer, &size, &system);
    }
    ::operator delete(buffer);
    return result;
}

// ---------------------------------------------------------------------------
// Conversions
// ---------------------------------------------------------------------------

inline UInt64 ToUInt64(const FILETIME& t)
{
    return ((UInt64)t.dwHighDateTime << 32) + t.dwLowDateTime;
}

inline FILETIME ToFileTime(UInt64 t)
{
    FILETIME f;
    f.dwLowDateTime = (DWORD)t;
    f.dwHighDateTime = (DWORD)(t >> 32);
    return f;
}

// Win 0x43c9a0 (custom registers). (name inferred)
// NOTE: every caller passes (creation, last access, last write), so Times::m_modified receives
// the last ACCESS time and m_accessed the last WRITE time, while the setters (0x43d680,
// 0x43dfb0) write m_modified as last write time — copying times swaps them (as in the binary).
Times MakeTimes(const FILETIME& created, const FILETIME& second, const FILETIME& third)
{
    return Times(ToUInt64(created), ToUInt64(second), ToUInt64(third));
}

// Win 0x43cba0 (custom registers: path ECX, result EBX). All zero when the file is missing.
// (name inferred)
Times GetPathTimes(const String& path)
{
    Times times;
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (Win32GetFileAttributesEx(path, &data))
        times = MakeTimes(data.ftCreationTime, data.ftLastAccessTime, data.ftLastWriteTime);
    return times;
}

// Inlined everywhere (e.g. Win 0x43cc40). Readable and removable are always set, executable
// never. NOTE: INVALID_FILE_ATTRIBUTES is not checked by the attribute getters of the elements
// themselves (0x43cc40, 0x43df70): a vanished file reads as a hidden read-only directory.
// (name inferred)
inline Attributes ToAttributes(DWORD attributes)
{
    return Attributes((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0, true,
                      (attributes & FILE_ATTRIBUTE_READONLY) == 0, true,
                      (attributes & FILE_ATTRIBUTE_HIDDEN) != 0, false);
}

// Inlined (e.g. Win 0x43cc10). NOTE: the directory flag is passed on to SetFileAttributes,
// which ignores it. (name inferred)
inline DWORD ToWin32Attributes(const Attributes& attributes)
{
    DWORD result = attributes.m_isDirectory ? FILE_ATTRIBUTE_DIRECTORY : 0;
    if (!attributes.m_writable)
        result |= FILE_ATTRIBUTE_READONLY;
    if (attributes.m_hidden)
        result |= FILE_ATTRIBUTE_HIDDEN;
    return result;
}

inline String ChildPath(const String& directory, const String& name)
{
    String path(directory);
    path << "\\" << name;
    return path;
}

// True for a drive letter of an existing drive (inlined in Win 0x43e440, 0x43e550).
inline bool IsExistingDrive(const String& name)
{
    if (name.GetLength() != 1)
        return false;
    wint_t c = ::towlower(name[0]);
    if ((UInt32)(c - 'a') >= 26)
        return false;
    return ((1u << (c - 'a')) & ::GetLogicalDrives()) != 0;
}

} // namespace

// ===========================================================================
// Win32Directory
// ===========================================================================

// Win 0x43ca10 (parent in ECX)
Win32Directory::Win32Directory(const String& name, Directory* parent, const String& path)
    : Directory(name, parent), m_path(path)
{
}

// Win 0x43ca80 (sdd 0x43cae0)
Win32Directory::~Win32Directory()
{
}

// Win 0x43cc10
bool Win32Directory::DoSetAttributes(const Attributes& attributes)
{
    return Win32SetFileAttributes(m_path, ToWin32Attributes(attributes));
}

// Win 0x43cc40
Attributes Win32Directory::DoGetAttributes() const
{
    return ToAttributes(Win32GetFileAttributes(m_path));
}

// Win 0x43cc80
Times Win32Directory::DoGetTimes() const
{
    return GetPathTimes(m_path);
}

// Win 0x43cca0
// NOTE: does not check that the directory exists (callers test DoDirectoryExists first).
Directory* Win32Directory::DoGetDirectory(const String& name)
{
    return new Win32Directory(name, this, ChildPath(m_path, name));
}

// Win 0x43cd60
bool Win32Directory::DoCreateDirectory(const String& name)
{
    return Win32CreateDirectory(ChildPath(m_path, name));
}

// Win 0x43cdf0
// `create` opens with OPEN_ALWAYS, otherwise OPEN_EXISTING; existing contents are kept (a
// truncation goes through `size`). Returns a new (unreferenced) file.
File* Win32Directory::DoOpenFile(const String& name, const OpenMode& mode, bool create, const UInt64* size)
{
    String path = ChildPath(m_path, name);
    DWORD access = 0;
    if (mode.m_read)
        access = GENERIC_READ;
    if (mode.m_write)
        access |= GENERIC_WRITE;
    DWORD share = 0;
    if (mode.m_shareRead)
        share = FILE_SHARE_READ;
    if (mode.m_shareWrite)
        share |= FILE_SHARE_WRITE;
    HANDLE handle = Win32CreateFile(path, access, share, create ? OPEN_ALWAYS : OPEN_EXISTING, 0);
    if (handle == INVALID_HANDLE_VALUE)
        return 0;
    File* file = new Win32File(name, this, mode, path, handle);
    if (size && !file->SetSize(*size)) {
        file->Release();   // refcount 0 -> deleted
        file = 0;
    }
    return file;
}

// Win 0x43cf30
// A directory is emptied first: its files are deleted, then every sub-directory through
// DoDelete(name\sub) (collected into a list before recursing), then it is removed.
// NOTE: the contents are deleted whatever `recursive` says; the flag only allows clearing the
// read-only attribute of the items (MacOSX uses it the same way through m_removable).
// NOTE: when a file inside cannot be deleted the find handle is leaked; FindClose is also
// called with INVALID_HANDLE_VALUE when the listing failed. Both as in the binary; fixed: the
// handle is closed on that return, and only a valid one (low).
bool Win32Directory::DoDelete(const String& name, bool recursive)
{
    String path = ChildPath(m_path, name);
    DWORD attributes = Win32GetFileAttributes(path);
    if (attributes == INVALID_FILE_ATTRIBUTES)
        return false;

    if (!(attributes & FILE_ATTRIBUTE_DIRECTORY)) {
        if (recursive && (attributes & FILE_ATTRIBUTE_READONLY))
            Win32SetFileAttributes(path, attributes & ~FILE_ATTRIBUTE_READONLY);
        return Win32DeleteFile(path);
    }

    std::list<String> directories;
    WIN32_FIND_DATAW data;
    HANDLE find;
    {
        String pattern(path);
        pattern << "\\*";
        find = Win32FindFirstFile(pattern, &data);
    }
    static const String s_dot(".");      // Win 0xb44c78
    static const String s_dotDot("..");  // Win 0xb44c74
    if (find != INVALID_HANDLE_VALUE) {
        do {
            String entry(reinterpret_cast<const UniChar16*>(data.cFileName));
            if (entry == s_dot || entry == s_dotDot)
                continue;
            if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                directories.push_back(entry);
            } else {
                String filePath = ChildPath(path, entry);
                if (recursive && (data.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
                    Win32SetFileAttributes(filePath, data.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
                if (!Win32DeleteFile(filePath)) {
                    ::FindClose(find);
                    return false;
                }
            }
        } while (Win32FindNextFile(find, &data));
    }
    if (find != INVALID_HANDLE_VALUE)
        ::FindClose(find);

    for (std::list<String>::iterator it = directories.begin(); it != directories.end(); ++it) {
        if (!DoDelete(ChildPath(name, *it), recursive))
            return false;
    }

    if (recursive && (attributes & FILE_ATTRIBUTE_READONLY))
        Win32SetFileAttributes(path, attributes & ~FILE_ATTRIBUTE_READONLY);
    bool result = Win32RemoveDirectory(path);
    ::GetLastError();   // NOTE: result unused (debugging leftover in the binary)
    return result;
}

// Win 0x43d450
bool Win32Directory::DoGetChildAttributes(const String& name, Attributes* attributes)
{
    DWORD value = Win32GetFileAttributes(ChildPath(m_path, name));
    if (value == INVALID_FILE_ATTRIBUTES)
        return false;
    *attributes = ToAttributes(value);
    return true;
}

// Win 0x43d530
bool Win32Directory::DoSetChildAttributes(const String& name, const Attributes& attributes)
{
    return Win32SetFileAttributes(ChildPath(m_path, name), ToWin32Attributes(attributes));
}

// Win 0x43d5d0 — always succeeds (zero times for a missing child).
bool Win32Directory::DoGetChildTimes(const String& name, Times* times)
{
    *times = GetPathTimes(ChildPath(m_path, name));
    return true;
}

// Win 0x43d680 — directories are accepted without changing anything.
bool Win32Directory::DoSetChildTimes(const String& name, const Times& times)
{
    String path = ChildPath(m_path, name);
    DWORD attributes = Win32GetFileAttributes(path);
    if (attributes == INVALID_FILE_ATTRIBUTES)
        return false;
    if (attributes & FILE_ATTRIBUTE_DIRECTORY)
        return true;
    HANDLE handle = Win32CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    OPEN_EXISTING, 0);
    if (handle == INVALID_HANDLE_VALUE)
        return false;
    FILETIME created = ToFileTime(times.m_created);
    FILETIME accessed = ToFileTime(times.m_accessed);
    FILETIME modified = ToFileTime(times.m_modified);
    BOOL ok = ::SetFileTime(handle, &created, &accessed, &modified);
    ::CloseHandle(handle);
    return ok != 0;
}

// Win 0x43d7b0 — true when the listing fails or holds nothing but "." and "..".
bool Win32Directory::DoIsEmpty()
{
    String pattern(m_path);
    pattern << String("\\*");
    WIN32_FIND_DATAW data;
    HANDLE find = Win32FindFirstFile(pattern, &data);
    bool done = find == INVALID_HANDLE_VALUE;
    String dot(".");
    String dotDot("..");
    while (!done) {
        String entry(reinterpret_cast<const UniChar16*>(data.cFileName));
        if (entry != dot && entry != dotDot)
            break;
        done = !Win32FindNextFile(find, &data);
    }
    ::FindClose(find);
    return done;
}

// Win 0x43d9e0
bool Win32Directory::DoDirectoryExists(const BasicArray<String>& path)
{
    String relative;
    relative.Join((UniChar16)'\\', path);
    DWORD attributes = Win32GetFileAttributes(ChildPath(m_path, relative));
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

// Win 0x43dad0
bool Win32Directory::DoFileExists(const String& name)
{
    DWORD attributes = Win32GetFileAttributes(ChildPath(m_path, name));
    return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

// Win 0x43db60
// The pattern is given to FindFirstFile and then checked again with MatchWildcard (Win32 also
// matches 8.3 short names). Directories get their (zero) size from the find data as well.
void Win32Directory::DoEnumerate(const String& pattern, DescriptorSet& entries)
{
    WIN32_FIND_DATAW data;
    HANDLE find = Win32FindFirstFile(ChildPath(m_path, pattern), &data);
    static const String s_dot(".");      // Win 0xb44c6c
    static const String s_dotDot("..");  // Win 0xb44c68
    if (find != INVALID_HANDLE_VALUE) {
        do {
            String entry(reinterpret_cast<const UniChar16*>(data.cFileName));
            if (entry == s_dot || entry == s_dotDot)
                continue;
            if (!MatchWildcard(entry, pattern))
                continue;
            UInt64 size = ((UInt64)data.nFileSizeHigh << 32) + data.nFileSizeLow;
            entries.Insert(Descriptor(entry, ToAttributes(data.dwFileAttributes),
                                      MakeTimes(data.ftCreationTime, data.ftLastAccessTime,
                                                data.ftLastWriteTime),
                                      size));
        } while (Win32FindNextFile(find, &data));
    }
    ::FindClose(find);   // NOTE: also with INVALID_HANDLE_VALUE
}

// ===========================================================================
// Win32File
// ===========================================================================

// Win 0x43cb00 (name in ECX, parent in EDI)
Win32File::Win32File(const String& name, Directory* parent, const OpenMode& mode, const String& path,
                     HANDLE handle)
    : File(name, parent, mode), m_path(path), m_handle(handle)
{
}

// Win 0x43e300 (sdd 0x43cb80)
Win32File::~Win32File()
{
    ::CloseHandle(m_handle);
}

// Win 0x43df40
bool Win32File::DoSetAttributes(const Attributes& attributes)
{
    return Win32SetFileAttributes(m_path, ToWin32Attributes(attributes));
}

// Win 0x43df70
Attributes Win32File::DoGetAttributes() const
{
    return ToAttributes(Win32GetFileAttributes(m_path));
}

// Win 0x43dfb0
bool Win32File::DoSetTimes(const Times& times)
{
    FILETIME created = ToFileTime(times.m_created);
    FILETIME accessed = ToFileTime(times.m_accessed);
    FILETIME modified = ToFileTime(times.m_modified);
    return ::SetFileTime(m_handle, &created, &accessed, &modified) != 0;
}

// Win 0x43e010
Times Win32File::DoGetTimes() const
{
    return GetPathTimes(m_path);
}

// Win 0x439e10 (identical code shared with MemoryFile)
bool Win32File::DoRead(UInt64 position, UInt64 size, void* buffer, UInt64* read)
{
    return Seek(position) && Read(size, buffer, read);
}

// Win 0x43e030 (identical code shared with MemoryFile)
bool Win32File::DoWrite(UInt64 position, UInt64 size, const void* buffer, UInt64* written)
{
    return Seek(position) && Write(size, buffer, written);
}

// Win 0x43e080
// NOTE: only the low 32 bits of `size` are requested; a size >= 4 GiB always fails.
bool Win32File::DoRead(UInt64 size, void* buffer, UInt64* read)
{
    if (size == 0) {
        if (read)
            *read = 0;
        return true;
    }
    DWORD count = 0;
    ::ReadFile(m_handle, buffer, (DWORD)size, &count, 0);
    if (read)
        *read = count;
    return count == (DWORD)size && (size >> 32) == 0;
}

// Win 0x43e110
bool Win32File::DoWrite(UInt64 size, const void* buffer, UInt64* written)
{
    if (size == 0) {
        if (written)
            *written = 0;
        return true;
    }
    DWORD count = 0;
    ::WriteFile(m_handle, buffer, (DWORD)size, &count, 0);
    if (written)
        *written = count;
    return count == (DWORD)size && (size >> 32) == 0;
}

// Win 0x43e1a0
UInt64 Win32File::DoSize()
{
    DWORD high = 0;
    DWORD low = ::GetFileSize(m_handle, &high);
    return ((UInt64)high << 32) | low;
}

// Win 0x43e1d0 — the position is kept, clamped to the new size.
bool Win32File::DoSize(UInt64 size)
{
    UInt64 position = GetPosition();
    LONG high = (LONG)(size >> 32);
    ::SetFilePointer(m_handle, (LONG)size, &high, FILE_BEGIN);
    BOOL ok = ::SetEndOfFile(m_handle);
    Seek(position < size ? position : size);
    return ok != 0;
}

// Win 0x43e250
UInt64 Win32File::DoPosition()
{
    LONG high = 0;
    DWORD low = ::SetFilePointer(m_handle, 0, &high, FILE_CURRENT);
    return ((UInt64)(DWORD)high << 32) | low;
}

// Win 0x43e290 — seeking past the end fails.
// NOTE: the result check sign-extends the returned low dword into the high one (as the binary
// does), so positions with bit 31 set only verify when the high dword is 0xffffffff.
bool Win32File::DoPosition(UInt64 position)
{
    if (GetSize() < position)
        return false;
    LONG high = (LONG)(position >> 32);
    DWORD low = ::SetFilePointer(m_handle, (LONG)position, &high, FILE_BEGIN);
    Int64 reached = ((Int64)high << 32) | (Int64)(LONG)low;
    return reached == (Int64)position;
}

// ===========================================================================
// Win32FileProtocolDirectory
// ===========================================================================

// Win 0x43e370 — "c" -> "c:"
Directory* Win32FileProtocolDirectory::DoGetDirectory(const String& name)
{
    String path(name);
    path << ":";
    return new Win32Directory(name, this, path);
}

// Win 0x43e430
bool Win32FileProtocolDirectory::DoIsEmpty()
{
    return ::GetLogicalDrives() == 0;
}

// Win 0x43e440 — path[0] must be an existing drive letter; the rest must be a directory on it.
bool Win32FileProtocolDirectory::DoDirectoryExists(const BasicArray<String>& path)
{
    // NOTE: the binary reads path[0] unchecked (an empty path reads past the array); fixed
    if (path.IsEmpty() || !IsExistingDrive(path[0]))
        return false;
    if (path.GetSize() > 1) {
        String native;
        native.Join((UniChar16)'\\', path);
        native.Insert(1, 1, (UniChar16)':');   // "c\x\y" -> "c:\x\y"
        DWORD attributes = Win32GetFileAttributes(native);
        if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY))
            return false;
    }
    return true;
}

// Win 0x43e5a0
void Win32FileProtocolDirectory::DoEnumerate(const String& pattern, DescriptorSet& entries)
{
    String letters;
    Win32GetLogicalDriveLetters(letters);
    Attributes attributes(true, true);
    for (UInt32 i = 0; i < letters.GetLength(); ++i) {
        String letter = letters.Substring(i, 1);
        if (MatchWildcard(letter, pattern))
            entries.Insert(Descriptor(letter, attributes, Times(), 0));
    }
}

// Win 0x43e550
bool Win32FileProtocolDirectory::DoRootExists(const String& name)
{
    return IsExistingDrive(name);
}

// ===========================================================================
// Win32NetworkProtocolDirectory
// ===========================================================================

// Win 0x43e750 — "server" -> "\\server"
Directory* Win32NetworkProtocolDirectory::DoGetDirectory(const String& name)
{
    String path("\\\\");
    path << name;
    return new Win32Directory(name, this, path);
}

// Win 0x43e800 — path[0] is checked through the virtual DoRootExists, the rest on disk.
bool Win32NetworkProtocolDirectory::DoDirectoryExists(const BasicArray<String>& path)
{
    // NOTE: the binary reads path[0] unchecked (an empty path reads past the array); fixed
    if (path.IsEmpty() || !DoRootExists(path[0]))
        return false;
    if (path.GetSize() > 1) {
        String native;
        native.Join((UniChar16)'\\', path);
        native.Insert(0, 2, (UniChar16)'\\');   // "x\y" -> "\\x\y"
        DWORD attributes = Win32GetFileAttributes(native);
        if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY))
            return false;
    }
    return true;
}

// Win 0x43e8d0
bool Win32NetworkProtocolDirectory::DoRootExists(const String& name)
{
    String remote("\\\\");
    remote << name;
    NETRESOURCEW resource;
    resource.dwScope = RESOURCE_GLOBALNET;    // 2
    resource.dwType = RESOURCETYPE_DISK;      // 1
    resource.dwDisplayType = 0;
    resource.dwUsage = 0;
    resource.lpLocalName = 0;
    resource.lpRemoteName = const_cast<LPWSTR>(W(remote));
    resource.lpComment = 0;
    resource.lpProvider = 0;
    return Win32GetResourceInformation(resource) == NO_ERROR;
}

// ===========================================================================
// Win32StorageFactory
// ===========================================================================

// Win 0x42eaf0
bool Win32StorageFactory::Initialize()
{
    RefPointer<Directory> existing = Directory::GetRoot()->GetDirectory(String("file"), false);
    if (!existing.IsNull())
        return false;
    existing = Directory::GetRoot()->GetDirectory(String("network"), false);
    if (!existing.IsNull())
        return false;
    m_fileDirectory = new Win32FileProtocolDirectory(String("file"), Directory::GetRoot());
    m_networkDirectory = new Win32NetworkProtocolDirectory(String("network"), Directory::GetRoot());
    return true;
}

// Win 0x42ee30
void Win32StorageFactory::Shutdown()
{
    m_networkDirectory = 0;
    m_fileDirectory = 0;
}

// (not in the original) FlushFileBuffers on `from`, then MoveFileEx over `to`
// (MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH): `to` is the complete old file until the
// move makes it the complete new one. False when the flush or the move fails (`from` is left for
// the caller to delete).
bool Win32Directory::DoReplaceFile(const String& from, const String& to)
{
    const String source = ChildPath(m_path, from);
    const String target = ChildPath(m_path, to);
    HANDLE handle = Win32CreateFile(source, GENERIC_WRITE, FILE_SHARE_READ, OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL);
    if (handle == INVALID_HANDLE_VALUE)
        return false;
    const bool flushed = ::FlushFileBuffers(handle) != 0;
    if (!::CloseHandle(handle) || !flushed)
        return false;
    return ::MoveFileExW(W(source), W(target), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}

} // namespace LEGO

#endif // _WIN32
