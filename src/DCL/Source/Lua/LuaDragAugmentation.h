// LEGO Digital Designer — restored source
// LEGO::LuaDragAugmentation — Lua class "DragAugmentation" (base "ControlAugmentation"), attached
// as "drag": the user moves the control with the mouse (title strip or selection mask) and
// resizes it at the frame edges. Layout: selectionMask… (movable area), `frame="l t r b"` (resize
// borders), `title` (move strip height), allowSizeChange, allowPositionChange, constrainToParent
// (booleans, default true).
// Hit test (mode): outside 0; with a mask: kMove if movable and inside the mask; frame and title
// all 0: kMove if movable; else the ControlDragHelper edge bits of the borders hit (resizable),
// or kMove in the title strip. Cursors: kMove 5, left 12, top 10, right 13, bottom 11,
// top-left / bottom-right 15, top-right / bottom-left 14, none 1.
// TU 309 "LuaDragAugmentation.cpp": Mac 0x2b6e70-0x2b842b, Win 0x45cf40-0x45db8f.
// RTTI vmi {LuaControlAugmentation +0, ControlAugmentationControlObserver +0xc}, virtual base
// Object. Mac primary vtable 0x745d7c {D1 0x2b75d0, D0 0x2b7750, BindLuaObject 0x291a70,
// UnbindLuaObject 0x2b7b10, OnLayoutApplied 0x2b7b30, Attach 0x2b7270, OnFinalRelease
// 0x2b7930, OnMousePressed 0x2b7fe0, OnMouseMove 0x2b81f0, OnMouseReleased 0x2b8330};
// ctor Mac 0x2b6e70 (C1), Win 0x45cf40; Win Attach 0x45d250, OnLayoutApplied 0x45d4d0,
// OnMousePressed 0x45d7c0, OnMouseMove 0x45d820, OnMouseReleased 0x45d8b0, HitTest 0x45d8f0,
// UpdateCursor 0x45da70, sdd 0x45d410.
// Layout (Mac): LuaLayout +0x00..+0x0c, ControlAugmentationControlObserver +0x0c..+0x38,
// RefPointer<SelectionMask> +0x38, ControlDragHelper +0x3c (resizable +0x5c, movable +0x5d,
// limited +0x5e), mode +0x60, constrainToParent +0x64, frame left/top/right/bottom
// +0x68..+0x74, title +0x78, Object +0x7c (sizeof 0x84). Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Geometry.h"
#include "LuaControlAugmentation.h"
#include "ControlAugmentationControlObserver.h"
#include "ControlDragHelper.h"

namespace LEGO {

class SelectionMask;

class LuaDragAugmentation : public LuaControlAugmentation,
                            public ControlAugmentationControlObserver {
public:
    LuaDragAugmentation();                                                  // Mac 0x2b6e70, Win 0x45cf40
    virtual ~LuaDragAugmentation();                                         // Mac 0x2b75d0/0x2b7750 (body 0x2b7520), Win sdd 0x45d410

    // Detach(). Mac 0x2b7b10
    virtual void UnbindLuaObject();
    // LuaLayout's, then the attributes (see the header comment); a change while the mouse is
    // over the control updates the cursor. Mac 0x2b7b30, Win 0x45d4d0
    virtual void OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args);
    // ControlAugmentationControlObserver::Attach, then: mouse input on, hit-testable when
    // neither this nor the control has a selection mask, the cursor when the mouse is inside.
    virtual bool Attach(Control* control, const String& name);              // Mac 0x2b7270 (thunk 0x2b7500), Win 0x45d250

    // The drag mode at `position` (control coordinates). Mac 0x2b8040, Win 0x45d8f0
    UInt32 HitTest(const Vector2& position) const;
    // Cursor of the current mode (or of HitTest(position) when not dragging). Mac 0x2b73a0, Win 0x45da70
    void UpdateCursor(const Vector2& position);

    UInt32 GetMode() const { return m_mode; }
    const ControlDragHelper& GetDragHelper() const { return m_drag; }
    bool GetConstrainToParent() const { return m_constrainToParent; }
    Real32 GetFrameLeft() const { return m_frameLeft; }
    Real32 GetFrameTop() const { return m_frameTop; }
    Real32 GetFrameRight() const { return m_frameRight; }
    Real32 GetFrameBottom() const { return m_frameBottom; }
    Real32 GetTitleHeight() const { return m_title; }
    SelectionMask* GetMask() const { return m_mask; }

protected:
    // ControlAugmentationControlObserver's, then the Object one. Mac 0x2b7930, Win 0x45d4c0
    virtual void OnFinalRelease();
    // Button 0 pressed: start dragging in HitTest's mode at `position` (the grab point).
    virtual void OnMousePressed(Control* control, const Vector2& position, UInt32 button); // Mac 0x2b7fe0 (thunk 0x2b8190), Win 0x45d7c0
    // While dragging: ControlDragHelper::Drag (limited to the parent's size with
    // constrainToParent); always the cursor.
    virtual void OnMouseMove(Control* control, const Vector2& position);    // Mac 0x2b81f0 (thunk 0x2b8290), Win 0x45d820
    // Button 0 released: stop dragging.
    virtual void OnMouseReleased(Control* control, const Vector2& position, UInt32 button); // Mac 0x2b8330 (thunk 0x2b8370), Win 0x45d8b0

private:
    RefPointer<SelectionMask> m_mask;       // Mac +0x38
    ControlDragHelper         m_drag;       // Mac +0x3c
    UInt32                    m_mode;       // Mac +0x60 (0 = not dragging)
    bool                      m_constrainToParent; // Mac +0x64 (true)
    Real32                    m_frameLeft;  // Mac +0x68
    Real32                    m_frameTop;   // Mac +0x6c
    Real32                    m_frameRight; // Mac +0x70
    Real32                    m_frameBottom; // Mac +0x74
    Real32                    m_title;      // Mac +0x78
};

// Registers DragAugmentation (base ControlAugmentation) {Create}. Mac 0x2b7010, Win 0x45d1b0.
// (name inferred)
void RegisterLuaDragAugmentation(LuaPlus::LuaState* state);

} // namespace LEGO
