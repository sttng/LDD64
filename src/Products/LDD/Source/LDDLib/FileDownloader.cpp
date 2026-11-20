// LEGO Digital Designer — restored source
// TU 099 "FileDownloader.cpp" (Mac 0x1b1150-0x1b45ff, Win 0x6879f6-0x688534): FileSaver,
// FileDownloader and the out-of-line parts of FilesDownloader. See FileSaver.h /
// FileDownloader.h.
//
// Template instances emitted here (restored through the shared templates): Observer / NotifierT
// / Notifier of FileSaver and FileDownloader (Mac 0x1afeb0-0x1b0b00, 0x1b1df0-0x1b2030,
// 0x1b3cb0-0x1b4060), their BasicArray destructors (0x1b1050-0x1b1110), File::WrapArgs
// (0x1b2900 / 0x1b45b0).
#include "FileDownloader.h"
#include "FileSaver.h"

#include "File.h"
#include "ObjectFactory.h"
#include "Updater.h"

#include <iostream>
#include <typeinfo>

namespace LEGO {

// ===========================================================================================
// FileSaver
// ===========================================================================================

void FileSaver::NotifyProgress(UInt32 done, UInt32 total)
{
    Iterator it(*this);
    while (FileSaverObserver* observer = it.Next(m_observers))
        observer->OnSaveProgress(done, total);
}

void FileSaver::NotifyFailed()
{
    Iterator it(*this);
    while (FileSaverObserver* observer = it.Next(m_observers))
        observer->OnSaveFailed();
}

void FileSaver::NotifyFinished(UInt32 files)
{
    Iterator it(*this);
    while (FileSaverObserver* observer = it.Next(m_observers))
        observer->OnSaveFinished(files);
}

// Mac 0x1b1710, Win 0x68817c
void FileSaver::Update()
{
    NotifyProgress(1, 1);
    Detach();
    NotifyFinished(1);
}

// Mac 0x1b1200, Win 0x688228
void FileSaver::Start()
{
    NotifyProgress(0, 1);
    RefPointer<Updater> updater = MainUpdater::GetInstance();
    Attach(updater.Get());
}

// Mac 0x1b1c20
void FileSaver::OnFinalRelease()
{
    NotifierT<FileSaver, FileSaverObserver>::OnFinalRelease();
    UpdateObserver::OnFinalRelease();
}

// ===========================================================================================
// FileDownloader
// ===========================================================================================

void FileDownloader::NotifyStarted()
{
    Notifier<FileDownloaderObserver>::Iterator it(*this);
    while (FileDownloaderObserver* observer = it.Next(Notifier<FileDownloaderObserver>::m_observers))
        observer->OnDownloadStarted();
}

void FileDownloader::NotifyFinished(UInt32 files)
{
    Notifier<FileDownloaderObserver>::Iterator it(*this);
    while (FileDownloaderObserver* observer = it.Next(Notifier<FileDownloaderObserver>::m_observers))
        observer->OnDownloadFinished(files);
}

void FileDownloader::NotifyProgress(UInt32 received, UInt32 total)
{
    Notifier<FileDownloaderObserver>::Iterator it(*this);
    while (FileDownloaderObserver* observer = it.Next(Notifier<FileDownloaderObserver>::m_observers))
        observer->OnDownloadProgress(received, total);
}

void FileDownloader::NotifyFileProgress(UInt32 done, UInt32 total)
{
    Notifier<FileDownloaderObserver>::Iterator it(*this);
    while (FileDownloaderObserver* observer = it.Next(Notifier<FileDownloaderObserver>::m_observers))
        observer->OnDownloadFileProgress(done, total);
}

void FileDownloader::NotifyFailed()
{
    Notifier<FileDownloaderObserver>::Iterator it(*this);
    while (FileDownloaderObserver* observer = it.Next(Notifier<FileDownloaderObserver>::m_observers))
        observer->OnDownloadFailed();
}

// Mac 0x1b20d0, Win 0x687b58
void FileDownloader::OnConnected()
{
    LDDWebObserver::OnConnected();
    NotifyStarted();
}

// Mac 0x1b2350, Win 0x6882af
void FileDownloader::OnResponse(UInt32 size)
{
    if (m_communicator->GetContentLength() != size || size == 0) {
        OnFailed();
        return;
    }
    // NOTE (both builds): the old buffer is freed with scalar delete (AutoPointer).
    m_data.Reset(new UInt8[size]);
    m_communicator->GetContent(m_data.Get());

    File::WrapArgs args(m_data.Get(), size);
    Object* object = ObjectFactory::GetInstance()->CreateObject(typeid(File), args);
    RefPointer<File> file = object ? dynamic_cast<File*>(object) : 0;
    if (file.IsNull()) {
        OnFailed();
        return;
    }
    Path path;
    path.SetElement(file.Get());
    m_path.Assign(path);

    if (m_saver.Get())
        MainUpdater::ReleaseLater(m_saver.Get());
    m_saver = CreateSaver();
    if (m_saver.IsNull()) {
        OnFailed();
        return;
    }
    FileSaverObserver::Attach(m_saver.Get());
    m_saver->Start();
}

// Mac 0x1b2910, Win 0x687ba9
void FileDownloader::OnProgress(UInt32 received, UInt32 total)
{
    LDDWebObserver::OnProgress(received, total);
    NotifyProgress(received, total);
}

// Mac 0x1b2bc0, Win 0x687c08
void FileDownloader::OnFailed()
{
    LDDWebObserver::OnFailed();
    NotifyFailed();
}

// Mac 0x1b2e40 (thunk 0x1b30c0)
void FileDownloader::OnSaveProgress(UInt32 done, UInt32 total)
{
    NotifyFileProgress(done, total);
}

// Mac 0x1b30e0 (thunk 0x1b3390)
void FileDownloader::OnSaveFailed()
{
    SetFailed();
    NotifyFailed();
    MainUpdater::ReleaseLater(m_saver.Get());
    m_saver = 0;
}

// Mac 0x1b33b0 (thunk 0x1b3670)
void FileDownloader::OnSaveFinished(UInt32 files)
{
    SetSucceeded();
    NotifyFinished(files);
    MainUpdater::ReleaseLater(m_saver.Get());
    m_saver = 0;
}

// Mac 0x1b3690, Win 0x687ade
void FileDownloader::Download(const String& url, const String& fileName)
{
    ReleaseFile();
    m_fileName = fileName;
    Request(url, AString());
}

// Mac 0x1b40e0, Win 0x6880f4
RefPointer<FileSaver> FileDownloader::CreateSaver()
{
    return new FileSaver();
}

// Mac 0x1b3740, Win 0x6879f6
void FileDownloader::ReleaseFile()
{
    if (m_path.Exists())
        m_path.Delete(true);
    m_path = Path();
    m_fileName = String();
    m_data.Release();
}

// Mac 0x1b38c0
void FileDownloader::OnFinalRelease()
{
    NotifierT<FileDownloader, FileDownloaderObserver>::OnFinalRelease();
    FileSaverObserver::OnFinalRelease();
    LDDWebObserver::OnFinalRelease();
}

// ===========================================================================================
// FilesDownloader (out-of-line parts)
// ===========================================================================================

void FilesDownloader::Next()
{
    ++m_current;
    OnFilesProgress(m_current, m_files.GetSize());
    if (m_current < m_files.GetSize()) {
        Start();
        return;
    }
    // NOTE (Mac): the call passes m_current as an argument OnFilesFinished does not take.
    OnFilesFinished();
}

// Mac 0x1b41a0
void FilesDownloader::OnDownloadFinished(UInt32)
{
    ++m_doneCount;
    Next();
}

// Mac 0x1b4200, Win 0x68852d
void FilesDownloader::OnDownloadFailed()
{
    ++m_failedCount;
    Next();
}

// Mac 0x1b4260
void FilesDownloader::Start()
{
    const FileEntry& entry = m_files[m_current];
    String url(entry.first);
    url << entry.second;
    if (m_downloader.Get())
        MainUpdater::ReleaseLater(m_downloader.Get());
    m_downloader = CreateDownloader();
    Attach(m_downloader.Get());
    m_downloader->Download(url, entry.second);
}

} // namespace LEGO
