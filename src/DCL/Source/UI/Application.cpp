// LEGO Digital Designer — restored source
// LEGO::Application, Win32 twin of Application.mm (file name inferred) — Windows build only.
// Win 0x4223d0-0x422f90 + slots shared by folding elsewhere (0x422270 /
// 0x422280 / 0x51e790 / 0x4f5b70 / 0x459790 / 0x4aede0 / 0x424d30).
// Library code emitted here, not written out: ObjectRegistry::UnregisterAll 0x422290 (inline),
// BasicSet<HACCEL> ctor / dtor 0x422ff0 / 0x423080 and tree helpers 0x4230e0-0x4231b0, Path
// dtors 0x40ba10 / 0x40ba80.
#if defined(_WIN32)
#include "Application.h"
#include "Main.h"
#include "Platform.h"
#include "Preferences.h"
#include "TextLocale.h"
#include "ObjectRegistry.h"
#include "Updater.h"
#include "SystemUpdater.h"
#include "Directory.h"
#include "Path.h"
#include "Window.h"
#include "OpenGLCrashReport.h"

#include <windows.h>

#if defined(_MSC_VER) || defined(__clang__)
#pragma comment(lib, "version")   // GetFileVersionInfo (the original imports VERSION.dll)
#endif

namespace LEGO {

namespace {

Application*          s_instance = 0;            // Win 0xb4273c
// modal loop state (shared with the timer callback)
Int32                 s_updateDepth = 0;         // Win 0xb42740 (also counts modal loops)
bool                  s_inModal = false;         // Win 0xb42744 (Application::IsModal)
bool                  s_modalRunning = false;    // Win 0xb42745
ModalUpdateReciever*  s_modalReciever = 0;       // Win 0xb42748
Window*               s_modalWindow = 0;         // Win 0xb4274c

const UINT kStopModalMessage = 0x8001;           // WM_APP + 1, thread message of StopModal
// WM_APP + 2, thread message of SystemUpdater's wake-up: the queued main-thread work runs as soon
// as it is queued instead of at the next 10 ms pass.
const UINT kWakeMessage = 0x8002;
DWORD      s_mainThreadId = 0;

// Win 0x4226f0: one pass of the 10 ms timer.
void UpdatePass()
{
    // NOTE: the original reads the OpenGL crash counter (Win 0xb4275c, OpenGLCrashReport.cpp)
    // directly; HasOpenGLCrashed() is the closest restored accessor
    if (HasOpenGLCrashed())
        return;
    if (!s_inModal) {
        if (s_updateDepth == 0) {
            s_updateDepth = 1;
            MainUpdater::GetInstance()->Update();
            --s_updateDepth;
        }
    } else {
        s_modalWindow->ModalUpdate();
        if (s_modalReciever)
            s_modalReciever->ModalUpdate();
    }
    SystemUpdater::GetInstance()->Update();
}

// Win 0x4227f0: TIMERPROC of the main loop; the pass runs under __try with HandleCrash as the
// filter (funclet 0x422831).
VOID CALLBACK UpdateTimer(HWND, UINT, UINT_PTR, DWORD)
{
#if defined(_MSC_VER)
    __try {
        UpdatePass();
    } __except (HandleCrash(GetExceptionInformation())) {
    }
#else
    UpdatePass();   // NOTE: no SEH __try with mingw (the unhandled-exception filter remains)
#endif
}

inline bool IsUnicode()
{
    return Platform::GetInstance()->IsUnicode();
}

// Accelerators first (TranslateAcceleratorA, as the original), then translate + dispatch.
void DispatchLoopMessage(BasicSet<HACCEL>& accelerators, MSG& msg)
{
    for (BasicSet<HACCEL>::Iterator it = accelerators.Begin(); it != accelerators.End(); ++it)
        if (::TranslateAcceleratorA(msg.hwnd, *it, &msg))
            return;
    ::TranslateMessage(&msg);
    if (IsUnicode())
        ::DispatchMessageW(&msg);
    else
        ::DispatchMessageA(&msg);
}

// Messages for other windows that the modal loop swallows (input, menus, activation, close).
bool IsBlockedDuringModal(UINT message)
{
    switch (message) {
    case WM_SETFOCUS:          // 0x07
    case WM_CLOSE:             // 0x10
    case WM_QUERYENDSESSION:   // 0x11
    case WM_MOUSEACTIVATE:     // 0x21
    case WM_CHILDACTIVATE:     // 0x22
    case WM_VKEYTOITEM:        // 0x2e
    case WM_CHARTOITEM:        // 0x2f
    case WM_CONTEXTMENU:       // 0x7b
    case WM_COMMAND:           // 0x111
    case WM_SYSCOMMAND:        // 0x112
    case 0x319:                // WM_APPCOMMAND
        return true;
    default:
        break;
    }
    return (message >= 0xa0 && message <= 0xad)      // non-client mouse
        || (message >= 0x100 && message <= 0x10f)    // keyboard
        || (message >= 0x114 && message <= 0x126)    // scroll bars, menus, colour messages
        || (message >= 0x200 && message <= 0x20e)    // mouse
        || (message >= 0x211 && message <= 0x213)    // menu loop
        || (message >= 0x231 && message <= 0x233)    // size/move loop, dropped files
        || (message >= 0x281 && message <= 0x291)    // IME
        || (message >= 0x2a0 && message <= 0x2a3)    // mouse hover / leave
        || (message >= 0x300 && message <= 0x312);   // clipboard, hot keys
}

void PostWake()
{
    if (s_mainThreadId != 0)
        ::PostThreadMessageW(s_mainThreadId, kWakeMessage, 0, 0);
}

// Runs the queued main-thread work for a wake-up message (true: `msg` was one).
bool HandleWake(const MSG& msg)
{
    if (msg.hwnd != 0 || msg.message != kWakeMessage)
        return false;
    if (!HasOpenGLCrashed())
        SystemUpdater::RunQueued();
    return true;
}

} // namespace

// Win 0x4223d0
Application::Application(const String& name, BasicArray<String>& arguments,
                         const Preferences::ValueMap& defaults)
    : m_arguments(&arguments), m_result(true), m_inRunLoop(false), m_name(name)
{
    ::InitializeCriticalSection(&m_lock);
    Preferences::GetInstance()->Initialize(name, arguments, defaults);
    s_instance = this;
    // Win has no system language: only the "language" preference
    String language;
    if (Preferences::GetInstance()->GetValue(String("language"), language))
        TextLocale::GetInstance()->SetLanguage(language);
}

// Win 0x4225f0 (scalar deleting destructor 0x4225d0)
Application::~Application()
{
    ObjectRegistry::GetInstance()->UnregisterAll();
    ::EnterCriticalSection(&m_lock);
    s_instance = 0;
    ::LeaveCriticalSection(&m_lock);
    ::DeleteCriticalSection(&m_lock);
}

// Win 0x4226b0
Application* Application::GetInstance()
{
    return s_instance;
}

// Win 0x51e790 (folded: returns the member at +4)
const BasicArray<String>* Application::GetArguments() const
{
    return m_arguments;
}

// Win 0x422270 (both slots)
bool Application::CanShowAboutBox()
{
    return IsIdle();
}

bool Application::CanShowPreferences()
{
    return IsIdle();
}

// Win 0x4f5b70 (folded "return true")
bool Application::IsIdle()
{
    return true;
}

// Win 0x4226c0
void Application::Quit()
{
    if (CanQuit()) {
        OnQuit();
        StopRunLoop(true);
    }
}

// Win 0x459790 (folded empty function)
void Application::OnEvent7()
{
}

void Application::OnEvent8()
{
}

// Win 0x4f5b70
bool Application::CanQuit()
{
    return true;
}

// Win 0x459790
void Application::OnQuit()
{
}

// Win 0x4aede0 (folded "return false")
bool Application::NewDocument()
{
    return false;
}

// Win 0x424d30 (folded "return false" with one argument)
bool Application::OpenDocumentFile(const Path& /*path*/)
{
    return false;
}

bool Application::SaveDocument()
{
    return false;
}

bool Application::SaveDocumentAs()
{
    return false;
}

// Win 0x424d30
bool Application::SaveDocumentTo(const Path& /*path*/)
{
    return false;
}

bool Application::QuerySaveChanges()
{
    return false;
}

bool Application::CloseDocument()
{
    return false;
}

// Win 0x422860
bool Application::RunLoop()
{
    m_inRunLoop = true;
    m_result = true;
    UINT_PTR timer = ::SetTimer(0, 0, 10, UpdateTimer);
    s_mainThreadId = ::GetCurrentThreadId();
    SystemUpdater::SetWake(PostWake);
    MSG msg;
    while (m_inRunLoop) {
        BOOL got = IsUnicode() ? ::GetMessageW(&msg, 0, 0, 0) : ::GetMessageA(&msg, 0, 0, 0);
        if (!got)
            break;
        if (HandleWake(msg))
            continue;
        DispatchLoopMessage(m_accelerators, msg);
    }
    SystemUpdater::SetWake(0);
    ::KillTimer(0, timer);
    // dispatch what is still queued
    for (;;) {
        BOOL got = IsUnicode() ? ::PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)
                               : ::PeekMessageA(&msg, 0, 0, 0, PM_REMOVE);
        if (!got)
            break;
        DispatchLoopMessage(m_accelerators, msg);
    }
    m_inRunLoop = false;
    return m_result;
}

