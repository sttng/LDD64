/* Vortex 5.0.0 build 800 "vortexldd41" - Me message layer (restored).
 *
 * C API of the MathEngine/Karma heritage (src/Me/MeMessage.c). Three channels, each with a
 * handler pointer, plus an output function per channel and a level threshold for info/warning:
 *
 *   Mac data (Win: the table at 0xabc6d0)    default
 *   0x7794a0 MeInfoLevel                     0
 *   0x7794a4 MeWarningLevel                  0
 *   0x7794a8 MeInfoOutput                    MeDefaultOutput (fputs to stderr)
 *   0x7794ac MeWarningOutput                 MeDefaultOutput
 *   0x7794b0 MeFatalErrorOutput              MeDefaultOutput (Win: MessageBox "Vortex Toolkit")
 *   0x7794b4 MeInfoHandler                   MeDefaultInfoHandler     "VxInfo{%d}: "
 *   0x7794b8 MeWarningHandler                MeDefaultWarningHandler  "VxWarning{%d}: "
 *   0x7794bc MeFatalErrorHandler             MeDefaultFatalErrorHandler "VxFatalError{%d}: "
 *
 * The C++ Vx layer (Vx/VxMessage.h: Vx::VxInfo, Vx::VxWarning, ...) calls the same handler
 * pointers. Default handlers print when level <= the channel's level; the fatal one always
 * prints and exits(1). Names marked (name inferred) are not in either binary.
 */
#ifndef ME_MEMESSAGE_H
#define ME_MEMESSAGE_H

#include "Me/MeCall.h"

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (MEAPI *MeMessageHandler)(int level, const char* format, va_list args);
typedef void (MEAPI *MeOutputFn)(int level, const char* text); /* (name inferred) */

extern int MeInfoLevel;                    /* Mac 0x7794a0 (name inferred) */
extern int MeWarningLevel;                 /* Mac 0x7794a4 (name inferred) */
extern MeOutputFn MeInfoOutput;            /* Mac 0x7794a8 (name inferred) */
extern MeOutputFn MeWarningOutput;         /* Mac 0x7794ac (name inferred) */
extern MeOutputFn MeFatalErrorOutput;      /* Mac 0x7794b0 (name inferred) */
extern MeMessageHandler MeInfoHandler;     /* Mac 0x7794b4 (name inferred) */
extern MeMessageHandler MeWarningHandler;  /* Mac 0x7794b8 (name inferred) */
extern MeMessageHandler MeFatalErrorHandler; /* Mac 0x7794bc (name inferred) */
/* Win only (dead-stripped on Mac): a debug channel, level Win 0xaf2000, output 0xabc6ec,
 * handler 0xabc6fc ("VxDebug{%d}: ") (names inferred) */
extern int MeDebugLevel;
extern int MeFatalErrorLevel; /* Win 0xaf1ffc: set by VxSetFatalErrorLevel, never read */
extern MeOutputFn MeDebugOutput;
extern MeMessageHandler MeDebugHandler;
void MeDebug(int level, const char* format, ...);                /* Win 0x752c90 */
MeMessageHandler MEAPI MeSetDebugHandler(MeMessageHandler handler);    /* Win 0x752c00 */
void MEAPI MeDefaultDebugHandler(int level, const char* format, va_list args); /* Win 0x752a90 */
/* output setters, Win 0x752b20 / 0x752b40 / 0x752b60 / 0x752b80 (names inferred) */
MeOutputFn MEAPI MeSetInfoOutput(MeOutputFn fn);
MeOutputFn MEAPI MeSetWarningOutput(MeOutputFn fn);
MeOutputFn MEAPI MeSetFatalErrorOutput(MeOutputFn fn);
MeOutputFn MEAPI MeSetDebugOutput(MeOutputFn fn);

/* Mac 0x489e1e, Win 0x752c20 (name inferred): MeInfoHandler(level, format, args) */
void MeInfo(int level, const char* format, ...);
/* Mac 0x489dee, Win 0x752c40 (name inferred): MeWarningHandler(level, format, args) */
void MeWarning(int level, const char* format, ...);
/* Mac 0x489e4e (name inferred): puts("VxFatalError") then MeFatalErrorHandler(...) */
void MeFatalError(int level, const char* format, ...);

/* Mac 0x489d63 / 0x489d7d / 0x489d97 (names inferred): install a handler, return the old one */
MeMessageHandler MEAPI MeSetInfoHandler(MeMessageHandler handler);
MeMessageHandler MEAPI MeSetWarningHandler(MeMessageHandler handler);
MeMessageHandler MEAPI MeSetFatalErrorHandler(MeMessageHandler handler);

/* Default handlers: Mac 0x489f4d / Win 0x7528d0, Mac 0x489e8c / Win 0x752960,
 * Mac 0x48a00e / Win 0x7529f0 (names inferred) */
void MEAPI MeDefaultInfoHandler(int level, const char* format, va_list args);
void MEAPI MeDefaultWarningHandler(int level, const char* format, va_list args);
void MEAPI MeDefaultFatalErrorHandler(int level, const char* format, va_list args);
/* Mac 0x48a0c6 (name inferred): fputs(text, stderr) */
void MEAPI MeDefaultOutput(int level, const char* text);

/* Mac 0x489db1, Win 0x752cb0 (name inferred): the part of path after the last '/' */
const char* MEAPI MeFileNameFromPath(const char* path);

#ifdef __cplusplus
}
#endif

#endif
