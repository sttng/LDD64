// Vortex 5.0.0 build 800 "vortexldd41" - message API (restored header).
// Three channels, each a global handler pointer plus a level threshold:
//   info    Mac 0x7794b4 (default 0x489f4d prints "VxInfo{%d}: "),       level 0x7794a0
//   warning Mac 0x7794b8 (default 0x489e8c prints "VxWarning{%d}: "),    level 0x7794a4
//   fatal   Mac 0x7794bc (default 0x48a00e prints "VxFatalError{%d}: "), Win: MessageBox
//           "Vortex Toolkit" 0x7528b0 through the handler table 0xabc6d0
// The functions forward (level, format, va_list) to the handler; the default handlers print when
// level <= the threshold. LDD (Mac TU 422) sets both levels to 0 and installs three empty
// handlers (Mac 0x39e8c0 / 0x39e8d0 / 0x39e8e0): nothing is ever printed.
#ifndef VX_VXMESSAGE_H
#define VX_VXMESSAGE_H

#include <stdarg.h>
#include "Me/MeMessage.h"

namespace Vx {

// Handler (level, format, args) = MeMessageHandler: __stdcall on Win (Me/MeCall.h), so handlers
// LDD installs must be declared MEAPI.
typedef MeMessageHandler VxMessageHandler;

// Mac 0x463cbb (name: "VxFatalError{%d}" prefix of its default handler)
void VxFatalError(int level, const char* format, ...);
// Mac 0x463ced, Win 0x70db10 (prefix "VxWarning{%d}")
void VxWarning(int level, const char* format, ...);
// Mac 0x463d1f, Win 0x70daf0 (prefix "VxInfo{%d}")
void VxInfo(int level, const char* format, ...);
// Mac 0x463c82, Win 0x70db50: if (!condition) the fatal handler gets (0, format, args).
// (name inferred) e.g. VxAssert(mass > 0, "VxPart::setMass: mass must be larger than 0.\n")
void VxAssert(bool condition, const char* format, ...);

// Mac 0x463c4e: sets the info level (Mac 0x7794a0), returns the previous one (name inferred)
int VxSetInfoLevel(int level);
// Mac 0x463c68: sets the warning level (Mac 0x7794a4), returns the previous one (name inferred)
int VxSetWarningLevel(int level);

// Mac 0x463d77 -> 0x489d63: info handler (Mac 0x7794b4), returns the previous (name inferred)
VxMessageHandler VxSetInfoHandler(VxMessageHandler handler);
// Mac 0x463d64 -> 0x489d7d: warning handler (Mac 0x7794b8) (name inferred)
VxMessageHandler VxSetWarningHandler(VxMessageHandler handler);
// Mac 0x463d51 -> 0x489d97: fatal-error handler (Mac 0x7794bc) (name inferred)
VxMessageHandler VxSetFatalErrorHandler(VxMessageHandler handler);
// Win 0x70daa0: a third level (Win 0xaf1ffc, read by nothing that LDD links) (name inferred)
int VxSetFatalErrorLevel(int level);

// These are thin wrappers of the Me layer (Me/MeMessage.h): the handlers and levels are
// MeInfoHandler / MeWarningHandler / MeFatalErrorHandler and MeInfoLevel / MeWarningLevel.

} // namespace Vx

#endif
