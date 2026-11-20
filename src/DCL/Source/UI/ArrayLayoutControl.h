// LEGO Digital Designer — restored source
// LEGO::ArrayLayoutControl — lays out its controls in one row or column (optionally reversed)
// with spacing and end margins, optionally resizing itself ("off" / "single" = along the row /
// "both" = also across); re-lays out when a watched control changes rectangle, scale or
// visibility. Base of TextOutputControl; Lua: LuaArrayLayoutControl (TU 344).
// TU 343 "ArrayLayoutControl.cpp" (Mac 0x326990-0x32831b; Win 0x516e06-0x517601) + the
// BasicArray<RefPointer<Control> >::Insert instance next to it (TU 177, Mac 0x233ac0). Names
// inferred, except FromString's keywords.
//
// Layout (Mac): Control +0x000..+0x158, ControlObserver +0x158 (vptr, subjects +0x15c),
//   members below, Object +0x190 (sizeof 0x198). Win: ControlObserver +0x16c, horizontal
//   +0x194, reverse +0x195, inLayout +0x196, resize mode +0x198, spacing +0x19c, margins
//   +0x1a0 / +0x1a4, controls +0x184 (begin +0x188), Object +0x1ac.
// Virtual slots: Mac primary vtable 0x74b6ac (40 entries: Control 0..35, the four
//   ControlObserver overriders get primary slots 36..39, Itanium rule); Win 0xa28a24 = the
//   Control slots only (33), ControlObserver-in vtable 0xa28aac.
#pragma once
#include "LEGO/Types.h"
#include "Control.h"
#include "BasicArray.h"

namespace LEGO {

class String;

class ArrayLayoutControl : public Control, private ControlObserver {
public:
    // How the control fits itself to its controls. (names inferred from the keywords)
    enum ResizeMode {
        ResizeOff    = 0,   // "off": the size is left alone
        ResizeSingle = 1,   // "single": the length along the row follows the controls
        ResizeBoth   = 2    // "both": also the breadth (largest control)
    };

    // Mac 0x326aa0 (C1) / 0x326990 (C2, VTT), Win 0x516d1e (C2, called by TextOutputControl)
    explicit ArrayLayoutControl(ControlManager* manager);
    virtual ~ArrayLayoutControl();                     // Mac 0x3278a0/0x3279e0, Win 0x517129/0x516e06

    // true: one row (main axis x); false: one column. Default true. Mac 0x326bc0 / 0x326bd0, Win – / 0x516e36
    bool IsHorizontal() const { return m_horizontal; }
    void SetHorizontal(bool horizontal);
    ResizeMode GetResizeMode() const { return (ResizeMode)m_resizeMode; }     // Mac 0x3270c0
    void SetResizeMode(ResizeMode mode);                                       // Mac 0x3270d0, Win 0x516e52
    // Reverse: the first control sits at the far end. Mac 0x3270f0 / 0x327100
    bool IsReversed() const { return m_reversed; }
    void SetReversed(bool reversed);
    // Values are rounded to whole units. Mac 0x327130 / 0x327140, Win – / 0x516e6e
    Real32 GetSpacing() const { return m_spacing; }
    void SetSpacing(Real32 spacing);
    Real32 GetLeadingMargin() const { return m_leadingMargin; }
    void SetLeadingMargin(Real32 margin);                                      // Mac 0x3271e0, Win 0x516eb4
    Real32 GetTrailingMargin() const { return m_trailingMargin; }
    // Lays out again only when the control resizes itself. Mac 0x327280, Win 0x516ef9
    void SetTrailingMargin(Real32 margin);

    // The control becomes a child (last) and the last array element. Mac 0x327320, Win 0x516f47
    void AddControl(Control* control);
    // The same at `index` (clamped to the count). Mac 0x3273e0, Win 0x516fd2
    void InsertControl(Control* control, UInt32 index);
    // Removes it from the array (and from the children when this is its parent).
    // Mac 0x3274d0, Win 0x517052
    void RemoveControl(Control* control);
    void RemoveAllControls();                                                  // Mac 0x327670, Win 0x5170ce
    UInt32 GetControlCount() const { return m_controls.GetSize(); }           // Mac 0x327710
    typedef BasicArray<RefPointer<Control> >::ConstIterator ControlIterator;
    ControlIterator BeginControls() const { return m_controls.Begin(); }      // Mac 0x327730
    ControlIterator EndControls() const { return m_controls.End(); }          // Mac 0x327740

    // Positions the visible controls (status bit 2) and resizes the control (resize mode).
    // Mac 0x326c00, Win 0x5171ba
    void Layout();

protected:
    // ObserverN part first (leaves every watched control), then Control's.
    virtual void OnFinalRelease();                                             // Mac 0x327b50, Win 0x5171a6

    // ---- ControlObserver handlers (Mac primary slots 36..39, thunks 0x327db0 / 0x327ee0 /
    // 0x328010 / 0x328130; Win ControlObserver-in slots 6..9): re-lay out unless a layout is running.
    virtual void OnRectangleChanged(Control* control, const Vector2& oldPosition,
                                    const Vector2& oldSize);                   // 36 Mac 0x327cb0, Win 0x5173c1
    virtual void OnScaleChanged(Control* control, const Vector2& oldScale);   // 37 Mac 0x327de0, Win 0x517439
    virtual void OnShown(Control* control);                                    // 38 Mac 0x327f10, Win 0x5174b1
    virtual void OnHidden(Control* control);                                   // 39 Mac 0x328030, Win 0x5174b1 (ICF)

    // The Control notifications of the same names stay the ones Control declares.
    using Control::OnRectangleChanged;
    using Control::OnScaleChanged;
    using Control::OnShown;
    using Control::OnHidden;

private:
    // `control` is in the array. (inlined std::find in each handler)
    bool Contains(Control* control) const;

    // ---- data (Mac / Win offsets) ----
    BasicArray<RefPointer<Control> > m_controls;   // +0x16c / +0x184
    bool   m_horizontal;                           // +0x17c / +0x194 true
    bool   m_reversed;                             // +0x17d / +0x195 false
    bool   m_inLayout;                             // +0x17e / +0x196 Layout running
    UInt32 m_resizeMode;                           // +0x180 / +0x198 ResizeSingle
    Real32 m_spacing;                              // +0x184 / +0x19c 0
    Real32 m_leadingMargin;                        // +0x188 / +0x1a0 0 (before the first control)
    Real32 m_trailingMargin;                       // +0x18c / +0x1a4 0 (after the last; size only)
                                                   // Object +0x190 / +0x1ac
};

// Lua / XML keyword ("off", "single", "both", case-insensitive, at `position`); false (and
// ResizeSingle) otherwise. Mac 0x328150, Win 0x517529 (name inferred)
bool FromString(ArrayLayoutControl::ResizeMode& value, const String& text, UInt32& position);

} // namespace LEGO
