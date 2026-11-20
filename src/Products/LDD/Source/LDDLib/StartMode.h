// LEGO Digital Designer — restored source
// LEGO::StartMode — the splash screen / installer that runs before EditMode: shows
//   "Graphics/StartScreen/start-screen.png" with a status text and a progress bar and walks
//   through the start-up steps on the control manager's update passes (each step is scheduled
//   with Goto*, which skips one pass so the new status text is drawn first):
//     1 compatibility-mode warning, then InitializeDesigner (web call; offline → 3)
//     2 patch check (newer LDD on the server: download and launch the installer, quit)
//     3 install bricks: copy the brick database (db.lif, or the unpacked "db" directory with
//       a copy thread) from the Assets directory into the user data directory when the
//       Assets one is newer
//     4 brick version list (web)            5 download brick patches (web)
//     6 install palettes (offline: only when the user palettes directory is empty → 7;
//       online: compare with the server's collections and download)
//     7 copy the palettes from Assets       8 done: TerminateCommand(false)
//   AppMain (Mac 0x13ad50) creates it, Create()s and Enter()s it, runs the application's run
//   loop until a TerminateCommand stops it, then ReleaseResources(), and goes on to EditMode
//   unless ShouldQuit().
// The helper classes of the start mode (TerminateCommand, the copy threads and their update
//   observers, the downloader handlers, InitializeDesignerCallerDelegator) are declared here
//   too (all emitted in StartMode.cpp).
//
// TU 087 "StartMode.cpp" (Mac 0x198e00-0x1a5b7b; Win 0x641c57-0x6488a6, scattered).
// RTTI (Mac): class StartMode : Mode (+0), UpdateObserver (+0x24c),
//   BrickVersionDownloaderHandler (+0x254), InitializeDesignerCallerHandler (+0x258), virtual
//   Object (+0x2ec); sizeof 0x2f4 (operator new in AppMain). Win: UpdateObserver +0x268,
//   handlers +0x274 / +0x278, Object +0x2f4.
// Mac vtable 0x716e7c (57 slots): Mode's 49 overriding OnFinalRelease (2, 0x19af50), Enter
//   (45, 0x19a070), Exit (46, 0x19a4d0); then the primary slots of the overriders of the
//   secondary bases: Update 49 (0x19b250), OnInitializeDesignerProgress 50 (0x19f490),
//   …Succeeded 51 (0x19f390), …Failed 52 (0x19f630), OnBrickVersionsProgress 53 (0x1a0030),
//   …Started 54 (0x1a01d0), …Downloaded 55 (0x1a01f0), …Failed 56 (0x1a0380).
//   Win 0x9d… (44 + handler vftables {0x643c9c, 0x645e34, 0x647536} / {0x643ca7, 0x6943f1,
//   0x645fc1, 0x646040}, UpdateObserver {…, 0x6483d2}).
// Every method and member name is inferred except the class names (RTTI), the strings and
// the preference keys.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "Version.h"
#include "BasicMap.h"
#include "Mode.h"
#include "Updater.h"
#include "Command.h"
#include "ProgressBarThread.h"
#include "ProgressBarControl.h"
#include "Directory.h"
#include "File.h"
#include "Path.h"
#include "PaletteNames.h"
#include "FileDownloader.h"
#include "InitializeDesignerCaller.h"
#include "BrickVersionDownloaderCaller.h"

namespace LEGO {

class StartMode;
class RenderControl;

// ===========================================================================================
// TerminateCommand — stops the application's run loop that AppMain runs for the start mode;
// `quit` decides whether LDD goes on (false) or quits (true). Mac sizeof 0x38: Command +0x00
// (enabled +0x24), start mode +0x28, quit +0x2c, Object +0x30. Mac vtable 0x76d81c {D1
// 0x1a4710, D0 0x1a4780, OnFinalRelease 0x1414d0, Do 0x1a4800, SetEnable, GetEnable,
// UpdateEnable 0x1a4830}; Win 0x9d… {Do 0x644a4f, 0x423a70, 0x423b30, UpdateEnable 0x630465}.
// ===========================================================================================
class TerminateCommand : public Command {
public:
    // (inline; Mac copies in StartMode::Create 0x199b45 / 0x199c02)
    TerminateCommand(StartMode* startMode, bool quit) : m_startMode(startMode), m_quit(quit) {}
    virtual ~TerminateCommand() {}                                           // Mac 0x1a4710/0x1a4780, Win sdd 0x645766

