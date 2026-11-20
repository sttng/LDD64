// LEGO Digital Designer — restored source
// LEGO::ToggleControl — TU 089 "ToggleControl.cpp" (Mac 0x1a5c00-0x1a724b, Win
// 0x51818c-0x518624).
// Not restored here: the NotifierT<ToggleControl, ToggleControlObserver> / Notifier<> /
// BasicArray<> template members the TU emits (Mac 0x1a6150-0x1a6580, 0x1a6ff0) — Notifier.h.
#include "ToggleControl.h"
#include "ControlManager.h"

namespace LEGO {

// Mac 0x1a5cf0 (C2 0x1a5c00), Win 0x518270
ToggleControl::ToggleControl(ControlManager* manager)
    : ButtonControl(manager),
      m_toggled(false),
      m_canUntoggle(true)
{
}

// Mac 0x1a6620/0x1a66e0, Win 0x51825a / 0x518238
ToggleControl::~ToggleControl()
{
}

// Mac 0x1a6720
void ToggleControl::OnFinalRelease()
{
    NotifierT<ToggleControl, ToggleControlObserver>::OnFinalRelease();
    RenderControl::OnFinalRelease();
}

// Mac 0x1a5df0, Win 0x518338
bool ToggleControl::Initialize()
{
    EnsureStateDescriptions(8);
    if (!ButtonControl::Initialize())
        return false;
    m_states[4 + STATE_UP].m_cursor = 1;
    m_states[4 + STATE_OVER].m_cursor = 2;
    m_states[4 + STATE_DOWN].m_cursor = 3;
    m_states[4 + STATE_DISABLED].m_cursor = 1;
    return true;
}

// Mac 0x1a6010, Win 0x5183c2
bool ToggleControl::IsToggled() const
{
    return m_toggled;
}

// Mac 0x1a6020, Win 0x5183c9
void ToggleControl::SetToggled(bool toggled)
{
    if (m_toggled == toggled)
        return;
    m_toggled = toggled;
    if (!toggled) {
        SetStateMapping(0, 0);
        SetStateMapping(1, 1);
        SetStateMapping(2, 2);
        SetStateMapping(3, 3);
        NotifyUntoggled();
    } else {
        SetStateMapping(0, 4);
        SetStateMapping(1, 5);
        SetStateMapping(2, 6);
        SetStateMapping(3, 7);
        NotifyToggled();
    }
}

// Mac 0x1a60d0, Win 0x51844f
void ToggleControl::SetCanUntoggle(bool canUntoggle)
{
    m_canUntoggle = canUntoggle;
}

// Mac 0x1a60f0, Win 0x51845c
bool ToggleControl::CanUntoggle() const
{
    return m_canUntoggle;
}

// Mac 0x1a6100, Win 0x518463
void ToggleControl::OnMouseClicked(const Vector2& position, UInt32 button)
{
    if (button == 0 && (m_canUntoggle || !m_toggled))
        SetToggled(!m_toggled);
    Control::OnMouseClicked(position, button);
}

// Mac 0x1a6790, Win 0x51818c
void ToggleControl::NotifyToggled()
{
    typedef NotifierT<ToggleControl, ToggleControlObserver> Base;
    Base::Iterator it(*this);
    while (ToggleControlObserver* observer = it.Next(Base::m_observers))
        observer->OnToggled(this);
}

// Mac 0x1a6a20, Win 0x5181e2
void ToggleControl::NotifyUntoggled()
{
    typedef NotifierT<ToggleControl, ToggleControlObserver> Base;
    Base::Iterator it(*this);
    while (ToggleControlObserver* observer = it.Next(Base::m_observers))
        observer->OnUntoggled(this);
}

} // namespace LEGO
