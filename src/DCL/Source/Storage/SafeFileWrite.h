// LEGO Digital Designer — not in the original (bug review M2).
// SafeFileWrite.h: checked, all-or-nothing file writes for the writers that the original lets
// fail silently (BOM export, HTML building instructions, screenshots, preferences, palette and
// library files). On the native disk storage a file is written to "<name>.tmp" next to it and
// then flushed and renamed over the file in one step (Directory::DoReplaceFile), so the old file
// stays complete until the new one is; elsewhere (memory, zip and LIF storage) it is written in
// place. Every step's result is checked. Header-only, so the tests that link the writers need no
// new source.
#ifndef LEGO_SAFEFILEWRITE_H
#define LEGO_SAFEFILEWRITE_H

#include "LEGO/Types.h"


#include "Directory.h"
#include "File.h"
#include "Path.h"
#include <string.h>

namespace LEGO {

// The file to write instead of `path`: "<name>.tmp" next to it when its directory can replace a
// file in one step, else `path` itself. A stale temporary file of an earlier failed write is
// removed. (not in the original; M2)
inline Path SafeWriteTarget(const Path& path)
{
    Directory* directory = path.GetParentDirectory();
    if (!directory || !directory->DoSupportsReplaceFile())
        return path;
    String name(path.GetName());
    name << ".tmp";
    Path target(name, directory);
    if (target.Exists())
        target.Delete(false);
    return target;
}

// Ends a write to `target` = SafeWriteTarget(path), after every file object of `target` has been
// released. `written`: every write step succeeded. On the disk storage the temporary file is then
// flushed and renamed over `path` (Directory::ReplaceFile: under the storage lock, refused for a
// read-only `path`); when anything failed it is deleted and `path` is as it was. A
// write in place that failed leaves the partial file (there is nothing to go back to). True when
// `path` now holds the complete new file. (not in the original; M2)
inline bool CommitSafeWrite(const Path& path, const Path& target, bool written)
{
    if (target == path)
        return written;
    Directory* directory = path.GetParentDirectory();
    if (written && directory->ReplaceFile(target.GetName(), path.GetName()))
        return true;
    target.Delete(false);
    return false;
}

// True when `file` holds exactly the `size` bytes of `data` (read back from the start).
// (not in the original; M2)
inline bool FileHoldsData(File* file, const void* data, UInt32 size)
{
    if (file->GetSize() != size)
        return false;
    const UInt32 kChunk = 0x10000;
    UInt8* buffer = new UInt8[kChunk];
    bool same = true;
    for (UInt32 position = 0; same && position < size; position += kChunk) {
        const UInt32 chunk = size - position < kChunk ? size - position : kChunk;
        UInt64 read = 0;
        same = file->ReadAt(position, chunk, buffer, &read) && read == chunk &&
               memcmp(buffer, (const UInt8*)data + position, chunk) == 0;
    }
    delete[] buffer;
    return same;
}

// Writes `size` bytes of `data` as the whole file `path`, all or nothing on the disk storage:
// written, cut to size, read back and compared, then put in place (SafeWriteTarget /
// CommitSafeWrite). (not in the original; M2)
inline bool WriteWholeFile(const Path& path, const void* data, UInt32 size)
{
    const Path target = SafeWriteTarget(path);
    bool written = false;
    {
        RefPointer<File> file = target.OpenFile(OpenMode(true, true), true);
        if (file) {
            written = (size == 0 || file->WriteAt(0, size, data, 0)) && file->SetSize(size) &&
                      FileHoldsData(file.Get(), data, size);
        }
    }
    return CommitSafeWrite(path, target, written);
}

// The size of the entry `name` ("a/b/c" for one in a sub-directory) of `archive` (0 when there
// is none). In a mounted archive this is the size written to the entry, also while it has not
// reached the file. (moved from LDDApplication.cpp; not in the original; M2)
inline UInt64 ArchiveEntrySize(Directory* archive, const String& name)
{
    RefPointer<Directory> directory(archive);
    String entry(name);
    const UInt32 slash = name.ReverseFind((UniChar16)'/');
    if (slash != String::npos) {
        directory = archive->GetDirectory(name.Substring(0, slash), false);
        if (directory.IsNull())
            return 0;
        entry = name.Substring(slash + 1);
    }
    DescriptorSet entries;
    directory->Enumerate(String("*"), entries);
    for (DescriptorSet::Iterator it = entries.Begin(); it != entries.End(); ++it) {
        if (!it->m_attributes.m_isDirectory && it->m_name.EqualsNoCase(entry))
            return it->m_size;
    }
    return 0;
}

// The entry `name` of a freshly mounted `archive` has `size` (> 0) bytes and reads (and
// inflates) completely. (moved from LDDApplication.cpp; not in the original; M2)
inline bool ArchiveEntryReadsBack(Directory* archive, const String& name, UInt64 size)
{
    if (size == 0)
        return false;
    RefPointer<File> file = archive->OpenFile(name, OpenMode(true, false, true, false), false, 0);
    if (file.IsNull() || file->GetSize() != size)
        return false;
    UInt8 buffer[0x4000];
    for (UInt64 position = 0; position < size;) {
        UInt64 count = size - position;
        if (count > sizeof(buffer))
            count = sizeof(buffer);
        UInt64 read = 0;
        if (!file->ReadAt(position, count, buffer, &read) || read != count)
            return false;
        position += count;
    }
    return true;
}

} // namespace LEGO

#endif // LEGO_SAFEFILEWRITE_H
