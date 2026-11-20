// LEGO Digital Designer — restored source
// OpenGLCrashReport.cpp (TU 282, name inferred): Mac 0x295000 (+ <iostream> initialiser
// 0x2951f0), Win 0x51cd0f. See OpenGLCrashReport.h.
#include "OpenGLCrashReport.h"
#include "LEGO/String.h"
#include "StringStream.h"
#include "Preferences.h"
#include "LEGO/Assert.h"
#include "Trace.h"

namespace LEGO {

namespace {
bool   s_openGLCrashed = false;   // Mac 0x77e8e9, Win 0xb42747
bool   s_inCrashReport = false;   // Mac 0x77e8ea, Win 0xb42764
#if defined(_WIN32)
UInt32 s_crashCount = 0;          // Win 0xb4275c
#else
UInt32 s_crashReported = 0;       // Mac 0x77e8ec
#endif
} // namespace

bool HasOpenGLCrashed()
{
    return s_openGLCrashed;
}

// Mac 0x295000, Win 0x51cd0f
void ReportOpenGLCrash(const char* file, UInt32 line)
{
    s_openGLCrashed = true;
#if defined(_WIN32)
    // Win: nothing while an assert box (or an earlier report) is up
    if (IsInsideAssert() || s_inCrashReport)
        return;
    s_inCrashReport = true;
    EnterAssert();
    ++s_crashCount;
#else
    // Mac: only the first crash is reported
    if (s_crashReported != 0 || s_inCrashReport)
        return;
    s_inCrashReport = true;
    s_crashReported = 1;
    EnterAssert();
#endif
    UInt32 level = 0;
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        String value;
        if (preferences->GetValue(String("CompatibilityModeLevel"), value))
            FromString(level, value);
    }
    String message;
    message << "Detected OpenGL crash when called from: " << file << ", " << line
            << ", compatibility mode level: " << level << ".";
    Trace(message, true);
    ShowGraphicFailure(file, line);
    s_inCrashReport = false;
}

} // namespace LEGO
