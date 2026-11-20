// LEGO Digital Designer — restored source
// TU "MacOSXDirectory.mm" Mac 0x329570-0x32bdb9 (52 functions), Mac only: VFS disk backend
// (MacOSXDirectory, MacOSXFile, MacOSXStorageFactory, NSFileHandle category Mac 0x329570) on
// NSFileManager / NSFileHandle (+ fstat). Objective-C behind __OBJC__; plain C++ builds (tests,
// test builds) use the HOST-ONLY POSIX fallbacks with the same semantics.
//
// Template / library code emitted in this TU (not written out here):
//   Mac 0x329660 / 0x32ba50 BasicMultiset<String, Less<String>> D1 / D0 (BasicMap.h),
//   0x32baa0 its _Rb_tree::_M_erase, 0x32bb00 Insert (insert_equal), 0x32bbe0 Erase(key).
// Mac static initialiser 0x32bd20: <iostream> Init and s_openPaths (Mac 0x77ef54).
#if !defined(_WIN32)

#include "MacOSXDirectory.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "BasicMap.h"

#include <CoreFoundation/CoreFoundation.h>
#include <sys/stat.h>
#include <iostream>

#if defined(__OBJC__)
#import <Foundation/Foundation.h>
#include <fcntl.h>      // (M2) DoReplaceFile
#include <stdio.h>
#include <unistd.h>
#else
// HOST-ONLY
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <string>
#include <vector>
#endif

// ---------------------------------------------------------------------------
// +[NSFileHandle(MacOSXStorage) fileHandleAtPath:AccessStyle:]   Mac 0x329570
// ---------------------------------------------------------------------------
// Read access when m_read or m_shareRead is set, write access when m_write or m_shareWrite is
// set. NOTE: a handle with write access is refused (nil) when the file has none of the
// write/execute permission bits (st_mode & 0333) — the read bits are not checked.
#if defined(__OBJC__)
@interface NSFileHandle (MacOSXStorage)
+ (id)fileHandleAtPath:(NSString*)path AccessStyle:(LEGO::OpenMode)style;
@end

@implementation NSFileHandle (MacOSXStorage)
+ (id)fileHandleAtPath:(NSString*)path AccessStyle:(LEGO::OpenMode)style
{
    struct stat info;
    NSFileHandle* handle;
    if ((style.m_read || style.m_shareRead) && (style.m_write || style.m_shareWrite)) {
        handle = [NSFileHandle fileHandleForUpdatingAtPath:path];
        if (handle) {
            fstat([handle fileDescriptor], &info);
            if ((info.st_mode & 0333) == 0)
                handle = nil;
        }
    } else if (style.m_read || style.m_shareRead) {
        handle = [NSFileHandle fileHandleForReadingAtPath:path];
    } else {
        handle = nil;
        if (style.m_write || style.m_shareWrite) {
            handle = [NSFileHandle fileHandleForWritingAtPath:path];
            if (handle) {
                fstat([handle fileDescriptor], &info);
                if ((info.st_mode & 0333) == 0)
                    return nil;
            }
        }
    }
    return handle;
}
@end
#endif

