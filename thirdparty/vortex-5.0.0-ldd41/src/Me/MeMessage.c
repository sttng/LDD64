/* Vortex 5.0.0 build 800 "vortexldd41" - Me message channels (restored).
 * Mac __text 0x489d63-0x48a0f0 (TU 530 Me block); Win 0x752850-0x752cf0 (handler table
 * 0xabc6e0-0xabc6fc, levels 0xaf1ff4-0xaf2000). Win also keeps a fourth "debug" channel
 * (dead-stripped on Mac). */
#include "Me/MeMessage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <windows.h>
#endif

/* Win: the CRT's _vsnprintf / _snprintf (VS2008's CRT has no C99 vsnprintf / snprintf; MinGW
   has both names). NOTE: as in the original, a truncated message is not NUL-terminated. */
#if defined(_WIN32)
#define ME_VSNPRINTF _vsnprintf
#define ME_SNPRINTF _snprintf
#else
#define ME_VSNPRINTF vsnprintf
#define ME_SNPRINTF snprintf
#endif

#if defined(_WIN32)
/* Win 0x752870: info/warning output: fputs(text, stderr) */
static void MEAPI MeDefaultOutputWin(int level, const char* text)
{
    (void)level;
    fputs(text, stderr);
}
/* Win 0x7528b0: fatal-error output (name inferred) */
static void MEAPI MeMessageBoxOutput(int level, const char* text)
{
    (void)level;
    MessageBoxA(0, text, "Vortex Toolkit", 0x12030);
}
/* Win 0x752890: debug output does nothing */
static void MEAPI MeNullOutput(int level, const char* text)
{
    (void)level;
    (void)text;
}
#endif

/* Mac 0x48a0c6 */
void MEAPI MeDefaultOutput(int level, const char* text)
{
    (void)level;
    fputs(text, stderr);
}

int MeInfoLevel = 0;      /* Mac 0x7794a0, Win 0xaf1ff4 */
int MeWarningLevel = 0;   /* Mac 0x7794a4, Win 0xaf1ff8 */
int MeFatalErrorLevel = 0; /* Win 0xaf1ffc (only set, by VxSetFatalErrorLevel) */
int MeDebugLevel = 0;     /* Win 0xaf2000 */
#if defined(_WIN32)
MeOutputFn MeInfoOutput = MeDefaultOutputWin;        /* Win 0xabc6e0 */
MeOutputFn MeWarningOutput = MeDefaultOutputWin;     /* Win 0xabc6e4 */
MeOutputFn MeFatalErrorOutput = MeMessageBoxOutput;  /* Win 0xabc6e8 */
MeOutputFn MeDebugOutput = MeNullOutput;             /* Win 0xabc6ec */
#else
MeOutputFn MeInfoOutput = MeDefaultOutput;           /* Mac 0x7794a8 */
MeOutputFn MeWarningOutput = MeDefaultOutput;        /* Mac 0x7794ac */
MeOutputFn MeFatalErrorOutput = MeDefaultOutput;     /* Mac 0x7794b0 */
MeOutputFn MeDebugOutput = MeDefaultOutput;          /* (not in the Mac binary) */
#endif
MeMessageHandler MeInfoHandler = MeDefaultInfoHandler;             /* Mac 0x7794b4, Win 0xabc6f0 */
MeMessageHandler MeWarningHandler = MeDefaultWarningHandler;       /* Mac 0x7794b8, Win 0xabc6f4 */
MeMessageHandler MeFatalErrorHandler = MeDefaultFatalErrorHandler; /* Mac 0x7794bc, Win 0xabc6f8 */
MeMessageHandler MeDebugHandler = MeDefaultDebugHandler;           /* Win 0xabc6fc */

/* Mac 0x489d63, Win 0x752ba0 */
MeMessageHandler MEAPI MeSetInfoHandler(MeMessageHandler handler)
{
    MeMessageHandler old = MeInfoHandler;
    MeInfoHandler = handler;
    return old;
}

/* Mac 0x489d7d, Win 0x752bc0 */
MeMessageHandler MEAPI MeSetWarningHandler(MeMessageHandler handler)
{
    MeMessageHandler old = MeWarningHandler;
    MeWarningHandler = handler;
    return old;
}

/* Mac 0x489d97, Win 0x752be0 */
MeMessageHandler MEAPI MeSetFatalErrorHandler(MeMessageHandler handler)
{
    MeMessageHandler old = MeFatalErrorHandler;
    MeFatalErrorHandler = handler;
    return old;
}

