// LEGO Digital Designer — restored source
// LEGO::HTMLBICommand — the building-guide mode's "HTML building instructions" command
//   (created by BGAutomaticMode::Initialize, Mac 0x353b3a / Win 0x6260dd): Do starts the
//   HTMLBIGenerator it owns; enabled while the scene is not empty. It is an
//   HTMLBIGeneratorObserver (disabled while a generation runs) but never attaches itself, and the
//   generator sends no notifications (both builds).
//
// TU 240 "HTMLBICommand.cpp" (Mac 0x270bf0-0x27163b, 30 functions; Win 0x65e6ef-0x65ea78).
// The TU also emits HTMLBIGeneratorObserver and Observer<HTMLBIGenerator,
// HTMLBIGeneratorObserver> (HTMLBIGenerator.h). Static initialiser 0x2715c0 = <iostream> + the
// 24-byte header static (see HTMLBIGenerator.cpp) — not code.
//
// RTTI: vmi {Command +0 (public), HTMLBIGeneratorObserver +0x28 (public)}, virtual base Object.
// Mac vtables: primary 0x738d4c {D1 0x271170, D0 0x2711d0, OnFinalRelease 0x271240, Do 0x270e40,
//   SetEnable 0x372e30, GetEnable 0x3730b0, UpdateEnable 0x270d60, OnGenerationStarted 0x270da0,
//   OnGenerationFinished 0x270de0, OnGenerationEvent 0x270e20}; observer-in 0x738d80 {0x2712d0,
//   0x271330, 0x2713a0, OnAttached, OnDetached, thunks 0x270dc0 / 0x270e00, 0x270e30};
//   Object-in 0x738db8 {0x2713c0, 0x271420, 0x2714a0, AddRef, Release}; base object destructor
//   with VTT 0x2714c0.
//   Win: Command-in 0x9d70b4 {Do 0x65e8b1, SetEnable, GetEnable, UpdateEnable 0x65e6ef};
//   observer-in 0x9d709c {OnAttached, OnDetached, 0x65e716, 0x65e721, 0x6943f1 (empty)};
//   Object-in 0x9d7088 {sdd 0x65e991 -> 0x65ea36, OnFinalRelease 0x65ea8c}. Constructor
//   Win 0x65e8bd.
// Layout (Mac / Win): Command +0x00 / +0x00, observer +0x28 / +0x2c, m_generator +0x30 / +0x38,
//   m_sceneControl +0x34 / +0x3c, m_window +0x38 / +0x40, Object +0x3c / +0x44.
// Member and method names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Command.h"
#include "HTMLBIGenerator.h"

namespace LEGO {

class BrickConnectSceneControl;
class BrickConnectSceneManager;
class RenderControlManager;
class BGMode;
class Window;

class HTMLBICommand : public Command, public HTMLBIGeneratorObserver {
public:
    // Enabled; creates (and references) the generator. NOTE: neither attaches to the generator
    // nor calls UpdateEnable (both builds). Mac 0x270bf0, Win 0x65e8bd
    HTMLBICommand(BrickConnectSceneControl* sceneControl, RenderControlManager* manager,
                  BrickConnectSceneManager* sceneManager, BGMode* mode, Window* window);
    virtual ~HTMLBICommand();                                                // Mac 0x271170/0x2711d0, Win sdd 0x65ea36

    // Generate(window). Mac 0x270e40, Win 0x65e8b1
    virtual void Do();
    // Enabled while the scene is not empty (NOTE: no null checks). Mac 0x270d60, Win 0x65e6ef
    virtual void UpdateEnable();

    // ---- HTMLBIGeneratorObserver ----
    // Disabled while generating. Mac 0x270da0 (thunk 0x270dc0), Win 0x65e716
    virtual void OnGenerationStarted();
    // Mac 0x270de0 (thunk 0x270e00), Win 0x65e721
    virtual void OnGenerationFinished();
    // Nothing. Mac 0x270e20 (thunk 0x270e30), Win 0x6943f1 (shared empty body)
    virtual void OnGenerationEvent();

protected:
    // Observer part, then NotifierT<Command>'s. Mac 0x271240 (thunks 0x2713a0 / 0x2714a0),
    // Win 0x65ea78
    virtual void OnFinalRelease();

    RefPointer<HTMLBIGenerator> m_generator;     // +0x30 / +0x38
    BrickConnectSceneControl*   m_sceneControl;  // +0x34 / +0x3c (not referenced)
    Window*                     m_window;        // +0x38 / +0x40
                                                 // Object +0x3c / +0x44
};

} // namespace LEGO
