// LEGO Digital Designer — restored source
// TU 000 "Application.mm" (file name inferred) — Mac 0x2030-0x35db, Mac build only: Application
// with the Objective-C classes LEGOApplication (NSApplication subclass without members) and
// LEGOApplicationDelegate (run loop / modal loop timers).
// Not written out: Mach-O entry 0x2030 (crt start, see main.mm), landing pad 0x22e0 (Ghidra
// merged it with InitializeCocoa), ObjectRegistry::UnregisterAll 0x2550 (inline), Path dtors
// 0x2d40 / 0x34f0, static initialiser 0x3560 (<iostream> + a header-level constant).
//
// Objective-C sits behind __OBJC__. HOST-ONLY (Linux-hosted Mac branch): the run loop is a plain
// loop at the NSTimer rate (10 ms) pumping host window events through LEGO::HostApp::g_pumpEvents
// (set by compat/host_app); the modal loop is a nested one.
#if !defined(_WIN32)
#include "Application.h"
#include "Platform.h"
#include "Preferences.h"
#include "TextLocale.h"
#include "ObjectRegistry.h"
#include "Updater.h"
#include "SystemUpdater.h"
#include "StringStream.h"
#include "Directory.h"
#include "Path.h"
#include "Window.h"

#include <CoreFoundation/CoreFoundation.h>
#include <pthread.h>
#include <iostream>

#if defined(__OBJC__)
#import <Cocoa/Cocoa.h>
#else
// HOST-ONLY / Linux
#include <unistd.h>
#if defined(LDD_LINUX)
#include "LinuxShell.h"
#endif
#include <chrono>
#include <condition_variable>
#include <mutex>
#endif

namespace LEGO {

namespace {
Application* s_instance = 0;   // Mac 0x77cee0
} // namespace

#if !defined(__OBJC__)
// HOST-ONLY: the host shell's event pump, called once per pass of the run / modal loops.
namespace HostApp {
void (*g_pumpEvents)() = 0;
} // namespace HostApp

namespace {
bool                 s_hostModalRunning = false;
bool                 s_hostInModal = false;

void HostPump()
{
#if defined(LDD_LINUX)
    LinuxShell::Pump();   // Linux: the GTK events
#endif
    if (HostApp::g_pumpEvents)
        HostApp::g_pumpEvents();
}

// The 10 ms pause between two passes, running the queued main-thread work (SystemUpdater::
// RunQueued) as soon as SystemUpdater::Enqueue wakes it.
std::mutex              s_hostWakeMutex;
std::condition_variable s_hostWakeCondition;
bool                    s_hostWakePending = false;

void HostWake()
{
    std::lock_guard<std::mutex> lock(s_hostWakeMutex);
    s_hostWakePending = true;
    s_hostWakeCondition.notify_one();
}

void HostSleep()
{
    const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now() + std::chrono::milliseconds(10);
    std::unique_lock<std::mutex> lock(s_hostWakeMutex);
    for (;;) {
        if (!s_hostWakeCondition.wait_until(lock, end, [] { return s_hostWakePending; }))
            return;
        s_hostWakePending = false;
        lock.unlock();
        SystemUpdater::RunQueued();
        lock.lock();
    }
}
} // namespace
#endif

} // namespace LEGO

#if defined(__OBJC__)
// =============================================================================================
// LEGOApplication — NSApplication subclass (instance size 76 = NSApplication's, no methods).
// =============================================================================================
@interface LEGOApplication : NSApplication
@end

@implementation LEGOApplication
@end

// =============================================================================================
// LEGOApplicationDelegate (NSObject, instance size 28)
// =============================================================================================
@interface LEGOApplicationDelegate : NSObject <NSApplicationDelegate> {   // (protocol: 10.6+ SDKs)
    NSTimer*                    timer;                          // +4  run-loop timer (update:)
    NSTimer*                    timerModal;                     // +8  modal timer (updateModal:)
    char                        useMUR;                         // +12 a modal loop runs
    LEGO::ModalUpdateReciever*  mur;                            // +16
    LEGO::Window*               modalWindow;                    // +20
    char                        applicationDidFinishLaunching;  // +24
}
- (void)startRunloop;
- (void)stopRunloop;
- (void)ModalRun:(LEGO::Window*)window andMUR:(LEGO::ModalUpdateReciever*)reciever;
- (void)ModalStop;
@end