/* Win 0x752c00 */
MeMessageHandler MEAPI MeSetDebugHandler(MeMessageHandler handler)
{
    MeMessageHandler old = MeDebugHandler;
    MeDebugHandler = handler;
    return old;
}

/* Win 0x752b20 / 0x752b40 / 0x752b60 / 0x752b80 */
MeOutputFn MEAPI MeSetInfoOutput(MeOutputFn fn)
{
    MeOutputFn old = MeInfoOutput;
    MeInfoOutput = fn;
    return old;
}
MeOutputFn MEAPI MeSetWarningOutput(MeOutputFn fn)
{
    MeOutputFn old = MeWarningOutput;
    MeWarningOutput = fn;
    return old;
}
MeOutputFn MEAPI MeSetFatalErrorOutput(MeOutputFn fn)
{
    MeOutputFn old = MeFatalErrorOutput;
    MeFatalErrorOutput = fn;
    return old;
}
MeOutputFn MEAPI MeSetDebugOutput(MeOutputFn fn)
{
    MeOutputFn old = MeDebugOutput;
    MeDebugOutput = fn;
    return old;
}

/* Mac 0x489db1, Win 0x752cb0: the path separator is '/' on Mac and '\\' on Windows */
const char* MEAPI MeFileNameFromPath(const char* path)
{
#if defined(_WIN32)
    const char sep = '\\';
#else
    const char sep = '/';
#endif
    const char* p = strchr(path, sep);
    while (p != 0) {
        path = p + 1;
        p = strchr(path, sep);
    }
    return path;
}

/* Mac 0x489dee, Win 0x752c40 */
void MeWarning(int level, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    MeWarningHandler(level, format, ap);
    va_end(ap);
}

/* Mac 0x489e1e, Win 0x752c20 */
void MeInfo(int level, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    MeInfoHandler(level, format, ap);
    va_end(ap);
}

/* Mac 0x489e4e (Win 0x752c60 prints with printf) */
void MeFatalError(int level, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
#if defined(_WIN32)
    printf("VxFatalError\n");
#else
    puts("VxFatalError");
#endif
    MeFatalErrorHandler(level, format, ap);
    va_end(ap);
}

/* Win 0x752c90 */
void MeDebug(int level, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    MeDebugHandler(level, format, ap);
    va_end(ap);
}

/* Mac 0x489e8c, Win 0x752960 */
void MEAPI MeDefaultWarningHandler(int level, const char* format, va_list args)
{
    char buf[5000];
    if (level <= MeWarningLevel) {
        ME_SNPRINTF(buf, 5000, "VxWarning{%d}: ", level);
        MeWarningOutput(level, buf);
        ME_VSNPRINTF(buf, 5000, format, args);
        MeWarningOutput(level, buf);
    }
}

/* Mac 0x489f4d, Win 0x7528d0 */
void MEAPI MeDefaultInfoHandler(int level, const char* format, va_list args)
{
    char buf[5000];
    if (level <= MeInfoLevel) {
        ME_SNPRINTF(buf, 5000, "VxInfo{%d}: ", level);
        MeInfoOutput(level, buf);
        ME_VSNPRINTF(buf, 5000, format, args);
        MeInfoOutput(level, buf);
    }
}

/* Mac 0x48a00e, Win 0x7529f0: prints, then exit(1) */
void MEAPI MeDefaultFatalErrorHandler(int level, const char* format, va_list args)
{
    char buf[5000];
    ME_SNPRINTF(buf, 5000, "VxFatalError{%d}: ", level);
#if defined(_WIN32)
    printf("%s\n", buf);
#else
    puts(buf);
#endif
    MeFatalErrorOutput(level, buf);
    ME_VSNPRINTF(buf, 5000, format, args);
#if defined(_WIN32)
    printf("%s\n", buf);
#else
    puts(buf);
#endif
    MeFatalErrorOutput(level, buf);
    exit(1);
}

/* Win 0x752a90 */
void MEAPI MeDefaultDebugHandler(int level, const char* format, va_list args)
{
    char buf[5000];
    if (level <= MeDebugLevel) {
        ME_SNPRINTF(buf, 5000, "VxDebug{%d}: ", level);
        MeDebugOutput(level, buf);
        ME_VSNPRINTF(buf, 5000, format, args);
        MeDebugOutput(level, buf);
    }
}
