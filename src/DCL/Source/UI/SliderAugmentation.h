// LEGO Digital Designer — restored source
// LEGO::SliderAugmentation — makes its control a slider thumb inside its parent: per axis a range
// [minimum, maximum] and a value; position = (parent size - size) / (maximum - minimum) * value;
// dragging the control or clicking into the parent sets the value (SliderAugmentationObservers
// are told). ContainerControl's scroll bars use two (named "tab"); Lua binds it too.
// TU 114 (Mac 0x1c82a0-0x1cd6ab, file guess "SliderAugmentation.cpp", with ContainerControl; Win
// 0x510ca6-0x5123d7, 0x5c9667-0x5ca0e6). ContainerControl.cpp is a separate file by choice.
//
// Layout (Mac / Win): SpyOnThisAndParentAugmentation +0x00..+0x30 / +0x2c (m_parent Mac +0x2c),
//   Notifier<SliderAugmentationObserver> +0x30 / +0x2c (Mac: observers +0x34, iterations +0x44),
//   two Axis +0x54 / +0x50 (Mac 0x24, Win 0x28 bytes each), m_flag9c +0x9c / +0xa0,
//   m_axis +0x9d / +0xa1, m_minimumSize +0xa0 / +0xa4, m_maximumSize +0xa8 / +0xac,
//   m_grab +0xb0 / +0xb4, Object +0xb8. Mac sizeof 0xc0 (ctor 0x1c8c90), Win 0xd0 (0x510ca9).
// Virtual slots: Mac vtable 0x76e530 {GetControl, Attach 0x1cb660, Detach (Spy), D1, D0,
//   OnFinalRelease 0x1cb820, OnDestroyed overrider 0x1cb8b0, OnMouseMove overrider 0x1cb8d0,
//   OnMousePressed overrider 0x1cbf00, OnRectangleChanged overrider 0x1cc520}; Win 0xa28458
//   {GetControl, Attach 0x510eb1, Detach 0x510c13}, ControlObserver-in 0xa2846c.
// Names: class names from RTTI; members inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Geometry.h"
#include "Notifier.h"
#include "ObserverN.h"
#include "SpyOnThisAndParentAugmentation.h"

namespace LEGO {

class SliderAugmentation;

// Mac vtable 0x71ff8c {D1 0x1ca2c0, D0 0x1ca310, OnFinalRelease 0x1ca380, OnAttached 0x1ca3e0,
// OnDetached 0x1ca3f0, OnSliderValueChanged 0x1ca160 (empty)}; Win 0x9c9d54 {…, 0x510ca6}.
class SliderAugmentationObserver : public ObserverN<SliderAugmentation, SliderAugmentationObserver> {
public:
    SliderAugmentationObserver() {}
    virtual ~SliderAugmentationObserver() {}                          // Mac 0x1ca2c0/0x1ca310, Win sdd 0x5c987e
    // The value of `axis` (0 horizontal, 1 vertical) changed. (name inferred)
    virtual void OnSliderValueChanged(SliderAugmentation* slider, Real64 value, UInt32 axis); // Mac 5 0x1ca160, Win 2 0x510ca6
};

class SliderAugmentation : public SpyOnThisAndParentAugmentation,
                           public Notifier<SliderAugmentationObserver> {
public:
    // One axis (Mac 0x24 bytes, Win 0x28). (names inferred)
    struct Axis {
        Axis() : m_minimum(0.0), m_maximum(0.0), m_value(0.0), m_reserved(0.0), m_enabled(false) {}

        Real64 m_minimum;    // +0x00
        Real64 m_maximum;    // +0x08
        Real64 m_value;      // +0x10
        Real64 m_reserved;   // +0x18 (never read in the restored code)
        bool   m_enabled;    // +0x20 minimum < maximum (SetMaximum)
    };

    SliderAugmentation();                                             // Mac 0x1c8c90, Win 0x510ca9
    virtual ~SliderAugmentation();                                    // Mac 0x1cb730/0x1cb7e0, Win 0x510ef6 (sdd 0x511351)

    // Spy attach, then: mouse input and enabled on the control, position from the value.
    virtual bool Attach(Control* control, const String& name);        // Mac 0x1cb660, Win 0x510eb1

    // The value of `axis`; UpdatePosition; observers on a change. Mac 0x1cad10, Win 0x456310
    void SetValue(Real64 value, UInt32 axis);
    // SetValue on m_axis. Win 0x4562f0 (EAX = this; Mac inlined: ContainerControl)
    void SetValue(Real64 value);
    // The maximum of `axis` (at least the minimum; the axis is enabled when above it); the value is
    // clamped; UpdatePosition; observers on a change. Mac 0x1ccdf0, Win 0x510dd0
    void SetMaximum(Real64 maximum, UInt32 axis);
    // SetMaximum on m_axis. Win 0x510db5 (Mac inlined)
    void SetMaximum(Real64 maximum);
    const Axis& GetAxis(UInt32 axis) const { return m_axes[axis]; }
    Real64 GetValue(UInt32 axis) const { return m_axes[axis].m_value; }
    // The axis ContainerControl drives (0 horizontal, 1 vertical). (name inferred)
    UInt32 GetDefaultAxis() const { return m_axis; }
    void SetDefaultAxis(UInt32 axis) { m_axis = (UInt8)axis; }
    // Clamps the values, then moves (truncated) and sizes (clamped to the size limits) the
    // control inside its parent. Mac 0x1caff0, Win 0x456400
    void UpdatePosition();

protected:
    // Observers and observed controls leave.
    virtual void OnFinalRelease();                                    // Mac 0x1cb820, Win 0x510f64
    // Spy: the parent changed — observe the new one, then UpdatePosition.
    virtual void OnDestroyed(Control* control);                       // Mac 0x1cb8b0 (thunk 0x1cc5c0), Win 0x510f8f
    // Dragging the control (left button held): the values from the new position.
    virtual void OnMouseMove(Control* control, const Vector2& position); // Mac 0x1cb8d0 (thunk 0x1cc620), Win 0x510fa7
    // On the control: remembers the grab point. On the parent: the values from the click.
    virtual void OnMousePressed(Control* control, const Vector2& position, UInt32 button); // Mac 0x1cbf00 (thunk 0x1cc640), Win 0x511176
    // The parent resized: UpdatePosition.
    virtual void OnRectangleChanged(Control* control, const Vector2& oldPosition,
                                    const Vector2& oldSize);          // Mac 0x1cc520 (thunk 0x1cc5f0), Win 0x51133b

    // Values from a thumb position `position` (parent coordinates), then UpdatePosition and the
    // observers for every enabled axis that changed (inlined twice).
    void SetValuesFromPosition(const Vector2& position);
    // Observers (this, value of `axis`, axis).
    void NotifyValueChanged(UInt32 axis);

    // ---- data (Mac offset / Win offset) ----
    Axis    m_axes[2];       // +0x54 / +0x50
    bool    m_flag9c;        // +0x9c / +0xa0 true (meaning unknown)
    UInt8   m_axis;          // +0x9d / +0xa1 1
    Vector2 m_minimumSize;   // +0xa0 / +0xa4 (0, 0)
    Vector2 m_maximumSize;   // +0xa8 / +0xac (100000, 100000)
    Vector2 m_grab;          // +0xb0 / +0xb4 mouse position on the control when pressed
                             // Object +0xb8
};

} // namespace LEGO
