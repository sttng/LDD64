// LEGO Digital Designer — restored source
// LEGO::BrickMode — a Mode with the 3D scene: a toolbar strip at the top, the client area below
//   it, an optional side panel inside the client area; while the mode is current the shared
//   scene holder (BrickModeGlobalObject, holding the BrickConnectSceneControl) sits in the
//   client area next to the side panel. Base of EditMode, ViewMode, BGMode.
// LEGO::DefaultPosAndSizeControl — element type of a BrickMode array (only its RTTI name is known).
//
// Home TU 084 "BrickModeGlobalObject.cpp" (Mac 0x181830-0x18554b, Win 0x62feb3-0x631090 with
// gaps); bodies in BrickModeGlobalObject.cpp. Lua class "BrickMode" (LuaBrickMode, TU 379).
//
// Mac layout (constructor 0x1831f0, C2 with VTT): Mode +0x000..+0x24c, ControlObserver +0x24c
//   (subjects +0x250..+0x260), members below, Object +0x280. Win: ControlObserver +0x268,
//   side panel +0x294, scene control +0x298, client +0x29c, Object +0x2a4 (vtable 0x9d2aa8).
// Virtual slots: Mac 0x7149ac (50 entries) = Mode's 49 (Enter 0x181830, Exit 0x181c50,
//   OnFinalRelease 0x183eb0 overridden) + the Mac-only primary entry 49 of the
//   ControlObserver::OnRectangleChanged overrider (0x183680, thunk 0x183690); Win 0x9d2b5c
//   (Mode's 44: Enter 0x630be3, Exit 0x630748), ControlObserver-in 0x9d2abc (slot 6 0x630c6f).
// Names: GetClientControl / GetToolbarControl are the Lua names (LuaBrickMode); the rest is
// inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Mode.h"
#include "Control.h"

namespace LEGO {

class BrickConnectSceneControl;
class BrickConnectSceneManager;
class RenderControl;

// Released through a non-virtual Object base (Mac BrickMode destructor 0x1836b0). Nothing in
// the restored TUs creates one; members not analysed. (RTTI name only)
class DefaultPosAndSizeControl : public Object {
public:
    virtual ~DefaultPosAndSizeControl() {}
};

class BrickMode : public Mode, public ControlObserver {
public:
    // Mac 0x1831f0 (C2), Win 0x630519: the scene control is shared (not referenced).
    BrickMode(ControlManager* manager, Window* window, BrickConnectSceneControl* sceneControl);
    virtual ~BrickMode();                                                   // Mac 0x183930/0x183bc0, Win sdd 0x630c4d

    // Mode::AttachTo, then the toolbar strip (RenderControl, top, full width) and the client
    // area (Control, from the toolbar's bottom to the bottom), then
    // BrickModeGlobalObject::Create. False when any step fails. Mac 0x183370 (name inferred)
    bool Create(Control* parent, Control* after);

    // ---- Mode ----
    // Mode::Enter; observes the client area, moves the scene holder into it (first child) and
    // the side panel too, then lays them out.
    virtual void Enter();                                                   // 45/40 Mac 0x181830, Win 0x630be3
    // Stops observing the side panel and the client area; Mode::Exit.
    virtual void Exit();                                                    // 46/41 Mac 0x181c50, Win 0x630748

    // ---- ControlObserver: the client area or the side panel changed size → Layout ----
    using RenderControl::OnRectangleChanged;
    virtual void OnRectangleChanged(Control* control, const Vector2& oldPosition,
                                    const Vector2& oldSize);                // 49/– Mac 0x183680 (thunk 0x183690), Win 0x630c6f

    Control* GetClientControl() const;                                      // Mac 0x181c90 (Lua name)
    Control* GetToolbarControl() const;                                     // Mac 0x181ca0 (Lua name)
    BrickConnectSceneControl* GetSceneControl() const;                      // Mac 0x183670, Win 0x62febe
    BrickConnectSceneManager* GetSceneManager() const;                      // Mac 0x183650, Win 0x62feb3
    // The side panel inside the client area (set by the subclasses). (accessors, names inferred)
    Control* GetSidePanel() const { return m_sidePanel; }
    void SetSidePanel(Control* panel) { m_sidePanel = panel; }

protected:
    // Detaches from the observed controls; Mode::OnFinalRelease.
    virtual void OnFinalRelease();                                          // 2/– Mac 0x183eb0 (thunks 0x183f20 …)

    // The scene holder fills the client area right of the side panel (at least its minimum
    // width; the side panel shrinks to its minimum first), the side panel ends where it starts.
    // Mac 0x1819d0, Win 0x630826 (name inferred)
    void Layout();

    // ---- data (Mac offsets) ----
    RefPointer<RenderControl>                     m_toolbar;       // +0x260 (500-byte RenderControl)
    BasicArray<RefPointer<DefaultPosAndSizeControl> > m_defaultPosAndSizeControls; // +0x264 (inferred)
    RefPointer<Control>                           m_sidePanel;     // +0x274 / Win +0x294
    BrickConnectSceneControl*                     m_sceneControl;  // +0x278 / Win +0x298
    RefPointer<Control>                           m_client;        // +0x27c / Win +0x29c (0x160-byte Control)
                                                                   // Object +0x280
};

} // namespace LEGO
