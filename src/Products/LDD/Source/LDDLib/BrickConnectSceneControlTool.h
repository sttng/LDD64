// LEGO Digital Designer — restored source
// LEGO::BrickConnectSceneControlTool — base class of the tools of the scene control (selection,
//   clone, hinge, paint, hide …): observes the active BrickConnectSceneControl both as its
//   BrickConnectSceneControlObserver and as a ControlObserver (keyboard focus), and tells the
//   control its cursor / mode while it has the focus. BrickConnectSceneManager keeps the active
//   and the default tool (SetActiveTool / SetDefaultTool).
//
// TU 094 "BrickConnectSceneControlTool.cpp" (Mac 0x1a9040-0x1a996b; Win ≈ 0x6611ff-0x66148d).
// Directory: LDD range on Win → LDDLib.
//
// Mac layout: BrickConnectSceneControlObserver +0x00 (subject +0x04), ControlObserver +0x08
//   (subjects +0x0c..+0x1c), members below, Object +0x2c (Object-in vtable 0x70bee4 seen with
//   other offsets in subclass construction vtables). Win: Object +0x38 (vtable 0x9d74e4).
// Virtual slots: Mac primary 0x70bdcc (27 entries): D1 0x1a92f0, D0 0x1a9410, OnFinalRelease
//   0x1a9570, OnAttached 0x1a9730, OnDetached 0x1a97d0, observer handlers 5-22 (inherited),
//   new 23 / 24, and the Mac-only primary entries 25 / 26 of the ControlObserver overriders
//   OnKeyboardFocusGained 0x1a9810 / OnKeyboardFocusLost 0x1a9890.
//   Win primary 0x9d757c (22 entries: OnAttached 0x66140a, OnDetached 0x661459, handlers,
//   new 20 = 0x627d5b, 21 = 0x65a12a); ControlObserver-in 0x9d74fc (31 entries).
// Lua class "Tool" (LuaTool, TU 280). Method and member names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Control.h"
#include "BrickConnectSceneControlObserver.h"
#include "BrickConnectSceneControl.h"

namespace LEGO {

class BrickConnectSceneControl;

class BrickConnectSceneControlTool : public BrickConnectSceneControlObserver,
                                     public ControlObserver {
public:
    // Mac 0x1a90f0 (C1; C2 0x1a9040), Win 0x6612ae: inactive, cursor 1, `cursorFlag`.
    explicit BrickConnectSceneControlTool(bool cursorFlag = false);
    virtual ~BrickConnectSceneControlTool();                                // Mac 0x1a92f0/0x1a9410, Win sdd 0x66148d

    // ---- new (Mac slot / Win slot) ----
    // Returns this. (name inferred; Mac 0x127fb0 "return this", Win 0x627d5b mov eax, ecx)
    virtual BrickConnectSceneControlTool* GetTool();                        // 23/20
    // Empty by default; one stack word on Win (ret 4): the group tree passes the parts of a
    // clicked group (GroupTreeViewControl Mac 0x27d140 / Win 0x677a6f). (name unknown)
    virtual void OnToolEvent24(const BasicArray<RefPointer<Part> >& parts); // 24/21 Mac 0x127fc0, Win 0x65a12a

    // ---- ControlObserver overriders (Mac-only primary entries 25 / 26) ----
    // The scene control got the keyboard focus while the tool is active: tell it (Mac 0x42a80).
    virtual void OnKeyboardFocusGained(Control* control, bool flag, Control* previous,
                                       UInt32 modifiers);                   // 25 Mac 0x1a9810 (thunk 0x1a9850), Win 0x6611ff
    virtual void OnKeyboardFocusLost(Control* control, Control* next);      // 26 Mac 0x1a9890 (thunk 0x1a98c0), Win 0x661230

    // ---- non-virtual (names inferred) ----
    // The tool is (in)active: while active and the scene control has the keyboard focus, the
    // control counts it as pressed (BrickConnectSceneControl::AddPressedButton). Mac 0x1a9170
    void SetActive(bool active);
    bool IsActive() const { return m_active; }
    BrickConnectSceneControl* GetSceneControl() const;                      // Mac 0x1a91b0, Win 0x6611fb (via 0x67bb7b)
    // Stores the tool cursor and hands it to the scene control. Mac 0x1a91c0
    void SetCursor(UInt32 cursor);
    UInt32 GetCursor() const { return m_cursor; }
    // Hands `cursor` (not stored) to the scene control. Mac 0x1a91f0, Win 0x661294
    void ApplyCursor(UInt32 cursor);

protected:
    // Releases the scene control reference, leaves both subjects.
    virtual void OnFinalRelease();                                          // 2  Mac 0x1a9570 (thunks 0x1a9600, 0x1a9690)
    // BrickConnectSceneControlObserver: keep `control` (+0x28, referenced through its
    // sub-object at +0x1e0), observe it as a Control, hand it the tool cursor (+0x20, +0x24).
    virtual void OnAttached(BrickConnectSceneControl* control);             // 3  Mac 0x1a9730, Win 0x66140a
    virtual void OnDetached(BrickConnectSceneControl* control);             // 4  Mac 0x1a97d0, Win 0x661459
    using ControlObserver::OnAttached;   // (the Control overloads stay visible)
    using ControlObserver::OnDetached;

    // ---- data (Mac offsets) ----
    bool                      m_active;          // +0x1c (inferred)
    bool                      m_focused;         // +0x1d the scene control has the keyboard focus
    UInt32                    m_cursor;          // +0x20 handed to the control (Mac 0x42ac0); 1 initially
    bool                      m_cursorFlag;      // +0x24
    // Referenced (AddRef / Release through the RenderVisualDebugger sub-object +0x1e0).
    RefPointer<BrickConnectSceneControl> m_sceneControl; // +0x28
                                                 // Object +0x2c
};

} // namespace LEGO
