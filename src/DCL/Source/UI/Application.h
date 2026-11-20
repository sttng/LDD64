// LEGO Digital Designer — restored source
// LEGO::Application — the application singleton: command line, run loop and modal loops (Mac:
// LEGOApplicationDelegate's NSTimer loop; Win: GetMessage / PeekMessage pumps), quitting, alerts,
// and the document / about / preferences hooks LDDApplication implements.
// TU 000 "Application.mm" (Mac 0x2030-0x35db, with LEGOApplicationDelegate / LEGOApplication);
// Win twin "Application.cpp" 0x4223d0-0x422f90. Directory UI/ is a guess (next to Window).
//
// Not polymorphic over Object: RTTI "class" without bases. Mac vtable 0x703928 (27 entries:
// D1 0x26a0, D0 0x26b0, slots 2-26 below); Win 0xa1f63c (26 entries: scalar deleting dtor
// 0x4225d0, then Win slot = Mac slot - 1).
// Mac layout (ctor 0x2070): vptr, arguments +0x04, exit result +0x08, in-run-loop +0x09,
// recursive pthread mutex +0x0c (0x2c bytes), String +0x38, application name String +0x48
// (the "LEGO Company/<name>" data directory), sizeof 0x58 (LDDApplication's next base starts
// there). Win (ctor 0x4223d0): arguments +0x04, flags +0x08/+0x09, CRITICAL_SECTION +0x0c,
// String +0x24, name +0x28, BasicSet<HACCEL> +0x2c (Win only).
// Name evidence: Quit = 0x2720 (CanQuit → OnQuit → StopRunLoop(true)); selectors startRunloop /
// stopRunloop / ModalRun:andMUR: / ModalStop / modalWindow; LDDApplication's overrides (CanQuit
// asks QuerySaveChanges, "SAVECHANGESTO" …). Other names are inferred (marked).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "Preferences.h"
#include "BasicMap.h"
#include "Version.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace LEGO {

class Directory;
class Path;
class Window;
class ModalUpdateReciever;

class Application {
public:
    // Mac 0x2070 / Win 0x4223d0: stores the instance (Mac 0x77cee0), the arguments, the
    // name; Preferences::Initialize(name, arguments, defaults); Preferences "language" →
    // TextLocale. (argument names inferred)
    Application(const String& name, BasicArray<String>& arguments,
                const Preferences::ValueMap& defaults);
    virtual ~Application();                                                 // Mac 0x26a0/0x26b0, Win sdd 0x4225d0

    static Application* GetInstance();                                      // Mac 0x26f0, Win 0x4226b0

    // ---- virtual slots (Mac slot / Win slot) ----
    virtual const BasicArray<String>* GetArguments() const;                 // 2/1   Mac 0x2710, Win 0x51e790
    // Enable the Mac menu items About / Preferences (-[NSApplication(LEGOApplicationCategory)
    // validateMenuItem:], TU 126); both default to IsIdle (tail call). (names inferred)
    virtual bool CanShowAboutBox();                                         // 3/2   Mac 0x33e0, Win 0x422270
    virtual bool CanShowPreferences();                                      // 4/3   Mac 0x33f0, Win 0x422270
    // True when no modal window / control blocks commands (LDD 0x173ff0). (name inferred)
    virtual bool IsIdle();                                                  // 5/4   Mac 0x3400 (true), Win 0x4f5b70
    // if (CanQuit()) { OnQuit(); StopRunLoop(true); } (name inferred)
    virtual void Quit();                                                    // 6/5   Mac 0x2720, Win 0x4226c0
    // Mac: called from -[LEGOApplicationDelegate applicationWillFinishLaunching:] (slot 7) and
    // applicationDidFinishLaunching: (slot 8); never called on Win. (names unknown)
    virtual void OnEvent7();                                                // 7/6   Mac 0x3410 (empty; name unknown)
    virtual void OnEvent8();                                                // 8/7   Mac 0x3420 (empty; name unknown)
    // LDD: QuerySaveChanges when idle. (name inferred)
    virtual bool CanQuit();                                                 // 9/8   Mac 0x3430 (true), Win 0x4f5b70
    // LDD: stores "mainWindowOrigin" / "mainWindowSize". (name inferred)
    virtual void OnQuit();                                                  // 10/9  Mac 0x3440 (empty), Win 0x459790
    // Document handling (defaults return false; names inferred from the LDD overrides). Not
    // overloads: the Win order equals the Mac order (MSVC would reverse an overload group).
    virtual bool NewDocument();                                             // 11/10 Mac 0x3450, Win 0x4aede0
    // Opens a file given from outside (dropped on / handed to the window: Window::OpenFile Win
    // 0x40f610 calls Win slot 11 with a Path; LDD 0x174a80 asks to save, then loads it).
    // (name inferred)
    virtual bool OpenDocumentFile(const Path& path);                        // 12/11 Mac 0x3460, Win 0x424d30
    virtual bool SaveDocument();                                            // 13/12 Mac 0x3470
    virtual bool SaveDocumentAs();                                          // 14/13 Mac 0x3480
    virtual bool SaveDocumentTo(const Path& path);                          // 15/14 Mac 0x3490, Win 0x424d30
    virtual bool QuerySaveChanges();                                        // 16/15 Mac 0x34a0
    virtual bool CloseDocument();                                           // 17/16 Mac 0x34b0
    // The application's main loop; returns the result StopRunLoop passed.
    virtual bool RunLoop();                                                 // 18/17 Mac 0x3270, Win 0x422860
    virtual void StopRunLoop(bool result);                                  // 19/18 Mac 0x32c0, Win 0x422aa0
    virtual bool IsInRunLoop() const;                                       // 20/19 Mac 0x34c0, Win 0x422280
    // Runs a modal loop for `window`, calling `reciever` meanwhile. (signature inferred)
    virtual void RunModal(Window* window, ModalUpdateReciever* reciever);   // 21/20 Mac 0x3310, Win 0x422ad0
    virtual void StopModal();                                               // 22/21 Mac 0x3370, Win 0x422d00
    virtual bool IsModal() const;                                           // 23/22 Mac 0x33b0, Win 0x422d20
    // Alert panel (Platform::Alert, Mac 0xb860); returns the button pressed. (signature inferred)
    virtual UInt32 Alert(UInt32 style, const String& message, const String& title,
                         const String& button1, const String& button2, const String& button3); // 24/23 Mac 0x2980, Win 0x422f90
    // Mac menu items About / Preferences (category LEGOApplicationCategory, TU 126). (names inferred)
    virtual void ShowAboutBox();                                            // 25/24 Mac 0x34d0 (empty), Win 0x459790
    virtual void ShowPreferences();                                         // 26/25 Mac 0x34e0 (empty), Win 0x459790