    // Stores `quit` in the start mode and stops the run loop. Mac 0x1a4800, Win 0x644a4f
    virtual void Do();
    virtual void UpdateEnable() { SetEnable(true); }                         // Mac 0x1a4830, Win 0x630465

private:
    StartMode* m_startMode;   // Mac +0x28
    bool       m_quit;        // Mac +0x2c
};

// ===========================================================================================
// CopyFilesPGThread — copies `name` from the Assets directory into `destination`: a file with
// one Directory::Copy (then made writable), a directory recursively file by file with a
// percentage. Member of the PGThreadUpdateObserver (not reference counted itself).
// Mac layout (relative): ProgressBarThread +0x00..+0x78, name +0x78, destination +0x88
// (sizeof 0x8c). Mac vtable 0x717288 {D1 0x1a2910, D0 0x1a2990, OnThreadStart, OnThreadEnd,
// Run 0x1a2a20, OnAttach, OnDetach}; Win 0x9d… {sdd 0x6435f3, …, Run 0x6486ee, …}.
// ===========================================================================================
class CopyFilesPGThread : public ProgressBarThread {
public:
    // (inline; Mac part of the PGThreadUpdateObserver constructor 0x1a31f0)
    CopyFilesPGThread(const String& name, Directory* destination) : m_name(name), m_destination(destination) {}
    virtual ~CopyFilesPGThread() {}                                          // Mac 0x1a2910/0x1a2990, Win 0x642205 / sdd 0x6435f3

protected:
    // Mac 0x1a2a20, Win 0x6486ee
    virtual bool Run();

    // Counts the files below `directory` (false when cancelled or a sub-directory does not
    // open). Mac 0x1a2c70, Win 0x648250
    bool CountFiles(Directory* directory, UInt32& count);
    // Copies every file of `source` into `destination` (made writable), recursively; updates
    // the percentage of `total`. False when cancelled or a sub-directory does not open.
    // Mac 0x1a2e70, Win 0x647fed
    bool CopyFiles(Directory* source, Directory* destination, UInt32& copied, UInt32 total);

    String                m_name;          // Mac +0x78
    RefPointer<Directory> m_destination;   // Mac +0x88
};

// ===========================================================================================
// PGThreadUpdateObserver — runs a CopyFilesPGThread and reports it to the start mode on the
// main updater's passes. Mac sizeof 0xa0: UpdateObserver +0x00 (subject +4), thread +0x08,
// start mode +0x94, Object +0x98. Mac vtable 0x7172b0 {D1 0x1a3410, D0 0x1a34e0, …, Update
// 0x1a32e0, 6-8 pure}; Win {OnAttached, OnDetached, Update 0x6455b9, 3 × _purecall}.
// ===========================================================================================
class PGThreadUpdateObserver : public UpdateObserver {
public:
    // Mac 0x1a31f0 (C2 with VTT)
    PGThreadUpdateObserver(StartMode* startMode, const String& name, Directory* destination)
        : m_thread(name, destination), m_startMode(startMode) {}
    virtual ~PGThreadUpdateObserver() {}                                     // Mac 0x1a3410/0x1a34e0 (body 0x1a3350), Win sdd 0x643b93

    // While the thread runs: OnProgress(percentage, 100); then (detached) OnFinished(100)
    // or OnFailed. Mac 0x1a32e0, Win 0x6455b9
    virtual void Update();                                                   // 5

    virtual void OnFinished(UInt32 maximum) = 0;                             // 6
    virtual void OnProgress(UInt32 value, UInt32 maximum) = 0;               // 7
    virtual void OnFailed() = 0;                                             // 8

