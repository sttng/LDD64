// LEGO Digital Designer — restored source
// LEGO::LDDPaneContainerControl — the edit mode's side pane: wraps the Lua palette panel (the
//   content, which carries the "SlidingHideAugmentation") with a 5-pixel grip strip along its
//   edge (drag it to resize the content) and the LDDPaneToggleControl handle that slides the
//   pane in and out (AnimateShowHideControlCommand). The container fits itself to its children.
//
// TU 287 "LDDPaneContainerControl.cpp": Mac 0x29aa60-0x29bb3b, Win 0x66d23b-0x66d874. The TU
// also emits the Control::GetAugmentation<SlidingHideAugmentation> instance (Mac 0x29b120, Win
// 0x66d4b5). Mac 0x29bac0 = <iostream> + a header static (not code).
// RTTI: vmi {Control +0, ControlObserver (Mac +0x158, Win +0x16c)}, virtual base Object.
//   Mac vtable 0x7412dc (41 entries: Control's 36, then the Itanium primary entries of the
//   ControlObserver overriders 36-40), ControlObserver-in 0x74138c {OnShown 0x29b9d0, OnHidden
//   0x29ba70, OnMouseMove 0x29b8f0, OnMousePressed 0x29b7d0, OnMouseReleased 0x29b840}; Win
//   0x9d826c (Object-in), ControlObserver-in 0x9d8284 {OnShown 0x66d485, OnHidden 0x66d831,
//   OnMouseMove 0x66d280, OnMousePressed 0x66d23b, OnMouseReleased 0x66d270}.
// Layout (Mac / Win): Control +0x000..+0x158 / +0x16c, ControlObserver +0x158 / +0x16c,
//   grip +0x16c / +0x184, handle +0x170 / +0x188, content +0x174 / +0x18c, observe content
//   flag +0x178 / +0x190, dragging +0x179 / +0x191, drag offset +0x17c / +0x194, Object +0x184
//   (Mac sizeof 0x18c, Win 0x1a8).
// Method and member names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "Geometry.h"
#include "Control.h"
#include "RenderControl.h"
#include "LDDPaneToggleControl.h"

namespace LEGO {

class AnimateShowHideControlCommand;

class LDDPaneContainerControl : public Control, public ControlObserver {
public:
    // EditMode passes the Lua palette panel and true (observe it: the grip only takes the
    // resize cursor / mouse while the content is visible). Mac 0x29aa60, Win 0x66d2e3
    LDDPaneContainerControl(ControlManager* manager, Control* content, bool observeContent);
    virtual ~LDDPaneContainerControl();                                     // Mac 0x29b370/0x29b4b0, Win sdd 0x66d463

    // Setup below `parent` (flags 7, empty rectangle), then the grip strip (RenderControl, 5 x 9,
    // image strip of LDDgraphics01.png; mouse input, hit-testable) on the content's far edge
    // (`rightSide`: the near one) stretched over the height, and the LDDPaneToggleControl on
    // the same edge, 8 pixels below the top, sliding the content through its
    // "SlidingHideAugmentation" with `command`; finally the container fits its children and,
    // when asked to, starts observing the content. False without the texture, without the
    // augmentation, or when a Setup fails. `rightSide` is a flag in both builds (Mac byte,
    // `2 * flag + 1` alignment; Win `(char)param != 0`).
    bool Create(Control* parent, bool rightSide, AnimateShowHideControlCommand* command); // Mac 0x29abe0, Win 0x66d525

protected:
    // The observer part (leaves every watched control), then Control's. Mac 0x29b620
    virtual void OnFinalRelease();

    // ---- ControlObserver (Mac observer slot, primary slot) ----
    using Control::OnShown;
    using Control::OnHidden;
    using Control::OnMouseMove;
    using Control::OnMousePressed;
    using Control::OnMouseReleased;
    // The content became visible: watch the grip, resize cursor (13) on it.
    virtual void OnShown(Control* control);                                 // 11, 39 Mac 0x29b980, Win 0x66d485
    // The content was hidden: stop watching the grip, arrow cursor.
    virtual void OnHidden(Control* control);                                // 12, 40 Mac 0x29ba20, Win 0x66d831
    // While dragging the grip: the content's size follows the mouse.
    virtual void OnMouseMove(Control* control, const Vector2& position);    // 19, 38 Mac 0x29b860, Win 0x66d280
    // A press on the grip starts a drag (the offset of the press is kept).
    virtual void OnMousePressed(Control* control, const Vector2& position, UInt32 button); // 20, 36 Mac 0x29b780, Win 0x66d23b
    virtual void OnMouseReleased(Control* control, const Vector2& position, UInt32 button); // 21, 37 Mac 0x29b820, Win 0x66d270

    RefPointer<RenderControl>        m_grip;            // Mac +0x16c, Win +0x184
    RefPointer<LDDPaneToggleControl> m_handle;          // Mac +0x170, Win +0x188
    Control*                         m_content;         // Mac +0x174, Win +0x18c
    bool                             m_observeContent;  // Mac +0x178, Win +0x190
    bool                             m_dragging;        // Mac +0x179, Win +0x191
    Vector2                          m_dragOffset;      // Mac +0x17c, Win +0x194
};

} // namespace LEGO