namespace LEGO {

namespace {

// Disk paths of every live MacOSXDirectory / MacOSXFile (name inferred). Mac 0x77ef54
// (vptr; std::multiset at 0x77ef58). DoDelete refuses to delete a path that is in use.
BasicMultiset<String, Less<String> > s_openPaths;

// Attribute bits from POSIX permissions (inlined at four places): S_IRUSR -> readable,
// S_IWUSR -> writable and removable, S_IXUSR -> executable; hidden is cleared. (name inferred)
inline void SetPermissionAttributes(Attributes& attributes, unsigned long permissions)
{
    attributes.m_readable   = (permissions & S_IRUSR) != 0;
    attributes.m_writable   = (permissions & S_IWUSR) != 0;
    attributes.m_removable  = (permissions & S_IWUSR) != 0;
    attributes.m_hidden     = false;
    attributes.m_executable = (permissions & S_IXUSR) != 0;
}

// Owner permission bits for `attributes`, merged into `permissions` (group/other bits kept).
// NOTE: a directory marked removable becomes user-writable even when not marked writable;
// directories are always user-executable. (name inferred)
inline unsigned long MakePermissions(const Attributes& attributes, unsigned long permissions)
{
    unsigned long write = (attributes.m_writable || (attributes.m_isDirectory && attributes.m_removable)) ? S_IWUSR : 0;
    unsigned long read  = attributes.m_readable ? S_IRUSR : 0;
    unsigned long exec  = (attributes.m_isDirectory || attributes.m_executable) ? S_IXUSR : 0;
    return (exec + (write | read)) | (permissions & ~(unsigned long)S_IRWXU);
}

#if defined(__OBJC__)

inline NSString* AppendPath(CFStringRef path, const String& name)  // (name inferred; inlined)
{
    return [(NSString*)path stringByAppendingPathComponent:(NSString*)name.GetCFString()];
}

// 1601-01-01 00:00 in the Gregorian calendar (inlined in 0x329790 / 0x3299e0).
// NOTE: built with the calendar's default (local) time zone, so the FILETIME-like tick
// values are relative to local time, not UTC. (name inferred)
inline NSDate* ReferenceDate()
{
    NSDateComponents* components = [[NSDateComponents alloc] init];
    [components setDay:1];
    [components setMonth:1];
    [components setYear:1601];
    NSCalendar* calendar = [[NSCalendar alloc] initWithCalendarIdentifier:NSGregorianCalendar];
    NSDate* date = [calendar dateFromComponents:components];
    [calendar release];
    [components release];
    return date;
}

// Mac 0x329690 (name inferred)
bool SetAttributesAtPath(NSString* path, const Attributes& attributes)
{
    NSFileManager* fileManager = [NSFileManager defaultManager];
    NSDictionary* current = [fileManager attributesOfItemAtPath:path error:nil];
    if (!current)
        return false;
    unsigned long permissions = MakePermissions(attributes, [current filePosixPermissions]);
    NSDictionary* changed = [NSDictionary dictionaryWithObject:[NSNumber numberWithUnsignedInt:(unsigned int)permissions]
                                                        forKey:NSFilePosixPermissions];
    return [fileManager setAttributes:changed ofItemAtPath:path error:nil];
}

// Mac 0x329790 (name inferred). Leaves *times unchanged when the item does not exist.
// m_accessed receives the modification date.
bool GetTimesAtPath(NSString* path, Times* times)
{
    NSDictionary* attributes = [[NSFileManager defaultManager] attributesOfItemAtPath:path error:nil];
    if (attributes) {
        NSDate* reference = ReferenceDate();
        times->m_created = (UInt64)([[attributes objectForKey:NSFileCreationDate] timeIntervalSinceDate:reference] * 10000000.0);
        UInt64 modified = (UInt64)([[attributes objectForKey:NSFileModificationDate] timeIntervalSinceDate:reference] * 10000000.0);
        times->m_modified = modified;
        times->m_accessed = modified;
    }
    return attributes != nil;
}

// Mac 0x3299e0 (name inferred). Sets creation and modification date (m_accessed is ignored).
bool SetTimesAtPath(NSString* path, const Times& times)
{
    NSDate* reference = ReferenceDate();
    NSDate* created = [[NSDate alloc] initWithTimeInterval:(double)times.m_created / 10000000.0 sinceDate:reference];
    NSDate* modified = [[NSDate alloc] initWithTimeInterval:(double)times.m_modified / 10000000.0 sinceDate:reference];
    NSFileManager* fileManager = [NSFileManager defaultManager];
    NSDictionary* attributes = [NSDictionary dictionaryWithObjects:[NSArray arrayWithObjects:created, modified, nil]
                                                           forKeys:[NSArray arrayWithObjects:NSFileCreationDate, NSFileModificationDate, nil]];
    BOOL result = [fileManager setAttributes:attributes ofItemAtPath:path error:nil];
    [created release];
    [modified release];
    return result;
}

// Inlined body of MacOSXDirectory/MacOSXFile::DoGetAttributes (0x329d30 / 0x32aed0).
inline Attributes GetAttributesAtPath(NSString* path)
{
    NSFileManager* fileManager = [NSFileManager defaultManager];
    BOOL isDirectory;
    if (![fileManager fileExistsAtPath:path isDirectory:&isDirectory])
        return Attributes();
    Attributes attributes(isDirectory != NO);
    NSDictionary* info = [fileManager attributesOfItemAtPath:path error:nil];
    if (info)
        SetPermissionAttributes(attributes, [info filePosixPermissions]);
    return attributes;
}

#else // !__OBJC__ ---------------------------------------------------------------------------

// HOST-ONLY: POSIX stand-ins for the Foundation calls used above. Paths are UTF-8.
typedef std::string HostPath;

const Int64 kHostEpochOffset = 11644473600LL;   // seconds 1601-01-01 .. 1970-01-01 (UTC)

HostPath ToHostPath(CFStringRef path)
{
    CFIndex size = CFStringGetLength(path) * 4 + 1;
    std::vector<char> buffer(size);
    if (!CFStringGetCString(path, &buffer[0], size, kCFStringEncodingUTF8))
        return HostPath();
    return HostPath(&buffer[0]);
}

// -[NSString stringByAppendingPathComponent:]
HostPath AppendPath(const HostPath& base, const HostPath& component)
{
    if (component.empty())
        return base;
    if (base.empty())
        return component;
    if (base[base.size() - 1] == '/')
        return base + component;
    return base + "/" + component;
}

HostPath AppendPath(CFStringRef path, const String& name)
{
    return AppendPath(ToHostPath(path), ToHostPath(name.GetCFString()));
}

bool SetAttributesAtPath(const HostPath& path, const Attributes& attributes)
{
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
        return false;
    return chmod(path.c_str(), (mode_t)MakePermissions(attributes, info.st_mode & 07777)) == 0;
}

// No creation date on Linux: m_created also receives the modification time.
bool GetTimesAtPath(const HostPath& path, Times* times)
{
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
        return false;
    UInt64 modified = (UInt64)((Int64)info.st_mtime + kHostEpochOffset) * 10000000ULL;
    times->m_created  = modified;
    times->m_modified = modified;
    times->m_accessed = modified;
    return true;
}

// Only the modification time can be set.
bool SetTimesAtPath(const HostPath& path, const Times& times)
{
    struct timeval values[2];
    Int64 seconds = (Int64)(times.m_modified / 10000000ULL) - kHostEpochOffset;
    values[0].tv_sec = (time_t)seconds;
    values[0].tv_usec = 0;
    values[1] = values[0];
    return utimes(path.c_str(), values) == 0;
}

Attributes GetAttributesAtPath(const HostPath& path)
{
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
        return Attributes();
    Attributes attributes(S_ISDIR(info.st_mode));
    SetPermissionAttributes(attributes, info.st_mode);
    return attributes;
}

// Entry names of a directory without "." and ".." (-contentsOfDirectoryAtPath:error:).
bool ListDirectory(const HostPath& path, std::vector<HostPath>& names)
{
    DIR* dir = opendir(path.c_str());
    if (!dir)
        return false;
    while (struct dirent* entry = readdir(dir)) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            names.push_back(entry->d_name);
    }
    closedir(dir);
    return true;
}

// Makes every directory below `path` user-writable (the NSDirectoryEnumerator loop of DoDelete).
bool MakeSubdirectoriesWritable(const HostPath& path)
{
    std::vector<HostPath> names;
    ListDirectory(path, names);
    for (size_t i = 0; i < names.size(); ++i) {
        HostPath child = AppendPath(path, names[i]);
        struct stat info;
        if (lstat(child.c_str(), &info) != 0)
            return false;
        if (S_ISDIR(info.st_mode)) {
            if (chmod(child.c_str(), (info.st_mode & 07777) | S_IWUSR) != 0)
                return false;
            if (!MakeSubdirectoriesWritable(child))
                return false;
        }
    }
    return true;
}

// -[NSFileManager removeItemAtPath:error:] (always recursive)
bool RemoveItem(const HostPath& path)
{
    struct stat info;
    if (lstat(path.c_str(), &info) != 0)
        return false;
    if (S_ISDIR(info.st_mode)) {
        std::vector<HostPath> names;
        if (!ListDirectory(path, names))
            return false;
        for (size_t i = 0; i < names.size(); ++i)
            if (!RemoveItem(AppendPath(path, names[i])))
                return false;
        return rmdir(path.c_str()) == 0;
    }
    return unlink(path.c_str()) == 0;
}

// The NSFileHandle stand-in: MacOSXFile::m_handle points to one of these.
struct HostFileHandle {
    int fd;
};

inline HostFileHandle* Host(NSFileHandle* handle) { return reinterpret_cast<HostFileHandle*>(handle); }

// +[NSFileHandle(MacOSXStorage) fileHandleAtPath:AccessStyle:] (same decisions as 0x329570)
NSFileHandle* FileHandleAtPath(const HostPath& path, const OpenMode& style)
{
    bool read = style.m_read || style.m_shareRead;
    bool write = style.m_write || style.m_shareWrite;
    int flags;
    if (read && write)
        flags = O_RDWR;
    else if (read)
        flags = O_RDONLY;
    else if (write)
        flags = O_WRONLY;
    else
        return 0;
    int fd = open(path.c_str(), flags);
    if (fd < 0)
        return 0;
    struct stat info;
    // HOST-ONLY: open(2) also opens a folder read-only, so Directory::DoCopy would copy a
    // sub-folder as a file (a file of the folder's size, then EISDIR on read). A folder is not a
    // file handle here, as on Windows (CreateFile fails on a folder), so DoCopy takes its folder
    // branch. (Whether the real -fileHandleForReadingAtPath: opens a folder is not checked on a
    // Mac; no 4.3.11 path copies a folder from disk.) (F5)
    if (fstat(fd, &info) != 0 || S_ISDIR(info.st_mode)) {
        close(fd);
        return 0;
    }
    if (write && (info.st_mode & 0333) == 0) {
        close(fd);
        return 0;
    }
    HostFileHandle* handle = new HostFileHandle;
    handle->fd = fd;
    return reinterpret_cast<NSFileHandle*>(handle);
}

#endif // __OBJC__

} // namespace

