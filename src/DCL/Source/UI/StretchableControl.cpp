// LEGO Digital Designer — restored source
// TU 080 "StretchableControl.cpp".
#include "StretchableControl.h"

namespace LEGO {

// Mac 0x16cbe0 / 0x16cc40 (body 0x16cf80: the parts are released last to first), Win 0x627bf6
StretchableControl::~StretchableControl()
{
}

// (inlined nine times into Create)
RenderControl* StretchableControl::CreatePart(UInt32 part, Texture* texture, const Rect& rect)
{
    m_parts[part] = new RenderControl(GetManager());
    RenderControl* control = m_parts[part].Get();
    if (!control->Initialize())
        return 0;
    control->Setup(this, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    control->SetImage(texture, &rect);
    return control;
}

// Mac 0x16bf70, Win 0x51983c
bool StretchableControl::Create(Texture* texture, const Rect& topLeft, const Rect& top,
                                const Rect& topRight, const Rect& left, const Rect& center,
                                const Rect& right, const Rect& bottomLeft, const Rect& bottom,
                                const Rect& bottomRight)
{
    if (!RenderControl::Initialize())
        return false;

    RenderControl* part = CreatePart(kTopLeft, texture, topLeft);
    if (!part)
        return false;
    part->SetAlignment(0x11, 0x11, 0.0f, 0.0f, 0);

    if (!(part = CreatePart(kTop, texture, top)))
        return false;
    part->SetAxisMargins(0, -topLeft.GetWidth(), topRight.GetWidth(), 0);
    part->SetAxisAlignment(1, 1, 1, 0.0f, 0);

    if (!(part = CreatePart(kTopRight, texture, topRight)))
        return false;
    part->SetAlignment(0x13, 0x13, 0.0f, 0.0f, 0);

    if (!(part = CreatePart(kLeft, texture, left)))
        return false;
    part->SetAxisAlignment(0, 1, 1, 0.0f, 0);
    part->SetAxisMargins(1, -topLeft.GetHeight(), bottomLeft.GetHeight(), 0);

    if (!(part = CreatePart(kCenter, texture, center)))
        return false;
    part->SetMargins(-left.GetWidth(), right.GetWidth(), -top.GetHeight(), bottom.GetHeight(), 0);

    if (!(part = CreatePart(kRight, texture, right)))
        return false;
    part->SetAxisAlignment(0, 3, 3, 0.0f, 0);
    part->SetAxisMargins(1, -topRight.GetHeight(), bottomRight.GetHeight(), 0);

    if (!(part = CreatePart(kBottomLeft, texture, bottomLeft)))
        return false;
    part->SetAlignment(0x31, 0x31, 0.0f, 0.0f, 0);

    if (!(part = CreatePart(kBottom, texture, bottom)))
        return false;
    part->SetAxisMargins(0, -bottomLeft.GetWidth(), bottomRight.GetWidth(), 0);
    part->SetAxisAlignment(1, 3, 3, 0.0f, 0);

    if (!(part = CreatePart(kBottomRight, texture, bottomRight)))
        return false;
    part->SetAlignment(0x33, 0x33, 0.0f, 0.0f, 0);
    return true;
}

} // namespace LEGO
