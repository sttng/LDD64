// LEGO Digital Designer — restored source
// TU 408 "FrameAugmentation.cpp".
#include "FrameAugmentation.h"

namespace LEGO {

// Mac 0x38da00 / 0x38da60, Win sdd 0x45b3c0
FrameAugmentation::~FrameAugmentation()
{
}

// Mac 0x38d720, Win 0x515b8d
bool FrameAugmentation::Attach(Control* control, const String& name)
{
    if (!ControlAugmentationControlObserver::Attach(control, name))
        return false;
    Layout();
    return true;
}

// Mac 0x38d7a0
void FrameAugmentation::Layout()
{
    GetControl()->SetPosition(m_frame.m_min);
    Control* parent = GetControl()->GetParent();
    if (parent) {
        Vector2 size(GetControl()->GetSize());
        size *= GetControl()->GetScale();
        parent->SetSize(size + m_frame.m_min + m_frame.m_max);
    }
}

// Mac 0x38d850
void FrameAugmentation::SetFrame(Real32 left, Real32 top, Real32 right, Real32 bottom)
{
    m_frame.m_min.x = left;
    m_frame.m_min.y = top;
    m_frame.m_max.x = right;
    m_frame.m_max.y = bottom;
    if (GetControl())
        Layout();
}

Real32 FrameAugmentation::GetLeft() const { return m_frame.m_min.x; }     // Mac 0x38d8a0
Real32 FrameAugmentation::GetTop() const { return m_frame.m_min.y; }      // Mac 0x38d8b0
Real32 FrameAugmentation::GetRight() const { return m_frame.m_max.x; }    // Mac 0x38d8c0
Real32 FrameAugmentation::GetBottom() const { return m_frame.m_max.y; }   // Mac 0x38d8d0

// Mac 0x38d8e0, Win 0x515bac
void FrameAugmentation::OnDestroyed(Control* control)
{
    if (GetControl() == control)
        Layout();
}

// Mac 0x38d940, Win 0x515bc5
void FrameAugmentation::OnRectangleChanged(Control* control, const Vector2& /*oldPosition*/,
                                           const Vector2& /*oldSize*/)
{
    if (GetControl() == control)
        Layout();
}

// Mac 0x38d9a0, Win 0x515bde
void FrameAugmentation::OnScaleChanged(Control* control, const Vector2& /*oldScale*/)
{
    if (GetControl() == control)
        Layout();
}

} // namespace LEGO
