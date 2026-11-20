// LEGO Digital Designer — restored source
// LEGO::SliderAugmentation, LEGO::SliderAugmentationObserver — part of TU 114 (Mac
// 0x1c8c90, 0x1cad10-0x1cc520 + 0x1ccdf0, 0x1ca160; Win 0x510ca6-0x511351); the rest of the TU
// is ContainerControl.cpp.
// Not restored here: the NotifierT<SliderAugmentation, …> / ObserverN<SliderAugmentation, …> /
// BasicArray<> template members (Mac 0x1ca2c0-0x1ca660, 0x1cb1a0-0x1cb5c0, 0x1cc760-0x1ccbf0)
// and the this-adjusting thunks.
#include "SliderAugmentation.h"
#include "Control.h"
#include <math.h>

namespace LEGO {

// Mac 0x1ca160, Win 0x510ca6
void SliderAugmentationObserver::OnSliderValueChanged(SliderAugmentation* /*slider*/,
                                                      Real64 /*value*/, UInt32 /*axis*/)
{
}

// Mac 0x1c8c90, Win 0x510ca9
SliderAugmentation::SliderAugmentation()
    : m_flag9c(true),
      m_axis(1),
      m_minimumSize(0.0f, 0.0f),
      m_maximumSize(100000.0f, 100000.0f),
      m_grab(0.0f, 0.0f)
{
}

// Mac 0x1cb730/0x1cb7e0, Win 0x510ef6 / 0x511351
SliderAugmentation::~SliderAugmentation()
{
}

// Mac 0x1cb820, Win 0x510f64
void SliderAugmentation::OnFinalRelease()
{
    NotifierT<SliderAugmentation, SliderAugmentationObserver>::OnFinalRelease();
    SpyOnThisAndParentAugmentation::OnFinalRelease();
}

// Mac 0x1cb660 (ControlAugmentation::Attach and the Spy part inlined), Win 0x510eb1
bool SliderAugmentation::Attach(Control* control, const String& name)
{
    if (!SpyOnThisAndParentAugmentation::Attach(control, name))
        return false;
    Control* own = GetControl();
    own->SetInputFlags(own->GetInputFlags() | 1);
    GetControl()->SetEnabled(true);
    UpdatePosition();
    return true;
}

// Mac 0x1cb8b0 (thunk 0x1cc5c0), Win 0x510f8f
void SliderAugmentation::OnDestroyed(Control* control)
{
    SpyOnThisAndParentAugmentation::OnDestroyed(control);
    UpdatePosition();
}

// Mac 0x1cc520 (thunk 0x1cc5f0), Win 0x51133b
void SliderAugmentation::OnRectangleChanged(Control* control, const Vector2& /*oldPosition*/,
                                            const Vector2& /*oldSize*/)
{
    if (m_parent == control)
        UpdatePosition();
}

// Mac 0x1caff0, Win 0x456400
void SliderAugmentation::UpdatePosition()
{
    for (UInt32 i = 0; i < 2; ++i) {
        Axis& axis = m_axes[i];
        if (!axis.m_enabled)
            continue;
        Real64 value = axis.m_value;
        if (value <= axis.m_minimum)
            value = axis.m_minimum;
        if (axis.m_maximum <= value)
            value = axis.m_maximum;
        axis.m_value = value;
    }
    Control* control = GetControl();
    if (!control || !m_parent)
        return;
    Vector2 position(GetControl()->GetPosition());
    Vector2 size(GetControl()->GetSize());
    Vector2 parentSize(m_parent->GetSize());
    if (m_axes[0].m_enabled)
        position.x = (Real32)(((Real64)(parentSize.x - size.x) /
                               (m_axes[0].m_maximum - m_axes[0].m_minimum)) * m_axes[0].m_value);
    if (m_axes[1].m_enabled)
        position.y = (Real32)(((Real64)(parentSize.y - size.y) /
                               (m_axes[1].m_maximum - m_axes[1].m_minimum)) * m_axes[1].m_value);
    modff(position.x, &position.x);
    modff(position.y, &position.y);
    if (size.x <= m_minimumSize.x)
        size.x = m_minimumSize.x;
    if (m_maximumSize.x <= size.x)
        size.x = m_maximumSize.x;
    if (size.y <= m_minimumSize.y)
        size.y = m_minimumSize.y;
    if (m_maximumSize.y <= size.y)
        size.y = m_maximumSize.y;
    GetControl()->SetRectangle(Rect(position, size));
}

// The observer loop of Mac 0x1cad10 / 0x1ccdf0 / 0x1cb8d0 / 0x1cbf00 (inlined each time).
void SliderAugmentation::NotifyValueChanged(UInt32 axis)
{
    typedef NotifierT<SliderAugmentation, SliderAugmentationObserver> Base;
    Base::Iterator it(*this);
    while (SliderAugmentationObserver* observer = it.Next(Base::m_observers))
        observer->OnSliderValueChanged(this, m_axes[axis].m_value, axis);
}

// Mac 0x1cad10, Win 0x456310
void SliderAugmentation::SetValue(Real64 value, UInt32 axis)
{
    Real64 old = m_axes[axis].m_value;
    m_axes[axis].m_value = value;
    UpdatePosition();
    if (old == m_axes[axis].m_value)
        return;
    NotifyValueChanged(axis);
}

// Mac 0x1ccdf0, Win 0x510dd0
void SliderAugmentation::SetMaximum(Real64 maximum, UInt32 axis)
{
    Axis& a = m_axes[axis];
    Real64 old = a.m_value;
    Real64 minimum = a.m_minimum;
    Real64 newMaximum = maximum <= minimum ? minimum : maximum;
    a.m_maximum = newMaximum;
    a.m_enabled = minimum < maximum;
    Real64 value = old;
    if (value <= minimum)
        value = minimum;
    if (newMaximum <= value)
        value = newMaximum;
    a.m_value = value;
    UpdatePosition();
    if (old == a.m_value)
        return;
    NotifyValueChanged(axis);
}

// Win 0x4562f0 (Mac inlined: ContainerControl)
void SliderAugmentation::SetValue(Real64 value)
{
    SetValue(value, m_axis != 0 ? 1 : 0);
}

// Win 0x510db5 (Mac inlined: ContainerControl::Layout)
void SliderAugmentation::SetMaximum(Real64 maximum)
{
    SetMaximum(maximum, m_axis != 0 ? 1 : 0);
}

// Inlined in Mac 0x1cb8d0 / 0x1cbf00 (Win 0x510fa7 / 0x511176).
void SliderAugmentation::SetValuesFromPosition(const Vector2& position)
{
    Vector2 size(GetControl()->GetSize());
    Vector2 parentSize(m_parent->GetSize());
    Real64 old0 = m_axes[0].m_value;
    Real64 old1 = m_axes[1].m_value;
#if defined(_WIN32)
    // x87: the size difference is not rounded to float.
    Real64 travelX = (Real64)parentSize.x - (Real64)size.x;
    Real64 travelY = (Real64)parentSize.y - (Real64)size.y;
#else
    Real64 travelX = (Real64)(parentSize.x - size.x);
    Real64 travelY = (Real64)(parentSize.y - size.y);
#endif
    if (m_axes[0].m_enabled)
        m_axes[0].m_value = (Real64)position.x *
                            ((m_axes[0].m_maximum - m_axes[0].m_minimum) / travelX);
    if (m_axes[1].m_enabled)
        m_axes[1].m_value = (Real64)position.y *
                            ((m_axes[1].m_maximum - m_axes[1].m_minimum) / travelY);
    UpdatePosition();
    if (m_axes[0].m_enabled && m_axes[0].m_value != old0)
        NotifyValueChanged(0);
    if (m_axes[1].m_enabled && m_axes[1].m_value != old1)
        NotifyValueChanged(1);
}

// Mac 0x1cb8d0 (thunk 0x1cc620), Win 0x510fa7
void SliderAugmentation::OnMouseMove(Control* control, const Vector2& position)
{
    if (GetControl() != control)
        return;
    if (!(control->GetPressedButtons() & 1))
        return;
    Vector2 offset = GetControl()->GetPosition() - m_grab;
    SetValuesFromPosition(offset + position);
}

// Mac 0x1cbf00 (thunk 0x1cc640), Win 0x511176
void SliderAugmentation::OnMousePressed(Control* control, const Vector2& position, UInt32 /*button*/)
{
    if (GetControl() == control) {
        if (!(control->GetPressedButtons() & 1))
            return;
        m_grab = position;
        return;
    }
    if (GetControl()->GetParent() != control)
        return;
    SetValuesFromPosition(position);
}

} // namespace LEGO
