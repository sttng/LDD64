// LEGO Digital Designer — restored source
// TU 174 "StretchableToggleControl.cpp".
#include "StretchableToggleControl.h"

namespace LEGO {

// Mac 0x2142b0 / 0x214310
StretchableToggleControl::~StretchableToggleControl()
{
}

// Mac 0x232ca0, Win 0x519f26
bool StretchableToggleControl::Create(Texture* texture, const Vector3& columns, const Vector3& rows)
{
    if (!ToggleControl::Initialize())
        return false;
    if (!m_image.Create(texture, columns, rows, GetManager()))
        return false;
    // (BasicArray<Vector2>::Insert instance Mac 0x232e30, Win 0x51a04e)
    m_imagePositions.Resize(m_states.GetSize(), Vector2());
    return true;
}

// Mac 0x232d60, Win 0x519f8a
void StretchableToggleControl::OnRectangleChanged(const Vector2& oldPosition, const Vector2& oldSize)
{
    ToggleControl::OnRectangleChanged(oldPosition, oldSize);
    m_image.SetSize(m_size);
}

// Mac 0x232da0, Win 0x519fac
void StretchableToggleControl::UpdateState()
{
    ToggleControl::UpdateState();
    m_image.SetImagePosition(m_imagePositions[GetStateMapping(GetState())]);
}

// Mac 0x232df0, Win 0x519fd9
void StretchableToggleControl::Draw(ControlDrawArgs& args)
{
    args.Setup(false);
    m_image.Draw(args);
    RenderControl::Draw(args);
}

} // namespace LEGO
