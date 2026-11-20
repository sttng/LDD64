// LEGO Digital Designer — restored source
// LDDMain — the application's start-up: built-in preference defaults, the main window with its
//   OpenGL render and control manager, AppMain (declared in Main.h: application, Lua bindings and
//   scripts, cursor, StartMode, scene manager / control, modes, commands, Lua console, the main
//   loop and the teardown), the error handler that shows LDD errors as message boxes, the
//   graphics-failure handler (OpenGLCrashReport.h) and the Win single-instance hand-off.
//
// TU 060 "LDDMain.cpp" (file name inferred): Mac 0x138d40-
// 0x1423cf (sub-TUs 060a 0x138d40-0x13fad0, 060b 0x13fad0-0x1423d0), Win 0x5c7b1e-0x5cd662 with
// gaps (+ the Win-only single instance 0x5c7ba0 / 0x5c85bb, ). BitmapMaskFactory
// (0x13ff20-0x1402e0) is restored in Base/BitmapMask.cpp; the header-inline helpers of
// StringStream.h (0x1423d0-) are emitted here on the Mac.
// All names of this file are inferred (no symbols); the RTTI names LDDErrorHandler, ExitCommand
// and BackgroundControlObserver are original.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "Error.h"
#include "Preferences.h"
#include "Command.h"
#include "Control.h"
#include "Window.h"

namespace LEGO {

class OpenGLRender;
class RenderControlManager;
class BrickConnectSceneControl;
class ReportedError;

// The error facility of LDD's own errors ('LDD ' application errors; Mac 0x77d600 (initialised
// by the TU's static initialiser), Win 0xab0010 = {0x204c4444, 9}).
const UInt64 kLDDErrorFacility = 0x00000009204c4444ULL;

// An error code of severity error in the LDD facility. Win 0x5c7b1e (Mac inlined)
ErrorCode MakeLDDError(UInt16 value);

// The built-in preference defaults (window geometry, URLs, folder names, switches, the
// "LEGO Creations" folders below the documents). Mac 0x138d40, Win 0x5ca35e
void SetDefaultPreferences(Preferences::ValueMap& defaults);

// The main window's style from the preferences: minimum size, size (at least the minimum),
// origin clamped into the virtual desktop, title "LEGO Digital Designer", shown (Win: icon 101).
// Mac 0x139e20, Win 0x5c872f
void GetMainWindowStyle(WindowStyle& style);

// The main window (LDDWindow with the CompatibilityModeLevel preference; the default geometry
// 1024 x 768 at (0, 0) when the stored one fails), its OpenGLRender (Render::CreateArgs
// 2000 / 0), the image loader's render and "Graphics/imagedescriptions.xml", the
// RenderControlManager (layer 1 takes mouse + keyboard input and is hit-testable), the
// TooltipManager, the bitmap codecs and the BitmapMask factory. Reports "Could not create
// window" (0 / 5), "Requires OpenGL version 1.1 or above" (1), "Could not open Assets
// directory" (0x11), "Could not load image descriptions" (2) and returns false on failure.
// Mac 0x13a1c0, Win 0x5cb675
bool CreateMainWindow(RefPointer<Window>& window, RefPointer<OpenGLRender>& render,
                      RefPointer<RenderControlManager>& controlManager);

// Shows every LDD error of a reported error tree as a message box ("SETUPERRORTITLE"); error 8
// (the brick database) also sets the "Cleanup" preference so that the next start reinstalls.
// RTTI LEGO::LDDErrorHandler. Mac vtable {D1 0x13fad0, D0 0x1406f0, HandleError 0x140700,
// Visit 0x140710} (both jump to 0x140770), VisitFunction-in {0x140720, 0x140730, 0x140750};
// Win vtables 0x9c95ac / 0x9c95a0 (constructor inlined 0x5c7b39, body 0x5c847b).
class LDDErrorHandler : public ErrorHandler, public VisitFunction {
public:
    LDDErrorHandler() {}
    virtual ~LDDErrorHandler() {}                                        // Mac 0x13fad0/0x1406f0, Win 0x5c7b76
    virtual void HandleError(ReportedError& error);                      // Mac 0x140700, Win 0x5c847b
    virtual void Visit(ReportedError& error);                            // Mac 0x140710 (thunk 0x140750)

private:
    // The message box of `error`, then its children. Mac 0x140770, Win 0x5c847b
    void Show(ReportedError& error);
};

// "ldd.commands.common.quit": Application::Quit. RTTI LEGO::ExitCommand. Mac vtable {D1
// 0x141790, D0 0x141800, OnFinalRelease 0x1414d0, Do 0x141880, SetEnable, GetEnable,
// UpdateEnable 0x1418a0}; Win Do 0x5c9c8f (constructor 0x5c9c21, sdd 0x5c9dc5). Mac sizeof 0x30.
class ExitCommand : public Command {
public:
    ExitCommand() : Command(true) {}                                     // (inline, Mac AppMain 0x13cc88)
    virtual ~ExitCommand() {}                                            // Mac 0x141790/0x141800
    virtual void Do();                                                   // Mac 0x141880, Win 0x5c9c8f
    virtual void UpdateEnable();                                         // Mac 0x1418a0
};

// Observes the background layer (the manager's layer 1): when it gets the keyboard focus, the
// scene control takes it. RTTI LEGO::BackgroundControlObserver. Mac vtable {D1 0x141280, D0
// 0x141350, …, OnKeyboardFocusGained 0x141390 (27)}, Object-in {0x1413b0, 0x1413d0, …}; Win
// constructor 0x5c956b, OnKeyboardFocusGained 0x5c95ec, sdd 0x5c9730. Mac sizeof 0x20.
class BackgroundControlObserver : public ControlObserver {
public:
    explicit BackgroundControlObserver(BrickConnectSceneControl* sceneControl);  // (inline, Mac 0x13bf7b)
    virtual ~BackgroundControlObserver();                                 // Mac 0x141280/0x141350
    virtual void OnKeyboardFocusGained(Control* control, bool flag, Control* previous,
                                       UInt32 modifiers);                 // Mac 0x141390, Win 0x5c95ec

private:
    RefPointer<BrickConnectSceneControl> m_sceneControl;   // Mac +0x14
};

#if defined(_WIN32)
// Single instance: creates the named mutex "LDD20"; when it existed already,
// finds the running LDD window (class "DCLWindowClass", title containing `applicationName`),
// posts it the file of the "Open" preference as a global atom (message 0x8000), brings it to
// the front (restored when minimized) and returns true (AppMain then returns 0). Win 0x5c85bb
bool ActivateOtherInstance(const String& applicationName);
#endif

} // namespace LEGO