// Win 0x422aa0
void Application::StopRunLoop(bool result)
{
    if (m_inRunLoop) {
        m_inRunLoop = false;
        m_result = result;
        ::PostQuitMessage(0);
    }
}

// Win 0x422280
bool Application::IsInRunLoop() const
{
    return m_inRunLoop;
}

// Win 0x422ad0 (window = first stack argument; `this` unused)
void Application::RunModal(Window* window, ModalUpdateReciever* reciever)
{
    // NOTE: the original ends every run with s_modalRunning / s_inModal = false and receiver /
    // window = 0. A run started inside another one (e.g. an LDDAlertBox opened while a Lua dialog
    // runs ModalRun) thereby also ends the enclosing run: its do/while sees s_modalRunning
    // false and returns as soon as the inner dialog closes; IsModal() is false from then on.
    // fixed: the enclosing run's state is saved on entry and handed back on exit (as Application.mm does).
    // s_modalRunning gets its saved value back: true while the outer run goes on, false only when
    // StopModal was already called for the outer run before the inner one started (StopModal has
    // no target, it stops the innermost run). Outside a modal run the saved values are the
    // original's false / 0.
    const bool outerRunning = s_modalRunning;
    const bool outerInModal = s_inModal;
    ModalUpdateReciever* const outerReciever = s_modalReciever;
    Window* const outerWindow = s_modalWindow;
    ++s_updateDepth;
    s_inModal = true;
    s_modalReciever = reciever;
    s_modalWindow = window;
    s_modalRunning = true;
    MSG msg;
    do {
        BOOL got = IsUnicode() ? ::GetMessageW(&msg, 0, 0, 0) : ::GetMessageA(&msg, 0, 0, 0);
        if (!got)
            break;
        if (HandleWake(msg))
            continue;
        if (msg.hwnd == 0 || msg.hwnd == window->GetHWND()) {
            ::TranslateMessage(&msg);
            if (IsUnicode())
                ::DispatchMessageW(&msg);
            else
                ::DispatchMessageA(&msg);
            if (msg.hwnd == 0 && msg.message == kStopModalMessage)
                break;
        } else if (!IsBlockedDuringModal(msg.message)) {
            ::TranslateMessage(&msg);
            if (IsUnicode())
                ::DispatchMessageW(&msg);
            else
                ::DispatchMessageA(&msg);
        }
    } while (s_modalRunning);
    if (msg.message == WM_QUIT)
        ::PostQuitMessage((int)msg.wParam);   // hand the quit on to the main loop
    if (IsUnicode())
        ::PeekMessageW(&msg, 0, kStopModalMessage, kStopModalMessage, PM_REMOVE);
    else
        ::PeekMessageA(&msg, 0, kStopModalMessage, kStopModalMessage, PM_REMOVE);
    --s_updateDepth;
    s_modalRunning = outerRunning;   // the enclosing run's state (false / 0 outside one)
    s_inModal = outerInModal;
    s_modalReciever = outerReciever;
    s_modalWindow = outerWindow;
}