namespace {
// The 10 ms timer, scheduled for the default, event-tracking and modal-panel run-loop modes.
NSTimer* ScheduleTimer(id target, SEL selector)
{
    NSTimer* t = [NSTimer timerWithTimeInterval:0.01 target:target selector:selector userInfo:nil
                                        repeats:YES];
    [[NSRunLoop currentRunLoop] addTimer:t forMode:NSDefaultRunLoopMode];
    [[NSRunLoop currentRunLoop] addTimer:t forMode:NSEventTrackingRunLoopMode];
    [[NSRunLoop currentRunLoop] addTimer:t forMode:NSModalPanelRunLoopMode];
    return t;
}

// [NSApp stop:] takes effect after the next event: post an empty application-defined one.
void PostWakeUpEvent()
{
    NSEvent* event = [NSEvent otherEventWithType:NSApplicationDefined location:NSZeroPoint
                                   modifierFlags:0
                                       timestamp:[NSDate timeIntervalSinceReferenceDate]
                                    windowNumber:0 context:nil subtype:0 data1:0 data2:0];
    [NSApp postEvent:event atStart:YES];
}

// SystemUpdater's wake-up: a run-loop source of the main run loop in the modes of the 10 ms
// timer, which runs the queued main-thread work as soon as it is queued.
CFRunLoopSourceRef s_wakeSource = 0;
CFRunLoopRef       s_wakeLoop = 0;

void WakePerform(void* /*info*/)
{
    LEGO::SystemUpdater::RunQueued();
}

void Wake()
{
    CFRunLoopSourceSignal(s_wakeSource);
    CFRunLoopWakeUp(s_wakeLoop);
}

void InstallWake()
{
    if (s_wakeSource == 0) {
        CFRunLoopSourceContext context = { 0, 0, 0, 0, 0, 0, 0, 0, 0, WakePerform };
        s_wakeSource = CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &context);
        s_wakeLoop = (CFRunLoopRef)CFRetain(CFRunLoopGetCurrent());
        CFRunLoopAddSource(s_wakeLoop, s_wakeSource, (CFStringRef)NSDefaultRunLoopMode);
        CFRunLoopAddSource(s_wakeLoop, s_wakeSource, (CFStringRef)NSEventTrackingRunLoopMode);
        CFRunLoopAddSource(s_wakeLoop, s_wakeSource, (CFStringRef)NSModalPanelRunLoopMode);
    }
    LEGO::SystemUpdater::SetWake(Wake);
}
} // namespace

@implementation LEGOApplicationDelegate
// Mac 0x29b0
- (id)init
{
    self = [super init];
    if (self) {
        useMUR = 0;
        mur = 0;
        modalWindow = 0;
        applicationDidFinishLaunching = 0;
    }
    return self;
}

// Mac 0x2a00
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender
{
    return LEGO::Application::GetInstance()->CanQuit() ? NSTerminateNow : NSTerminateCancel;
}

// Mac 0x2a30
- (void)applicationWillTerminate:(NSNotification*)notification
{
    LEGO::Application::GetInstance()->OnQuit();
}

// Mac 0x2a50
- (void)applicationWillFinishLaunching:(NSNotification*)notification
{
    LEGO::Application::GetInstance()->OnEvent7();
}

// Mac 0x2a70: ends the [NSApp run] of InitializeCocoa.
- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    [NSApp stop:self];
    PostWakeUpEvent();
    applicationDidFinishLaunching = 1;
    LEGO::Application::GetInstance()->OnEvent8();
}

// Mac 0x2b50: before launching finished the file is kept for the start-up code, afterwards it is
// opened at once.
- (BOOL)application:(NSApplication*)sender openFile:(NSString*)filename
{
    LEGO::Application* application = LEGO::Application::GetInstance();
    if (!applicationDidFinishLaunching) {
        application->SetDocumentToOpen(
            LEGO::Platform::GetInstance()->NativeToVFSPath(LEGO::String((CFStringRef)filename)));
        return YES;
    }
    LEGO::Path path(LEGO::String(), 0);
    path.Append(LEGO::Platform::GetInstance()->NativeToVFSPath(LEGO::String((CFStringRef)filename)));
    return LEGO::Application::GetInstance()->OpenDocumentFile(path);
}

// Mac 0x2da0
- (void)update:(NSTimer*)sender
{
    if (!useMUR) {
        LEGO::MainUpdater::GetInstance()->Update();
        LEGO::SystemUpdater::GetInstance()->Update();
    }
}

// Mac 0x2e70
- (void)startRunloop
{
    if (![NSApp isRunning]) {
        timer = ScheduleTimer(self, @selector(update:));
        InstallWake();
        [NSApp run];
    }
}

// Mac 0x2f80
- (void)stopRunloop
{
    [timer invalidate];
    LEGO::SystemUpdater::SetWake(0);
    [NSApp stop:self];
    PostWakeUpEvent();
}

