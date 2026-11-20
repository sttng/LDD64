// LEGO Digital Designer — restored source
// OpenGL crash guard: renderer GL calls sit in try/catch(...) (Win /EHa also catches driver
// access violations); the handler reports __FILE__/__LINE__ once per run, the application shows
// "GraphicFailureText" and raises CompatibilityModeLevel for the next start.
// TU 282 (names inferred): Mac 0x295000-0x29522f; Win 0x51cd0f.
#pragma once
#include "LEGO/Types.h"

namespace LEGO {

// Traces "Detected OpenGL crash when called from: <file>, <line>, compatibility mode level:
// <n>." and calls ShowGraphicFailure — only for the first crash.
void ReportOpenGLCrash(const char* file, UInt32 line);               // Mac 0x295000, Win 0x51cd0f

// True once ReportOpenGLCrash was called. (Mac 0x77e8e9, Win 0xb42747; reader not located)
bool HasOpenGLCrashed();

// Application start-up code (TU 060: Mac 0x13fae0, Win 0x5c8a3b; in LDDMain.cpp):
// "GraphicFailureTitle" / "GraphicFailureText" message, raises CompatibilityModeLevel.
void ShowGraphicFailure(const char* file, UInt32 line);

} // namespace LEGO

// Wraps a block of GL calls like the original's try { … } catch (...) { report }.
#define LDD_GL_TRY   try {
#define LDD_GL_CATCH } catch (...) { LEGO::ReportOpenGLCrash(__FILE__, __LINE__); }
