// LEGO Digital Designer — restored source
// LEGO::BrickVersionDownloaderCaller — downloads the brick version list of the server
//   (a FileDownloaderObserver of its own downloader) and reports to its handler. Home TU 223
//   "BrickVersionDownloaderCaller.cpp" (Mac 0x267cf0-0x26841f); the constructor is inline
//   (copy in StartMode, Mac 0x19c910).
// LEGO::BrickVersionDownloaderHandler — the interface StartMode implements (RTTI class; Mac
//   vtable 4 slots, no destructor).
// LEGO::BrickVersionLoaderXML — reads that list (TU 103 "BrickVersionLoaderXML.cpp", Mac
//   0x1b8360-0x1b93ff; <BrickSets><BrickSet version fullUpdate><File name/>…).
//
// Mac layout of the caller: FileDownloaderObserver (vptr, subject +4), downloader +0x08, handler
// +0x0c, Object +0x10 (sizeof 0x18). Mac vtable 0x7371bc {D1 0x268120, D0 0x2681c0, …,
// 5 0x268070, 6 0x268080, 7 0x2680c0, 8 0x2680e0, 9 0x268100}.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicMap.h"
#include "Version.h"
#include "Path.h"
#include "FileDownloader.h"

namespace LEGO {

class File;

class BrickVersionDownloaderHandler {
public:
    // (Mac slots; names inferred; arguments from the calls in Mac 0x268080-0x268100)
    virtual void OnBrickVersionsProgress(UInt32 received, UInt32 total) = 0;          // 0
    // `done` of `total` files saved (FileDownloaderObserver::OnDownloadFileProgress).
    virtual void OnBrickVersionsFileProgress(UInt32 done, UInt32 total) = 0;          // 1
    // The list was saved to `file`; `files` = what the downloader reported (1).
    virtual void OnBrickVersionsDownloaded(UInt32 files, const Path& file) = 0;       // 2
    virtual void OnBrickVersionsFailed() = 0;                                         // 3

protected:
    ~BrickVersionDownloaderHandler() {}
};

class BrickVersionDownloaderCaller : public FileDownloaderObserver {
public:
    explicit BrickVersionDownloaderCaller(BrickVersionDownloaderHandler* handler)
        : m_handler(handler) {}
    virtual ~BrickVersionDownloaderCaller() {}                               // Mac 0x268120/0x2681c0

    virtual void OnDownloadStarted() {}                                      // 5 Mac 0x268070
    // -> handler 2 with the downloaded file. Mac 0x268080, Win 0x686f09 (?)
    virtual void OnDownloadFinished(UInt32 files);                           // 6
    // -> handler 0. Mac 0x2680c0
    virtual void OnDownloadProgress(UInt32 received, UInt32 total);          // 7
    // -> handler 1. Mac 0x2680e0
    virtual void OnDownloadFileProgress(UInt32 done, UInt32 total);          // 8
    // -> handler 3. Mac 0x268100
    virtual void OnDownloadFailed();                                         // 9

    // Downloads <geometry URL>BrickVersion.xml with a new FileDownloader. Mac 0x267cf0,
    // Win 0x686f41 (?)
    void Start();

protected:
    RefPointer<FileDownloader>     m_downloader;   // Mac +0x08
    BrickVersionDownloaderHandler* m_handler;      // Mac +0x0c
};

// version -> archive file name of that brick set version (Mac vtable 0x76ddec).
typedef BasicMap<Version, String, Less<Version> > BrickVersionMap;

// Mac vtable 0x71e2d8 {D1 0x1b8820, D0 0x1b8830, Load 0x1b8360}; used as a stack object.
class BrickVersionLoaderXML {
public:
    BrickVersionLoaderXML() {}
    virtual ~BrickVersionLoaderXML() {}                                      // Mac 0x1b8820/0x1b8830, Win 0x641c3a

    // The versions newer than `current` from `file` into `versions`; `fullUpdate` is set when
    // a newer full (not patch) update exists. False when the file does not parse.
    // Mac 0x1b8360, Win 0x686db2
    virtual bool Load(File* file, BrickVersionMap& versions, bool& fullUpdate, Version current);
};

} // namespace LEGO
