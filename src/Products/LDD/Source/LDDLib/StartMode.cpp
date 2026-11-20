// LEGO Digital Designer — restored source
// LEGO::StartMode and its helper classes — see StartMode.h. TU 087 "StartMode.cpp" (Mac
// 0x198e00-0x1a5b7b; Win 0x641c57-0x6488a6 scattered; static initialiser 0x1a5b00 =
// `#include <iostream>`).
#include "StartMode.h"
#include "LDDApplication.h"
#include "LDDUtilities.h"
#include "LDDWebCommunicator.h"
#include "RenderControl.h"
#include "RenderControlManager.h"
#include "ControlManager.h"
#include "Texture.h"
#include "Color.h"
#include "Geometry.h"
#include "Preferences.h"
#include "StringStream.h"
#include "TextLocale.h"
#include "Platform.h"
#include "Updater.h"
#include "Thread.h"
#include "ObjectFactory.h"
#include "LiffStorageDirectory.h"
#include "BrickDatabase.h"

namespace LEGO {

namespace {

// A preference read the way the Mac build inlines it (GetValue, then FromString when the key
// exists; the value keeps its default otherwise). (restoration helper)
template <class T>
void ReadPreference(const char* key, T& value)
{
    RefPointer<Preferences> preferences = Preferences::GetInstance();
    String text;
    if (preferences->GetValue(String(key), text))
        FromString(value, text);
}

// `count` placeholders replaced (String::ReplaceAll 0x28b720 with full ranges).
void ReplacePlaceholder(String& text, const char* placeholder, const String& value)
{
    text.ReplaceAll(String(placeholder), 0, String::npos, value, 0, String::npos);
}

// "<MB>.<two decimals>" of a byte count (Mac 0x19f9e0, inlined twice).
String FormatMegabytes(UInt32 bytes)
{
    String text;
    text << (UInt64)(bytes >> 20) << ".";
    const UInt64 hundredths = ((UInt64)bytes * 100) >> 20;
    text << (hundredths % 100) / 10 << hundredths % 10;
    return text;
}

Render* GetRender(ControlManager* manager)
{
    return static_cast<RenderControlManager*>(manager)->GetRender();
}

// (not in the original; M3) Puts the complete copy `temporary` of `directory` in place of
// `name`: the old element is renamed aside first and renamed back when the new one cannot be
// moved in; it is deleted only once the new one is in place. On failure the temporary copy is
// removed and `name` is what it was. Directory has no locked public rename, so the DoRename slot
// is called directly (native rename on MacOSXDirectory, checked copy + delete on Win32Directory);
// the start mode does this on the main thread while nothing else holds these entries.
bool ReplaceWithCopy(Directory* directory, const String& temporary, const String& name)
{
    String backup(name);
    backup << ".old";
    directory->Delete(backup, true);
    const bool hadOld = directory->Exists(name);
    bool replaced = !hadOld || directory->DoRename(name, backup);
    if (replaced && !directory->DoRename(temporary, name)) {
        replaced = false;
        if (hadOld) {
            directory->Delete(name, true);
            directory->DoRename(backup, name);
        }
    }
    if (!replaced) {
        directory->Delete(temporary, true);
        return false;
    }
    if (hadOld)
        directory->Delete(backup, true);
    return true;
}

} // namespace

// ===========================================================================================
// TerminateCommand
// ===========================================================================================

// Mac 0x1a4800, Win 0x644a4f
void TerminateCommand::Do()
{
    m_startMode->m_quit = m_quit;
    Application::GetInstance()->StopRunLoop(true);
}

// ===========================================================================================
// CopyFilesPGThread
// ===========================================================================================

// Mac 0x1a2a20, Win 0x6486ee
bool CopyFilesPGThread::Run()
{
    RefPointer<Directory> assets =
        Platform::GetInstance()->GetSystemDirectory(Platform::kAssetsDirectory, true);
    if (assets.IsNull())
        return false;
    bool result;
    if (assets->FileExists(m_name)) {
        // NOTE: the copy result is not checked (both builds); fixed: a failed copy fails the
        // thread (the observer's failure alert) (M3).
        result = assets->Copy(m_name, *m_destination, String(), true, false);
        Attributes attributes;
        m_destination->GetAttributes(m_name, &attributes);
        attributes.m_writable = true;
        m_destination->SetAttributes(m_name, attributes);
    } else {
        RefPointer<Directory> source = assets->GetDirectory(m_name, false);
        RefPointer<Directory> destination = m_destination->GetDirectory(m_name, true);
        result = false;
        if (source.Get() && destination.Get()) {
            UInt32 count = 0;
            if (!CountFiles(source, count)) {
                result = false;
            } else if (count == 0) {
                result = true;
            } else {
                UInt32 copied = 0;
                const bool copiedAll = CopyFiles(source, destination, copied, count);
                result = copiedAll && copied == count;
            }
        }
    }
    return result;
}

// Mac 0x1a2c70, Win 0x648250
bool CopyFilesPGThread::CountFiles(Directory* directory, UInt32& count)
{
    DescriptorSet entries;
    directory->Enumerate(String("*"), entries);
    for (DescriptorSet::ConstIterator it = entries.Begin(); it != entries.End(); ++it) {
        if (m_cancelled)
            return false;
        const String name(it->m_name);
        if (!it->m_attributes.m_isDirectory) {
            ++count;
            continue;
        }
        RefPointer<Directory> subdirectory = directory->GetDirectory(name, false);
        if (subdirectory.IsNull() || !CountFiles(subdirectory, count))
            return false;
    }
    return true;
}

// Mac 0x1a2e70, Win 0x647fed
bool CopyFilesPGThread::CopyFiles(Directory* source, Directory* destination, UInt32& copied, UInt32 total)
{
    DescriptorSet entries;
    source->Enumerate(String("*"), entries);
    for (DescriptorSet::ConstIterator it = entries.Begin(); it != entries.End(); ++it) {
        if (m_cancelled)
            return false;
        const String name(it->m_name);
        if (!it->m_attributes.m_isDirectory) {
            // NOTE: the original ignores the copy result and counts the file as copied, so a
            // failed copy still reports success; fixed: it fails the thread (M3).
            if (!source->Copy(name, *destination, String(), true, false))
                return false;
            Attributes attributes;
            destination->GetAttributes(name, &attributes);
            attributes.m_writable = true;
            destination->SetAttributes(name, attributes);
            ++copied;
            m_progress = (UInt32)(Int64)((Real32)copied * 100.0f) / total;
            continue;
        }
        RefPointer<Directory> sourceDirectory = source->GetDirectory(name, false);
        RefPointer<Directory> destinationDirectory = destination->GetDirectory(name, true);
        if (sourceDirectory.IsNull() || destinationDirectory.IsNull())
            return false;
        if (!CopyFiles(sourceDirectory, destinationDirectory, copied, total))
            return false;
    }
    return true;
}

// ===========================================================================================
// PGThreadUpdateObserver and the two copy observers
// ===========================================================================================

// Mac 0x19e360
void PGThreadUpdateObserver::Start()
{
    RefPointer<Updater> updater = MainUpdater::GetInstance();
    Attach(updater);
    m_thread.Start(0);
}

// Mac 0x1a32e0, Win 0x6455b9
void PGThreadUpdateObserver::Update()
{
    if (m_thread.IsRunning()) {
        OnProgress(m_thread.GetProgress(), 100);
        return;
    }
    Detach();
    if (m_thread.GetResult())
        OnFinished(100);
    else
        OnFailed();
}

// Mac 0x1a35d0, Win 0x646c57
void CopyBricksPGThreadUpdateObserver::OnFinished(UInt32)
{
    StartMode* startMode = m_startMode;
    if (LDDApplication::GetInstance()->IsOnline())
        startMode->GotoCheckBrickVersions();
    else
        startMode->GotoInstallPalettes();
}

// Mac 0x1a3610
void CopyBricksPGThreadUpdateObserver::OnProgress(UInt32 value, UInt32 maximum)
{
    m_startMode->SetStepProgress(value, maximum);
}

// Mac 0x1a36e0, Win 0x643747
void CopyBricksPGThreadUpdateObserver::OnFailed()
{
    m_startMode->OnInstallBricksFailed();
}

// Mac 0x1a38c0, Win 0x646c62
void CopyPalettesPGThreadUpdateObserver::OnFinished(UInt32)
{
    m_startMode->GotoLaunch();
}

// Mac 0x1a38e0
void CopyPalettesPGThreadUpdateObserver::OnProgress(UInt32 value, UInt32 maximum)
{
    m_startMode->SetStepProgress(value, maximum);
}

// Mac 0x1a39b0, Win 0x6437e5
void CopyPalettesPGThreadUpdateObserver::OnFailed()
{
    m_startMode->OnCopyPalettesFailed();
}

// ===========================================================================================
// PatchDownloaderHandler
// ===========================================================================================

// Mac 0x19db90, Win 0x646454
void PatchDownloaderHandler::Start(Directory* directory)
{
    String url;
#if defined(_WIN32)
    url = LDDWebCommunicator::GetInstance()->GetInstallerURLPC();
#else
    url = LDDWebCommunicator::GetInstance()->GetInstallerURLMac();
#endif
    const UInt32 slash = url.FindLastOf(String("/"), String::npos);
    const String fileName = url.Substring(slash + 1, url.GetLength());
    if (m_downloader.Get())
        MainUpdater::ReleaseLater(m_downloader.Get());
#if !defined(_WIN32)
    directory->Delete(String("LDD.pkg"), true);
#endif
    m_downloader = new ArchiveFileDownloader(directory);
    Attach(m_downloader);
    m_downloader->Download(url, fileName);
}

// Mac 0x1a0ef0, Win 0x6476ea (→ 0x6476b1)
void PatchDownloaderHandler::OnDownloadFinished(UInt32)
{
    StartMode* startMode = m_startMode;
#if defined(_WIN32)
    const String fileName = m_downloader->GetFileName();
#else
    const String unused("");
#endif
    startMode->ClearDownloadTexts();
    startMode->SetStepProgress(1, 1);
#if defined(_WIN32)
    const bool launched = StartMode::LaunchInstaller(fileName);
#else
    const bool launched = StartMode::LaunchInstaller();
#endif
    if (!launched)
        startMode->OnPatchFailed();
    else
        startMode->m_quitCommand->Do();
}

// Mac 0x1a1000, Win 0x643d20
void PatchDownloaderHandler::OnDownloadProgress(UInt32 received, UInt32 total)
{
    m_startMode->OnPatchDownloadProgress(received, total);
}

// Mac 0x1a1030, Win 0x646553
void PatchDownloaderHandler::OnDownloadFailed()
{
    m_startMode->OnPatchFailed();
}

// ===========================================================================================
// BricksDownloaderHandler
// ===========================================================================================

// Mac 0x19f130, Win 0x64655b
void BricksDownloaderHandler::Start(Directory* userDirectory, const BrickVersionMap& versions, bool fullUpdate)
{
    m_userDirectory = userDirectory;
    m_database = 0;
    if (versions.IsEmpty()) {
        m_startMode->SetStepProgress(1, 1);
        m_startMode->GotoInstallPalettes();
        return;
    }
    const String url(LDDWebCommunicator::GetInstance()->GetGeometryURL());
    for (BrickVersionMap::ConstIterator it = versions.Begin(); it != versions.End(); ++it)
        m_files.PushBack(FileEntry(String(url), String(it->second)));
    OnFilesProgress(0, m_files.GetSize());
    if (!fullUpdate)
        PrepareDatabase(userDirectory);
    FilesDownloader::Start();
}

// Mac 0x1a11a0, Win 0x64418c
void BricksDownloaderHandler::PrepareDatabase(Directory* userDirectory)
{
    if (userDirectory->FileExists(String("db.lif"))) {
        // db.lif is patched in memory.
        RefPointer<Directory> memory = Directory::GetRoot()->GetDirectory(String("memory"), false);
        RefPointer<File> file = memory->OpenFile(String("db.lif"), OpenMode(true, true), true, 0);
        const bool created = file.Get() != 0;
        file = 0;
        if (created && m_userDirectory->Copy(String("db.lif"), *memory, String(""), true, false)) {
            m_database = memory->GetDirectory(String("db"), false);
            m_inMemory = true;
        }
        return;
    }
    if (userDirectory->DirectoryExists(String("db"), 0)) {
        // The unpacked database is patched in place.
        m_database = m_userDirectory->GetDirectory(String("db"), false);
        m_inMemory = false;
        return;
    }
    // No database yet: a new LIFF in memory.
    RefPointer<Directory> memory = Directory::GetRoot()->GetDirectory(String("memory"), false);
    BasicArray<String> path;
    path.PushBack(String("db.lif"));
    // NOTE: the original passes the (empty) database member as the arguments' directory.
    Directory::OpenArgs args(m_database.Get(), path, true);
    Object* object = ObjectFactory::GetInstance()->CreateObject(typeid(LiffStorageDirectory), args);
    m_database = object ? dynamic_cast<LiffStorageDirectory*>(object) : 0;
    m_inMemory = true;
}

// Mac 0x1a21d0, Win 0x643d28
void BricksDownloaderHandler::OnDownloadProgress(UInt32 received, UInt32 total)
{
    m_startMode->OnBricksDownloadProgress(received, total);
}

// Mac 0x1a21f0
void BricksDownloaderHandler::OnDownloadFileProgress(UInt32 done, UInt32 total)
{
    m_startMode->ShowFileProgress(done, total, String());
}

// Mac 0x1a2260
void BricksDownloaderHandler::OnDownloadFailed()
{
    m_database = 0;
    m_inMemory = true;
    m_userDirectory = 0;
    m_startMode->OnBricksDownloadFailed();
}

// NOTE: the original goes on after a failure: FilesDownloader::Next downloads the next archive
// (taken for the full database while m_database is still null) and calls OnFilesFinished, which
// copies the in-memory db.lif over the user's one whatever it holds. With UnpackBricks the
// user's "DB" is deleted before the archive is copied into it, so a failed copy leaves no
// database (the alert's YES goes on offline); an in-memory db.lif that cannot be created gives
// no alert at all. Fixed (M3): the unpacked database is copied into "DB.new" and swapped in only
// when complete; every failure keeps the user's database, ends the download (m_current at the
// end, m_userDirectory dropped as the "aborted" mark, the in-memory copy deleted) and shows the
// failure alert once. OnFilesFinished: see there.
// Mac 0x1a18c0, Win 0x646654 (fixed; the original below)
void BricksDownloaderHandler::OnFilesProgress(UInt32 done, UInt32 count)
{
    if (m_userDirectory.IsNull())
        return;   // aborted
    if (done != 0 && m_database.IsNull()) {
        // The first archive is a full database: it replaces the user's one.
        RefPointer<Directory> archive = GetSubject()->GetPath().GetDirectory(false);
        if (archive.IsNull()) {
            // failed below
        } else if (Preferences::GetInstance()->GetBool(String("UnpackBricks"), false)) {
            m_inMemory = false;
            const String temporary("DB.new");
            m_userDirectory->Delete(temporary, true);
            bool copied;
            {
                RefPointer<Directory> fresh = m_userDirectory->GetDirectory(temporary, true);
                copied = fresh.Get() && archive->Copy(String("db/*"), *fresh, String(), false, false);
            }
            if (!copied)
                m_userDirectory->Delete(temporary, true);
            else if (ReplaceWithCopy(m_userDirectory, temporary, String("DB")))
                m_database = m_userDirectory->GetDirectory(String("DB"), false);
        } else {
            m_inMemory = true;
            RefPointer<Directory> memory = Directory::GetRoot()->GetDirectory(String("memory"), false);
            RefPointer<File> file = memory->OpenFile(String("db.lif"), OpenMode(true, true), true, 0);
            const bool created = file.Get() != 0;
            file = 0;
            if (created && archive->Copy(String("db.lif"), *memory, String(), true, true)) {
                archive = 0;
                GetSubject()->ReleaseFile();
                m_database = memory->GetDirectory(String("db"), false);
            }
        }
        if (m_database.IsNull()) {
            if (m_inMemory) {
                RefPointer<Directory> memory = Directory::GetRoot()->GetDirectory(String("memory"), false);
                memory->Delete(String("db.lif"), true);
            }
            m_current = m_files.GetSize();
            m_database = 0;
            m_inMemory = true;
            m_userDirectory = 0;
            m_startMode->OnBricksDownloadFailed();
            return;
        }
    }
    m_startMode->ClearDownloadTexts();
    m_startMode->SetStepProgress(done, count);
}

// Mac 0x1a1f60 (fixed; the original below): the in-memory db.lif is copied to "db.lif.new" and
// swapped in only when complete; a failure keeps the user's db.lif and shows the download
// failure alert instead of going on to the palettes; nothing after an abort.
void BricksDownloaderHandler::OnFilesFinished()
{
    m_database = 0;
    if (m_userDirectory.IsNull())
        return;   // aborted: OnBricksDownloadFailed has moved on already
    bool written = true;
    if (m_inMemory) {
        RefPointer<Directory> memory = Directory::GetRoot()->GetDirectory(String("memory"), false);
        const String temporary("db.lif.new");
        m_userDirectory->Delete(temporary, true);
        written = memory->Copy(String("db.lif"), *m_userDirectory, temporary, true, false);
        if (!written)
            m_userDirectory->Delete(temporary, true);
        else
            written = ReplaceWithCopy(m_userDirectory, temporary, String("db.lif"));
        memory->Delete(String("db.lif"), true);
    }
    m_inMemory = true;
    m_userDirectory = 0;
    if (!written) {
        m_startMode->OnBricksDownloadFailed();
        return;
    }
    m_startMode->SetStepProgress(1, 1);
    m_startMode->GotoInstallPalettes();
}

// Mac 0x1a22b0
FileDownloader* BricksDownloaderHandler::CreateDownloader()
{
    if (m_database.IsNull())
        return new FileDownloader();
    return new BricksPatchDownloader(m_database);
}

// ===========================================================================================
// PalettesDownloaderHandler
// ===========================================================================================

// Mac 0x19e550, Win 0x646ab9
void PalettesDownloaderHandler::Start(Directory* directory, const PaletteNameSet& names)
{
    if (names.IsEmpty()) {
        m_startMode->SetStepProgress(1, 1);
        m_startMode->GotoLaunch();
        return;
    }
    m_directory = directory;
    const String url(LDDWebCommunicator::GetInstance()->GetCollectionURL());
    for (PaletteNameSet::ConstIterator it = names.Begin(); it != names.End(); ++it) {
        String fileName(it->m_name);
        fileName << ".lif";
        m_files.PushBack(FileEntry(String(url), String(fileName)));
    }
    OnFilesProgress(0, m_files.GetSize());
    FilesDownloader::Start();
}

// Mac 0x1a26d0, Win 0x643d38
void PalettesDownloaderHandler::OnDownloadProgress(UInt32 received, UInt32 total)
{
    m_startMode->SetSubStepProgress(received, total);
}

// Mac 0x1a27b0, Win 0x646c00
void PalettesDownloaderHandler::OnDownloadFailed()
{
    m_directory = 0;
    m_startMode->OnPalettesDownloadFailed();
}

// Mac 0x1a2540
void PalettesDownloaderHandler::OnFilesProgress(UInt32 done, UInt32 count)
{
    m_startMode->SetStepProgress(done, count);
}

// Mac 0x1a2610
void PalettesDownloaderHandler::OnFilesFinished()
{
    m_directory = 0;
    m_startMode->SetStepProgress(1, 1);
    m_startMode->GotoLaunch();
}

// Mac 0x1a27e0
FileDownloader* PalettesDownloaderHandler::CreateDownloader()
{
    return new SavingFileDownloader(m_directory);
}

// ===========================================================================================
// InitializeDesignerCallerDelegator
// ===========================================================================================

// Mac 0x1a4bf0, Win 0x643957
void InitializeDesignerCallerDelegator::OnResponse(UInt32 size)
{
    InitializeDesignerCaller::OnResponse(size);
    if (HasError()) {
        OnFailed();
        return;
    }
    m_handler->OnInitializeDesignerSucceeded(size, m_data);
}

// Mac 0x1a4c40, Win 0x64398e
void InitializeDesignerCallerDelegator::OnProgress(UInt32 received, UInt32 total)
{
    InitializeDesignerCaller::OnProgress(received, total);
    m_handler->OnInitializeDesignerProgress(received, total);
}

// Mac 0x1a4c80, Win 0x6439a9
void InitializeDesignerCallerDelegator::OnFailed()
{
    InitializeDesignerCaller::OnFailed();
    m_handler->OnInitializeDesignerFailed();
}

// ===========================================================================================
// StartMode
// ===========================================================================================

// Mac 0x198e00, Win 0x642820
StartMode::StartMode(ControlManager* manager, Window* window)
    : Mode(manager, window),
      m_state(kStateNone),
      m_skipUpdate(false),
      m_quit(true),
      m_compatibilityWarning(false),
      m_bytesReceivedFormat(LocalizedString(String("BYTESRECEIVED"))),
      m_updatedFilesFormat(LocalizedString(String("UPDATEDFILES"))),
      m_step(0),
      m_stepCount(1),
      m_stepValue(0),
      m_stepMaximum(1),
      m_subStepValue(0),
      m_subStepMaximum(1)
{
}

// Mac 0x19a9d0 / 0x19ac60 (body 0x19a4f0 with the VTT; thunks 0x19aa30-0x19aed0), Win sdd 0x643a07
StartMode::~StartMode()
{
}

// Mac 0x199170
bool StartMode::Create(Control* parent, Control* after)
{
    if (!AttachTo(parent, after))
        return false;
    SetColor(Color(0.7f, 0.7f, 0.7f, 1.0f));
    ControlManager* manager = m_manager;

    UInt32 compatibilityLevel = 0;
    ReadPreference("CompatibilityModeLevel", compatibilityLevel);
    if (compatibilityLevel != 0) {
        // Safe mode: the whole mode tiled with a pattern.
        m_safeModeTile = new RenderControl(manager);
        m_safeModeTile->Initialize();   // NOTE: the result is not checked
        m_safeModeTile->Setup(this, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
        m_safeModeTile->SetTransparent(true);
        m_safeModeTile->SetMargins(0.0f, 0.0f, 0.0f, 0.0f, 0);
        RefPointer<Texture> tile = LoadApplicationTexture(
            String("Graphics/StartScreen/SafeTile.png"), GetRender(manager));
        if (tile.IsNull())
            return false;
        m_safeModeTile->SetImage(tile, 0);
        m_safeModeTile->SetTileImage(true);
    }

    RefPointer<Texture> image = LoadApplicationTexture(
        String("Graphics/StartScreen/start-screen.png"), GetRender(manager));
    if (image.IsNull())
        return false;

    m_splash = new RenderControl(manager);
    if (!m_splash->Initialize())
        return false;
    m_splash->Setup(this, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    Rect splashRect(Vector2(0.0f, 0.0f), Vector2(648.0f, 488.0f));
    m_splash->SetImage(image, &splashRect);
    m_splash->SetAxisAlignment(0, 2, 2, 0.0f, 0);
    m_splash->SetAxisAlignment(1, 2, 2, 0.0f, 0);
    m_controls.PushBack(RefPointer<Control>(m_splash.Get()));

    m_statusText = new RenderControl(manager);
    if (!m_statusText->Initialize())
        return false;
    m_statusText->Setup(m_splash, 0, 7,
                        Rect(Vector2(0.0f, 100.0f), Vector2(m_splash->GetSize().x, 20.0f)), 0, false);
    m_statusText->SetTransparent(true);
    m_statusText->SetTextAlignment(2, 0);
    m_statusText->SetFont(LDDApplication::GetInstance()->GetFont(11));

    m_downloadText = new RenderControl(manager);
    if (!m_downloadText->Initialize())
        return false;
    m_downloadText->Setup(m_splash, 0, 7, Rect(Vector2(450.0f, 190.0f), Vector2(200.0f, 40.0f)), 0, false);
    m_downloadText->SetTransparent(true);
    m_downloadText->SetTextAlignment(2, 0);
    m_downloadText->SetFont(LDDApplication::GetInstance()->GetFont(11));

    m_fileText = new RenderControl(manager);
    if (!m_fileText->Initialize())
        return false;
    m_fileText->Setup(m_splash, 0, 7, Rect(Vector2(450.0f, 220.0f), Vector2(200.0f, 40.0f)), 0, false);
    m_fileText->SetTransparent(true);
    m_fileText->SetTextAlignment(2, 0);
    m_fileText->SetFont(LDDApplication::GetInstance()->GetFont(11));

    m_quitCommand = new TerminateCommand(this, true);
    m_doneCommand = new TerminateCommand(this, false);

    m_progressBar = new ProgressBarControl(manager);
    if (!m_progressBar->Create(m_splash, 0))
        return false;
    m_progressBar->SetAxisAlignment(0, 2, 2, 0.0f, 0);
    m_progressBar->SetAxisAlignment(1, 1, 1, -380.0f, 0);
    return true;
}

// Mac 0x199f60
void StartMode::ReleaseResources()
{
    m_brickVersionFile = 0;
    m_initializeDesignerCaller = 0;
    m_patchHandler = 0;
    m_brickVersionCaller = 0;
    m_bricksHandler = 0;
    m_palettesHandler = 0;
    m_copyObserver = 0;
}

// Mac 0x19a4e0
bool StartMode::ShouldQuit() const
{
    return m_quit;
}

// Mac 0x19a070, Win 0x645785
void StartMode::Enter()
{
    Mode::Enter();

    bool doServerCall = true;
    ReadPreference("DoServerCall", doServerCall);
    LDDApplication::GetInstance()->SetOnline(doServerCall);

    String palettesName;
    Preferences::GetInstance()->GetValue(String("userPalettesDirName"), palettesName);
    m_palettesDirectory = Application::GetInstance()->GetUserDataDirectory();
    if (m_palettesDirectory.Get())
        m_palettesDirectory = m_palettesDirectory->GetDirectory(palettesName, true);

    UInt32 compatibilityLevel = 0;
    ReadPreference("CompatibilityModeLevel", compatibilityLevel);
    if (compatibilityLevel != 0)
        m_compatibilityWarning = true;

    m_statusText->SetText(LocalizedString(String("STARTMODEINITIALIZATION")));
    m_step = 0;
    m_stepCount = 1;
    m_stepValue = 0;
    m_stepMaximum = 1;
    m_subStepValue = 0;
    m_subStepMaximum = 1;
    UpdateProgressBar();
    m_state = kStateInitialize;
    UpdateObserver::Attach(m_manager);
}

// Mac 0x19a4d0, Win 0x641c57
void StartMode::Exit()
{
    Mode::Exit();
}

// Mac 0x19af50
void StartMode::OnFinalRelease()
{
    if (IsCurrent())
        Exit();
    UpdateObserver::OnFinalRelease();
    Mode::OnFinalRelease();
}

// Mac 0x19b250 (thunk 0x19d8b0), Win 0x6483d2
void StartMode::Update()
{
    if (m_skipUpdate) {
        m_skipUpdate = false;
        return;
    }
    UpdateObserver::Detach();
    const UInt32 state = m_state;
    m_state = kStateNone;
    switch (state) {
    case kStateInitialize:
        if (m_compatibilityWarning) {
            LocalizedString message(String("InSafeModeText"));
            LocalizedString title(String("InSafeModeTitle"));
            LocalizedString ok(String("OK"));
            LocalizedString checkBox(String("DontShowThisMessageAtStartup"));
            LDDApplication::GetInstance()->AlertWithCheckBox(
                0, String("ShowCompatibilityModeWarningAtStartup"), checkBox, message, title, ok,
                String(), String());
        }
        if (m_palettesDirectory.Get()) {
            InitializeDesigner(false);
            return;
        }
        {
            LocalizedString message(String("SETUPERRORGENERIC"));
            LocalizedString title(String("INTERNALERRORTITLE"));
            LocalizedString ok(String("OK"));
            Application::GetInstance()->Alert(2, message, title, ok, String(), String());
            m_quitCommand->Do();
        }
        return;
    case kStateCheckForPatch:
        CheckForPatch();
        return;
    case kStateInstallBricks:
        InstallBricks();
        return;
    case kStateCheckBrickVersions:
        CheckBrickVersions();
        return;
    case kStateDownloadBricks:
        DownloadBricks();
        return;
    case kStateInstallPalettes:
        InstallPalettes();
        return;
    case kStateCopyPalettes:
        CopyPalettes();
        return;
    case kStateLaunch:
        m_doneCommand->Do();
        return;
    default:
        m_quitCommand->Do();
        return;
    }
}

// ---- progress ---------------------------------------------------------------------------------

// (inlined, e.g. Mac 0x19d8d0)
void StartMode::BeginStep(UInt32 remaining)
{
    const UInt32 previous = m_step;
    m_step = previous + 1;
    m_stepCount = previous + remaining;
    if (m_stepCount <= m_step)
        m_stepCount = m_step;
    m_stepValue = 0;
    m_stepMaximum = 1;
    m_subStepValue = 0;
    m_subStepMaximum = 1;
    UpdateProgressBar();
}

// (inlined, e.g. Mac 0x1a3610)
void StartMode::SetStepProgress(UInt32 value, UInt32 maximum)
{
    m_stepValue = value;
    if (maximum <= value)
        maximum = value;
    m_stepMaximum = maximum > 1 ? maximum : 1;
    m_subStepValue = 0;
    m_subStepMaximum = 1;
    UpdateProgressBar();
}

// (inlined, e.g. Mac 0x19f490)
void StartMode::SetSubStepProgress(UInt32 value, UInt32 maximum)
{
    m_subStepValue = value;
    if (maximum <= value)
        maximum = value;
    m_subStepMaximum = maximum > 1 ? maximum : 1;
    UpdateProgressBar();
}

// (inlined everywhere)
void StartMode::UpdateProgressBar()
{
    if (m_step == 0) {
        m_progressBar->SetMaximum(1.0f);
        m_progressBar->SetValue(0.0f);
        return;
    }
    m_progressBar->SetMaximum((Real32)(m_stepCount * m_stepMaximum * m_subStepMaximum));
    m_progressBar->SetValue(
        (Real32)(((m_step - 1) * m_stepMaximum + m_stepValue) * m_subStepMaximum + m_subStepValue));
}

// ---- scheduling ---------------------------------------------------------------------------------

void StartMode::Schedule(State state, const char* statusKey)
{
    m_statusText->SetText(LocalizedString(String(statusKey)));
    m_state = state;
    m_skipUpdate = true;
    UpdateObserver::Attach(m_manager);
}

// Mac 0x19da30, Win 0x6459df
void StartMode::GotoCheckForPatch()
{
    BeginStep(7);
    Schedule(kStateCheckForPatch, "STARTMODECHECKFORPATCH");
}

// Mac 0x19d8d0, Win 0x645a78
void StartMode::GotoInstallBricks()
{
    BeginStep(6);
    Schedule(kStateInstallBricks, "STARTMODEBRICKSINSTALL");
}

// Mac 0x19ec40
void StartMode::GotoCheckBrickVersions()
{
    BeginStep(5);
    Schedule(kStateCheckBrickVersions, "STARTMODEBRICKSINSTALL");
}

// Mac 0x19eda0
void StartMode::GotoDownloadBricks()
{
    BeginStep(4);
    Schedule(kStateDownloadBricks, "STARTMODEBRICKSINSTALL");
}

// Mac 0x19e3f0, Win 0x645b11
void StartMode::GotoInstallPalettes()
{
    BeginStep(3);
    Schedule(kStateInstallPalettes, "STARTMODEPALETTEINSTALL");
}

// Mac 0x19e7a0, Win 0x645d7b
void StartMode::GotoCopyPalettes()
{
    BeginStep(2);
    Schedule(kStateCopyPalettes, "STARTMODEPALETTEINSTALL");
}

// Mac 0x19e900
void StartMode::GotoLaunch()
{
    BeginStep(1);
    SetStepProgress(1, 1);
    Schedule(kStateLaunch, "STARTMODELAUNCHING");
}

// ---- the steps ----------------------------------------------------------------------------------

// Mac 0x19b800, Win 0x646d31
void StartMode::InitializeDesigner(bool retry)
{
    if (!LDDApplication::GetInstance()->IsOnline()) {
        GotoInstallBricks();
        return;
    }
    if (!retry)
        BeginStep(8);
    m_statusText->SetText(LocalizedString(String("STARTMODEINITIALIZATION")));
    if (m_initializeDesignerCaller.Get())
        MainUpdater::ReleaseLater(m_initializeDesignerCaller.Get());
    m_initializeDesignerCaller = new InitializeDesignerCallerDelegator(this);
    m_initializeDesignerCaller->Call();
}

// Mac 0x19baa0, Win 0x646e22
void StartMode::CheckForPatch()
{
    bool forcePatching = false;
    ReadPreference("ForcePatching", forcePatching);
    if (forcePatching) {
        // Once only.
        String value;
        ToString(false, value);
        Preferences::GetInstance()->SetValue(String("ForcePatching"), value);
    }
    bool ignorePatching = false;
    ReadPreference("IgnorePatching", ignorePatching);

    bool patch = forcePatching;
    if (!ignorePatching && !forcePatching) {
        Version version;
        if (!Application::GetInstance()->GetVersion(version)) {
            LocalizedString message(String("SETUPERRORGENERIC"));
            LocalizedString title(String("INTERNALERRORTITLE"));
            LocalizedString ok(String("OK"));
            Application::GetInstance()->Alert(2, message, title, ok, String(), String());
            m_quitCommand->Do();
            return;
        }
        if (m_initializeDesignerCaller->GetLatestVersion() > version) {
            LocalizedString message(String("DOPATCH"));
            String latest;
            ToString(m_initializeDesignerCaller->GetLatestVersion(), latest);
            message.ReplaceAll(String("%s"), latest);
            LocalizedString title(String("DOPATCHTITLE"));
            LocalizedString yes(String("YES"));
            LocalizedString no(String("NO"));
            if (Application::GetInstance()->Alert(0, message, title, yes, no, String()) == 0)
                patch = true;
            else
                LDDApplication::GetInstance()->SetOnline(false);
        }
    }
    if (!patch) {
        GotoInstallBricks();
        return;
    }

    m_statusText->SetText(LocalizedString(String("STARTMODEDOWNLOADINGPATCH")));
    BeginStep(1);
    RefPointer<Directory> user = Application::GetInstance()->GetUserDataDirectory();
    if (m_patchHandler.Get())
        MainUpdater::ReleaseLater(m_patchHandler.Get());
    m_patchHandler = new PatchDownloaderHandler(this);
    m_patchHandler->Start(user);
}

// Mac 0x19c450, Win 0x64726f
void StartMode::InstallBricks()
{
    const Version assetsVersion = GetAssetsBrickDatabaseVersion();
    const Version userVersion = GetUserBrickDatabaseVersion();
    if (!(assetsVersion > userVersion)) {
        if (LDDApplication::GetInstance()->IsOnline())
            GotoCheckBrickVersions();
        else
            GotoInstallPalettes();
        return;
    }

    String databaseName;
    Preferences::GetInstance()->GetValue(String("userBrickDBDirName"), databaseName);
    RefPointer<Directory> user = Application::GetInstance()->GetUserDataDirectory();
    bool unpackBricks = false;
    ReadPreference("UnpackBricks", unpackBricks);

    String fileName(databaseName);
    fileName << ".lif";
    RefPointer<Directory> assets =
        Platform::GetInstance()->GetSystemDirectory(Platform::kAssetsDirectory, true);
    if (!unpackBricks && assets.Get() && assets->FileExists(fileName)) {
        // The packed database: one copy on the main thread.
        // NOTE: the original copies straight over the user's db.lif and ignores the result: a
        // copy that fails part way leaves a truncated database and the start goes on with it;
        // fixed: copied to "<name>.lif.new", made writable, then swapped in; any failure keeps
        // the old database and ends in the INSTALLAPPBRICKSFAILED alert (M3).
        String temporary(fileName);
        temporary << ".new";
        user->Delete(temporary, true);
        bool installed = assets->Copy(fileName, *user, temporary, true, false);
        if (installed) {
            Attributes attributes;
            installed = user->GetAttributes(temporary, &attributes);
            attributes.m_writable = true;
            installed = installed && user->SetAttributes(temporary, attributes);
        }
        if (!installed)
            user->Delete(temporary, true);
        else
            installed = ReplaceWithCopy(user, temporary, fileName);
        if (!installed) {
            OnInstallBricksFailed();
            return;
        }
        if (LDDApplication::GetInstance()->IsOnline())
            GotoCheckBrickVersions();
        else
            GotoInstallPalettes();
        return;
    }
    // The unpacked database directory: copied by a thread.
    m_copyObserver = new CopyBricksPGThreadUpdateObserver(this, databaseName, user);
    m_copyObserver->Start();
}

// Mac 0x19c910
void StartMode::CheckBrickVersions()
{
    if (m_brickVersionCaller.Get())
        MainUpdater::ReleaseLater(m_brickVersionCaller.Get());
    m_brickVersionCaller = new BrickVersionDownloaderCaller(this);
    m_brickVersionCaller->Start();
}

// Mac 0x19ca00
void StartMode::DownloadBricks()
{
    BrickVersionMap versions;
    const Version userVersion = GetUserBrickDatabaseVersion();
    bool fullUpdate = false;
    BrickVersionLoaderXML loader;
    if (!loader.Load(m_brickVersionFile, versions, fullUpdate, userVersion)) {
        OnBricksDownloadFailed();
        return;
    }
    m_newestBrickVersion = userVersion;
    if (!versions.IsEmpty()) {
        BrickVersionMap::ConstIterator last = versions.End();
        --last;
        m_newestBrickVersion = last->first;
    }
    if (m_bricksHandler.Get())
        MainUpdater::ReleaseLater(m_bricksHandler.Get());
    RefPointer<Directory> user = Application::GetInstance()->GetUserDataDirectory();
    m_bricksHandler = new BricksDownloaderHandler(this);
    m_bricksHandler->Start(user, versions, fullUpdate);
}

// Mac 0x19cca0
void StartMode::InstallPalettes()
{
    if (!LDDApplication::GetInstance()->IsOnline()) {
        // Offline: the shipped palettes, unless the user already has some.
        DescriptorSet entries;
        m_palettesDirectory->Enumerate(String("*"), entries);
        if (entries.IsEmpty())
            GotoCopyPalettes();
        else
            GotoLaunch();
        return;
    }

    DescriptorSet entries;
    m_palettesDirectory->Enumerate(String("*"), entries);
    // The server's collections (Mac 0x135300 returns them as PaletteNames).
    PaletteNames download;
    static_cast<PaletteNameSet&>(download) = LDDWebCommunicator::GetInstance()->GetCollections();
    // The palettes of the last start-up …
    PaletteNames installed;
    {
        String text;
        if (Preferences::GetInstance()->GetValue(String("PaletteNames"), text))
            FromString(installed, text);
    }
    // … that still exist as files …
    PaletteNames present;
    for (PaletteNameSet::ConstIterator it = installed.Begin(); it != installed.End(); ++it) {
        String fileName(it->m_name);
        fileName << ".lif";
        if (entries.Contains(Descriptor(fileName, Attributes(), Times(), 0)))
            present.Insert(*it);
    }
    // … and are what the server has are kept; the rest is downloaded.
    installed.Clear();
    for (PaletteNameSet::ConstIterator it = present.Begin(); it != present.End(); ++it) {
        PaletteNameSet::Iterator found = download.Find(*it);
        if (found != download.End()) {
            download.Erase(found);
            installed.Insert(*it);
        }
    }
    // Every other entry of the palettes directory goes.
    for (DescriptorSet::ConstIterator entry = entries.Begin(); entry != entries.End(); ++entry) {
        const String name(entry->m_name);
        bool keep = false;
        for (PaletteNameSet::ConstIterator it = installed.Begin(); it != installed.End(); ++it) {
            String fileName(it->m_name);
            fileName << ".lif";
            if (name.EqualsNoCase(fileName)) {
                keep = true;
                break;
            }
        }
        if (!keep)
            m_palettesDirectory->Delete(name, true);
    }
    if (m_palettesHandler.Get())
        MainUpdater::ReleaseLater(m_palettesHandler.Get());
    m_palettesHandler = new PalettesDownloaderHandler(this);
    m_palettesHandler->Start(m_palettesDirectory, download);
}

// Mac 0x19d5d0
void StartMode::CopyPalettes()
{
    DescriptorSet entries;
    m_palettesDirectory->Enumerate(String("*"), entries);
    for (DescriptorSet::ConstIterator it = entries.Begin(); it != entries.End(); ++it)
        m_palettesDirectory->Delete(String(it->m_name), false);
    const String name = m_palettesDirectory->GetPath(false);
    RefPointer<Directory> parent = m_palettesDirectory->GetParent();
    m_copyObserver = new CopyPalettesPGThreadUpdateObserver(this, name, parent);
    m_copyObserver->Start();
}

// ---- InitializeDesignerCallerHandler ----------------------------------------------------------

// Mac 0x19f490 (copy 0x19f560 for the handler base), Win 0x643c9c
void StartMode::OnInitializeDesignerProgress(UInt32 received, UInt32 total)
{
    SetSubStepProgress(received, total);
}

// Mac 0x19f390 (thunk 0x19f460), Win 0x645e34
void StartMode::OnInitializeDesignerSucceeded(UInt32, const InitializeDesignerData& data)
{
    SetStepProgress(1, 1);
    if (m_initializeDesignerCaller->HasError()) {
        OnInitializeDesignerFailed();
        return;
    }
    LDDWebCommunicator::GetInstance()->SetInitializeDesignerData(data);
    GotoCheckForPatch();
}

// Mac 0x19f630 (thunk 0x19f8a0), Win 0x647536
void StartMode::OnInitializeDesignerFailed()
{
    LocalizedString message(String("INITIALIZEDESIGNERFAILED"));
    LocalizedString title(String("INITIALIZEDESIGNERFAILEDTITLE"));
    LocalizedString ok(String("OK"));
    LocalizedString cancel(String("CANCEL"));
    LocalizedString retry(String("RETRY"));
    const UInt32 button = Application::GetInstance()->Alert(1, message, title, ok, cancel, retry);
    if (button == 0) {
        LDDApplication::GetInstance()->SetOnline(false);
        GotoInstallBricks();
    } else if (button == 1) {
        m_quitCommand->Do();
    } else if (button == 2) {
        InitializeDesigner(true);
    }
}

// ---- BrickVersionDownloaderHandler ------------------------------------------------------------

// Mac 0x1a0030 (copy 0x1a0100), Win 0x643ca7
void StartMode::OnBrickVersionsProgress(UInt32 received, UInt32 total)
{
    SetSubStepProgress(received, total);
}

// Mac 0x1a01d0 (thunk 0x1a01e0), Win 0x6943f1
void StartMode::OnBrickVersionsFileProgress(UInt32, UInt32)
{
}

// Mac 0x1a01f0 (thunk 0x1a0350), Win 0x645fc1
void StartMode::OnBrickVersionsDownloaded(UInt32, const Path& file)
{
    SetStepProgress(1, 1);
    m_brickVersionFile = 0;
    if (file.Exists())
        m_brickVersionFile = file.OpenFile(OpenMode(true, false, true, false), false);
    GotoDownloadBricks();
}

// Mac 0x1a0380 (thunk 0x1a05b0), Win 0x646040
void StartMode::OnBrickVersionsFailed()
{
    OnBricksDownloadFailed();
}

// ---- failures -----------------------------------------------------------------------------------

// Mac 0x19ef00 (and the identical 0x1a0380), Win 0x64619f
void StartMode::OnBricksDownloadFailed()
{
    LocalizedString message(String("BRICKSDOWNLOADFAILED"));
    LocalizedString title(String("BRICKSDOWNLOADFAILEDTITLE"));
    LocalizedString yes(String("YES"));
    LocalizedString no(String("NO"));
    if (Application::GetInstance()->Alert(0, message, title, yes, no, String()) == 0) {
        LDDApplication::GetInstance()->SetOnline(false);
        GotoInstallPalettes();
    } else {
        m_quitCommand->Do();
    }
}

// Mac 0x19fe00, Win 0x645e7a
void StartMode::OnPatchFailed()
{
    LocalizedString message(String("PATCHFAILED"));
    LocalizedString title(String("PATCHFAILEDTITLE"));
    LocalizedString yes(String("YES"));
    LocalizedString no(String("NO"));
    if (Application::GetInstance()->Alert(0, message, title, yes, no, String()) == 0) {
        LDDApplication::GetInstance()->SetOnline(false);
        GotoInstallBricks();
    } else {
        m_quitCommand->Do();
    }
}

// Mac 0x1a08f0, Win 0x6462e6
void StartMode::OnPalettesDownloadFailed()
{
    LocalizedString message(String("PALETTEDOWNLOADFAILED"));
    LocalizedString title(String("PALETTEDOWNLOADFAILEDTITLE"));
    LocalizedString yes(String("YES"));
    LocalizedString no(String("NO"));
    if (Application::GetInstance()->Alert(0, message, title, yes, no, String()) == 0) {
        LDDApplication::GetInstance()->SetOnline(false);
        GotoCopyPalettes();
    } else {
        m_quitCommand->Do();
    }
}

// Mac 0x1a0b20
void StartMode::OnInstallBricksFailed()
{
    LocalizedString message(String("INSTALLAPPBRICKSFAILED"));
    LocalizedString title(String("INTERNALERRORTITLE"));
    LocalizedString ok(String("OK"));
    Application::GetInstance()->Alert(2, message, title, ok, String(), String());
    m_quitCommand->Do();
}

// Mac 0x1a0d00
void StartMode::OnCopyPalettesFailed()
{
    LocalizedString message(String("REESTABLISHPALETTESFAILED"));
    LocalizedString title(String("REESTABLISHPALETTESFAILEDTITLE"));
    LocalizedString ok(String("OK"));
    Application::GetInstance()->Alert(2, message, title, ok, String(), String());
    m_quitCommand->Do();
}

// ---- download texts -----------------------------------------------------------------------------

// Mac 0x19f9e0
void StartMode::ShowDownloadProgress(UInt32 received, UInt32 total, const String& name)
{
    String text(m_bytesReceivedFormat);
    ReplacePlaceholder(text, "%s1", FormatMegabytes(received));
    ReplacePlaceholder(text, "%s2", FormatMegabytes(total));
    ReplacePlaceholder(text, "%s3", name);
    m_downloadText->SetText(text);
}

// Mac 0x1a06f0
void StartMode::ShowFileProgress(UInt32 done, UInt32 total, const String& name)
{
    String text(m_updatedFilesFormat);
    String doneText;
    doneText << done;
    ReplacePlaceholder(text, "%s1", doneText);
    String totalText;
    totalText << total;
    ReplacePlaceholder(text, "%s2", totalText);
    ReplacePlaceholder(text, "%s3", name);
    m_fileText->SetText(text);
}

// Mac 0x19fd60
void StartMode::ClearDownloadTexts()
{
    m_fileText->SetText(String());
    m_downloadText->SetText(String());
}

// Mac 0x19f8c0
void StartMode::OnPatchDownloadProgress(UInt32 received, UInt32 total)
{
    ShowDownloadProgress(received, total, String());
    SetSubStepProgress(received, total);
}

// Mac 0x1a05d0
void StartMode::OnBricksDownloadProgress(UInt32 received, UInt32 total)
{
    ShowDownloadProgress(received, total, String());
    SetSubStepProgress(received, total);
}

// ---- helpers ------------------------------------------------------------------------------------

// Mac 0x19dfb0
Version StartMode::GetAssetsBrickDatabaseVersion()
{
    RefPointer<Directory> assets =
        Platform::GetInstance()->GetSystemDirectory(Platform::kAssetsDirectory, true);
    if (assets.IsNull())
        return Version();
    RefPointer<Directory> directory = assets->GetDirectory(String("db"), false);
    if (directory.IsNull())
        return Version();
    RefPointer<BrickDatabase> database = new BrickDatabase(directory);
    return database->GetVersion();
}

// Mac 0x19e150, Win 0x643252
Version StartMode::GetUserBrickDatabaseVersion()
{
    String databaseName;
    Preferences::GetInstance()->GetValue(String("userBrickDBDirName"), databaseName);
    RefPointer<Directory> user = Application::GetInstance()->GetUserDataDirectory();
    if (user.IsNull())
        return Version();
    RefPointer<Directory> directory = user->GetDirectory(databaseName, false);
    if (directory.IsNull())
        return Version();
    RefPointer<BrickDatabase> database = new BrickDatabase(directory);
    return database->GetVersion();
}

#if defined(_WIN32)
// Win 0x642b98
bool StartMode::LaunchInstaller(const String& name)
{
    RefPointer<Directory> user = Application::GetInstance()->GetUserDataDirectory();
    Path path(name, user);
    return Platform::GetInstance()->OpenWithApplication(path, true);
}
#else
// Mac 0x19ead0
bool StartMode::LaunchInstaller()
{
    RefPointer<Directory> user = Application::GetInstance()->GetUserDataDirectory();
    Path path(String("LDD.pkg"), user);
    return Platform::GetInstance()->OpenWithApplication(path, true);
}
#endif

} // namespace LEGO
