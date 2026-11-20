// LEGO Digital Designer — restored source
// TU 152 "BoxFilterControl.cpp": Mac 0x213e00-0x215aab, Win 0x66d067-0x66d1f5 (+ 0x62c001-0x62c31d).
// Not code: Mac 0x215a30 = <iostream> + a header static.
#include "BoxFilterControl.h"
#include "LDDApplication.h"

namespace LEGO {

// Mac 0x214c10 (D1) / 0x214c70 (D0), Win sdd 0x62c31d
BoxFilterControl::~BoxFilterControl()
{
}

// Mac 0x214ce0 (thunks 0x214e90 … 0x2152d0)
void BoxFilterControl::OnFinalRelease()
{
    Notifier<BoxFilterControlObserver>::OnFinalRelease();
    ToggleControl::OnFinalRelease();
}

// Mac 0x213e00, Win 0x66d067
bool BoxFilterControl::Create(Texture* texture, const Vector3& columns, const Vector3& rows,
                              Control* parent, Control* after)
{
    if (!StretchableToggleControl::Create(texture, columns, rows))
        return false;
    Setup(parent, after, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    SetTransparent(true);
    SetHorizontalAlignment(0);
    SetHorizontalTextOffset(5.0f);
    SetVerticalAlignment(2);
    SetFontForAllStates(LDDApplication::GetInstance()->GetFont(14));
    String text;
    if (!m_bag->m_brandFilter)
        text << (String("      ") << m_bag->m_name);
    else
        text << m_bag->m_name;
    SetTextForAllStates(text);
    ResizeHeightToFitText(0);
    return true;
}

// Mac 0x213ff0 (thunk 0x214000), Win 0x66d18d
const BrickFilter* BoxFilterControl::GetFilter() const
{
    return m_filter;
}

// Mac 0x214010, Win 0x66d191
void BoxFilterControl::OnMouseClicked(const Vector2& position, UInt32 button)
{
    typedef NotifierT<BoxFilterControl, BoxFilterControlObserver> BoxNotifier;
    BoxNotifier::Iterator pass(*this);
    while (BoxFilterControlObserver* observer = pass.Next(BoxNotifier::m_observers))
        observer->OnBoxFilterClicked(this);
    Control::OnMouseClicked(position, button);
}

} // namespace LEGO