// Win 0x422d00
void Application::StopModal()
{
    s_modalRunning = false;
    ::PostMessageA(0, kStopModalMessage, 0, 0);
}

// Win 0x422d20
bool Application::IsModal() const
{
    return s_inModal;
}

// Win 0x422d30
bool Application::GetVersion(Version& version)
{
    const String& executable = (*GetArguments())[0];
    const DWORD size = ::GetFileVersionInfoSizeW((LPCWSTR)executable.GetBuffer(), 0);
    UInt8* data = new UInt8[size];
    bool ok = false;
    VS_FIXEDFILEINFO* info = 0;
    UINT length = 0;
    if (::GetFileVersionInfoW((LPCWSTR)GetArguments()->operator[](0).GetBuffer(), 0, size, data)
        && ::VerQueryValueW(data, L"\\", (LPVOID*)&info, &length)) {
        version.m_major = HIWORD(info->dwFileVersionMS);
        version.m_minor = LOWORD(info->dwFileVersionMS);
        version.m_build = HIWORD(info->dwFileVersionLS);
        version.m_revision = LOWORD(info->dwFileVersionLS);
        ok = true;
    }
    delete[] data;
    return ok;
}

// Win 0x422e50
Directory* Application::GetUserDataDirectory()
{
    RefPointer<Directory> appData =
        Platform::GetInstance()->GetSystemDirectory(Platform::kAppDataDirectory, true);
    if (appData.IsNull())
        return 0;
    String path("LEGO Company/");
    path << m_name;
    // NOTE: the reference is dropped before returning; the directory stays alive in its parent's
    // VFS cache
    return appData->GetDirectory(path, true).Get();
}

// Win 0x422f90: MessageBoxW, abort/retry/ignore -> 0/1/2; style and buttons are not used.
UInt32 Application::Alert(UInt32 /*style*/, const String& message, const String& title,
                          const String& /*button1*/, const String& /*button2*/,
                          const String& /*button3*/)
{
    const int answer = ::MessageBoxW(0, (LPCWSTR)message.GetBuffer(), (LPCWSTR)title.GetBuffer(),
                                     MB_SETFOREGROUND | MB_TOPMOST | MB_TASKMODAL | MB_ICONHAND |
                                         MB_ABORTRETRYIGNORE);   // 0x52012
    if (answer == IDABORT)
        return 0;
    return answer == IDRETRY ? 1 : 2;
}

// Win 0x459790
void Application::ShowAboutBox()
{
}

void Application::ShowPreferences()
{
}

} // namespace LEGO
#endif // _WIN32