    // Attaches to the main updater and starts the thread. Mac 0x19e360
    void Start();

protected:
    CopyFilesPGThread m_thread;      // Mac +0x08
    StartMode*        m_startMode;   // Mac +0x94
};

// Copies the brick database. Mac vtable 0x717324 {…, 0x1a35d0, 0x1a3610, 0x1a36e0}; Win
// {…, 0x6455b9, 0x646c57, 0x643d40, 0x643747}.
class CopyBricksPGThreadUpdateObserver : public PGThreadUpdateObserver {
public:
    CopyBricksPGThreadUpdateObserver(StartMode* startMode, const String& name, Directory* destination)
        : PGThreadUpdateObserver(startMode, name, destination) {}
    virtual ~CopyBricksPGThreadUpdateObserver() {}                           // Mac 0x1a3700/0x1a37d0, Win sdd 0x643bb5

    virtual void OnFinished(UInt32 maximum);                                 // Mac 0x1a35d0, Win 0x646c57
    virtual void OnProgress(UInt32 value, UInt32 maximum);                   // Mac 0x1a3610
    virtual void OnFailed();                                                 // Mac 0x1a36e0, Win 0x643747
};

// Copies the palettes. Mac vtable 0x7173ac {…, 0x1a38c0, 0x1a38e0, 0x1a39b0}.
class CopyPalettesPGThreadUpdateObserver : public PGThreadUpdateObserver {
public:
    CopyPalettesPGThreadUpdateObserver(StartMode* startMode, const String& name, Directory* destination)
        : PGThreadUpdateObserver(startMode, name, destination) {}
    virtual ~CopyPalettesPGThreadUpdateObserver() {}                         // Mac 0x1a39d0/0x1a3aa0, Win sdd 0x643bd7

    virtual void OnFinished(UInt32 maximum);                                 // Mac 0x1a38c0, Win 0x646c62
    virtual void OnProgress(UInt32 value, UInt32 maximum);                   // Mac 0x1a38e0
    virtual void OnFailed();                                                 // Mac 0x1a39b0, Win 0x6437e5
};

// ===========================================================================================
// PatchDownloaderHandler — downloads the LDD installer and launches it. Mac sizeof 0x18:
// FileDownloaderObserver (vptr, subject +4), downloader +0x08, start mode +0x0c, Object
// +0x10. Mac vtable 0x7170e8 {D1 0x1a3ea0, D0 0x1a3f40, …, 0x1a0ee0, 0x1a0ef0, 0x1a1000,
// 0x1a1020, 0x1a1030}; Win (0x20 bytes) {…, 0x5c8021, 0x6476ea, 0x643d20, 0x6943f1, 0x646553}.
// ===========================================================================================
class PatchDownloaderHandler : public FileDownloaderObserver {
public:
    explicit PatchDownloaderHandler(StartMode* startMode) : m_startMode(startMode) {}
    virtual ~PatchDownloaderHandler() {}                                     // Mac 0x1a3ea0/0x1a3f40, Win sdd 0x64572a

    // Downloads the installer of the server into `directory` (Mac: deletes LDD.pkg first).
    // Mac 0x19db90, Win 0x646454
    void Start(Directory* directory);

    virtual void OnDownloadStarted() {}                                      // 5 Mac 0x1a0ee0
    // Launches the installer, then quits (PATCHFAILED alert when it does not start).
    // Mac 0x1a0ef0, Win 0x6476ea
    virtual void OnDownloadFinished(UInt32 files);                           // 6
    virtual void OnDownloadProgress(UInt32 received, UInt32 total);          // 7 Mac 0x1a1000, Win 0x643d20
    virtual void OnDownloadFileProgress(UInt32, UInt32) {}                   // 8 Mac 0x1a1020
    virtual void OnDownloadFailed();                                         // 9 Mac 0x1a1030, Win 0x646553

private:
    RefPointer<FileDownloader> m_downloader;   // Mac +0x08 / Win +0x0c
    StartMode*                 m_startMode;    // Mac +0x0c
};

// ===========================================================================================
// BricksDownloaderHandler — downloads the brick database patches of the server (one archive
// per version) and applies them to the user's database (in memory or unpacked "DB").
// Mac sizeof 0x40: FilesDownloader +0x00..+0x28, database +0x28, start mode +0x2c,
// in memory +0x30, user directory +0x34, Object +0x38. Mac vtable 0x717154 {D1 0x1a4360,
// D0 0x1a4420, …, 7 0x1a21d0, 8 0x1a21f0, 9 0x1a2260, 13 0x1a18c0, 14 0x1a1f60, 15 0x1a22b0};
// Win {…, 0x643d28, 0x643b8b, 0x646ab9 …}.
// ===========================================================================================
class BricksDownloaderHandler : public FilesDownloader {
public:
    explicit BricksDownloaderHandler(StartMode* startMode)
        : m_startMode(startMode), m_inMemory(false) {}
    virtual ~BricksDownloaderHandler() {}                                    // Mac 0x1a4360/0x1a4420, Win 0x64409f / sdd 0x64557b

