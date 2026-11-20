// LEGO Digital Designer — restored source
// LEGO::FileDownloader — downloads one URL (GET, LDDWebObserver) into memory, wraps the bytes in
//   an in-memory File and hands it to a FileSaver (CreateSaver) that stores it; reports to its
//   FileDownloaderObservers. Home TU 099 "FileDownloader.cpp" (Mac 0x1b1150-0x1b45ff, Win
//   0x6879f6-0x688534).
// LEGO::FileDownloaderObserver — Observer<FileDownloader, FileDownloaderObserver>; its vtable
//   and inline destructor are emitted where the observers live (TU 087 Mac 0x1a3b90).
// LEGO::FilesDownloader — a FileDownloaderObserver that downloads a list of (URL, file name)
//   pairs one after the other with downloaders it creates itself; no key function (vtables
//   in every user, e.g. TU 087 Mac 0x717c1c); out-of-line parts in TU 099 (Mac 0x1b41a0,
//   0x1b4200, Start 0x1b4260).
// LEGO::ArchiveFileDownloader / BricksPatchDownloader — TU 098 "ArchiveFileSaver.cpp" (savers:
//   ArchiveFileSaver / BricksPatchFileSaver, FileSaver.h).
// LEGO::SavingFileDownloader — TU 101 "SavingFileSaver.cpp" (saver: SavingFileSaver).
//
// FileDownloader RTTI: vmi {LDDWebObserver +0, FileSaverObserver +0x44, Notifier<FileDownloader-
// Observer> +0x4c}, virtual Object (Mac +0xa4, sizeof 0xac). Mac vtable 0x71bdfc (19 slots):
// LDDWebObserver's 0-13 (OnConnected 5 0x1b20d0, OnResponse 6 0x1b2350, OnProgress 7 0x1b2910,
// OnFailed 8 0x1b2bc0, OnFinalRelease 2 0x1b38c0), the FileSaverObserver overrides 14
// OnSaveProgress 0x1b2e40, 15 OnSaveFailed 0x1b30e0, 16 OnSaveFinished 0x1b33b0 (thunks
// 0x1b30c0 / 0x1b3390 / 0x1b3670), 17 Download 0x1b3690, 18 CreateSaver 0x1b40e0.
// Win: OnConnected 0x687b58, OnResponse 0x6882af, OnProgress 0x687ba9, OnFailed 0x687c08,
// Download 0x687ade, CreateSaver 0x6880f4, ReleaseFile 0x6879f6, GetFileName 0x688535.
// Every method and member name is inferred except the class names (RTTI) and the strings.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "AutoPointer.h"
#include "Notifier.h"
#include "Path.h"
#include "LDDWebObserver.h"
#include "FileSaver.h"

namespace LEGO {

class FileDownloader;
class Directory;

// Mac vtable 0x717c9c (10 slots: D1 0x1a3b90, D0 0x1a3bc0, OnFinalRelease 0x1a3c10, OnAttached
// 0x1a3c60, OnDetached 0x1a3c70, 5-9 pure); Win {OnAttached, OnDetached, 5 × _purecall}.
class FileDownloaderObserver : public Observer<FileDownloader, FileDownloaderObserver> {
public:
    FileDownloaderObserver() {}
    virtual ~FileDownloaderObserver() {}                                     // Mac 0x1a3b90/0x1a3bc0, Win sdd 0x6424c6

