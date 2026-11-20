// LEGO Digital Designer — restored source
// TU "TraceHook.cpp": TraceHook, Trace and the trace settings. Mac 0x1ce70-0x1ddc3, Win
// 0x4312a0-0x4317c0 (+ 0x430ff0, 0x4311c0). NSProcessInfo (SetName) behind __OBJC__, the Carbon
// log file behind __APPLE__; host builds use the HOST-ONLY fallbacks (no log file).
#include "Trace.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "SystemTime.h"

#if defined(_WIN32)
#include <windows.h>
#include <mmsystem.h>
#include <shlobj.h>
#include <string.h>
#else
#include <pthread.h>
#include <stdio.h>
#if defined(__APPLE__)
#include <CoreServices/CoreServices.h>
#endif
#include <CoreFoundation/CoreFoundation.h>
#if defined(__OBJC__)
#import <Foundation/Foundation.h>
#endif
#endif

namespace LEGO {

namespace {

// Win timeGetTime() / Mac 0x1cdc0 at the same spots: inline platform helper (name inferred).
inline UInt32 GetTraceTime()
{
#if defined(_WIN32)
    return (UInt32)::timeGetTime();
#else
    return (UInt32)GetMilliseconds();
#endif
}

// Every line traced with kTraceToHistory. Mac 0x77d0e8 (static init 0x1dd00), Win 0xae1660.
BasicArray<String> s_history;

// Holds the trace lock for the duration of Trace() (unlocked on unwind in both builds).
class TraceLocker {
public:
    explicit TraceLocker(TraceHook* hook) : m_hook(hook) { m_hook->Lock(); }
    ~TraceLocker() { m_hook->Unlock(); }
private:
    TraceHook* m_hook;
};

} // namespace

// ===========================================================================
// TraceHook
// ===========================================================================

TraceHook* TraceHook::s_instance = 0;

// Mac 0x1daf0, Win 0x4311c0
TraceHook::TraceHook()
    : m_flags(kTraceToDebugger)
#if defined(_WIN32)
    , m_file(INVALID_HANDLE_VALUE)
#else
    , m_logOpen(false)
    , m_logFork(-1)
#endif
{
#if defined(_WIN32)
    ::InitializeCriticalSection(&m_lock);
#else
    pthread_mutexattr_t attr;
    ::pthread_mutexattr_init(&attr);
    ::pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    ::pthread_mutex_init(&m_lock, &attr);
    ::pthread_mutexattr_destroy(&attr);
#endif
    s_instance = this;
    m_startTime = GetTraceTime();
    SetName(String());
}

// Mac 0x1dc60 (D2), 0x1dc10 (D1), 0x1dc20 (D0); Win 0x4312a0, 0x431340 (scalar deleting)
TraceHook::~TraceHook()
{
    CloseLog();
    s_instance = 0;
#if defined(_WIN32)
    ::DeleteCriticalSection(&m_lock);
#else
    ::pthread_mutex_destroy(&m_lock);
#endif
}

// Mac 0x1ce70, Win 0x431360
TraceHook* TraceHook::Get()
{
    // Mac guard 0x77d110 / RefPointer 0x77d10c; Win guard 0xb44c80 / RefPointer 0xb44968.
    static RefPointer<TraceHook> s_hook(new TraceHook);
    return s_instance;
}

void TraceHook::Lock()
{
#if defined(_WIN32)
    ::EnterCriticalSection(&m_lock);
#else
    ::pthread_mutex_lock(&m_lock);
#endif
}

void TraceHook::Unlock()
{
#if defined(_WIN32)
    ::LeaveCriticalSection(&m_lock);
#else
    ::pthread_mutex_unlock(&m_lock);
#endif
}

bool TraceHook::IsLogOpen() const
{
#if defined(_WIN32)
    return m_file != INVALID_HANDLE_VALUE;
#else
    return m_logOpen;
#endif
}

// inlined into the dtor and SetName in both builds
void TraceHook::CloseLog()
{
#if defined(_WIN32)
    if (m_file != INVALID_HANDLE_VALUE) {
        ::CloseHandle(m_file);
        m_file = INVALID_HANDLE_VALUE;
    }
#else
    if (m_logOpen) {
#if defined(__APPLE__)
        ::FSCloseFork(m_logFork);
#endif
        m_logOpen = false;
        m_logFork = -1;
    }
#endif
}

// Mac 0x1d830 (Win: inlined into Trace)
void TraceHook::OpenLog()
{
#if defined(_WIN32)
    // NOTE: Win calls a CreateFile wrapper (Win 0x415210, other TU) that uses CreateFileA
    // on non-Unicode systems; arguments: FILE_SHARE_READ, GENERIC_WRITE, CREATE_ALWAYS,
    // FILE_ATTRIBUTE_NORMAL.
    m_file = ::CreateFileW((LPCWSTR)m_logPath.GetBuffer(), GENERIC_WRITE, FILE_SHARE_READ, 0,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
#elif defined(__APPLE__)
    // ~/Library/Logs/<name>.log, appended to (never truncated)
    FSRef library, logs, file;
    ::FSFindFolder(kUserDomain, kDomainLibraryFolderType, kDontCreateFolder, &library);
    String logsName("Logs");
    ::FSMakeFSRefUnicode(&library, logsName.GetLength(), logsName.GetBuffer(),
                         kTextEncodingUnknown, &logs);
    String fileName(m_name);
    fileName << ".log";
    OSErr err = ::FSMakeFSRefUnicode(&logs, fileName.GetLength(), fileName.GetBuffer(),
                                     kTextEncodingUnknown, &file);
    if (err != noErr) {
        err = ::FSCreateFileUnicode(&logs, fileName.GetLength(), fileName.GetBuffer(),
                                    kFSCatInfoNone, 0, &file, 0);
        if (err != noErr)
            return;
    }
    HFSUniStr255 fork;
    ::FSGetDataForkName(&fork);
    if (::FSOpenFork(&file, fork.length, fork.unicode, fsRdWrPerm, &m_logFork) == noErr)
        m_logOpen = true;
#else
    // HOST-ONLY: no log file outside Mac OS X.
#endif
}

// Mac 0x1d0c0, Win 0x430ff0
void TraceHook::SetName(const String& text)
{
#if defined(_WIN32)
    String name(text.IsEmpty() ? String("DCLTrace.txt") : text);
#else
    String name;
    if (text.IsEmpty()) {
#if defined(__OBJC__)
        name = String((CFStringRef)[[NSProcessInfo processInfo] processName]);
#else
        name = String("DCLTrace");    // HOST-ONLY
#endif
    } else {
        name = text;
    }
#endif
    if (!m_name.EqualsNoCase(name)) {
        m_name = name;
#if defined(_WIN32)
        m_logPath.Clear();
        char folder[MAX_PATH];
        if (::SHGetSpecialFolderPathA(0, folder, CSIDL_APPDATA, TRUE)) {
            m_logPath.Assign(folder, (UInt32)strlen(folder));
            if (!m_logPath.IsEmpty()) {
                if (!m_logPath.EndsWith((UniChar16)'\\'))
                    m_logPath << "\\";
                m_logPath << "LEGO Company\\" << m_name;
            }
        }
#endif
        CloseLog();
    }
}

// ===========================================================================
// Free functions
// ===========================================================================

// Mac 0x1cf00, Win 0x431410
UInt32 GetTraceFlags()
{
    return TraceHook::Get()->m_flags;
}

// Mac 0x1cf90, Win 0x431420
void SetTraceFlags(UInt32 flags)
{
    TraceHook::Get()->m_flags = flags;
}

// Mac 0x1d020, Win 0x431430
void SetTraceName(const String& name)
{
    TraceHook::Get()->SetName(name);
}

// Mac 0x1d1a0, Win 0x431440
const String& GetTraceHistory()
{
    static String s_text;   // Mac 0x77d0f8 (guard 0x77d108), Win 0xb44c40 (guard 0xb44c44)
    // NOTE: the original reads and replaces s_history without the trace lock that Trace()
    // holds while it appends to it, so a Trace() on another thread can corrupt the array.
    // fixed: the history (and s_text) are handled under the trace lock.
    TraceLocker lock(TraceHook::Get());
    s_text.Clear();
    for (BasicArray<String>::Iterator it = s_history.Begin(); it != s_history.End(); ++it)
        s_text.Append(*it);
    s_history.Clear();
    s_history.PushBack(s_text);
    return s_text;
}

// Mac 0x1d290, Win 0x431570
// Both builds re-fetch TraceHook::Get() before every member access (kept here).
void Trace(const String& text, bool newLine)
{
    TraceLocker lock(TraceHook::Get());
    String s;
    UInt32 start = 0;
    if (newLine) {
        start = GetTraceTime();
        s << "Elapsed(" << (UInt32)(start - TraceHook::Get()->m_startTime) << ") " << text;
    } else {
        s = text;
    }

    if (TraceHook::Get()->m_flags & kTraceToDebugger) {
#if defined(_WIN32)
        ::OutputDebugStringA(s.ToAString().GetBuffer());
        if (newLine)
            ::OutputDebugStringA("\r\n");
#else
        if (newLine) {
#if defined(__APPLE__)
            ::CFShow(s.GetCFString());                  // appends the newline itself
#else
            ::printf("%s\n", s.ToAString().GetBuffer());  // HOST-ONLY stand-in for CFShow
#endif
        } else {
            ::printf("%s", s.ToAString().GetBuffer());
        }
#endif
    }

    if (TraceHook::Get()->m_flags & kTraceToHistory) {
        // NOTE: appends the line break to s itself, so the log file below gets it too
        // (Win then writes a second "\r\n" for newLine).
        s_history.PushBack(s << "\r\n");
    }

    if (TraceHook::Get()->m_flags & kTraceToFile) {
        if (!TraceHook::Get()->IsLogOpen())
            TraceHook::Get()->OpenLog();
        if (TraceHook::Get()->IsLogOpen()) {
#if defined(_WIN32)
            AString bytes = s.ToAString();
            DWORD written;
            ::WriteFile(TraceHook::Get()->m_file, bytes.GetBuffer(), bytes.GetLength(),
                        &written, 0);
            if (newLine)
                ::WriteFile(TraceHook::Get()->m_file, "\r\n", 2, &written, 0);
            ::FlushFileBuffers(TraceHook::Get()->m_file);
#elif defined(__APPLE__)
            if (newLine)
                s << "\n";
            CFDataRef data = ::CFStringCreateExternalRepresentation(0, s.GetCFString(),
                                                                    kCFStringEncodingUTF8, '?');
            if (data) {
                ::FSWriteFork(TraceHook::Get()->m_logFork, fsFromLEOF, 0,
                              ::CFDataGetLength(data), ::CFDataGetBytePtr(data), 0);
                ::CFRelease(data);
            }
#endif
        }
    }

    if (newLine)
        TraceHook::Get()->m_startTime += GetTraceTime() - start;
}

// Mac 0x1da30, Win 0x4317b0
void TraceVerbose(const String& text, bool newLine)
{
    if (TraceHook::Get()->m_flags & kTraceVerbose)
        Trace(text, newLine);
}

} // namespace LEGO