    // One file per version (geometry URL of the server); without versions the step is done.
    // Unless `fullUpdate`, the existing database is prepared for patching first.
    // Mac 0x19f130, Win 0x64655b
    void Start(Directory* userDirectory, const BrickVersionMap& versions, bool fullUpdate);

    virtual void OnDownloadProgress(UInt32 received, UInt32 total);          // 7 Mac 0x1a21d0, Win 0x643d28
    virtual void OnDownloadFileProgress(UInt32 done, UInt32 total);          // 8 Mac 0x1a21f0
    virtual void OnDownloadFailed();                                         // 9 Mac 0x1a2260
    // The first finished archive replaces the database (unpacked "DB" or db.lif in memory).
    virtual void OnFilesProgress(UInt32 done, UInt32 count);                 // 13 Mac 0x1a18c0, Win 0x646654
    // Writes the in-memory db.lif back, then palettes. Mac 0x1a1f60
    virtual void OnFilesFinished();                                          // 14
    // A BricksPatchDownloader of the database (a plain FileDownloader without one).
    virtual FileDownloader* CreateDownloader();                              // 15 Mac 0x1a22b0

protected:
    // The database the patches apply to: db.lif copied into "memory", the unpacked "db"
    // directory, or a new in-memory LIFF. Mac 0x1a11a0, Win 0x64418c
    void PrepareDatabase(Directory* userDirectory);

    RefPointer<Directory> m_database;        // Mac +0x28
    StartMode*            m_startMode;       // Mac +0x2c
    bool                  m_inMemory;        // Mac +0x30
    RefPointer<Directory> m_userDirectory;   // Mac +0x34
};

// ===========================================================================================
// PalettesDownloaderHandler — downloads the palette collections ("<name>.lif") of the server
// into the palettes directory. Mac sizeof 0x38: FilesDownloader, directory +0x28, start mode
// +0x2c, Object +0x30. Mac vtable 0x7171ec {D1 0x1a44c0, D0 0x1a4560, …, 7 0x1a26d0, 8
// 0x1a27a0, 9 0x1a27b0, 13 0x1a2540, 14 0x1a2610, 15 0x1a27e0}.
// ===========================================================================================
class PalettesDownloaderHandler : public FilesDownloader {
public:
    explicit PalettesDownloaderHandler(StartMode* startMode) : m_startMode(startMode) {}
    virtual ~PalettesDownloaderHandler() {}                                  // Mac 0x1a44c0/0x1a4560, Win sdd 0x64559a

    // Mac 0x19e550, Win 0x646ab9
    void Start(Directory* directory, const PaletteNameSet& names);