// ---------------------------------------------------------------------------
// MacOSXDirectory
// ---------------------------------------------------------------------------

// Mac 0x329c60
MacOSXDirectory::MacOSXDirectory(const String& name, Directory* parent, CFStringRef path)
    : Directory(name, parent)
{
    m_path = CFStringCreateCopy(0, path);
    s_openPaths.Insert(String(m_path));
}

// Mac 0x329e50 (D1 0x329ee0, D0 0x329ef0)
// NOTE: Erase removes every entry of the path, also those of other live objects for the
// same path (e.g. an open file that was opened twice).
MacOSXDirectory::~MacOSXDirectory()
{
    s_openPaths.Erase(String(m_path));
    CFRelease(m_path);
}

// Mac 0x329d10
bool MacOSXDirectory::DoSetAttributes(const Attributes& attributes)
{
#if defined(__OBJC__)
    return SetAttributesAtPath((NSString*)m_path, attributes);
#else
    return SetAttributesAtPath(ToHostPath(m_path), attributes);
#endif
}

// Mac 0x329d30
Attributes MacOSXDirectory::DoGetAttributes() const
{
#if defined(__OBJC__)
    return GetAttributesAtPath((NSString*)m_path);
#else
    return GetAttributesAtPath(ToHostPath(m_path));
#endif
}

