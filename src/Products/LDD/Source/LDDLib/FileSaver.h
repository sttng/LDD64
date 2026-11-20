// LEGO Digital Designer — restored source
// LEGO::FileSaver / FileSaverObserver — the step that stores a downloaded file (FileDownloader):
//   an UpdateObserver of the main updater that reports progress, failure and the number of files
//   saved to its observers. The plain FileSaver keeps the file in memory (one pass, 1 file).
//   Home TU 099 "FileDownloader.cpp" (Mac 0x1b1150-0x1b2350; Win 0x68814a-0x6882ae).
// LEGO::ArchiveFileSaver — copies every file of a downloaded archive (the downloaded file opened
//   as a directory: a zip) into a destination directory, one file per update pass; the list of
//   updates comes from CollectUpdates (every file: copy). TU 098 "ArchiveFileSaver.cpp".
// LEGO::BricksPatchFileSaver — an ArchiveFileSaver for a brick database patch: the archive's "DB"
//   directory; with an "update.xml" there the add / overwrite / remove lists of that file are the
//   updates (BrickUpdateLoaderXML). TU 098.
// LEGO::SavingFileSaver — copies the downloaded file into a directory under a given name (one
//   pass). TU 101 "SavingFileSaver.cpp".
//
// RTTI: FileSaver : UpdateObserver (+0), Notifier<FileSaverObserver> (+8), virtual Object. Mac
// sizeof 0x34 (Object +0x2c). Mac vtable slots: 0/1 D1/D0, 2 OnFinalRelease 0x1b1c20, 3/4
// OnAttached/OnDetached, 5 Update 0x1b1710, 6 Start 0x1b1200 (Win: Update 0x68817c, Start
// 0x688228, sdd 0x688209).
// FileSaverObserver: Observer<FileSaver, FileSaverObserver>, Mac D1/D0 0x1afed0/0x1aff00, slots
// 5-7 pure.
// Every method and member name is inferred except the class names (RTTI), the strings and
// ArchiveFileSaver::UpdateType (RTTI of its BasicArray).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Notifier.h"
#include "Updater.h"
#include "Path.h"

namespace LEGO {

class FileSaver;
class Directory;

class FileSaverObserver : public Observer<FileSaver, FileSaverObserver> {
public:
    FileSaverObserver() {}
    virtual ~FileSaverObserver() {}                                          // Mac 0x1afed0/0x1aff00

    // `done` of `total` steps (files) saved.
    virtual void OnSaveProgress(UInt32 done, UInt32 total) = 0;              // 5
    virtual void OnSaveFailed() = 0;                                         // 6
    // Finished; `files` = the number of files saved (ArchiveFileSaver: 0, see its Update).
    virtual void OnSaveFinished(UInt32 files) = 0;                           // 7
};

class FileSaver : public UpdateObserver, public Notifier<FileSaverObserver> {
public:
    FileSaver() {}                                                           // inline (C2 Mac 0x1b1150)
    virtual ~FileSaver() {}                                                  // Mac 0x1b1d70/0x1b1f00, Win sdd 0x688209

    // One pass: reports 1 of 1, leaves the updater and reports one file saved. Mac 0x1b1710,
    // Win 0x68817c
    virtual void Update();
    // Reports 0 of 1 and registers with the main updater. Mac 0x1b1200, Win 0x688228
    virtual void Start();

protected:
    // The observers leave, then this leaves the updater. Mac 0x1b1c20
    virtual void OnFinalRelease();

    // (inlined notification loops)
    void NotifyProgress(UInt32 done, UInt32 total);
    void NotifyFailed();
    void NotifyFinished(UInt32 files);
};

class ArchiveFileSaver : public FileSaver {
public:
    // What Update does with one file (RTTI LEGO::ArchiveFileSaver::UpdateType; values from
    // Update and BricksPatchFileSaver::CollectUpdates, names inferred).
    enum UpdateType {
        kAdd       = 0,   // copy; traces when the file already exists
        kOverwrite = 1,   // copy; traces when the file does not exist
        kCopy      = 2,   // copy
        kRemove    = 3    // delete from the destination; traces when it does not exist
    };
    typedef Pair<UpdateType, BasicArray<String> > UpdateEntry;   // (type, path components)

    // `destination` = where the files go, `archive` = the downloaded file (opened as a
    // directory). Mac 0x1ac830 (C2 0x1accb0), Win 0x688eeb
    ArchiveFileSaver(Directory* destination, const Path& archive);
    virtual ~ArchiveFileSaver() {}                                           // Mac 0x1ae350/0x1ae470, Win sdd 0x689575

    // One update per pass, then (all done) leaves the updater, drops the directories and
    // reports. A failed copy / delete fails the save. Mac 0x1ad280, Win 0x689214
    virtual void Update();
    // Collects the updates (when the archive opened), FileSaver::Start, reports 0 of n.
    // Mac 0x1acf40, Win 0x688b6a
    virtual void Start();
    // Every file below `directory` (recursively) as a kCopy update; `prefix` = the components
    // of `directory` below the archive root. Mac 0x1ae5d0, Win 0x6895b3
    virtual void CollectUpdates(Directory* directory, const BasicArray<String>& prefix);   // 7

protected:
    // Leaves the updater, drops both directories and the updates (shared end of Update).
    void Finish();

    // Mac offsets (FileSaver 0x2c)
    RefPointer<Directory>       m_archive;       // +0x2c the archive root (null: not an archive)
    RefPointer<Directory>       m_destination;   // +0x30
    BasicArray<UpdateEntry>     m_updates;       // +0x34
    UInt32                      m_current;       // +0x44 next update
                                                 // Object +0x48 (sizeof 0x50)
};

class BricksPatchFileSaver : public ArchiveFileSaver {
public:
    // The archive root becomes <archive>/DB. Mac 0x1aeda0, Win 0x689075
    BricksPatchFileSaver(Directory* destination, const Path& archive);
    virtual ~BricksPatchFileSaver() {}                                       // Mac 0x1afc30/0x1afd50, Win sdd 0x689594

    // Without "update.xml" in the archive root: ArchiveFileSaver::CollectUpdates. With it: the
    // <add> files as kAdd, <overwrite> as kOverwrite, <remove> as kRemove (paths split at '/');
    // `directory` and `prefix` are not used then. Mac 0x1af230, Win 0x689724
    virtual void CollectUpdates(Directory* directory, const BasicArray<String>& prefix);
};

class SavingFileSaver : public FileSaver {
public:
    // Copies `source` to `name` in `directory`; reports 0 of 1 and registers with the main
    // updater at once. Mac 0x1b5480, Win (inlined in 0x688933)
    SavingFileSaver(const String& name, Directory* directory, const Path& source);
    virtual ~SavingFileSaver() {}                                            // Mac 0x1b6860/0x1b6980, Win sdd 0x688676

    // Mac 0x1b5ee0, Win 0x688695
    virtual void Update();

protected:
    Path m_source;   // Mac +0x2c
    Path m_target;   // Mac +0x44
                     // Object +0x5c (sizeof 0x64)
};

} // namespace LEGO