// Mac 0x3060
- (void)updateModal:(NSTimer*)sender
{
    // NOTE: the original calls modalWindow without a check. A nested ModalRun: ends with
    // modalWindow = 0 (and timerModal = 0) while the timer of the outer loop keeps firing, so
    // the next tick calls through NULL. fixed: checked here, and a nested ModalRun: hands the
    // outer loop its state back (below) (M20).
    if (modalWindow)
        modalWindow->ModalUpdate();
    if (mur)
        mur->ModalUpdate();
    LEGO::SystemUpdater::GetInstance()->Update();
}

// Mac 0x3100
- (void)ModalRun:(LEGO::Window*)window andMUR:(LEGO::ModalUpdateReciever*)reciever
{
    // (M20) the state of an enclosing modal loop (all 0 outside one)
    NSTimer* outerTimer = timerModal;
    char outerUseMUR = useMUR;
    LEGO::ModalUpdateReciever* outerMUR = mur;
    LEGO::Window* outerWindow = modalWindow;
    useMUR = 1;
    mur = reciever;
    modalWindow = window;
    timerModal = ScheduleTimer(self, @selector(updateModal:));
    // Window::Invalidate(0) (Mac 0x21d30): [[window contentView] setNeedsDisplay:YES]
#if defined(__LP64__)
    window->Invalidate(0);   // PORT64: also marks the subviews (layer-backed views, WindowMac.mm)
#else
    [[(NSWindow*)window->GetNativeWindow() contentView] setNeedsDisplay:YES];
#endif
    [NSApp runModalForWindow:(NSWindow*)window->GetNativeWindow()];
    [timerModal invalidate];
    timerModal = outerTimer;                    // (M20) the outer loop's timer stays its own
    useMUR = outerUseMUR;
    mur = outerMUR;
    modalWindow = outerWindow;
}

// Mac 0x3240
- (void)ModalStop
{
    [NSApp abortModal];
}
@end
#endif // __OBJC__

namespace LEGO {

// Mac 0x2070
Application::Application(const String& name, BasicArray<String>& arguments,
                         const Preferences::ValueMap& defaults)
    : m_arguments(&arguments), m_result(true), m_inRunLoop(false), m_name(name)
{
    pthread_mutexattr_t attributes;
    pthread_mutexattr_init(&attributes);
    pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&m_lock, &attributes);
    pthread_mutexattr_destroy(&attributes);
    Preferences::GetInstance()->Initialize(name, arguments, defaults);
    s_instance = this;
    // the system language wins; without one the "language" preference
    String language;
    InitializeCocoa(language);
    if (!language.IsEmpty() || Preferences::GetInstance()->GetValue(String("language"), language))
        TextLocale::GetInstance()->SetLanguage(language);
}

// Mac 0x2300
void Application::InitializeCocoa(String& language)
{
#if defined(__OBJC__)
    [LEGOApplication sharedApplication];
    [NSApp setDelegate:[[[LEGOApplicationDelegate alloc] init] autorelease]];
    [NSBundle loadNibNamed:@"MainMenu" owner:NSApp];
    NSArray* localizations = [[NSBundle mainBundle] preferredLocalizations];
    if ([localizations count] == 0) {
        language.Clear();
    } else {
        CFStringRef identifier = CFLocaleCreateCanonicalLocaleIdentifierFromString(
            0, (CFStringRef)[localizations objectAtIndex:0]);
        language = String(identifier);
        CFRelease(identifier);
    }
    // until applicationDidFinishLaunching: stops it
    [NSApp run];
#else
    // HOST-ONLY: no bundle localizations
    language.Clear();
#endif
}

// Mac 0x24a0 (D1 0x26a0, D0 0x26b0)
Application::~Application()
{
    ObjectRegistry::GetInstance()->UnregisterAll();
    pthread_mutex_lock(&m_lock);
    s_instance = 0;
    pthread_mutex_unlock(&m_lock);
    pthread_mutex_destroy(&m_lock);
}

// Mac 0x26f0
Application* Application::GetInstance()
{
    return s_instance;
}

// Mac 0x2710
const BasicArray<String>* Application::GetArguments() const
{
    return m_arguments;
}

// Mac 0x2720
void Application::Quit()
{
    if (CanQuit()) {
        OnQuit();
        StopRunLoop(true);
    }
}

