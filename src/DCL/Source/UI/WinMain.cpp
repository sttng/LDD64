// LEGO Digital Designer — restored source
// "WinMain.cpp" (file name inferred) — Windows build only. Win 0x5ce233-0x5ceb10
// (placed by /LTCG after the product's AppMain): command-line splitter 0x5ce233,
// SetCrashDumpOptions 0x5ce3be, HandleCrash 0x5ce42c, inner main 0x5ce8c4, WinMain 0x5cea85 (+
// __except filter funclet 0x5ceaae), compiler-generated import thunks 0x5cead2-0x5ceb10. Before
// WinMain the original runs the MSVC CRT (entry 0x6cad93 -> __tmainCRTStartup 0x6cac15); the
// restored build is entered from mingw's WinMainCRTStartup.
#if defined(_WIN32)
#include "Main.h"
#include "Thread.h"
#include "Trace.h"

#include <windows.h>
#include <dbghelp.h>
#include <tlhelp32.h>
#include <locale.h>
#include <stdlib.h>
#include <wchar.h>

#if defined(_MSC_VER) || defined(__clang__)
#pragma comment(lib, "dbghelp")
#endif

namespace LEGO {

namespace {

WCHAR         s_dumpDirectory[MAX_PATH];             // Win 0xaeeca8 (GetTempPathW at start-up)
WCHAR         s_dumpName[MAX_PATH] = L"DCLMINIDUMP"; // Win 0xab0548
bool          s_fullDump = false;                     // Win 0xaeeeb0
volatile LONG s_crashingThread = 0;                   // Win 0xaeeeb4

// _itow(value, buffer, 10) at the end of `text` (Win 0x431ba0 is the CRT's _itow).
void AppendNumber(WCHAR* text, unsigned value)
{
    _itow((int)value, text + wcslen(text), 10);
}

// "-" value, with a leading "0" for values below 10.
void AppendTwoDigits(WCHAR* text, unsigned value)
{
    wcscat(text, L"-");
    if (value < 10)
        wcscat(text, L"0");
    AppendNumber(text, value);
}

} // namespace

// Win 0x5ce233 (a copy of the MSVC CRT's wparse_cmdline)
void ParseCommandLine(const wchar_t* commandLine, wchar_t** argv, wchar_t* characters,
                      int* argumentCount, int* characterCount)
{
    const wchar_t* p = commandLine;
    *characterCount = 0;
    *argumentCount = 1;
    if (argv)
        *argv++ = characters;

    // the program name: quoted, or up to the first blank
    if (*p == L'"') {
        while (*++p != L'"' && *p != 0) {
            ++*characterCount;
            if (characters)
                *characters++ = *p;
        }
        ++*characterCount;
        if (characters)
            *characters++ = 0;
        if (*p == L'"')
            ++p;
    } else {
        wchar_t c;
        do {
            ++*characterCount;
            if (characters)
                *characters++ = *p;
            c = *p++;
        } while (c != L' ' && c != 0 && c != L'\t');
        if (c == 0)
            --p;
        else if (characters)
            characters[-1] = 0;
    }

    // the arguments
    bool inQuote = false;
    for (;;) {
        while (*p == L' ' || *p == L'\t')
            ++p;
        if (*p == 0)
            break;
        if (argv)
            *argv++ = characters;
        ++*argumentCount;
        for (;;) {
            bool copy = true;
            unsigned backslashes = 0;
            while (*p == L'\\') {
                ++p;
                ++backslashes;
            }
            if (*p == L'"') {
                if ((backslashes & 1) == 0) {
                    if (inQuote && p[1] == L'"')
                        ++p;            // "" inside quotes is a literal quote
                    else
                        copy = false;
                    inQuote = !inQuote;
                }
                backslashes >>= 1;
            }
            while (backslashes--) {
                if (characters)
                    *characters++ = L'\\';
                ++*characterCount;
            }
            if (*p == 0 || (!inQuote && (*p == L' ' || *p == L'\t')))
                break;
            if (copy) {
                if (characters)
                    *characters++ = *p;
                ++*characterCount;
            }
            ++p;
        }
        if (characters)
            *characters++ = 0;
        ++*characterCount;
    }
    if (argv)
        *argv++ = 0;
    ++*argumentCount;
}

// Win 0x5ce3be
void SetCrashDumpOptions(const String* directory, const String* name, const bool* fullDump)
{
    if (directory) {
        wcscpy(s_dumpDirectory, (const wchar_t*)directory->GetBuffer());
        // NOTE: an empty directory reads the character before the string (GetAt(npos))
        if (directory->GetAt(directory->GetLength() - 1) != '\\')
            wcscat(s_dumpDirectory, L"\\");
    }
    if (name)
        wcscpy(s_dumpName, (const wchar_t*)name->GetBuffer());
    if (fullDump)
        s_fullDump = *fullDump;
}

// Win 0x5ce42c
LONG WINAPI HandleCrash(EXCEPTION_POINTERS* exception)
{
    const DWORD thread = ::GetCurrentThreadId();
    const LONG previous = ::InterlockedCompareExchange(&s_crashingThread, (LONG)thread, 0);
    if (previous != 0) {
        if ((DWORD)previous == thread)
            ::TerminateProcess(::GetCurrentProcess(), 3);
        else
            ::SuspendThread(::GetCurrentThread());
        return EXCEPTION_CONTINUE_EXECUTION;   // -1
    }

    // suspend every other thread of the process until no new one shows up
    const DWORD process = ::GetCurrentProcessId();
    int suspendedBefore = -1;
    for (;;) {
        HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snapshot == INVALID_HANDLE_VALUE)
            break;
        int suspended = 0;
        THREADENTRY32 entry;
        entry.dwSize = sizeof(entry);   // 0x1c
        if (::Thread32First(snapshot, &entry)) {
            do {
                if (entry.th32OwnerProcessID == process && entry.th32ThreadID != thread) {
                    HANDLE handle = ::OpenThread(THREAD_SUSPEND_RESUME, FALSE, entry.th32ThreadID);
                    if (handle) {
                        if (::SuspendThread(handle) != (DWORD)-1)
                            ++suspended;
                        ::CloseHandle(handle);
                    }
                }
            } while (::Thread32Next(snapshot, &entry));
        }
        ::CloseHandle(snapshot);
        if (suspended == suspendedBefore)
            break;
        suspendedBefore = suspended;
    }