    virtual void OnDownloadProgress(UInt32 received, UInt32 total);          // 7 Mac 0x1a26d0, Win 0x643d38
    virtual void OnDownloadFileProgress(UInt32, UInt32) {}                   // 8 Mac 0x1a27a0
    virtual void OnDownloadFailed();                                         // 9 Mac 0x1a27b0, Win 0x646c00
    virtual void OnFilesProgress(UInt32 done, UInt32 count);                 // 13 Mac 0x1a2540
    virtual void OnFilesFinished();                                          // 14 Mac 0x1a2610
    virtual FileDownloader* CreateDownloader();                              // 15 Mac 0x1a27e0

protected:
    RefPointer<Directory> m_directory;   // Mac +0x28
    StartMode*            m_startMode;   // Mac +0x2c
};

// ===========================================================================================
// InitializeDesignerCallerDelegator — forwards the call's events to a handler (the start
// mode). Mac sizeof 0xcc: InitializeDesignerCaller +0x00..+0xc0, handler +0xc0, Object +0xc4.
// Mac vtable 0x76d8d8 {D1 0x1a4b10, D0 0x1a4bb0, …, 6 0x1a4bf0, 7 0x1a4c40, 8 0x1a4c80, …};
// Win {…, 0x643957, 0x64398e, 0x6439a9, …}.
// ===========================================================================================
class InitializeDesignerCallerDelegator : public InitializeDesignerCaller {
public:
    // (inline; Mac copy in StartMode 0x19b800)
    explicit InitializeDesignerCallerDelegator(InitializeDesignerCallerHandler* handler) : m_handler(handler) {}
    virtual ~InitializeDesignerCallerDelegator() {}                          // Mac 0x1a4b10/0x1a4bb0, Win sdd 0x643c7a