// Mac 0x329de0
bool MacOSXDirectory::DoSetTimes(const Times& times)
{
#if defined(__OBJC__)
    return SetTimesAtPath((NSString*)m_path, times);
#else
    return SetTimesAtPath(ToHostPath(m_path), times);
#endif
}

// Mac 0x329e00 (zero times when the item is missing)
Times MacOSXDirectory::DoGetTimes() const
{
    Times times;
#if defined(__OBJC__)
    GetTimesAtPath((NSString*)m_path, &times);
#else
    GetTimesAtPath(ToHostPath(m_path), &times);
#endif
    return times;
}

// Mac 0x329f30 — does not check that the directory exists (callers ask DoDirectoryExists).
Directory* MacOSXDirectory::DoGetDirectory(const String& name)
{
#if defined(__OBJC__)
    return new MacOSXDirectory(name, this, (CFStringRef)AppendPath(m_path, name));
#else
    HostPath path = AppendPath(m_path, name);
    CFStringRef cf = CFStringCreateWithCString(0, path.c_str(), kCFStringEncodingUTF8);
    MacOSXDirectory* directory = new MacOSXDirectory(name, this, cf);
    CFRelease(cf);
    return directory;
#endif
}

// Mac 0x329fb0 (no intermediate directories)
bool MacOSXDirectory::DoCreateDirectory(const String& name)
{
#if defined(__OBJC__)
    NSFileManager* fileManager = [NSFileManager defaultManager];
    return [fileManager createDirectoryAtPath:AppendPath(m_path, name) withIntermediateDirectories:NO
                                   attributes:nil error:nil];
#else
    return mkdir(AppendPath(m_path, name).c_str(), 0777) == 0;
#endif
}

// Mac 0x32a020
// With `create`, a file that cannot be opened is (re)created empty — NOTE: this also
// truncates an existing file that merely refused the requested access.
File* MacOSXDirectory::DoOpenFile(const String& name, const OpenMode& mode, bool create, const UInt64* size)
{
#if defined(__OBJC__)
    NSString* path = AppendPath(m_path, name);
    NSFileManager* fileManager = [NSFileManager defaultManager];
    NSFileHandle* handle = [NSFileHandle fileHandleAtPath:path AccessStyle:mode];
    if (!handle && !create)
        return 0;
    if (!handle) {
        if (![fileManager createFileAtPath:path contents:nil attributes:nil])
            return 0;
        handle = [NSFileHandle fileHandleAtPath:path AccessStyle:mode];
        if (!handle)
            return 0;
    }
    if (size) {
        [handle truncateFileAtOffset:*size];
        [handle seekToFileOffset:0];
    }
    return new MacOSXFile(name, this, mode, (CFStringRef)path, handle);
#else
    HostPath path = AppendPath(m_path, name);
    NSFileHandle* handle = FileHandleAtPath(path, mode);
    if (!handle && !create)
        return 0;
    if (!handle) {
        int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0)
            return 0;
        close(fd);
        handle = FileHandleAtPath(path, mode);
        if (!handle)
            return 0;
    }
    if (size) {
        if (ftruncate(Host(handle)->fd, (off_t)*size) == 0)
            lseek(Host(handle)->fd, 0, SEEK_SET);
    }
    CFStringRef cf = CFStringCreateWithCString(0, path.c_str(), kCFStringEncodingUTF8);
    File* file = new MacOSXFile(name, this, mode, cf, handle);
    CFRelease(cf);
    return file;
