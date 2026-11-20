// LEGO Digital Designer — restored source
// Trace output: debugger/console, log file and in-memory history.
// Implementation: TU "TraceHook.cpp" (Mac 0x1ce70-0x1ddc3, Win 0x4312a0-0x4317c0).
//
// Output channels are selected by the TraceHook flags (default kTraceToDebugger):
//   Win: OutputDebugString, %APPDATA%\LEGO Company\<name> (default name "DCLTrace.txt")
//   Mac: stdout (printf / CFShow), ~/Library/Logs/<name>.log (default name = process name)
// The name can be set on the command line with "-TraceName <name>" (Mac main 0x8c70).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace LEGO {

// Bits of TraceHook::m_flags (names inferred)
enum TraceFlags {
    kTraceToDebugger = 0x1,   // Win OutputDebugStringA, Mac printf/CFShow
    kTraceToFile     = 0x2,   // log file (opened on first use)
    kTraceVerbose    = 0x4,   // enables TraceVerbose()
    kTraceToHistory  = 0x8    // keep every line in memory, see GetTraceHistory()
};

// Writes text to all enabled channels. newLine: prefix "Elapsed(<ms>) " and terminate the
// line; the time spent inside Trace() is excluded from the elapsed time.
void Trace(const String& text, bool newLine);                        // Mac 0x1d290  Win 0x431570
void TraceVerbose(const String& text, bool newLine);                 // Mac 0x1da30  Win 0x4317b0 (name inferred)

UInt32        GetTraceFlags();                                        // Mac 0x1cf00  Win 0x431410 (name inferred)
void          SetTraceFlags(UInt32 flags);                            // Mac 0x1cf90  Win 0x431420 (name inferred)
void          SetTraceName(const String& name);                       // Mac 0x1d020  Win 0x431430 (name inferred)
// Joins the history into one string, which then replaces the history. (name inferred)
const String& GetTraceHistory();                                      // Mac 0x1d1a0  Win 0x431440

// ---------------------------------------------------------------------------
// TraceHook — the trace singleton. RTTI: Win ".?AUTraceHook@LEGO@@" (struct).
// sizeof: Mac 0x50, Win 0x34.
// Vtable (Mac 0x75fcf8): D1 0x1dc10, D0 0x1dc20, Object slots; (Win 0xa20000): 0x431340.
// ---------------------------------------------------------------------------
struct TraceHook : public Object {
    TraceHook();                                                      // Mac 0x1daf0  Win 0x4311c0
    virtual ~TraceHook();                                             // Mac 0x1dc60/0x1dc10/0x1dc20  Win 0x4312a0/0x431340

    static TraceHook* Get();                                          // Mac 0x1ce70  Win 0x431360 (name inferred)

    void SetName(const String& name);                                 // Mac 0x1d0c0  Win 0x430ff0 (name inferred)
    bool IsLogOpen() const;                                           // inline in both builds (name inferred)
    void OpenLog();                                                   // Mac 0x1d830 (Win inlined) (name inferred)
    void CloseLog();                                                  // inline in both builds (name inferred)
    void Lock();
    void Unlock();

    UInt32           m_flags;       // +0x08 TraceFlags, default kTraceToDebugger
    UInt32           m_startTime;   // +0x0c ms; advanced by the time spent tracing
#if defined(_WIN32)
    HANDLE           m_file;        // +0x10 INVALID_HANDLE_VALUE when closed
    String           m_logPath;     // +0x14 %APPDATA%\LEGO Company\<name>
    CRITICAL_SECTION m_lock;        // +0x18
    String           m_name;        // +0x30
#else
    bool             m_logOpen;     // +0x10
#if defined(__LP64__)
    Int32            m_logFork;     // PORT64: FSIORefNum is an int in the 64-bit SDK (FSOpenFork)
#else
    Int16            m_logFork;     // +0x12 FSIORefNum, -1 when closed
#endif
    pthread_mutex_t  m_lock;        // +0x14 (recursive)
    String           m_name;        // +0x40
#endif

    static TraceHook* s_instance;   // Mac 0x77d114  Win 0xb42754 (set by ctor, cleared by dtor)
};

} // namespace LEGO