    // Parses; an error reply fails, else the handler gets the data. Mac 0x1a4bf0, Win 0x643957
    virtual void OnResponse(UInt32 size);
    virtual void OnProgress(UInt32 received, UInt32 total);                  // Mac 0x1a4c40, Win 0x64398e
    virtual void OnFailed();                                                 // Mac 0x1a4c80, Win 0x6439a9

private:
    InitializeDesignerCallerHandler* m_handler;   // Mac +0xc0
};

// ===========================================================================================
// StartMode
// ===========================================================================================
class StartMode : public Mode,
                  public UpdateObserver,
                  public BrickVersionDownloaderHandler,
                  public InitializeDesignerCallerHandler {
public:
    // The steps of Update (m_state). (names inferred)
    enum State {
        kStateNone               = 0,
        kStateInitialize         = 1,
        kStateCheckForPatch      = 2,
        kStateInstallBricks      = 3,
        kStateCheckBrickVersions = 4,
        kStateDownloadBricks     = 5,
        kStateInstallPalettes    = 6,
        kStateCopyPalettes       = 7,
        kStateLaunch             = 8
    };

    // Mac 0x198e00, Win 0x642820 (operator new 0x2f4)
    StartMode(ControlManager* manager, Window* window);
    virtual ~StartMode();                                                    // Mac 0x19a9d0/0x19ac60 (body 0x19a4f0), Win sdd 0x643a07

    // Mode::AttachTo(parent, after), grey background (0.7), the safe-mode tile with a
    // CompatibilityModeLevel, the splash image (648 x 488, centred) with the status, download
    // and file texts and the progress bar (380 above the bottom). Mac 0x199170
    bool Create(Control* parent, Control* after);
    // Releases the downloaders / web callers / copy threads after the run loop. Mac 0x199f60
    void ReleaseResources();
    // True when LDD must quit (start-up failed or the window was closed; set by
    // TerminateCommand, initially true). AppMain goes on to EditMode only when false.
    // Mac 0x19a4e0
    bool ShouldQuit() const;

    // Mode::Enter, then DoServerCall → LDDApplication::SetOnline, the user palettes directory
    // ("userPalettesDirName" in the user data directory, created), CompatibilityModeLevel →
    // the warning, "STARTMODEINITIALIZATION", progress reset, step 1. Mac 0x19a070, Win 0x645785
    virtual void Enter();                                                    // 45
    virtual void Exit();                                                     // 46 Mac 0x19a4d0, Win 0x641c57

    // --- UpdateObserver: runs the scheduled step (see the header comment). Mac 0x19b250
    //     (thunk 0x19d8b0), Win 0x6483d2
    virtual void Update();                                                   // 49

    // --- InitializeDesignerCallerHandler
    virtual void OnInitializeDesignerProgress(UInt32 received, UInt32 total);           // 50 Mac 0x19f490 (copy 0x19f560), Win 0x643c9c
    // The server data go to LDDWebCommunicator, then the patch check. Mac 0x19f390, Win 0x645e34
    virtual void OnInitializeDesignerSucceeded(UInt32 size, const InitializeDesignerData& data); // 51
    // INITIALIZEDESIGNERFAILED: OK = offline, CANCEL = quit, RETRY. Mac 0x19f630, Win 0x647536
    virtual void OnInitializeDesignerFailed();                                          // 52

    // --- BrickVersionDownloaderHandler
    virtual void OnBrickVersionsProgress(UInt32 received, UInt32 total);     // 53 Mac 0x1a0030 (copy 0x1a0100), Win 0x643ca7
    virtual void OnBrickVersionsFileProgress(UInt32 done, UInt32 total);     // 54 Mac 0x1a01d0, Win 0x6943f1 (empty)
    // Keeps the list file open for DownloadBricks. Mac 0x1a01f0, Win 0x645fc1
    virtual void OnBrickVersionsDownloaded(UInt32 files, const Path& file);  // 55
    // BRICKSDOWNLOADFAILED: YES = offline palettes, NO = quit. Mac 0x1a0380, Win 0x646040
    virtual void OnBrickVersionsFailed();                                    // 56

protected:
    // If current, Exit; leaves the updater; Mode's part. Mac 0x19af50 (thunks 0x19afd0-0x19b1d0)
    virtual void OnFinalRelease();

private:
    friend class TerminateCommand;
    friend class CopyBricksPGThreadUpdateObserver;
    friend class CopyPalettesPGThreadUpdateObserver;
    friend class PatchDownloaderHandler;
    friend class BricksDownloaderHandler;
    friend class PalettesDownloaderHandler;

    // ---- the steps ----
    // Step 1/retry: offline → InstallBricks; else the InitializeDesigner call. Mac 0x19b800, Win 0x646d31
    void InitializeDesigner(bool retry);
    void CheckForPatch();                                                    // 2 Mac 0x19baa0, Win 0x646e22
    void InstallBricks();                                                    // 3 Mac 0x19c450, Win 0x64726f
    void CheckBrickVersions();                                               // 4 Mac 0x19c910
    void DownloadBricks();                                                   // 5 Mac 0x19ca00
    void InstallPalettes();                                                  // 6 Mac 0x19cca0
    void CopyPalettes();                                                     // 7 Mac 0x19d5d0

    // Schedule a step: next progress step, status text, state, skip one update pass.
    void GotoCheckForPatch();                                                // Mac 0x19da30, Win 0x6459df
    void GotoInstallBricks();                                                // Mac 0x19d8d0, Win 0x645a78
    void GotoCheckBrickVersions();                                           // Mac 0x19ec40
    void GotoDownloadBricks();                                               // Mac 0x19eda0
    void GotoInstallPalettes();                                              // Mac 0x19e3f0, Win 0x645b11
    void GotoCopyPalettes();                                                 // Mac 0x19e7a0, Win 0x645d7b
    void GotoLaunch();                                                       // Mac 0x19e900
    void Schedule(State state, const char* statusKey);                       // (restoration helper; inlined in the Goto*)

    // ---- failures (alerts) ----
    void OnBricksDownloadFailed();                                           // Mac 0x19ef00, Win 0x64619f
    void OnPatchFailed();                                                    // Mac 0x19fe00, Win 0x645e7a
    void OnPalettesDownloadFailed();                                         // Mac 0x1a08f0, Win 0x6462e6
    void OnInstallBricksFailed();                                            // Mac 0x1a0b20
    void OnCopyPalettesFailed();                                             // Mac 0x1a0d00

    // ---- download texts ----
    // "BYTESRECEIVED" with %s1 / %s2 = MB received / total (two decimals), %s3 = `name`.
    // Mac 0x19f9e0
    void ShowDownloadProgress(UInt32 received, UInt32 total, const String& name);
    // "UPDATEDFILES" with %s1 / %s2 = files, %s3 = `name`. Mac 0x1a06f0
    void ShowFileProgress(UInt32 done, UInt32 total, const String& name);
    void ClearDownloadTexts();                                               // Mac 0x19fd60
    // The text and the sub-step progress. Mac 0x19f8c0 (patch), 0x1a05d0 (bricks)
    void OnPatchDownloadProgress(UInt32 received, UInt32 total);
    void OnBricksDownloadProgress(UInt32 received, UInt32 total);

    // ---- progress bar: steps × step progress × sub-step progress (inlined everywhere) ----
    // Next step of at least `remaining` steps (the step progress restarts).
    void BeginStep(UInt32 remaining);
    void SetStepProgress(UInt32 value, UInt32 maximum);                      // (resets the sub-step)
    void SetSubStepProgress(UInt32 value, UInt32 maximum);
    void UpdateProgressBar();

    // ---- helpers (static) ----
    // info.xml version of Assets/db (zero without). Mac 0x19dfb0
    static Version GetAssetsBrickDatabaseVersion();
    // info.xml version of the user database ("userBrickDBDirName"). Mac 0x19e150, Win 0x643252
    static Version GetUserBrickDatabaseVersion();
#if defined(_WIN32)
    // Opens the downloaded installer `name` of the user data directory. Win 0x642b98
    static bool LaunchInstaller(const String& name);
#else
    // Opens LDD.pkg of the user data directory. Mac 0x19ead0
    static bool LaunchInstaller();
#endif

    // ---- data (Mac / Win offsets) ----
    // Mode +0x000..+0x24c, UpdateObserver +0x24c (subject +0x250) / +0x268,
    // BrickVersionDownloaderHandler +0x254 / +0x274, InitializeDesignerCallerHandler +0x258 / +0x278
    UInt32                                      m_state;                  // +0x25c / +0x27c State
    bool                                        m_skipUpdate;             // +0x260 / +0x280
    bool                                        m_quit;                   // +0x261 / +0x281 (initially true)
    bool                                        m_compatibilityWarning;   // +0x262 / +0x282
    RefPointer<RenderControl>                   m_statusText;             // +0x264 / +0x284
    RefPointer<RenderControl>                   m_downloadText;           // +0x268 / +0x288
    RefPointer<RenderControl>                   m_fileText;               // +0x26c / +0x28c
    RefPointer<RenderControl>                   m_safeModeTile;           // +0x270 / +0x290
    String                                      m_bytesReceivedFormat;    // +0x274 / +0x294 "BYTESRECEIVED"
    String                                      m_updatedFilesFormat;     // +0x284 / +0x298 "UPDATEDFILES"
    RefPointer<TerminateCommand>                m_doneCommand;            // +0x294 / +0x29c quit = false
    RefPointer<TerminateCommand>                m_quitCommand;            // +0x298 / +0x2a0 quit = true
    RefPointer<InitializeDesignerCallerDelegator> m_initializeDesignerCaller; // +0x29c / +0x2a4
    RefPointer<PatchDownloaderHandler>          m_patchHandler;           // +0x2a0 / +0x2a8
    RefPointer<BrickVersionDownloaderCaller>    m_brickVersionCaller;     // +0x2a4 / +0x2ac
    RefPointer<BricksDownloaderHandler>         m_bricksHandler;          // +0x2a8 / +0x2b0
    RefPointer<PalettesDownloaderHandler>       m_palettesHandler;        // +0x2ac / +0x2b4
    RefPointer<PGThreadUpdateObserver>          m_copyObserver;           // +0x2b0 / +0x2b8
    RefPointer<Directory>                       m_palettesDirectory;      // +0x2b4 / +0x2bc
    RefPointer<File>                            m_brickVersionFile;       // +0x2b8 / +0x2c0
    Version                                     m_newestBrickVersion;     // +0x2bc / +0x2c4
    RefPointer<RenderControl>                   m_splash;                 // +0x2cc / +0x2d4
    RefPointer<ProgressBarControl>              m_progressBar;            // +0x2d0 / +0x2d8
    UInt32                                      m_step;                   // +0x2d4 / +0x2dc
    UInt32                                      m_stepCount;              // +0x2d8 / +0x2e0
    UInt32                                      m_stepValue;              // +0x2dc
    UInt32                                      m_stepMaximum;            // +0x2e0
    UInt32                                      m_subStepValue;           // +0x2e4
    UInt32                                      m_subStepMaximum;         // +0x2e8
                                                                          // Object +0x2ec / +0x2f4
};

} // namespace LEGO