    // ---- non-virtual ----
    // The per-user data directory (holds the user brick database): "<Platform AppData
    // directory>/LEGO Company/<application name>", created if needed; owned by the platform cache
    // (the caller takes its own reference). (name inferred)
    Directory* GetUserDataDirectory();                                      // Mac 0x2880, Win 0x422e50
    const String& GetName() const { return m_name; }
    // The application's version: Mac CFBundleVersion "a.b.c.d" of the main bundle; Win the
    // VS_FIXEDFILEINFO file version of the executable (GetArguments()[0]). False without one.
    // (name inferred)
    bool GetVersion(Version& version);                                      // Mac 0x2760, Win 0x422d30
    // A document handed over before launching finished (Mac application:openFile:, a VFS path at
    // +0x38 / Win +0x24); read by LDD's start-up. (accessors, names inferred)
    const String& GetDocumentToOpen() const { return m_documentToOpen; }
    void SetDocumentToOpen(const String& vfsPath) { m_documentToOpen = vfsPath; }
#if defined(_WIN32)
    // Accelerator tables tried by the message loop (TranslateAcceleratorA) before dispatching.
    // (accessor, name inferred; nothing in LDD adds one)
    BasicSet<HACCEL>& GetAccelerators() { return m_accelerators; }
#endif

protected:
    // ---- data (Mac offsets) ----
    const BasicArray<String>* m_arguments;     // +0x04 / Win +0x04
    bool                      m_result;        // +0x08 (StopRunLoop argument; 1 initially)
    bool                      m_inRunLoop;     // +0x09 / Win +0x09
#if defined(_WIN32)
    CRITICAL_SECTION          m_lock;          // Win +0x0c (guards the instance pointer)
#else
    pthread_mutex_t           m_lock;          // +0x0c recursive (guards the instance pointer)
#endif
    String                    m_documentToOpen; // +0x38 / Win +0x24 (VFS path; see GetDocumentToOpen)
    String                    m_name;          // +0x48 / Win +0x28
#if defined(_WIN32)
    BasicSet<HACCEL>          m_accelerators;  // Win +0x2c (RTTI BasicSet<HACCEL__*, Less<…>>)
#endif

private:
#if !defined(_WIN32)
    // Mac: creates NSApp (LEGOApplication) with a LEGOApplicationDelegate, loads MainMenu.nib, runs
    // NSApp until applicationDidFinishLaunching: stops it, and returns the main bundle's first
    // preferred localization (canonical locale id; empty without one). (name inferred)
    static void InitializeCocoa(String& language);                          // Mac 0x2300
#endif
    Application(const Application&);
    Application& operator=(const Application&);
};

} // namespace LEGO
