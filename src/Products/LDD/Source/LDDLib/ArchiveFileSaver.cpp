// LEGO Digital Designer — restored source
// TU 098 "ArchiveFileSaver.cpp" (Mac 0x1ac5e0-0x1b114f, Win 0x687ac2-0x689a0d): ArchiveFileSaver,
// BricksPatchFileSaver and the downloaders that create them (ArchiveFileDownloader,
// BricksPatchDownloader). See FileSaver.h / FileDownloader.h.
//
// Template instances emitted here (restored through the shared templates): BasicArray of the
// updates (Mac 0x1accb0-0x1ae210, 0x1b0ba0; Win sdd 0x688ec9), BasicArray<String> push
// (0x1ae9b0), the FileSaver Observer / Notifier instances (0x1afeb0-0x1b0b00).
// NOTE (both builds): the "User brick database: …" messages of Update are built and dropped
// (no trace call; presumably a compiled-out trace macro).
#include "FileSaver.h"
#include "FileDownloader.h"
#include "BrickUpdateLoaderXML.h"

#include "Directory.h"
#include "File.h"

#include <iostream>

namespace LEGO {

// ===========================================================================================
// ArchiveFileDownloader / BricksPatchDownloader
// ===========================================================================================

// Mac 0x1ac6a0 (thunk 0x1ac6e0)
void ArchiveFileDownloader::OnSaveFailed()
{
    m_directory = 0;
    FileDownloader::OnSaveFailed();
    ReleaseFile();
}

// Mac 0x1ac720 (thunk 0x1ac770)
void ArchiveFileDownloader::OnSaveFinished(UInt32 files)
{
    m_directory = 0;
    FileDownloader::OnSaveFinished(files);
    ReleaseFile();
}

// Mac 0x1ac7c0, Win 0x6891a4
RefPointer<FileSaver> ArchiveFileDownloader::CreateSaver()
{
    return new ArchiveFileSaver(m_directory.Get(), m_path);
}

// Mac 0x1aed30, Win 0x689505
RefPointer<FileSaver> BricksPatchDownloader::CreateSaver()
{
    return new BricksPatchFileSaver(m_directory.Get(), m_path);
}

// ===========================================================================================
// ArchiveFileSaver
// ===========================================================================================

// Mac 0x1ac830 (C2 0x1accb0), Win 0x688eeb
ArchiveFileSaver::ArchiveFileSaver(Directory* destination, const Path& archive)
    : m_destination(destination), m_current(0)
{
    m_archive = archive.GetDirectory(false);
}

// Mac 0x1acf40, Win 0x688b6a
void ArchiveFileSaver::Start()
{
    if (m_archive.Get())
        CollectUpdates(m_archive.Get(), BasicArray<String>());
    FileSaver::Start();
    NotifyProgress(m_current, m_updates.GetSize());
}

void ArchiveFileSaver::Finish()
{
    Detach();
    m_archive = 0;
    m_destination = 0;
    m_updates.Clear();
    m_current = 0;
}

// Mac 0x1ad280, Win 0x689214
void ArchiveFileSaver::Update()
{
    if (m_current == m_updates.GetSize()) {
        Finish();
        // NOTE (both builds): reports the size of the list after clearing it (0).
        NotifyFinished(m_updates.GetSize());
        return;
    }
    const UpdateEntry& update = m_updates[m_current];
    String name;
    name.Join(String("/"), update.second);
    bool ok = true;
    switch (update.first) {
    case kAdd:
        if (m_destination->FileExists(update.second)) {
            String message("User brick database: newly added file: ");
            message << name << " already exists";                              // NOTE: dropped
        }
        ok = m_archive->Copy(update.second, *m_destination, update.second, true, true);
        break;
    case kOverwrite:
        if (!m_destination->FileExists(update.second)) {
            String message("User brick database: file to be overwritten: ");
            message << name << " does not exist";                              // NOTE: dropped
        }
        ok = m_archive->Copy(update.second, *m_destination, update.second, true, true);
        break;
    case kRemove:
        if (m_destination->FileExists(update.second)) {
            ok = m_destination->Delete(update.second, true);
        } else {
            String message("User brick database: Could not delete '");
            message << name << String("', because it does not exist.");        // NOTE: dropped
        }
        break;
    default:
        ok = m_archive->Copy(update.second, *m_destination, update.second, true, true);
        break;
    }
    if (!ok) {
        Finish();
        NotifyFailed();
        return;
    }
    ++m_current;
    NotifyProgress(m_current, m_updates.GetSize());
}

// Mac 0x1ae5d0, Win 0x6895b3
void ArchiveFileSaver::CollectUpdates(Directory* directory, const BasicArray<String>& prefix)
{
    DescriptorSet entries;
    directory->Enumerate(String("*"), entries);
    for (DescriptorSet::Iterator it = entries.Begin(); it != entries.End(); ++it) {
        const String name(it->m_name);
        BasicArray<String> path(prefix);
        path.PushBack(name);
        if (!it->m_attributes.m_isDirectory) {
            m_updates.PushBack(UpdateEntry(kCopy, path));
        } else {
            RefPointer<Directory> child = directory->GetDirectory(name, false);
            CollectUpdates(child.Get(), path);
        }
    }
}

// ===========================================================================================
// BricksPatchFileSaver
// ===========================================================================================

// Mac 0x1aeda0, Win 0x689075
BricksPatchFileSaver::BricksPatchFileSaver(Directory* destination, const Path& archive)
    : ArchiveFileSaver(destination, archive)
{
    Path root;
    root.Assign(archive);
    root.Append(String("DB"));
    m_archive = root.GetDirectory(false);
}

namespace {

// Each '/' path of `files` as an update of `type`.
void AddUpdates(BasicArray<ArchiveFileSaver::UpdateEntry>& updates,
                ArchiveFileSaver::UpdateType type, const BasicArray<String>& files)
{
    for (UInt32 i = 0; i < files.GetSize(); ++i) {
        BasicArray<String> path;
        files[i].Split(String("/"), path);
        updates.PushBack(ArchiveFileSaver::UpdateEntry(type, path));
    }
}

} // namespace

// Mac 0x1af230, Win 0x689724
void BricksPatchFileSaver::CollectUpdates(Directory* directory, const BasicArray<String>& prefix)
{
    if (!m_archive->FileExists(String("update.xml"))) {
        ArchiveFileSaver::CollectUpdates(directory, prefix);
        return;
    }
    RefPointer<File> file = m_archive->OpenFile(String("update.xml"), OpenMode(true, false, true, false), false, 0);
    BrickUpdateLoaderXML loader;
    BasicArray<String> added;
    BasicArray<String> modified;
    BasicArray<String> deleted;
    // NOTE (both builds): no check for a missing file (an unreadable update.xml fails Load).
    if (!loader.Load(file.Get(), added, modified, deleted))
        return;
    AddUpdates(m_updates, kAdd, added);
    AddUpdates(m_updates, kOverwrite, modified);
    AddUpdates(m_updates, kRemove, deleted);
}

} // namespace LEGO