// Mac 0x2760: CFBundleVersion "a.b.c.d" (fields that do not parse stay 0).
bool Application::GetVersion(Version& version)
{
#if defined(__OBJC__)
    CFBundleRef bundle = CFBundleGetMainBundle();
    if (!bundle)
        return false;
    String text((CFStringRef)CFBundleGetValueForInfoDictionaryKey(bundle, kCFBundleVersionKey));
    if (text.IsEmpty())
        return false;
    version = Version();
    UInt32 position = 0;
    if (FromString(version.m_major, text, position)) {
        ++position;
        if (FromString(version.m_minor, text, position)) {
            ++position;
            if (FromString(version.m_build, text, position)) {
                ++position;
                FromString(version.m_revision, text, position);
            }
        }
    }
    return true;
#elif defined(LDD_LINUX) && defined(LDD_VERSION_STRING)
    // Linux: the build's version (CMakeLists.txt, as CFBundleVersion and the Windows FILEVERSION)
    String text(LDD_VERSION_STRING);
    version = Version();
    UInt32 position = 0;
    if (FromString(version.m_major, text, position)) {
        ++position;
        if (FromString(version.m_minor, text, position)) {
            ++position;
            if (FromString(version.m_build, text, position)) {
                ++position;
                FromString(version.m_revision, text, position);
            }
        }
    }
    return true;
#else
    (void)version;
    return false;   // HOST-ONLY: no main bundle
#endif
}

// Mac 0x2880
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

// Mac 0x2980
UInt32 Application::Alert(UInt32 /*style*/, const String& message, const String& title,
                          const String& button1, const String& button2, const String& button3)
{
    return Platform::Alert(message, &title, &button1, &button2, &button3);
}

// Mac 0x3270
bool Application::RunLoop()
{
    m_result = true;
    m_inRunLoop = true;
#if defined(__OBJC__)
    [(LEGOApplicationDelegate*)[NSApp delegate] startRunloop];
#else
    // HOST-ONLY: the NSTimer loop of startRunloop / update:
    SystemUpdater::SetWake(HostWake);
    while (m_inRunLoop) {
        HostPump();
        MainUpdater::GetInstance()->Update();
        SystemUpdater::GetInstance()->Update();
        HostSleep();
    }
    SystemUpdater::SetWake(0);
#endif
    m_inRunLoop = false;
    return m_result;
}

// Mac 0x32c0
void Application::StopRunLoop(bool result)
{
    if (m_inRunLoop) {
        m_inRunLoop = false;
        m_result = result;
#if defined(__OBJC__)
        [(LEGOApplicationDelegate*)[NSApp delegate] stopRunloop];
#endif
    }
}

// Mac 0x3310: only inside the run loop.
void Application::RunModal(Window* window, ModalUpdateReciever* reciever)
{
    if (!m_inRunLoop)
        return;
#if defined(__OBJC__)
    [(LEGOApplicationDelegate*)[NSApp delegate] ModalRun:window andMUR:reciever];
#else
    // HOST-ONLY: the updateModal: timer as a nested loop
    s_hostInModal = true;
    s_hostModalRunning = true;
    while (s_hostModalRunning) {
        HostPump();
        window->ModalUpdate();
        if (reciever)
            reciever->ModalUpdate();
        SystemUpdater::GetInstance()->Update();
        HostSleep();
    }
    s_hostInModal = false;
#endif
}

// Mac 0x3370
void Application::StopModal()
{
#if defined(__OBJC__)
    [(LEGOApplicationDelegate*)[NSApp delegate] ModalStop];
#else
    s_hostModalRunning = false;
#endif
}

// Mac 0x33b0
bool Application::IsModal() const
{
#if defined(__OBJC__)
    return [NSApp modalWindow] != nil;
#else
    return s_hostInModal;
#endif
}

// Mac 0x33e0
bool Application::CanShowAboutBox()
{
    return IsIdle();
}

// Mac 0x33f0
bool Application::CanShowPreferences()
{
    return IsIdle();
}

// Mac 0x3400
bool Application::IsIdle()
{
    return true;
}

// Mac 0x3410
void Application::OnEvent7()
{
}

// Mac 0x3420
void Application::OnEvent8()
{
}

// Mac 0x3430
bool Application::CanQuit()
{
    return true;
}

// Mac 0x3440
void Application::OnQuit()
{
}

// Mac 0x3450
bool Application::NewDocument()
{
    return false;
}

// Mac 0x3460
bool Application::OpenDocumentFile(const Path& /*path*/)
{
    return false;
}

// Mac 0x3470
bool Application::SaveDocument()
{
    return false;
}

// Mac 0x3480
bool Application::SaveDocumentAs()
{
    return false;
}

// Mac 0x3490
bool Application::SaveDocumentTo(const Path& /*path*/)
{
    return false;
}

// Mac 0x34a0
bool Application::QuerySaveChanges()
{
    return false;
}

// Mac 0x34b0
bool Application::CloseDocument()
{
    return false;
}

// Mac 0x34c0
bool Application::IsInRunLoop() const
{
    return m_inRunLoop;
}

// Mac 0x34d0
void Application::ShowAboutBox()
{
}

// Mac 0x34e0
void Application::ShowPreferences()
{
}

} // namespace LEGO

#endif // !_WIN32