    // The connection is up.
    virtual void OnDownloadStarted() = 0;                                    // 5
    // Downloaded and saved; `files` = what the saver reported (FileSaverObserver::OnSaveFinished).
    virtual void OnDownloadFinished(UInt32 files) = 0;                       // 6
    // `received` of `total` bytes.
    virtual void OnDownloadProgress(UInt32 received, UInt32 total) = 0;      // 7
    // `done` of `total` files saved (FileSaverObserver::OnSaveProgress).
    virtual void OnDownloadFileProgress(UInt32 done, UInt32 total) = 0;      // 8
    virtual void OnDownloadFailed() = 0;                                     // 9
};

class FileDownloader : public LDDWebObserver, public FileSaverObserver,
                       public Notifier<FileDownloaderObserver> {
public:
    // Inline in the original (Mac copy 0x1a2320 in TU 087, Win 0x644764): a GET with the 30 s
    // timeout.
    FileDownloader() : LDDWebObserver(false, 30000) {}
    virtual ~FileDownloader() {}                                             // Mac 0x1b3c50/0x1b3e70

    // ---- LDDWebObserver ----
    // Mac 0x1b20d0, Win 0x687b58
    virtual void OnConnected();
    // The body (`size` bytes) as an in-memory File, then saved by CreateSaver's saver. Fails
    // when the size does not match or is 0, no File can be made or there is no saver.
    // Mac 0x1b2350, Win 0x6882af
    virtual void OnResponse(UInt32 size);
    // Mac 0x1b2910, Win 0x687ba9
    virtual void OnProgress(UInt32 received, UInt32 total);
    // Mac 0x1b2bc0, Win 0x687c08
    virtual void OnFailed();

    // ---- FileSaverObserver ----
    virtual void OnSaveProgress(UInt32 done, UInt32 total);                  // 14 Mac 0x1b2e40
    virtual void OnSaveFailed();                                             // 15 Mac 0x1b30e0
    virtual void OnSaveFinished(UInt32 files);                               // 16 Mac 0x1b33b0

    // Drops the last file and requests `url`; the file is called `fileName`.
    // Mac 0x1b3690 (17), Win 0x687ade
    virtual void Download(const String& url, const String& fileName);
    // A new saver of the downloaded file (m_path). Mac 0x1b40e0 (18), Win 0x6880f4
    virtual RefPointer<FileSaver> CreateSaver();

    // The downloaded file (an element of an in-memory File). Mac 0x1b38b0
    const Path& GetPath() const { return m_path; }
    // The file name given to Download. Win 0x688535 (inlined on Mac)
    String GetFileName() const { return m_fileName; }
    // Deletes the downloaded file and drops the data. Mac 0x1b3740, Win 0x6879f6
    void ReleaseFile();

protected:
    // The FileDownloaderObservers leave, this leaves the saver, then LDDWebObserver.
    // Mac 0x1b38c0
    virtual void OnFinalRelease();

    void NotifyStarted();
    void NotifyFinished(UInt32 files);
    void NotifyProgress(UInt32 received, UInt32 total);
    void NotifyFileProgress(UInt32 done, UInt32 total);
    void NotifyFailed();

    // Mac offsets
    String                m_fileName;   // +0x70
    Path                  m_path;       // +0x80
    AutoPointer<UInt8>    m_data;       // +0x98 the body (new[] of the size)
    RefPointer<FileSaver> m_saver;      // +0xa0
                                        // Object +0xa4
};

// TU 098: saves the downloaded archive's files into `directory`. Mac sizeof 0xb0.
class ArchiveFileDownloader : public FileDownloader {
public:
    // Mac 0x1ac5e0
    explicit ArchiveFileDownloader(Directory* directory) : m_directory(directory) {}
    virtual ~ArchiveFileDownloader() {}                                      // Mac 0x1aca30/0x1acb70, Win sdd 0x688af4

    // Drop the directory, FileDownloader's handling, then ReleaseFile.
    // Mac 0x1ac6a0 / 0x1ac720 (thunks 0x1ac6e0 / 0x1ac770)
    virtual void OnSaveFailed();
    virtual void OnSaveFinished(UInt32 files);
    // An ArchiveFileSaver of the archive into the directory. Mac 0x1ac7c0, Win 0x6891a4
    virtual RefPointer<FileSaver> CreateSaver();

protected:
    RefPointer<Directory> m_directory;   // Mac +0xa4
};

// TU 098, Mac ctor 0x1aec40 (operator new 0xb0): patches the brick database `database`.
class BricksPatchDownloader : public ArchiveFileDownloader {
public:
    explicit BricksPatchDownloader(Directory* database) : ArchiveFileDownloader(database) {}  // Mac 0x1aec40
    virtual ~BricksPatchDownloader() {}                                      // Mac 0x1aefb0/0x1af0f0, Win sdd 0x688b16