#endif
}

// Mac 0x32a1c0
// Fails while a MacOSXDirectory/MacOSXFile for the path is alive. `recursive` first makes the
// item removable and every sub-directory user-writable. NOTE: the removal itself is always
// recursive (-removeItemAtPath:error:), with or without the flag.
bool MacOSXDirectory::DoDelete(const String& name, bool recursive)
{
#if defined(__OBJC__)
    NSString* path = AppendPath(m_path, name);
    if (s_openPaths.Count(String((CFStringRef)path)) != 0)
        return false;
    NSFileManager* fileManager = [NSFileManager defaultManager];
    if (recursive) {
        Attributes attributes;
        if (!DoGetChildAttributes(name, &attributes))
            return false;
        if (!attributes.m_removable) {
            attributes.m_removable = true;
            if (!DoSetChildAttributes(name, attributes))
                return false;
        }
        NSDirectoryEnumerator* enumerator = [fileManager enumeratorAtPath:path];
        for (NSString* subPath in enumerator) {
            NSDictionary* subAttributes = [enumerator fileAttributes];
            if (!subAttributes)
                return false;
            if ([subAttributes fileType] == NSFileTypeDirectory) {
                NSNumber* permissions = [NSNumber numberWithUnsignedInt:(unsigned int)([subAttributes filePosixPermissions] | S_IWUSR)];
                NSDictionary* changed = [NSDictionary dictionaryWithObject:permissions forKey:NSFilePosixPermissions];
                if (![fileManager setAttributes:changed ofItemAtPath:[path stringByAppendingPathComponent:subPath] error:nil])
                    return false;
            }
        }
    }
    return [fileManager removeItemAtPath:path error:nil];
#else
    HostPath path = AppendPath(m_path, name);
    {
        CFStringRef cf = CFStringCreateWithCString(0, path.c_str(), kCFStringEncodingUTF8);
        String key(cf);
        CFRelease(cf);
        if (s_openPaths.Count(key) != 0)
            return false;
    }
    if (recursive) {
        Attributes attributes;
        if (!DoGetChildAttributes(name, &attributes))
            return false;
        if (!attributes.m_removable) {
            attributes.m_removable = true;
            if (!DoSetChildAttributes(name, attributes))
                return false;
        }
        if (!MakeSubdirectoriesWritable(path))
            return false;
    }
    return RemoveItem(path);
#endif
}

// Mac 0x32a5d0
bool MacOSXDirectory::DoRename(const String& from, const String& to)
{
#if defined(__OBJC__)
    NSFileManager* fileManager = [NSFileManager defaultManager];
    return [fileManager moveItemAtPath:AppendPath(m_path, from) toPath:AppendPath(m_path, to) error:nil];
#else
    HostPath destination = AppendPath(m_path, to);
    struct stat info;
    if (lstat(destination.c_str(), &info) == 0)
        return false;                                  // -moveItemAtPath: never overwrites
    return ::rename(AppendPath(m_path, from).c_str(), destination.c_str()) == 0;
#endif
}

// Mac 0x32a670
// Updates *attributes in place: the directory bit as soon as the item exists, the permission
// bits (1-5) only when its attributes can be read; bits 6-7 are kept.
bool MacOSXDirectory::DoGetChildAttributes(const String& name, Attributes* attributes)
{
#if defined(__OBJC__)
    NSString* path = AppendPath(m_path, name);
    NSFileManager* fileManager = [NSFileManager defaultManager];
    BOOL isDirectory;
    if (![fileManager fileExistsAtPath:path isDirectory:&isDirectory])
        return false;
    attributes->m_isDirectory = isDirectory != NO;
    NSDictionary* info = [fileManager attributesOfItemAtPath:path error:nil];
    if (!info)
        return false;
    SetPermissionAttributes(*attributes, [info filePosixPermissions]);
    return true;
#else
    struct stat info;
    if (stat(AppendPath(m_path, name).c_str(), &info) != 0)
        return false;
    attributes->m_isDirectory = S_ISDIR(info.st_mode);
    SetPermissionAttributes(*attributes, info.st_mode);
    return true;
#endif
}

