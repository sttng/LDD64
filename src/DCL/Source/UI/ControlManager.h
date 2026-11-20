// LEGO Digital Designer — restored source
// LEGO::ControlManager — the control tree of one Window: three layer roots, mouse hover and
// capture, keyboard focus, modal control, cursor, manager augmentations (tooltips). Observes the
// window's input (WindowObserver), routes it to the controls and its ControlManagerObservers;
// also an Updater and an UpdateObserver of the window's updater.
// LEGO::ControlManagerObserver = Observer<ControlManager, ...>. LEGO::HitTester (Mac only): asked
// by the native window whether a click belongs to LDD.
// TU 027 "Control.cpp" sub-TUs 027a (Mac 0xd7130-0xde100) and 027c (0xe76b0-0xeb1dc); Win
// 0x506281-0x5072e9 + helpers 0x507585-0x50788e, 0x509d52, 0x50a2e2, 0x51387d. Own file
// ControlManager.cpp by choice (the original compiled it with Control); it also holds the
// Mac-only NativeControlHitTester / ControlManagerNativeControlKeyboardFocusObserver. Directory
// UI/ is a guess.
//
// Mac layout (ctor 0xd7ad0, VTT form; sizeof 0xd4 + Object):
//   +0x00 WindowObserver (WindowInput vptr; Observer<Window> vptr +0x04, window +0x08)
//   +0x0c Notifier<ControlManagerObserver> (observers +0x10, iterations +0x20)
//   +0x30 UpdateObserver (subject +0x34)   +0x38 Updater (observers +0x3c, iterations +0x4c)
//   +0x5c … members below … +0xd4 (RenderControlManager continues), virtual base Object.
//   Object-in-ControlManager vtable 0x706e28 (offset-to-top -0xd4).
// Win (ctor 0x506446, `this` on the stack): WindowInput vfptr +0, vbptr +4, Observer<Window>
//   vfptr +8, Notifier +0x14 (observers +0x18 …), UpdateObserver vfptr +0x14?/+0x2c, Updater +0x30;
//   every member below is Mac offset + 0x10 (window +0x6c, layers +0x70, cursor +0x7c, mouse
//   control +0x80, focus +0x84, clicked controls +0x90, flags +0xa0-0xa7, mouse position +0xa8,
//   buttons +0xb0, modifiers +0xb4, modal control +0xbc, factory +0xc0, augmentations +0xc4,
//   update lock +0xe8), Object at +0xf0 (vtable 0xa27f74).
//
// Virtual slots: Mac primary vtable 0x706d00 (38 entries), Win 0xa27ec4 (34 entries):
//   Mac 0-7   HumanInput overrides (input routing)                   Win 0-7
//   Mac 8/9   D1/D0 (Win: scalar deleting dtor in the Object vtable)  —
//   Mac 10-23 WindowInput (overrides 11, 12, 13, 15, 17, 19-23)       Win 8-21
//   Mac 24    WindowObserver::OnObserverQuery24                       Win 22
//   Mac 25    OnFinalRelease overrider (Mac-only primary entry)        —
//   Mac 26    Initialize                                              Win 23
//   Mac 27    Update overrider (UpdateObserver + Updater; Mac-only)   — (Win 0x506d4c in the
//                                                                       UpdateObserver vtable)
//   Mac 28-37 ControlManager                                          Win 24-33
// ControlManagerObserver handler names: OnManagerKeyPressed, OnManagerKeyReleased,
// OnManagerModalControl, OnManagerWindowKeyboardFocusChanged, OnManagerWindowEnterMenuLoop,
// OnManagerWindowExitMenuLoop are LuaControlManagerObserver's Lua callbacks (TU 319); the others
// are inferred from the functions that send them (marked).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "Geometry.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Notifier.h"
#include "Updater.h"
#include "ObjectFactory.h"
#include "Window.h"
#include "ControlAugmentation.h"