    // A BricksPatchFileSaver. Mac 0x1aed30, Win 0x689505
    virtual RefPointer<FileSaver> CreateSaver();
};

// TU 101, inline ctor Mac 0x1a2830 (operator new 0xc0), Win 0x6448cc: saves into `directory`.
class SavingFileDownloader : public FileDownloader {
public:
    explicit SavingFileDownloader(Directory* directory) : m_directory(directory) {}
    virtual ~SavingFileDownloader() {}                                       // Mac 0x1b5a50/0x1b5c70

    // A SavingFileSaver of the file to its name in the directory (the name is kept).
    // Mac 0x1b53f0, Win 0x688933
    virtual RefPointer<FileSaver> CreateSaver();

protected:
    RefPointer<Directory> m_directory;   // Mac +0xa4
    String                m_savedName;   // Mac +0xa8 (written by CreateSaver, never read)
};

// Mac layout: FileDownloaderObserver (vptr, subject +4), files +0x08, failed +0x18, done
// +0x1c, current +0x20, downloader +0x24, Object +0x28. Inline constructor (zeroes).
// Mac vtable (16 slots): 5 0x1a4230 (empty), 6 0x1b41a0, 7/8 pure, 9 0x1b4200, IsFinished 10
// 0x1a4240, GetFailedCount 11 0x1a4260 (+0x18), GetDoneCount 12 0x1a4270 (+0x1c), 13-15 pure.
class FilesDownloader : public FileDownloaderObserver {
public:
    typedef Pair<String, String> FileEntry;   // (URL directory, file name) (name inferred)

    FilesDownloader() : m_failedCount(0), m_doneCount(0), m_current(0) {}
    virtual ~FilesDownloader() {}                                            // Mac 0x1a4160/0x1a41c0, Win sdd 0x6448ad

    virtual void OnDownloadStarted() {}                                      // 5 Mac 0x1a4230
    // Counts the file and goes on (next file or OnFilesFinished). Mac 0x1b41a0 (TU 099)
    virtual void OnDownloadFinished(UInt32 files);                           // 6
    // Counts the failure and goes on. Mac 0x1b4200 (TU 099), Win 0x68852d
    virtual void OnDownloadFailed();                                         // 9

    virtual bool   IsFinished() const     { return m_doneCount + m_failedCount == m_files.GetSize(); } // 10 Mac 0x1a4240
    virtual UInt32 GetFailedCount() const { return m_failedCount; }          // 11 Mac 0x1a4260
    virtual UInt32 GetDoneCount() const   { return m_doneCount; }            // 12 Mac 0x1a4270
    // Files handled / count (the users also call it with 0 before the first download).
    virtual void OnFilesProgress(UInt32 done, UInt32 count) = 0;             // 13
    virtual void OnFilesFinished() = 0;                                      // 14
    // A new downloader for the next file.
    virtual FileDownloader* CreateDownloader() = 0;                          // 15

    // Downloads file m_current: URL = first + second. Mac 0x1b4260 (TU 099)
    void Start();

protected:
    // After a result: next file, progress, then the next download or OnFilesFinished.
    // (inlined in Mac 0x1b41a0 / 0x1b4200)
    void Next();

    BasicArray<FileEntry>      m_files;         // Mac +0x08
    UInt32                     m_failedCount;   // Mac +0x18
    UInt32                     m_doneCount;     // Mac +0x1c
    UInt32                     m_current;       // Mac +0x20
    RefPointer<FileDownloader> m_downloader;    // Mac +0x24
                                                // Object +0x28
};

} // namespace LEGO
