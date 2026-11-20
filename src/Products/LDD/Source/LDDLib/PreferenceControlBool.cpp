// LEGO Digital Designer — restored source
// TU 132 "PreferenceControlBool.cpp" (Mac 0x1e8db0-0x1ea0fb; Win 0x651073-0x65154a,
// 0x68a011-0x68a327): PreferenceControlBool and the out-of-line parts of PreferenceControl.
// Static initialiser Mac 0x1ea080: <iostream> + a header static.
#include "PreferenceControlBool.h"

#include "Control.h"
#include "Preferences.h"
#include "StringStream.h"
#include "LDDUtilities.h"

#include <iostream>

namespace LEGO {

// ---- PreferenceControl ---------------------------------------------------------------------------

// Mac 0x1e93c0 (D1) / 0x1e9420 (D0), thunks 0x1e94a0-0x1e96f0; Win sdd 0x651073
PreferenceControl::~PreferenceControl()
{
}

// Mac 0x1e9490, Win 0x650ee3
bool PreferenceControl::NeedsRestart() const
{
    return m_needsRestart;
}

// ---- PreferenceControlBool -----------------------------------------------------------------------

// Mac 0x1e9770 (D1) / 0x1e97d0 (D0), thunks 0x1e98e0-0x1e9da0; Win sdd 0x65154a
PreferenceControlBool::~PreferenceControlBool()
{
}

// Mac 0x1e8db0, Win 0x68a23c
bool PreferenceControlBool::Create(Control* parent)
{
    if (!RenderControl::Initialize())
        return false;
    if (!CreateCheckBox(m_manager, this, Vector2(), m_label, 45.0f, m_toggle, m_box,
                        m_labelControl))
        return false;
    // NOTE (both builds): the value stays uninitialised when the preference has no value
    // (every key of the dialog has a default); false here.
    bool value = false;
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        String text;
        if (preferences->GetValue(m_key, text))
            FromString(value, text);
    }
    m_toggle->SetToggled(value);
    ToggleControlObserver::Attach(m_toggle);
    Setup(parent, 0, 7, Rect(Vector2(), m_toggle->GetSize()), 0, false);
    SetTransparent(true);
    return true;
}

// Mac 0x1e8f90, Win 0x68a022
void PreferenceControlBool::Revert()
{
    m_toggle->SetToggled(m_value);
}

// Mac 0x1e98b0, Win 0x6511b5
bool PreferenceControlBool::IsChanged() const
{
    return m_toggle->IsToggled() != m_value;
}

// Mac 0x1e8fc0, Win 0x68a03b
void PreferenceControlBool::Store()
{
    RefPointer<Preferences> preferences = Preferences::GetInstance();
    bool value = m_toggle->IsToggled();
    String text;
    ToString(value, text);
    preferences->SetValue(m_key, text);
}

// Mac 0x1e9090, Win 0x68a099
void PreferenceControlBool::Remember()
{
    m_value = m_toggle->IsToggled();
}

// Mac 0x1e90c0, Win 0x68a0b2
void PreferenceControlBool::ResetToDefault()
{
    Preferences::GetInstance()->RemoveValue(m_key);
    // NOTE (both builds): uninitialised without a default value; false here.
    bool value = false;
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        String text;
        if (preferences->GetValue(m_key, text))
            FromString(value, text);
    }
    m_toggle->SetToggled(value);
}

// Mac 0x1e91f0, Win 0x68a142
void PreferenceControlBool::Enable(bool enabled)
{
    if (!enabled) {
        m_toggle->SetOpacity(0.25f);
        m_box->SetOpacity(0.25f);
        m_labelControl->SetOpacity(0.25f);
    } else {
        m_toggle->SetOpacity(1.0f);
        m_box->SetOpacity(1.0f);
        m_labelControl->SetOpacity(1.0f);
    }
    m_toggle->SetEnabled(enabled);
}

// Mac 0x1e9290, Win 0x68a1cc
bool PreferenceControlBool::IsOn() const
{
    if (!m_toggle->IsToggled())
        return false;
    return m_toggle->GetStatus().m_enabledInTree;
}

// Mac 0x1e92d0, Win 0x68a1f5
void PreferenceControlBool::OnMouseClicked(const Vector2& position, UInt32 button)
{
    if (m_toggle->GetStatus().m_enabledInTree)
        m_toggle->SetToggled(!m_toggle->IsToggled());
    Control::OnMouseClicked(position, button);
}

// Mac 0x1e9320 (thunk 0x1e9340), Win 0x68a011
void PreferenceControlBool::OnToggled(ToggleControl*)
{
    Store();
}

// Mac 0x1e9370 (thunk 0x1e9390), Win 0x68a011 (ICF)
void PreferenceControlBool::OnUntoggled(ToggleControl*)
{
    Store();
}

// Mac 0x1e9840
void PreferenceControlBool::OnFinalRelease()
{
    ToggleControlObserver::OnFinalRelease();
    RenderControl::OnFinalRelease();
}

} // namespace LEGO
