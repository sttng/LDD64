// LEGO Digital Designer — restored source
// TU 295 "DialControl.cpp" (Mac 0x2a3450-0x2a59db, Win 0x51b310-0x51bf06).
// Arithmetic: EvalReal (MathUtil.h) where the Win build keeps an x87 intermediate.
#include "DialControl.h"
#include "ControlManager.h"
#include "Texture.h"
#include "Color.h"
#include "MathUtil.h"
#include <math.h>

namespace LEGO {

typedef NotifierT<DialControl, DialControlObserver> DialNotifier;

// Mac 0x2a3450 (C1)
DialControl::DialControl(ControlManager* manager)
    : Control(manager), m_imageRect(0.0f, 0.0f, 0.0f, 0.0f), m_angle(0.0f), m_flag1a4(false),
      m_state(StateNormal)
{
    m_stateDescriptionIndex[0] = 0;
    m_stateDescriptionIndex[1] = 1;
    m_stateDescriptionIndex[2] = 2;
    m_stateDescriptionIndex[3] = 3;
}

// Mac 0x2a4340 (D1), 0x2a4100 (D2 body), 0x2a4480 (D0); Win sdd 0x51b310, body 0x51b744
DialControl::~DialControl()
{
}

// Mac 0x2a3690, Win 0x51b340
bool DialControl::Initialize(const Vector2& imageSize, const Vector2& center)
{
    Rect image(Vector2(0.0f, 0.0f), imageSize);
    m_stateDescriptions.Resize(4, StateDescription());
    m_stateDescriptions[0].m_cursor = 1;
    m_stateDescriptions[1].m_cursor = 2;
    m_stateDescriptions[2].m_cursor = 3;
    m_stateDescriptions[3].m_cursor = 1;
    m_imageSize = imageSize;
    m_center = center;
    // The circle around the centre that holds every corner of the picture.
    Real32 d0 = Distance(center, image.GetMin());
    Real32 d1 = Distance(center, image.GetTopRight());
    Real32 radius = d0 <= d1 ? d1 : d0;
    Real32 d2 = Distance(center, image.GetBottomLeft());
    Real32 d3 = Distance(center, image.GetMax());
    Real32 other = d2 <= d3 ? d3 : d2;
    if (radius <= other)
        radius = other;
    SetMinimumSize(Vector2(radius + radius, radius + radius));
    SetInputFlags(m_inputFlags | 1);
    return Setup(0, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
}

// Mac 0x2a3980, Win 0x51b510
void DialControl::SetStateDescription(UInt32 index, const StateDescription& description)
{
    StateDescription& target = m_stateDescriptions[index];
    target.m_image = description.m_image;
    target.m_offset = description.m_offset;
    target.m_cursor = description.m_cursor;
    if (m_stateDescriptionIndex[m_state] == index)
        UpdateState();
}

// Mac 0x2a3a30, Win 0x51b91c
void DialControl::UpdateState()
{
    const StateDescription& description = m_stateDescriptions[m_stateDescriptionIndex[m_state]];
    m_image = description.m_image;
    m_imageRect = Rect(description.m_offset, m_imageSize);
    SetCursor(description.m_cursor, false);
}

// Win 0x51b8fe (Mac inlined)
void DialControl::SetState(UInt32 state)
{
    if (m_state == state)
        return;
    m_state = state;
    UpdateState();
    Redraw();
}

void DialControl::Redraw()
{
    if (!m_manager->IsUpdateLocked())
        Invalidate(false);
}

// Mac 0x2a3af0, Win 0x51b568
void DialControl::SetAngle(Real32 angle)
{
    Real32 oldAngle = m_angle;
    if (oldAngle == angle)
        return;
    m_angle = angle;
    UpdateTransform();
    Redraw();
    DialNotifier::Iterator it(*this);
    while (DialControlObserver* observer = it.Next(DialNotifier::m_observers))
        observer->OnAngleChanged(this, angle, oldAngle);
}

// Mac 0x2a3df0, Win 0x51b7e9
void DialControl::UpdateTransform()
{
    Matrix3D translation;
    Matrix3D rotation;
    translation.SetTranslation(Vector3(-m_center.x, -m_center.y, 0.0f));
    rotation.SetRotationZ(m_angle);
    rotation.Multiply(translation);
    m_transform.SetIdentity();
    Vector3& origin = m_transform.GetTranslation();
    origin.x = (Real32)floor((double)(m_size.x * 0.5f));
    origin.y = (Real32)floor((double)(0.5f * m_size.y));
    m_transform.Multiply(rotation);
    m_inverseTransform.SetInverse(m_transform);
}

// Win 0x51b6e5 (Mac inlined in 0x2a3f20 / 0x2a49a0)
bool DialControl::IsOverImage(const Vector2& point) const
{
    Vector3 local = Vector3(point.x, point.y, 0.0f) * m_inverseTransform;
    Rect image(m_imageSize);
    return image.Contains(Vector2(local.x, local.y));
}

// Win 0x51b973 (Mac inlined in 0x2a49a0 / 0x2a4b80)
void DialControl::TrackAngle(const Vector2& point)
{
    Vector2 center(m_size.x * 0.5f, m_size.y * 0.5f);
    Vector2 delta = point - center;
    Real32 angle = ATan2(delta.x, -delta.y);
    // 2 pi as the float 0x40c90fdb (Mac literal, Win .data 0xb43df4)
    Real32 wrap = angle < 0.0f ? 6.28318548f : 0.0f;
    SetAngle(wrap + angle);
}

// Mac 0x2a3f20, Win 0x51b61d
bool DialControl::IsInside(const Vector2& point) const
{
    RefPointer<const SelectionMask> mask = m_selectionMask;
    if (mask.Get()) {
        // The mask is centred on the control.
        Vector2 local(point);
        local.x = (Real32)(EvalReal(point.x) + (EvalReal(m_size.x) - EvalReal(mask->GetSize().x)) * 0.5f);
        local.y = (Real32)(EvalReal(point.y) + (EvalReal(m_size.y) - EvalReal(mask->GetSize().y)) * 0.5f);
        return Control::IsInside(local);
    }
    if (Control::IsInside(point))
        return true;
    return IsOverImage(point);
}

// Mac 0x2a4790, Win 0x51b9f9
void DialControl::OnRectangleChanged(const Vector2& oldPosition, const Vector2& oldSize)
{
    UpdateTransform();
    Control::OnRectangleChanged(oldPosition, oldSize);
}

// Mac 0x2a4880, Win 0x51bab8
void DialControl::OnHidden()
{
    if (m_status.m_enabledInTree)
        SetState(StateNormal);
    Control::OnHidden();
}

// Mac 0x2a48e0, Win 0x51bad5
void DialControl::OnEnabled()
{
    SetState(m_status.m_mouseInside ? StateOver : StateNormal);
    Control::OnEnabled();
}

// Mac 0x2a4950, Win 0x51baf4
void DialControl::OnDisabled()
{
    SetState(StateDisabled);
    Control::OnDisabled();
}

// Mac 0x2a49a0, Win 0x51bb09
void DialControl::OnMouseMove(const Vector2& position)
{
    if (m_status.m_enabledInTree) {
        SetState(HasMouseCapture() || IsOverImage(position) ? StateOver : StateNormal);
        if (HasMouseCapture())
            TrackAngle(position);
    }
    Control::OnMouseMove(position);
}

// Mac 0x2a4b80, Win 0x51bb5d
void DialControl::OnMousePressed(const Vector2& position, UInt32 button)
{
    if (button == 0) {
        SetState(StatePressed);
        TrackAngle(position);
        CaptureMouse();
    }
    Control::OnMousePressed(position, button);
}

// Mac 0x2a4ca0, Win 0x51bb96
void DialControl::OnMouseReleased(const Vector2& position, UInt32 button)
{
    if (button == 0) {
        if (HasMouseCapture())
            ReleaseMouseCapture();
        SetState(m_status.m_mouseInside ? StateOver : StateNormal);
    }
    Control::OnMouseReleased(position, button);
}

// Mac 0x2a47b0, Win 0x51ba14
void DialControl::Draw(ControlDrawArgs& args)
{
    if (m_image.Get()) {
        args.Setup(false);
        args.MultiplyWorldMatrix(m_transform);
        Rect textureRect(m_imageRect);
        textureRect.Divide(m_image->GetImageSize());
        args.DrawTexture(Rect(m_imageSize), m_image, textureRect, Color::White(), true);
    }
    m_status.m_invalid = 0;
}

// Mac 0x2a45f0, Win (Object-in vtable)
void DialControl::OnFinalRelease()
{
    DialNotifier::OnFinalRelease();
    Control::OnFinalRelease();
}

} // namespace LEGO
