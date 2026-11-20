// LEGO Digital Designer — restored source
// TU 032 (Mac 0xf1a10-0xf200b, file name unknown; Win 0x515d17, 0x515fbe).
#include "ControlDragHelper.h"
#include "Control.h"
#include "MathUtil.h"

namespace LEGO {

// Mac 0xf1a10, Win 0x515d17 (EAX = mouse, ECX = control, EDX = this, EDI = mode)
void ControlDragHelper::Drag(UInt32 mode, const Vector2& mouse, Control* control)
{
    if (mode == kNone)
        return;
    Vector2 m(mouse);
    if (m_limited) {
        Vector2 p = m + control->GetPosition();
        if (p.x < m_limits.m_min.x)
            p.x = m_limits.m_min.x;
        if (m_limits.m_max.x < p.x)
            p.x = m_limits.m_max.x;
        if (p.y < m_limits.m_min.y)
            p.y = m_limits.m_min.y;
        if (m_limits.m_max.y < p.y)
            p.y = m_limits.m_max.y;
        m = p - control->GetPosition();
    }
    if (mode == kMove && m_movable) {
        control->SetPosition((control->GetPosition() - m_grab) + m);
        return;
    }
    if (!m_resizable)
        return;
    switch (mode) {
    case kLeft:
        Resize(mode, Vector2(m_grab.x - m.x, 0.0f), Vector2(m_grab.x - m.x, 0.0f), false, false, control);
        break;
    case kTop:
        Resize(mode, Vector2(0.0f, m_grab.y - m.y), Vector2(0.0f, m_grab.y - m.y), false, false, control);
        break;
    case kTop | kLeft:
        Resize(mode, m_grab - m, m_grab - m, false, false, control);
        break;
    case kRight:
        Resize(mode, Vector2(), Vector2(m.x - m_grab.x, 0.0f), true, false, control);
        break;
    case kTop | kRight:
        Resize(mode, Vector2(0.0f, m_grab.y - m.y), Vector2(m.x - m_grab.x, m_grab.y - m.y), true,
               false, control);
        break;
    case kBottom:
        Resize(mode, Vector2(), Vector2(0.0f, m.y - m_grab.y), false, true, control);
        break;
    case kBottom | kLeft:
        Resize(mode, Vector2(m_grab.x - m.x, 0.0f), Vector2(m_grab.x - m.x, m.y - m_grab.y), false,
               true, control);
        break;
    case kBottom | kRight:
        Resize(mode, Vector2(), m - m_grab, true, true, control);
        break;
    default:
        break;
    }
}

// Mac 0xf1db0, Win 0x515fbe
void ControlDragHelper::Resize(UInt32 mode, const Vector2& positionDelta, const Vector2& sizeDelta,
                               bool keepX, bool keepY, Control* control)
{
    Vector2 size(control->GetSize());
    Vector2 position(control->GetPosition());
    Vector2 grab(m_grab);
    Vector2 newPosition = position - positionDelta;
    Vector2 newSize = size + sizeDelta;
    if (newSize.x < m_minimumSize.x) {
        if (!keepX)
            newPosition.x = (Real32)((EvalReal)newPosition.x - ((EvalReal)m_minimumSize.x - newSize.x));
        newSize.x = m_minimumSize.x;
    }
    if (newSize.y < m_minimumSize.y) {
        if (!keepY)
            newPosition.y = (Real32)((EvalReal)newPosition.y - ((EvalReal)m_minimumSize.y - newSize.y));
        newSize.y = m_minimumSize.y;
    }
    if (keepX)
        m_grab.x = (Real32)((EvalReal)newSize.x - ((EvalReal)size.x - m_grab.x));
    if (keepY)
        m_grab.y = (Real32)((EvalReal)newSize.y - ((EvalReal)size.y - m_grab.y));
    control->SetRectangle(Rect(newPosition, newSize));
    if (control->GetPosition() != newPosition || control->GetSize() != newSize) {
        // The control refused the rectangle (its min / max size): keep the undragged edges in place.
        if (mode & kLeft)
            newPosition.x = (Real32)((EvalReal)position.x + size.x - control->GetSize().x);
        if (mode & kTop)
            newPosition.y = (Real32)((EvalReal)position.y + size.y - control->GetSize().y);
        if (mode & kRight)
            m_grab.x = (Real32)((EvalReal)grab.x - size.x + control->GetSize().x);
        if (mode & kBottom)
            m_grab.y = (Real32)((EvalReal)grab.y - size.y + control->GetSize().y);
        control->SetPosition(newPosition);
    }
}

} // namespace LEGO