    // "-YYYY-MM-DD-hh-mm-ss"
    WCHAR stamp[32];
    memset(stamp, 0, sizeof(stamp));
    SYSTEMTIME now;
    ::GetLocalTime(&now);
    wcscat(stamp, L"-");
    AppendNumber(stamp, now.wYear);
    AppendTwoDigits(stamp, now.wMonth);
    AppendTwoDigits(stamp, now.wDay);
    AppendTwoDigits(stamp, now.wHour);
    AppendTwoDigits(stamp, now.wMinute);
    AppendTwoDigits(stamp, now.wSecond);

    MINIDUMP_EXCEPTION_INFORMATION information;
    information.ThreadId = thread;
    information.ExceptionPointers = exception;
    information.ClientPointers = TRUE;

    WCHAR path[0x428];
    memset(path, 0, sizeof(path));
    wcscpy(path, L"\\\\?\\");
    wcscat(path, s_dumpDirectory);
    wcscat(path, s_dumpName);
    wcscat(path, stamp);
    wcscat(path, L".mdmp");
    HANDLE file = ::CreateFileW(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (file != INVALID_HANDLE_VALUE) {
        ::MiniDumpWriteDump(::GetCurrentProcess(), process, file, MiniDumpNormal, &information, 0, 0);
        ::CloseHandle(file);
    }
    if (s_fullDump) {
        wcscpy(path, L"\\\\?\\");
        wcscat(path, s_dumpDirectory);
        wcscat(path, s_dumpName);
        wcscat(path, stamp);
        wcscat(path, L".dmp");
        file = ::CreateFileW(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (file != INVALID_HANDLE_VALUE) {
            ::MiniDumpWriteDump(::GetCurrentProcess(), process, file,
                                (MINIDUMP_TYPE)(MiniDumpWithDataSegs | MiniDumpWithFullMemory |
                                                MiniDumpWithHandleData),   // 7
                                &information, 0, 0);
            ::CloseHandle(file);
        }
    }

    wcscpy(path, L"Caught exception, stack trace saved at '");
    wcscat(path, s_dumpDirectory);
    wcscat(path, s_dumpName);
    wcscat(path, stamp);
    wcscat(path, L".mdmp'.\r\n\r\nThe application will close now.");
    Trace(String((const UniChar16*)path), true);
    ::ExitProcess(3);
    return EXCEPTION_EXECUTE_HANDLER;   // not reached
}

// Win 0x5ce8c4
int WinMainInner()
{
    ::GetTempPathW(MAX_PATH, s_dumpDirectory);
    BasicArray<String> arguments;
    String commandLine((const UniChar16*)::GetCommandLineW());
    int argumentCount = 0;
    int characterCount = 0;
    ParseCommandLine((const wchar_t*)commandLine.GetBuffer(), 0, 0, &argumentCount, &characterCount);
    // the argv table, then the characters (PORT64: the original's size assumes 4-byte pointers;
    // 64-bit sizes the table with sizeof, 32-bit keeps the original expression)
    wchar_t** argv = (wchar_t**)::operator new(
        sizeof(wchar_t*) > 4 ? argumentCount * sizeof(wchar_t*) + characterCount * sizeof(wchar_t)
                             : (characterCount + argumentCount * 2) * 2);
    wchar_t* characters = (wchar_t*)(argv + argumentCount);
    ParseCommandLine((const wchar_t*)commandLine.GetBuffer(), argv, characters, &argumentCount,
                     &characterCount);
    for (int i = 0; i < argumentCount - 1; ++i)
        arguments.PushBack(String((const UniChar16*)argv[i]));
    ::operator delete(argv);

    // "-TraceName <name>": the first option of that name (options are skipped with their value)
    for (String* it = arguments.Begin(); it != arguments.End();) {
        if (!it->StartsWith((UniChar16)'-')) {
            ++it;
            continue;
        }
        String* value = it + 1;
        if (value == arguments.End())
            break;
        if (it->CompareNoCase(String("-TraceName")) == 0) {
            SetTraceName(*value);
            break;
        }
        it += 2;
    }
    setlocale(LC_NUMERIC, "C");
    Thread thread;
    thread.Attach();
    int result = AppMain(arguments);
    thread.Detach();
    return result;
}

} // namespace LEGO

// Win 0x5cea85
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    int result = -1;
    ::SetUnhandledExceptionFilter(LEGO::HandleCrash);
#if defined(_MSC_VER)
    __try {
        result = LEGO::WinMainInner();
    } __except (LEGO::HandleCrash(GetExceptionInformation())) {
    }
#else
    // NOTE: no SEH __try with the mingw toolchain; the unhandled-exception filter above still
    // catches what the original's __except (filter funclet 0x5ceaae) would
    result = LEGO::WinMainInner();
#endif
    return result;
}

#endif // _WIN32