// Mac 0x32a750
bool MacOSXDirectory::DoSetChildAttributes(const String& name, const Attributes& attributes)
{
    return SetAttributesAtPath(AppendPath(m_path, name), attributes);
}

// Mac 0x32a7a0
bool MacOSXDirectory::DoGetChildTimes(const String& name, Times* times)
{
    return GetTimesAtPath(AppendPath(m_path, name), times);
}

// Mac 0x32a7f0
bool MacOSXDirectory::DoSetChildTimes(const String& name, const Times& times)
{
    return SetTimesAtPath(AppendPath(m_path, name), times);
}

// Mac 0x32a840
// NOTE: true only when the directory cannot be listed — an existing (even empty) directory is
// never reported as empty.
bool MacOSXDirectory::DoIsEmpty()
{
#if defined(__OBJC__)
    NSFileManager* fileManager = [NSFileManager defaultManager];
    return [fileManager contentsOfDirectoryAtPath:(NSString*)m_path error:nil] == nil;
#else
    std::vector<HostPath> names;
    return !ListDirectory(ToHostPath(m_path), names);
#endif
}

// Mac 0x32a890 — the whole chain must name one existing directory below this one.
bool MacOSXDirectory::DoDirectoryExists(const BasicArray<String>& path)
{
#if defined(__OBJC__)
    NSMutableArray* components = [NSMutableArray arrayWithCapacity:path.GetSize() + 1];
    [components addObject:(NSString*)m_path];
    for (UInt32 i = 0; i < path.GetSize(); ++i)
        [components addObject:(NSString*)path[i].GetCFString()];
    NSString* fullPath = [NSString pathWithComponents:components];
    BOOL isDirectory;
    BOOL exists = [[NSFileManager defaultManager] fileExistsAtPath:fullPath isDirectory:&isDirectory];
    return isDirectory && exists;
#else
    HostPath fullPath = ToHostPath(m_path);
    for (UInt32 i = 0; i < path.GetSize(); ++i)
        fullPath = AppendPath(fullPath, ToHostPath(path[i].GetCFString()));
    struct stat info;
    return stat(fullPath.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
#endif
}

// Mac 0x32a990
bool MacOSXDirectory::DoFileExists(const String& name)
{
#if defined(__OBJC__)
    NSFileManager* fileManager = [NSFileManager defaultManager];
    BOOL isDirectory;
    return [fileManager fileExistsAtPath:AppendPath(m_path, name) isDirectory:&isDirectory] && !isDirectory;
#else
    struct stat info;
    return stat(AppendPath(m_path, name).c_str(), &info) == 0 && !S_ISDIR(info.st_mode);
#endif
}

// Mac 0x32aa10
// NOTE: `attributes` and `times` live outside the loop: an entry whose attributes/times cannot
// be read inherits those of the previous entry. Sizes are 0 for directories.
void MacOSXDirectory::DoEnumerate(const String& pattern, DescriptorSet& entries)
{
#if defined(__OBJC__)
    NSFileManager* fileManager = [NSFileManager defaultManager];
    NSArray* contents = [fileManager contentsOfDirectoryAtPath:(NSString*)m_path error:nil];
    if (!contents)
        return;
    Times times;
    Attributes attributes;
    for (UInt32 i = 0; i < [contents count]; ++i) {
        NSString* name = [contents objectAtIndex:i];
        if (!MatchWildcard(String((CFStringRef)name), pattern))
            continue;
        NSString* path = [(NSString*)m_path stringByAppendingPathComponent:name];
        NSFileManager* manager = [NSFileManager defaultManager];
        BOOL isDirectory;
        if ([manager fileExistsAtPath:path isDirectory:&isDirectory]) {
            attributes.m_isDirectory = isDirectory != NO;
            NSDictionary* info = [manager attributesOfItemAtPath:path error:nil];
            if (info)
                SetPermissionAttributes(attributes, [info filePosixPermissions]);
        }
        GetTimesAtPath(path, &times);
        UInt64 size = 0;
        if (!attributes.m_isDirectory)
            size = [[[fileManager attributesOfItemAtPath:path error:nil] objectForKey:NSFileSize] unsignedLongLongValue];
        entries.Insert(Descriptor(String((CFStringRef)name), attributes, times, size));
    }
#else
    std::vector<HostPath> names;
    if (!ListDirectory(ToHostPath(m_path), names))
        return;
    Times times;
    Attributes attributes;
    for (UInt32 i = 0; i < names.size(); ++i) {
        CFStringRef cf = CFStringCreateWithCString(0, names[i].c_str(), kCFStringEncodingUTF8);
        String name(cf);
        CFRelease(cf);
        if (!MatchWildcard(name, pattern))
            continue;
        HostPath path = AppendPath(ToHostPath(m_path), names[i]);
        struct stat info;
        if (stat(path.c_str(), &info) == 0) {
            attributes.m_isDirectory = S_ISDIR(info.st_mode);
            SetPermissionAttributes(attributes, info.st_mode);
        }
        GetTimesAtPath(path, &times);
        UInt64 size = 0;
        if (!attributes.m_isDirectory && stat(path.c_str(), &info) == 0)
            size = (UInt64)info.st_size;
        entries.Insert(Descriptor(name, attributes, times, size));
    }
#endif
}

// ---------------------------------------------------------------------------
// MacOSXFile
// ---------------------------------------------------------------------------

// Mac 0x32ad80
MacOSXFile::MacOSXFile(const String& name, Directory* parent, const OpenMode& mode, CFStringRef path,
                       NSFileHandle* handle)
    : File(name, parent, mode)
{
#if defined(__OBJC__)
    m_handle = [handle retain];
#else
    m_handle = handle;                                 // HOST-ONLY: takes ownership
#endif
    m_path = CFStringCreateCopy(0, path);
    s_openPaths.Insert(String(m_path));
}

// Mac 0x32b550 (D1 0x32b6b0, D0 0x32b6c0). NOTE: Erase as in ~MacOSXDirectory.
MacOSXFile::~MacOSXFile()
{
    s_openPaths.Erase(String(m_path));
#if defined(__OBJC__)
    [m_handle closeFile];
    [m_handle release];
#else
    close(Host(m_handle)->fd);
    delete Host(m_handle);
#endif
    CFRelease(m_path);
}

// Mac 0x32aeb0
bool MacOSXFile::DoSetAttributes(const Attributes& attributes)
{
#if defined(__OBJC__)
    return SetAttributesAtPath((NSString*)m_path, attributes);
#else
    return SetAttributesAtPath(ToHostPath(m_path), attributes);
#endif
}

// Mac 0x32aed0
Attributes MacOSXFile::DoGetAttributes() const
{
#if defined(__OBJC__)
    return GetAttributesAtPath((NSString*)m_path);
#else
    return GetAttributesAtPath(ToHostPath(m_path));
#endif
}

// Mac 0x32af80
bool MacOSXFile::DoSetTimes(const Times& times)
{
#if defined(__OBJC__)
    return SetTimesAtPath((NSString*)m_path, times);
#else
    return SetTimesAtPath(ToHostPath(m_path), times);
#endif
}

// Mac 0x32afa0
Times MacOSXFile::DoGetTimes() const
{
    Times times;
#if defined(__OBJC__)
    GetTimesAtPath((NSString*)m_path, &times);
#else
    GetTimesAtPath(ToHostPath(m_path), &times);
#endif
    return times;
}

// Mac 0x32aff0 — through the locked public wrappers.
bool MacOSXFile::DoRead(UInt64 position, UInt64 size, void* buffer, UInt64* read)
{
    if (!Seek(position))
        return false;
    return Read(size, buffer, read);
}

// Mac 0x32b030
bool MacOSXFile::DoWrite(UInt64 position, UInt64 size, const void* buffer, UInt64* written)
{
    if (!Seek(position))
        return false;
    return Write(size, buffer, written);
}

// Mac 0x32b070 — true only when all `size` bytes were read.
// NOTE: the length is passed as a 32-bit NSUInteger.
bool MacOSXFile::DoRead(UInt64 size, void* buffer, UInt64* read)
{
    if (size == 0) {
        if (read)
            *read = 0;
        return true;
    }
    bool result = true;
#if defined(__OBJC__)
    NSData* data = nil;
    @try {
        data = [m_handle readDataOfLength:(NSUInteger)size];
        [data getBytes:buffer];
        if (read)
            *read = [data length];
        result = [data length] == size;
    }
    @catch (NSException* exception) {
        if (read)
            *read = data ? [data length] : 0;
        result = false;
    }
#else
    UInt32 length = (UInt32)size;
    UInt32 done = 0;
    while (done < length) {
        ssize_t n = ::read(Host(m_handle)->fd, (char*)buffer + done, length - done);
        if (n < 0) {                                   // NSFileHandleOperationException
            if (read)
                *read = 0;
            return false;
        }
        if (n == 0)
            break;
        done += (UInt32)n;
    }
    if (read)
        *read = done;
    result = done == size;
#endif
    return result;
}

// Mac 0x32b230 — reports all bytes as written unless NSFileHandle raises.
bool MacOSXFile::DoWrite(UInt64 size, const void* buffer, UInt64* written)
{
    if (size == 0) {
        if (written)
            *written = 0;
        return true;
    }
    bool result = true;
#if defined(__OBJC__)
    NSData* data = [NSData dataWithBytes:buffer length:(NSUInteger)size];
    @try {
        [m_handle writeData:data];
        if (written)
            *written = size;
    }
    @catch (NSException* exception) {
        if (written)
            *written = 0;
        result = false;
    }
#else
    UInt32 length = (UInt32)size;
    UInt32 done = 0;
    while (done < length) {
        ssize_t n = ::write(Host(m_handle)->fd, (const char*)buffer + done, length - done);
        if (n <= 0) {                                  // NSFileHandleOperationException
            if (written)
                *written = 0;
            return false;
        }
        done += (UInt32)n;
    }
    if (written)
        *written = size;
#endif
    return result;
}

// Mac 0x32b370
UInt64 MacOSXFile::DoSize()
{
    struct stat info;
#if defined(__OBJC__)
    fstat([m_handle fileDescriptor], &info);
#else
    fstat(Host(m_handle)->fd, &info);
#endif
    return (UInt64)info.st_size;
}

// Mac 0x32b3b0 — truncating moves the file pointer to `size`; a position below the new size
// is restored afterwards (and also after a failure).
bool MacOSXFile::DoSize(UInt64 size)
{
    UInt64 position = GetPosition();
    bool result = true;
#if defined(__OBJC__)
    @try {
        [m_handle truncateFileAtOffset:size];
        if (position < size)
            Seek(position);
    }
    @catch (NSException* exception) {
        Seek(position);
        result = false;
    }
#else
    if (ftruncate(Host(m_handle)->fd, (off_t)size) != 0) {
        Seek(position);
        return false;
    }
    lseek(Host(m_handle)->fd, (off_t)size, SEEK_SET);
    if (position < size)
        Seek(position);
#endif
    return result;
}

// Mac 0x32b4d0
UInt64 MacOSXFile::DoPosition()
{
#if defined(__OBJC__)
    return [m_handle offsetInFile];
#else
    return (UInt64)lseek(Host(m_handle)->fd, 0, SEEK_CUR);
#endif
}

// Mac 0x32b500 — positions beyond the end of the file are refused.
bool MacOSXFile::DoPosition(UInt64 position)
{
    bool result = position <= GetSize();
    if (result) {
#if defined(__OBJC__)
        [m_handle seekToFileOffset:position];
#else
        lseek(Host(m_handle)->fd, (off_t)position, SEEK_SET);
#endif
    }
    return result;
}

// ---------------------------------------------------------------------------
// MacOSXStorageFactory
// ---------------------------------------------------------------------------

// Mac 0x32b700
bool MacOSXStorageFactory::Initialize()
{
    s_openPaths.Clear();
    RefPointer<Directory> existing = Directory::GetRoot()->GetDirectory(String("file"), false);
    if (!existing.IsNull())
        return false;
    m_disk = new MacOSXDirectory(String("file"), Directory::GetRoot(), CFSTR("/"));
    return true;
}

// Mac 0x32b8f0
void MacOSXStorageFactory::Shutdown()
{
    if (!m_disk.IsNull())
        m_disk = 0;
}

// (not in the original; M2) fsync of `from`, then rename(2) over `to`: `to` is the complete old
// file until the rename makes it the complete new one. False when the flush or the rename fails
// (`from` is left for the caller to delete).
bool MacOSXDirectory::DoReplaceFile(const String& from, const String& to)
{
#if defined(__OBJC__)
    const char* source = [AppendPath(m_path, from) fileSystemRepresentation];
    const char* target = [AppendPath(m_path, to) fileSystemRepresentation];
    std::string sourcePath(source ? source : "");
    std::string targetPath(target ? target : "");
#else
    const std::string sourcePath = AppendPath(m_path, from);
    const std::string targetPath = AppendPath(m_path, to);
#endif
    if (sourcePath.empty() || targetPath.empty())
        return false;
    const int fd = ::open(sourcePath.c_str(), O_RDONLY);
    if (fd < 0)
        return false;
    const bool flushed = ::fsync(fd) == 0;
    if (::close(fd) != 0 || !flushed)
        return false;
    return ::rename(sourcePath.c_str(), targetPath.c_str()) == 0;
}

} // namespace LEGO

#endif // !_WIN32