namespace LEGO {

class Control;
class ControlManager;
class Cursor;
struct ControlDrawArgs;

// ===========================================================================================
// ControlManagerObserver — Mac vtable 0x70700c (21 entries: Observer slots 0-4, handlers 5-20,
// all empty 0xe8800-0xe88f0); Win 0x9c975c (18 entries: OnAttached, OnDetached, handlers =
// Mac slot - 3), Object-in 0x9c9748.
// ===========================================================================================
class ControlManagerObserver : public Observer<ControlManager, ControlManagerObserver> {
public:
    ControlManagerObserver() {}
    virtual ~ControlManagerObserver() {}                                     // Mac 0xe8760/0xe8790, Win 0x45a290

    // (Mac slot, Win slot = Mac - 3; sender)
    virtual void OnManagerResized() {}                                       // 5   slot 29 (name inferred)
    virtual void OnManagerMouseEnter(Control* /*control*/, const Vector2& /*position*/) {} // 6 slot 30 (name inferred)
    virtual void OnManagerMouseLeave(Control* /*control*/, const Vector2& /*position*/) {} // 7 slot 31 (name inferred)
    virtual void OnManagerMouseMove(const Vector2& /*position*/) {}          // 8   OnMouseMove (name inferred)
    virtual void OnManagerMouseDown(const Vector2& /*position*/, UInt32 /*button*/) {} // 9 OnMouseDown (name inferred)
    virtual void OnManagerMouseUp(const Vector2& /*position*/, UInt32 /*button*/) {}   // 10 OnMouseUp (name inferred)
    virtual void OnManagerMouseWheel(UInt32 /*axis*/, Real32 /*delta*/) {}   // 11  OnMouseWheel (name inferred)
    // Previous control first (Mac 0xd9120).
    virtual void OnManagerMouseOverChanged(Control* /*previous*/, Control* /*current*/) {} // 12 slot 32 (name inferred)
    // Previous focus first (Mac 0xd8a00 / 0xd7620).
    virtual void OnManagerKeyboardFocusChanged(bool /*flag*/, Control* /*previous*/, Control* /*current*/,
                                               UInt32 /*modifiers*/) {}       // 13  slot 33 (name inferred)
    virtual void OnManagerKeyPressed(UInt32 /*key*/, UInt32 /*modifiers*/, bool /*repeat*/) {} // 14 OnKeyDown
    virtual void OnManagerKeyReleased(UInt32 /*key*/, UInt32 /*modifiers*/) {} // 15 OnKeyUp
    virtual void OnManagerCharacter(UniChar16 /*c*/, UInt32 /*modifiers*/, bool /*repeat*/) {} // 16 OnCharacter (name inferred)
    virtual void OnManagerModalControl(bool /*modal*/, Control* /*control*/) {} // 17 SetModalControl
    virtual void OnManagerWindowKeyboardFocusChanged(bool /*focused*/, UInt32 /*modifiers*/) {} // 18 OnInput13
    virtual void OnManagerWindowEnterMenuLoop() {}                           // 19  OnInput22
    virtual void OnManagerWindowExitMenuLoop() {}                            // 20  OnInput23
};

#if !defined(_WIN32)
// ===========================================================================================
// HitTester (Mac only) — RTTI class with virtual base Object, abstract (no own vtable).
// [LEGOMacWindow setHitTester:] (Window::SetHitTester, Mac 0x231e0) keeps one; the window asks
// whether a mouse-down belongs to LDD's controls (true) or a native Cocoa control (false), and
// calls Activate when the native control takes the click. Implemented by NativeControlHitTester
// (ControlManager.cpp, Mac vtable 0x706bec). (method names inferred)
// ===========================================================================================
class HitTester : public virtual Object {
public:
    virtual bool HitTest(const Vector2& position) = 0;                       // slot 0
    // Returns the result of the focus change: -[LEGOMacDrawingView resignFirstResponder]
    // (Mac 0x216e40) answers it.
    virtual bool Activate() = 0;                                             // slot 1
    virtual ~HitTester() {}                                                  // slots 2/3
};
#endif

// ===========================================================================================
// ControlManager
// ===========================================================================================
class ControlManager : public WindowObserver,
                       public Notifier<ControlManagerObserver>,
                       public UpdateObserver,
                       public Updater {
public:
    // Observes `window` (referenced). Mac 0xd7ad0 (C2 with VTT), Win 0x506446
    explicit ControlManager(Window* window);
    virtual ~ControlManager();                                              // Mac 0xdbc00/0xdbe90, Win 0x506d6f (sdd 0x506613)

    // ---- HumanInput (Mac slot / Win slot): route to the controls, notify the observers ----
    virtual void OnMouseMove(const Vector2& position);                      // 0/0 Mac 0xd95a0, Win 0x5068a6
    virtual void OnMouseDown(const Vector2& position, UInt32 button);       // 1/1 Mac 0xd98d0, Win 0x5068ab
    virtual void OnMouseUp(const Vector2& position, UInt32 button);         // 2/2 Mac 0xd9ec0, Win 0x50690e
    virtual void OnMouseDoubleClick(const Vector2& position, UInt32 button); // 3/3 Mac 0xda180, Win 0x506971
    virtual void OnMouseWheel(UInt32 axis, Real32 delta);                   // 4/4 Mac 0xda1a0, Win 0x506989
    virtual void OnKeyDown(UInt32 key, UInt32 modifiers, bool repeat);      // 5/5 Mac 0xda480, Win 0x5069fd
    virtual void OnKeyUp(UInt32 key, UInt32 modifiers);                     // 6/6 Mac 0xda760, Win 0x506a6f
    virtual void OnCharacter(UniChar16 c, UInt32 modifiers, bool repeat);   // 7/7 Mac 0xdaa20, Win 0x506adb

    // ---- WindowInput overrides ----
    virtual void OnResize(const Vector2& size, const Vector2& clientSize);  // 11/9  Mac 0xe9450, Win 0x50628d
    virtual void OnActivate(bool active);                                   // 12/10 Mac 0xdad00, Win 0x506b52
    // WindowInput slot 13 = the window's keyboard focus changed (flag, modifiers).
    virtual void OnInput13(bool flag, UInt32 modifiers, UInt32 arg);        // 13/11 Mac 0xdad90, Win 0x506b93
    // False while a modal control is up.
    virtual bool OnCloseRequest();                                          // 15/13 Mac 0xe9470, Win 0x506281
    virtual bool OnInput17();                                               // 17/15 Mac 0xe9490, Win 0x50629d
    virtual void OnInput19();                                               // 19/17 Mac 0xdb0a0, Win 0x506c2b
    virtual void OnInput20(UInt32 arg);                                     // 20/18 Mac 0xdb220, Win 0x506c41
    virtual void OnInput21(UInt32 arg);                                     // 21/19 Mac 0xe94d0, Win 0x5062bd
    // WindowInput slots 22 / 23 = the window enters / leaves a menu tracking loop.
    virtual void OnInput22();                                               // 22/20 Mac 0xdb260, Win 0x506c62
    virtual void OnInput23();                                               // 23/21 Mac 0xdb520, Win 0x506ce1

    // ---- ControlManager (Mac slot / Win slot) ----
    // Object factory (+0xb0), the three layer roots, size from the window; Mac: the window's
    // native-control hit tester and keyboard-focus observer. (name inferred)
    virtual bool Initialize();                                              // 26/23 Mac 0xd8640, Win 0x50664b

protected:
    // Releases focus, mouse control, augmentations, cursor, observers, layers, factory, then
    // the base classes' references.
    virtual void OnFinalRelease();                                          // 25/–  Mac 0xdc180, Win 0x506e5e

public:
    // UpdateObserver::Update and Updater::Update: Updater pass, then the pending mouse-over
    // update (+0x95). Mac 0xdb7d0 (secondary entries 0xdb810, 0xdb850), Win 0x506d4c
    virtual void Update();                                                  // 27/–

    // The object factory (+0xb0), read inline by controls that create render objects
    // (StretchableImage Mac 0x233f90). (additive; name inferred)
    ObjectFactory* GetObjectFactory() const { return m_objectFactory.Get(); }

    // new Control(this, layer) (0x160 bytes) — the control enters slot `layer` itself. (name inferred)
    virtual void CreateLayer(UInt32 layer);                                 // 28/24 Mac 0xdc580, Win 0x506f03
    // Observer notifications (each default notifies the ControlManagerObservers; names inferred)
    virtual void NotifyResized();                                           // 29/25 Mac 0xdcb30, Win 0x506f3f
    virtual void NotifyMouseEnter(Control* control, const Vector2& position); // 30/26 Mac 0xdcda0, Win 0x506f89
    virtual void NotifyMouseLeave(Control* control, const Vector2& position); // 31/27 Mac 0xdd020, Win 0x506fdb
    virtual void NotifyMouseOverChanged(Control* previous, Control* current); // 32/28 Mac 0xdd2a0, Win 0x50702d
    virtual void NotifyKeyboardFocusChanged(bool flag, Control* previous, Control* current); // 33/29 Mac 0xdd520, Win 0x50707f
    // The mouse cursor object (+0x6c, not referenced). A Cursor that is also a Control joins
    // layer 2; false when it cannot. (names inferred)
    virtual bool SetCursor(Cursor* cursor);                                 // 34/30 Mac 0xdd7b0, Win 0x5070da
    virtual void RemoveCursor(Cursor* cursor);                              // 35/31 Mac 0xddee0, Win 0x507160
    // Manager augmentations by name (+0xb4); false when the name is taken. (names inferred)
    virtual bool AddAugmentation(ControlManagerAugmentation* augmentation); // 36/32 Mac 0xddf30, Win 0x50718a
    virtual void RemoveAugmentation(ControlManagerAugmentation* augmentation); // 37/33 Mac 0xde010, Win 0x5071fd

    // ---- non-virtual (names inferred) ----
    Window*  GetWindow() const { return m_window; }
    Control* GetLayer(UInt32 layer) const { return m_layers[layer]; }
    Control* GetKeyboardFocus() const { return m_keyboardFocus; }
    // (additive) Modifier state of the last key input (+0xa4; read inline by
    // LuaControlManager GetModifiers Mac 0x3690e0). (name inferred)
    UInt32   GetModifiers() const { return m_modifiers; }
    Control* GetMouseControl() const { return m_mouseControl; }
    Control* GetModalControl() const { return m_modalControl; }
    Cursor*  GetCursor() const { return m_cursor; }
    const Vector2& GetMousePosition() const { return m_mousePosition; }
    // The mouse position in `control`'s coordinates (Control::ManagerToLocal).
    Vector2  GetMousePosition(Control* control) const;                      // Mac 0xd89c0, Win 0x5066c9
    // The topmost control of layer 1 at `position`.
    Control* GetControlAt(const Vector2& position) const;                   // Mac 0xd7180, Win 0x5066c1
    // root->FindControlAt(position) (kept for existing callers).
    Control* FindControlAt(Control* root, const Vector2& position) const;
    // Moves the keyboard focus. `walkUp`: take the nearest parent that accepts the keyboard; `flag`
    // goes to OnKeyboardFocusGained / the observers. A change requested from a focus callback is
    // queued (+0x78) and applied after it. False when refused.
    bool     SetKeyboardFocus(Control* control, bool walkUp, bool flag);    // Mac 0xd8a00, Win 0x50788e
    bool     SetKeyboardFocus(Control* control);                            // Mac 0xd71d0, Win 0x5066e7 (…, false, false)
    // Starts (`modal`) or ends the modal state of `control` (+0xac): mouse capture and focus outside
    // it are dropped, the window disables its close box, OnManagerModalControl.
    bool     SetModalControl(bool modal, Control* control);                 // Mac 0xd8b60, Win 0x5066fb
    // Re-evaluates the control under the mouse (enter / leave / mouse-over, cursor shape).
    void     UpdateMouseOver();                                             // Mac 0xd9120, Win 0x507635
    // Attaches `cursor` (Cursor::Attach) or detaches the current one (null).
    bool     ChangeCursor(Cursor* cursor);                                  // Mac 0xd93d0, Win 0x506860
    void     SetCursorVisible(bool visible);                                // Mac 0xd9400, Win 0x506886
    // Cursor slot 6 (shape) when `flag` is set, else slot 7 (shape, 0).    Win 0x507864 (EAX = this; inlined on Mac)
    void     ApplyCursor(UInt32 shape, bool flag);
    // Draws layer 0 with m_updateLocked set (RenderControlManager).        Mac 0xd9440
    void     Draw(ControlDrawArgs& args);
    // Draws `control`'s subtree from a fresh state (offset 0, scale 1, clip = its size) when it
    // is connected, visible and buffered.                                   Mac 0xd9470
    static bool DrawControl(Control* control, ControlDrawArgs& args);
    ControlManagerAugmentation* GetAugmentation(const String& name) const;  // Mac 0xd9510, Win 0x51387d
    // Stores the mouse position (when it changed), notifies, OnMouseMove to the mouse control,
    // re-evaluates the mouse-over, moves the cursor.
    void     HandleMouseMove(const Vector2& position);                      // Mac 0xd95b0, Win 0x507585
    // Press / release / click / double-click routing to the mouse control; `doubleClick` reports
    // OnMouseDoubleClicked when the control was the last one clicked with `button`.
    void     HandleMouseButton(const Vector2& position, UInt32 button, bool down, bool doubleClick); // Mac 0xd9b90, Win 0x5072e9
    // Resizes the layer roots to `size` (window client size), redraws, NotifyResized.
    void     Resize(const Vector2& size);                                   // Mac 0xd8820, Win 0x507238
    // Gives `control` the mouse capture (it keeps all mouse input). False when the control is
    // not connected+visible, takes no mouse input or lies outside the modal control.
    bool     SetMouseCapture(Control* control);                             // Mac 0xde100 (Win inlined in 0x5079bc)
    // The capture ends without a new one (OnMouseCaptureLost).             Win 0x507b06 (inlined on Mac)
    bool     CancelMouseCapture();
    // True when there is no modal control or `control` is inside it (or null). Win 0x50799d (inlined on Mac)
    bool     IsInsideModal(Control* control) const;
    // Set while the manager draws (Draw); read inline by BrickConnectSceneControl.
    bool     IsUpdateLocked() const { return m_updateLocked; }

protected:
    friend class Control;

    // ---- data (Mac offsets; Win = Mac + 0x10) ----
    RefPointer<Window>         m_window;            // +0x5c
    RefPointer<Control>        m_layers[3];         // +0x60 (root control: layer 0; Win +0x70), +0x64, +0x68
    Cursor*                    m_cursor;            // +0x6c (not referenced)
    Control*                   m_mouseControl;      // +0x70 control under the mouse / capturing it
    Control*                   m_keyboardFocus;     // +0x74
    Control*                   m_pendingFocus;      // +0x78 focus requested during a focus change
    Control*                   m_savedFocus;        // +0x7c focus parked while the window is inactive / in a menu
    BasicArray<Control*>       m_clickedControls;   // +0x80 last control clicked with each button
    bool                       m_inFocusChange;     // +0x90 SetKeyboardFocus is notifying
    bool                       m_mouseCaptured;     // +0x91
    bool                       m_cursorVisible;     // +0x92 (handed to the cursor, slot 1)
    bool                       m_inMouseOverUpdate; // +0x93 UpdateMouseOver / capture callbacks run
    bool                       m_mouseOverRequested; // +0x94 UpdateMouseOver asked for meanwhile
    bool                       m_mouseOverDirty;    // +0x95 UpdateMouseOver pending (Update; Control sets it)
    bool                       m_inMenuLoop;        // +0x96 no mouse-over while a menu tracks
    bool                       m_active;            // +0x97 window active (true)
    Vector2                    m_mousePosition;     // +0x98
    UInt32                     m_mouseButtons;      // +0xa0 buttons down (bit per button)
    UInt32                     m_modifiers;         // +0xa4 keyboard modifiers of the last key input
    UInt32                     m_valueA8;           // +0xa8 (not set by the constructor; Control anchor marks)
    Control*                   m_modalControl;      // +0xac
    RefPointer<ObjectFactory>  m_objectFactory;     // +0xb0 (Initialize)
    BasicMap<String, RefPointer<ControlManagerAugmentation> > m_augmentations; // +0xb4
    bool                       m_updateLocked;      // +0xd0 (Win +0xe8)

private:
    ControlManager(const ControlManager&);
    ControlManager& operator=(const ControlManager&);
};

} // namespace LEGO

// (additive) Control::HasMouseCapture needs the complete Control; Control.h only
// forward-declares ControlManager, so this include makes no cycle.
#include "Control.h"

namespace LEGO {

// Control.h declares it; Win 0x50632a, Mac inlined.
inline bool Control::HasMouseCapture() const
{
    return m_manager->m_mouseControl == this && m_manager->m_mouseCaptured;
}

} // namespace LEGO
