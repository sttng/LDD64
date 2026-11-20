// LEGO Digital Designer — restored source
// LEGO::SliderControl — TU 272 "SliderControl.cpp" (Mac 0x286480-0x288c9b, Win
// 0x518aa7-0x519813).
// Not restored here: the NotifierT<SliderControl, SliderControlObserver> / Notifier<> /
// BasicArray<> template members the TU emits (Mac 0x288300-0x288a40) — Notifier.h.
//
// Arithmetic as in Control.cpp: Mac rounds every operation to float (SSE), Win keeps an
// expression's intermediates in x87 double precision (`Intermediate`).
#include "SliderControl.h"
#include "ControlManager.h"
#include "Preferences.h"
#include "StringStream.h"
#include "BitmapSelectionMask.h"
#include <math.h>

namespace LEGO {

namespace {

#if defined(_WIN32)
typedef double Intermediate;
#else
typedef Real32 Intermediate;
#endif
inline Intermediate X(Real32 value) { return (Intermediate)value; }

// Rounds `value` to a multiple of `step` (truncated quotient, then to the nearer multiple, ties
// away from zero); a zero step leaves it. Inlined on Mac, Win 0x4aecb0 (shared helper).
Real32 SnapToStep(Real32 value, Real32 step)
{
    if (step == 0.0f)
        return value;
    Real32 q = (Real32)(Int32)(value / step) * step;
    Real32 r = value - q;
    if (r >= 0.0f) {
        if (r >= step * 0.5f)
            q += step;
    } else if (r <= step * -0.5f) {
        q -= step;
    }
    return q;
}

} // namespace

// Mac 0x286750 (C2 0x286480)
SliderControl::SliderControl(ControlManager* manager, Real32 minimum, Real32 maximum, Real32 step,
                             Real32 value)
    : RenderControl(manager),
      m_track(0),
      m_thumb(0),
      m_texture(0),
      m_thumbUp(0.0f, 0.0f, 0.0f, 0.0f),
      m_thumbOver(0.0f, 0.0f, 0.0f, 0.0f),
      m_thumbDown(0.0f, 0.0f, 0.0f, 0.0f),
      m_thumbDisabled(0.0f, 0.0f, 0.0f, 0.0f),
      m_minimum(minimum),
      m_maximum(maximum),
      m_step(step),
      m_value(value),
      m_dragging(false)
{
}

// Mac 0x287e00/0x288020, Win 0x519367 / 0x518aa7
SliderControl::~SliderControl()
{
}

// Mac 0x288290
void SliderControl::OnFinalRelease()
{
    NotifierT<SliderControl, SliderControlObserver>::OnFinalRelease();
    RenderControl::OnFinalRelease();
}

// Mac 0x286dd0, Win 0x518c3f
bool SliderControl::Initialize()
{
    return Initialize(String(""));
}

// Mac 0x286a30, Win 0x518ad7
bool SliderControl::Initialize(const String& preferenceName)
{
    if (!RenderControl::Initialize())
        return false;

    m_track = new RenderControl(m_manager);
    if (!m_track->Initialize())
        return false;
    m_track->Setup(this, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    m_track->SetTransparent(true);

    m_thumb = new RenderControl(m_manager);
    if (!m_thumb->Initialize())
        return false;
    m_thumb->Setup(this, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    m_thumb->SetTransparent(true);

    SetInputFlags(m_inputFlags | 1);
    SetTransparent(true);
    SetPreferenceName(preferenceName);
    return true;
}

// Mac 0x286cd0, Win 0x5192f2
void SliderControl::SetPreferenceName(const String& name)
{
    m_preferenceName = name;
    if (m_preferenceName.IsEmpty())
        return;
    Real32 value = m_value;
    RefPointer<Preferences> preferences = Preferences::GetInstance();
    String text;
    if (preferences->GetValue(m_preferenceName, text))
        FromString(value, text);
    SetValue(value);
}

// Mac 0x287c00, Win 0x519360
const String& SliderControl::GetPreferenceName() const
{
    return m_preferenceName;
}

// Win 0x51911d (inlined on Mac)
void SliderControl::UpdateThumbPosition()
{
    const Vector2& trackPosition = m_track->GetPosition();
    const Vector2& trackSize = m_track->GetSize();
    Real32 x = (Real32)((X(trackSize.x) - X(m_thumb->GetSize().x)) *
                        ((X(m_value) - X(m_minimum)) / (X(m_maximum) - X(m_minimum))) +
                        X(trackPosition.x));
    m_thumb->SetPosition(Vector2(x, 0.0f));
}

// (inlined in OnMouseMove / OnMouseReleased; Win 0x518f81 / 0x519007)
Real32 SliderControl::ValueAt(Real32 x) const
{
    const Vector2& trackPosition = m_track->GetPosition();
    const Vector2& trackSize = m_track->GetSize();
    // the fraction is a stored float on both builds (Win 0x518fc0 fstp)
    Real32 fraction = (Real32)((X(x) - X(trackPosition.x)) / X(trackSize.x));
    Real32 value = (Real32)((X(m_maximum) - X(m_minimum)) * X(fraction) + X(m_minimum));
    return SnapToStep(value, m_step);
}

// Mac 0x286e40, Win 0x518c82
void SliderControl::SetImages(Texture* texture, const Rect& trackRect, const Rect& thumbUp,
                              const Rect& thumbOver, const Rect& thumbDown,
                              const Rect& thumbDisabled, Real32 trackY, BitmapMask* mask,
                              const Rect& trackMaskRect, const Vector2& trackMaskPoint,
                              const Rect& thumbMaskRect, const Vector2& thumbMaskPoint)
{
    m_texture = texture;
    m_thumbOver = thumbOver;
    m_thumbDown = thumbDown;
    m_thumbDisabled = thumbDisabled;
    m_thumbUp = thumbUp;
    m_track->SetSize(trackRect.GetSize());
    m_track->SetPosition(Vector2(0.0f, trackY));
    m_thumb->SetImage(texture, &thumbUp);
    if (!trackMaskRect.IsZero()) {
        RefPointer<BitmapSelectionMask> selectionMask = new BitmapSelectionMask();
        selectionMask->SetMask(mask, &trackMaskRect);
        selectionMask->Pick(trackMaskPoint, true);
        m_track->SetSelectionMask(selectionMask);
    }
    if (!thumbMaskRect.IsZero()) {
        RefPointer<BitmapSelectionMask> selectionMask = new BitmapSelectionMask();
        selectionMask->SetMask(mask, &thumbMaskRect);
        selectionMask->Pick(thumbMaskPoint, true);
        m_thumb->SetSelectionMask(selectionMask);
    }
    GrowToChildren(true, true, true, true);
    UpdateThumbPosition();
}

// Mac 0x287250, Win 0x518e7d
void SliderControl::SetMinMaxStep(Real32 minimum, Real32 maximum, Real32 step, Real32 value)
{
    m_minimum = minimum;
    m_maximum = maximum;
    m_step = step;
    SetValue(value);
}

// Mac 0x2872a0, Win 0x519167
void SliderControl::SetValue(Real32 value)
{
    Real32 oldValue = m_value;
    if (fabsf(value - oldValue) < 1e-5f)
        return;
    if (value < m_minimum)
        m_value = m_minimum;
    else if (value > m_maximum)
        m_value = m_maximum;
    else
        m_value = SnapToStep(value - m_minimum, m_step) + m_minimum;

    if (!m_preferenceName.IsEmpty()) {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        String text;
        ToString(m_value, text);
        preferences->SetValue(m_preferenceName, text);
    }
    UpdateThumbPosition();

    typedef NotifierT<SliderControl, SliderControlObserver> Base;
    Base::Iterator it(*this);
    while (SliderControlObserver* observer = it.Next(Base::m_observers))
        observer->OnValueChanged(this, m_value, oldValue);
}

// Mac 0x287b90, Win 0x5192dd
Real32 SliderControl::GetValue() const
{
    return m_value;
}

// Mac 0x287ba0, Win 0x5192b8
void SliderControl::IncreaseSteps(Int32 steps)
{
    SetValue((Real32)(X((Real32)steps) * X(m_step) + X(m_value)));
}

// Mac 0x287be0, Win 0x5192e4
Real32 SliderControl::GetMinimum() const
{
    return m_minimum;
}

// Mac 0x287bf0, Win 0x5192eb
Real32 SliderControl::GetMaximum() const
{
    return m_maximum;
}

// Mac 0x287740, Win 0x518eab
void SliderControl::UpdateThumbImage(const Vector2& position)
{
    if (!m_status.m_enabledInTree && !m_thumbDisabled.IsZero()) {
        m_thumb->SetImage(m_texture, &m_thumbDisabled);
        return;
    }
    if (m_thumbOver.IsZero())
        return;
    const Rect* image;
    Rect thumbRect(m_thumb->GetPosition(), m_thumb->GetSize());
    if (!thumbRect.Contains(position)) {
        if (m_dragging)
            return;
        image = &m_thumbUp;
    } else if (m_dragging && !m_thumbDown.IsZero()) {
        image = &m_thumbDown;
    } else {
        image = &m_thumbOver;
        if (!m_status.m_enabledInTree)
            return;
    }
    m_thumb->SetImage(m_texture, image);
}

// Mac 0x287830, Win 0x518f4b
void SliderControl::OnMousePressed(const Vector2& position, UInt32 button)
{
    if (button == 0) {
        CaptureMouse();
        m_dragging = true;
        UpdateThumbImage(position);
    }
    Control::OnMousePressed(position, button);
}

// Mac 0x287870, Win 0x518f81
void SliderControl::OnMouseMove(const Vector2& position)
{
    if (m_dragging && m_track->GetPosition().x < position.x)
        SetValue(ValueAt(position.x));
    UpdateThumbImage(position);
    Control::OnMouseMove(position);
}

// Mac 0x287960, Win 0x519007
void SliderControl::OnMouseReleased(const Vector2& position, UInt32 button)
{
    m_dragging = false;
    SetValue(ValueAt(position.x));
    ReleaseMouseCapture();
    UpdateThumbImage(position);
    Control::OnMouseReleased(position, button);
}

// Mac 0x287a40, Win 0x519083
void SliderControl::OnEnabled()
{
    Vector2 position = m_manager->GetMousePosition(this);
    UpdateThumbImage(position);
    Control::OnEnabled();
}

// Mac 0x287a80, Win 0x5190a7
void SliderControl::OnDisabled()
{
    m_thumb->SetImage(m_texture, &m_thumbDisabled);
    Control::OnDisabled();
}

// Mac 0x287ab0, Win 0x5190ca
void SliderControl::OnMouseEnter(const Vector2& position)
{
    UpdateThumbImage(position);
    Control::OnMouseEnter(position);
}

// Mac 0x287ad0, Win 0x5190e5
void SliderControl::OnMouseLeave(const Vector2& position)
{
    UpdateThumbImage(position);
    Control::OnMouseLeave(position);
}

// Mac 0x287af0, Win 0x519100
void SliderControl::OnRectangleChanged(const Vector2& oldPosition, const Vector2& oldSize)
{
    UpdateThumbPosition();
    RenderControl::OnRectangleChanged(oldPosition, oldSize);
}

} // namespace LEGO
