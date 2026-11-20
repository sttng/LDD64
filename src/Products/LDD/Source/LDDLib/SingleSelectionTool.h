// LEGO Digital Designer — restored source
// LEGO::SingleSelectionTool — the default tool of the edit mode: a click selects one part (the
//   filters' parts); with Shift held (modifier bit 0) it behaves like the multiple selection tool
//   (toggle) and shows its cursor (0x66 instead of 1).
//
// TU 110 "SingleSelectionTool.cpp" (Mac 0x1b9c70-0x1ba5eb; Win 0x684360-0x68468d).
// RTTI: SingleSelectionTool : SelectionTool. Mac vtable 0x71e96c (41 entries: SelectionTool's
//   with the keyboard overriders as Mac-only primary entries 39 / 40), sizeof 0x128 (m_shift
//   +0x11e); Win 0x9dc05c / ControlObserver-in 0x9dbfdc, sizeof 0x140. Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "SelectionTool.h"

namespace LEGO {

class SingleSelectionTool : public SelectionTool {
public:
    // SelectionTool(clickStickTool, true, true); cursor 1. Mac 0x1b9c70, Win 0x68457f
    explicit SingleSelectionTool(ClickStickTool* clickStickTool);
    virtual ~SingleSelectionTool();                                         // Mac 0x1ba2c0/0x1ba320, Win 0x68461a/0x68468d

    // ---- SelectionTool overrides (Shift: SelectionTool's) ----
    // The parts become the selection. Mac 0x1b9d30, Win 0x684360
    virtual void OnToolEvent24(const BasicArray<RefPointer<Part> >& parts); // 24/21
    // Mac 0x1b9d70, Win 0x684381
    virtual void OnPickPressed(bool clear);                                 // 27/22
    // The selection is cleared, then the clicked part (toggled when it was selected, else the
    // filters' parts) becomes the selection. Mac 0x1b9d90, Win 0x684458
    virtual void OnPickClicked(bool flag);                                  // 29/24
    // Empty (no clear on release). Mac 0x1ba0b0, Win 0x5c8021 (ICF)
    virtual void OnPickReleased();                                          // 33/28

    // ---- BrickConnectSceneControlTool / ControlObserver ----
    // Shift state from the modifiers, cursor 0x66 when held. Mac 0x1ba180 (thunk 0x1ba1b0),
    // Win 0x6843c8
    virtual void OnKeyboardFocusGained(Control* control, bool flag, Control* previous,
                                       UInt32 modifiers);
    // Shift off, cursor 1. Mac 0x1ba200 (thunk 0x1ba230), Win 0x6843fd
    virtual void OnKeyboardFocusLost(Control* control, Control* next);
    // Mac 0x1ba0c0 (entry 39, thunk 0x1ba0f0), Win 0x684392
    virtual void OnKeyPressed(Control* control, UInt32 key, UInt32 modifiers, bool repeat);
    // Mac 0x1ba120 (entry 40, thunk 0x1ba150), Win 0x6843ad
    virtual void OnKeyReleased(Control* control, UInt32 key, UInt32 modifiers);

protected:
    // SelectionTool's; Shift from the manager's modifiers (cursor 0x66 handed to the control).
    // Mac 0x1ba270, Win 0x684424
    virtual void OnAttached(BrickConnectSceneControl* control);
    using SelectionTool::OnAttached;

    bool m_shift;   // +0x11e
};

} // namespace LEGO
