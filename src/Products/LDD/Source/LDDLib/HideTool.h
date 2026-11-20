// LEGO Digital Designer — restored source
// LEGO::HideTool — "HideTool" (Lua LDD.tools): hides the selection (when the tool becomes active
//   or a band ends), the clicked part or the parts of a group tree click, with undo.
// LEGO::HideToolControl — the "unhide all" button the hide tool puts into the scene control's top
//   left corner (shown while bricks are hidden): a click shows every hidden brick, with undo.
// LEGO::UndoHideTool — the undo action of both (hides or shows its bricks again).
//
// TU 292 "HideTool.cpp" (Mac 0x29fdd0-0x2a17db; Win 0x67cbb9-0x67d54c).
// RTTI: HideTool : SelectionTool (Mac vtable 0x742a50, 39 entries; sizeof 0x12c: m_control
//   +0x120, Object +0x124; Win 0x9daf84). HideToolControl : ButtonControl,
//   BrickConnectSceneControlObserver (+0x218) (Mac vtable 0x74281c, 49 entries; sizeof 0x22c;
//   Win 0x9db094). UndoHideTool : UndoAction (Mac vtable 0x77535c {Execute 0x2a1540, D1, D0},
//   sizeof 0x24; Win 0x9daee8 {Execute 0x67cc8d}).
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "UndoManager.h"
#include "ButtonControl.h"
#include "BrickConnectSceneControlObserver.h"
#include "SelectionTool.h"

namespace LEGO {

class Brick;

// Mac layout: UndoAction vptr +0, m_bricks +0x04, m_control +0x14, m_hide +0x18, Object +0x1c.
class UndoHideTool : public UndoAction {
public:
    // Mac 0x2a02a0, Win 0x67cbb9
    UndoHideTool(BrickConnectSceneControl* control, const BasicArray<RefPointer<Brick> >& bricks,
                 bool hide);
    // Pushes the inverse action, then hides (m_hide) or shows the bricks. Mac 0x2a1540,
    // Win 0x67cc8d
    virtual void Execute();
    virtual ~UndoHideTool() {}                                              // Mac 0x2a1610/0x2a16c0, Win sdd 0x67d046

    BasicArray<RefPointer<Brick> > m_bricks;    // +0x04
    BrickConnectSceneControl*      m_control;   // +0x14 (not referenced)
    bool                           m_hide;      // +0x18
};

// Mac layout: ButtonControl +0x000..+0x217, BrickConnectSceneControlObserver +0x218 (subject
// +0x21c), m_control +0x220, Object +0x224.
class HideToolControl : public ButtonControl, public BrickConnectSceneControlObserver {
public:
    // A button of `control`'s manager that observes `control`. Mac 0x29fdd0, Win 0x67d0f6
    explicit HideToolControl(BrickConnectSceneControl* control);
    virtual ~HideToolControl() {}                                           // Mac 0x2a0e10/0x2a0e90, Win 0x67d1d1/0x67d263

    // Every hidden brick is shown again (UndoHideTool(…, true)); Control's. Mac 0x2a0110,
    // Win 0x67cd4c
    virtual void OnMouseClicked(const Vector2& position, UInt32 button);    // 21/18

protected:
    // Leaves the scene control, then the control's. Mac 0x29fed0 (thunks 0x29ff40 …)
    virtual void OnFinalRelease();
    // Bricks were hidden / shown: the button is shown while any brick is hidden. Mac 0x2a0380
    // (Mac primary entry 48, thunk 0x2a0490), Win 0x67cdf4
    virtual void OnEvent22(UInt32 hidden);

    BrickConnectSceneControl* m_control;   // +0x220
};

class HideTool : public SelectionTool {
public:
    // SelectionTool(clickStickTool, true, true), no control; cursor 0x73. Mac 0x2a04c0,
    // Win 0x67ce53
    explicit HideTool(ClickStickTool* clickStickTool);
    virtual ~HideTool();                                                    // Mac 0x2a1370/0x2a1430, Win sdd 0x67d024

    // The bricks of `parts` are hidden with an UndoHideTool(…, false) (nothing for no parts).
    // Mac 0x2a05b0, Win 0x67cf8a
    void Hide(const BasicArray<RefPointer<Part> >& parts);

    virtual void OnToolEvent24(const BasicArray<RefPointer<Part> >& parts); // 24/21 Mac 0x2a05a0, Win 0x67d065
    // Empty. Mac 0x2a1470, Win 0x5c8021 (ICF)
    virtual void OnPickDragged();                                           // 28/23
    // Hides the clicked part. Mac 0x2a0750, Win 0x67d083
    virtual void OnPickClicked(bool flag);                                  // 29/24
    // Hides the selection. Mac 0x2a0720, Win 0x67d06a
    virtual void OnPickReleased();                                          // 33/28

    HideToolControl* GetControl() const { return m_control; }

protected:
    // SelectionTool's; the first time the button (Graphics/EditScreen/UI02.png states at (2, 2),
    // (2, 51), (2, 99), 72 x 48, top-left aligned at (20, -10), tooltip "HIDE TOOL UNHIDE
    // TOOLTIP"); then the selection is hidden. Mac 0x2a08b0, Win 0x67d299
    virtual void OnAttached(BrickConnectSceneControl* control);
    using SelectionTool::OnAttached;

    RefPointer<HideToolControl> m_control;   // +0x120
};

} // namespace LEGO
