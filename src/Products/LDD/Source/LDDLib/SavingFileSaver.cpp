// LEGO Digital Designer — restored source
// TU 101 "SavingFileSaver.cpp" (Mac 0x1b53f0-0x1b6b1f, Win 0x688676-0x68899f): SavingFileSaver
// and SavingFileDownloader::CreateSaver. See FileSaver.h / FileDownloader.h.
// Also emitted here (Mac): the SavingFileDownloader destructors and thunks (0x1b5a50-0x1b5c70),
// a weak Path copy (0x1b6740).
#include "FileSaver.h"
#include "FileDownloader.h"

#include "Directory.h"

#include <iostream>

namespace LEGO {

// Mac 0x1b53f0, Win 0x688933
RefPointer<FileSaver> SavingFileDownloader::CreateSaver()
{
    m_savedName = m_fileName;
    return new SavingFileSaver(m_fileName, m_directory.Get(), m_path);
}

// Mac 0x1b5480
SavingFileSaver::SavingFileSaver(const String& name, Directory* directory, const Path& source)
{
    m_source.Assign(source);
    m_target.Assign(Path(name, directory));
    // NOTE (both builds): reports and registers here, before any observer is attached; Start
    // (FileDownloader::OnResponse) does both again.
    NotifyProgress(0, 1);
    RefPointer<Updater> updater = MainUpdater::GetInstance();
    Attach(updater.Get());
}

// Mac 0x1b5ee0, Win 0x688695
void SavingFileSaver::Update()
{
    if (!m_source.Exists()) {
        Detach();
        NotifyFailed();
        return;
    }
    {
        RefPointer<Directory> sourceDirectory = m_source.GetParentDirectory();
        RefPointer<Directory> targetDirectory = m_target.GetParentDirectory();
        // NOTE (both builds): the result of the copy is not checked.
        sourceDirectory->Copy(m_source.GetName(), *targetDirectory, m_target.GetName(), true, true);
    }
    NotifyProgress(1, 1);
    Detach();
    NotifyFinished(1);
}

} // namespace LEGO
