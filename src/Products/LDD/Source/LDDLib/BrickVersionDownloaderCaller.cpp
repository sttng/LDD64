// LEGO Digital Designer — restored source
// TU 223 "BrickVersionDownloaderCaller.cpp" (Mac 0x267cf0-0x26841f, Win 0x686f09-0x687005). See
// BrickVersionDownloaderCaller.h. Also emitted here (Mac): the destructors 0x268120 / 0x2681c0
// and their thunks (0x268270, 0x268320).
#include "BrickVersionDownloaderCaller.h"

#include "LDDWebCommunicator.h"
#include "Updater.h"

#include <iostream>

namespace LEGO {

// Mac 0x267cf0, Win 0x686f41 (?)
void BrickVersionDownloaderCaller::Start()
{
    String url(LDDWebCommunicator::GetInstance()->GetGeometryURL());
    url << "BrickVersion.xml";
    if (m_downloader.Get())
        MainUpdater::ReleaseLater(m_downloader.Get());
    m_downloader = new FileDownloader();
    Attach(m_downloader.Get());
    m_downloader->Download(url, String("BrickVersion.xml"));
}

// Mac 0x268080
void BrickVersionDownloaderCaller::OnDownloadFinished(UInt32 files)
{
    m_handler->OnBrickVersionsDownloaded(files, m_downloader->GetPath());
}

// Mac 0x2680c0
void BrickVersionDownloaderCaller::OnDownloadProgress(UInt32 received, UInt32 total)
{
    m_handler->OnBrickVersionsProgress(received, total);
}

// Mac 0x2680e0
void BrickVersionDownloaderCaller::OnDownloadFileProgress(UInt32 done, UInt32 total)
{
    m_handler->OnBrickVersionsFileProgress(done, total);
}

// Mac 0x268100
void BrickVersionDownloaderCaller::OnDownloadFailed()
{
    m_handler->OnBrickVersionsFailed();
}

} // namespace LEGO
