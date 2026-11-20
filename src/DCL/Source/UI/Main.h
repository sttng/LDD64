// LEGO Digital Designer — restored source
// Process entry of a DCL application (file and all names inferred): the framework provides the
// platform `main`, the product AppMain.
//   Mac: TU 006 "main.mm" — main 0x8c70 (called by the Mach-O entry 0x2030, exit(result))
//   Win: "WinMain.cpp" (Win 0x5ce233-0x5ceb10) — WinMain 0x5cea85, inner main
//        0x5ce8c4, command-line splitter 0x5ce233 (a copy of the CRT's wparse_cmdline),
//        crash-dump options 0x5ce3be, unhandled-exception filter 0x5ce42c (MiniDumpWriteDump
//        into "<temp>\DCLMINIDUMP-<date>.mdmp").
// Both build the argument array (argv[0] included), honour "-TraceName <name>" (SetTraceName),
// set LC_NUMERIC to "C" and run AppMain on an attached LEGO::Thread.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "BasicArray.h"

#if defined(_WIN32)
#include <windows.h>
#endif

// The application's main function, provided by the product (LDD: Mac 0x13ad50, Win 0x5cbc41,
// global namespace and signature inferred from both callers).
int AppMain(LEGO::BasicArray<LEGO::String>& arguments);

namespace LEGO {

#if defined(_WIN32)
// Where the unhandled-exception filter writes its dumps: `directory` (native, '\' appended when
// missing; default: the temp directory at start-up), file name prefix `name` (default
// "DCLMINIDUMP"), and whether a full-memory ".dmp" goes next to the ".mdmp". Null keeps the
// current value. LDD's AppMain passes (null, the application name, preference
// "CreateLargeStackDump"). (name inferred)
void SetCrashDumpOptions(const String* directory, const String* name, const bool* fullDump); // Win 0x5ce3be

// SetUnhandledExceptionFilter / __except filter: suspends every other thread, writes the
// minidump(s), traces "Caught exception, stack trace saved at '…'" and exits with code 3. A
// second exception on the same thread terminates at once; on another thread that thread
// suspends itself. Also the filter of Application's timer callback (Win 0x4227f0). (name inferred)
LONG WINAPI HandleCrash(EXCEPTION_POINTERS* exception);                                     // Win 0x5ce42c

// Splits a command line like the MSVC CRT: a first pass with null buffers counts `argumentCount`
// (incl. the terminating null entry) and `characterCount`; the second fills `argv` and the
// strings in `characters`. (name inferred; Win 0x5ce233: command line EAX, characters EDX,
// characterCount ESI)
void ParseCommandLine(const wchar_t* commandLine, wchar_t** argv, wchar_t* characters,
                      int* argumentCount, int* characterCount);                             // Win 0x5ce233

// The inner main: temp path for dumps, arguments from GetCommandLineW, -TraceName, locale,
// thread, AppMain. (name inferred)
int WinMainInner();                                                                         // Win 0x5ce8c4
#endif

} // namespace LEGO
