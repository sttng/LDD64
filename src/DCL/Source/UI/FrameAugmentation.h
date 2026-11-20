// LEGO Digital Designer — restored source
// LEGO::FrameAugmentation — makes its control's parent a frame around it: the control sits at
// (left, top) and the parent is sized to its scaled size + (left + right, top + bottom); re-applied
// when the control changes parent, rectangle or scale. Lua: LuaFrameAugmentation (TU 409).
// TU 408 "FrameAugmentation.cpp": Mac 0x38d720-0x38dcfb, Win 0x45b3c0 (+ 0x515b8d-0x515bf6).
//
// RTTI (Mac): vmi, base ControlAugmentationControlObserver, virtual base Object.
//   Primary vtable 0x756adc {GetControl 0x113da0, Attach 0x38d720, Detach 0x163740, D1 0x38da00,
//   D0 0x38da60, OnFinalRelease 0x163790, OnDestroyed 0x38d8e0, OnRectangleChanged 0x38d940,
//   OnScaleChanged 0x38d9a0}; ControlObserver-in 0x756b0c (slots 7/9/10 → thunks 0x38d910,
//   0x38d970, 0x38d9d0); Object-in 0x756bac {0x38dba0, 0x38dc00, 0x163850, AddRef, Release}.
//   Layout Mac: CACO +0x00..+0x2c, Rect frame +0x2c, Object +0x3c (sizeof 0x44).
//   Win: vtable 0xa21a7c {GetControl 0x52f4b0, Attach 0x515b8d, Detach 0x45a430}, ControlObserver-in
//   0xa21a8c (slots 4/6/7: 0x515bac, 0x515bc5, 0x515bde), Object-in 0xa21b0c {sdd 0x45bb40,
//   OnFinalRelease 0x45bb50, AddRef, Release}; Object at +0x3c. Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Geometry.h"
#include "ControlAugmentationControlObserver.h"

namespace LEGO {

class FrameAugmentation : public ControlAugmentationControlObserver {
public:
    // Frame (0, 0, 0, 0). Inline (C2 emitted in TU 409, Mac 0x38dd00).
    FrameAugmentation() : m_frame(0.0f, 0.0f, 0.0f, 0.0f) {}
    virtual ~FrameAugmentation();                                           // Mac 0x38da00/0x38da60, Win sdd 0x45b3c0

    // ControlAugmentationControlObserver::Attach (inlined on Mac), then Layout.
    virtual bool Attach(Control* control, const String& name);              // Mac 0x38d720, Win 0x515b8d

    // ---- ControlObserver: each re-lays out when `control` is the augmented control ----
    // (Control slot 6: sent after the parent changed, see Control.h)
    virtual void OnDestroyed(Control* control);                             // Mac 0x38d8e0 (thunk 0x38d910), Win 0x515bac
    virtual void OnRectangleChanged(Control* control, const Vector2& oldPosition,
                                    const Vector2& oldSize);                // Mac 0x38d940 (thunk 0x38d970), Win 0x515bc5
    virtual void OnScaleChanged(Control* control, const Vector2& oldScale); // Mac 0x38d9a0 (thunk 0x38d9d0), Win 0x515bde

    // Stores the frame and lays out when attached. Mac 0x38d850
    void SetFrame(Real32 left, Real32 top, Real32 right, Real32 bottom);
    Real32 GetLeft() const;                                                 // Mac 0x38d8a0
    Real32 GetTop() const;                                                  // Mac 0x38d8b0
    Real32 GetRight() const;                                                // Mac 0x38d8c0
    Real32 GetBottom() const;                                               // Mac 0x38d8d0

    // Control position = (left, top); parent size = size * scale + (left, top) + (right, bottom).
    // Mac 0x38d7a0 (Win inlined)
    void Layout();

private:
    Rect m_frame;   // Mac +0x2c: m_min = (left, top), m_max = (right, bottom) (margins, not corners)
};

} // namespace LEGO
