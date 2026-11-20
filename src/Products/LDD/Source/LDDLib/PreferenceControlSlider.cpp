// LEGO Digital Designer — restored source
// TU 285 "PreferenceControlSlider.cpp" (Mac 0x299420-0x29aa1b; Win 0x68a90b-0x68ac81,
// 0x68aca3): PreferenceControlSlider. Static initialiser Mac 0x29a9a0: <iostream> + a header
// static.
#include "PreferenceControlSlider.h"

#include "Control.h"
#include "Preferences.h"
#include "StringStream.h"
#include "LDDApplication.h"

#include <iostream>

namespace LEGO {

// Mac 0x299420, Win 0x68aae4
PreferenceControlSlider::PreferenceControlSlider(ControlManager* manager, bool needsRestart)
    : PreferenceControl(manager, needsRestart), m_value(0.0f)
{
}

// Mac 0x29a220 (D1) / 0x29a440 (D0), thunks 0x29a280-0x29a630; Win sdd 0x68ac81
PreferenceControlSlider::~PreferenceControlSlider()
{
}

// Mac 0x299510, Win 0x68aca3
bool PreferenceControlSlider::Create(Control* parent, Real32 minimum, Real32 maximum,
                                     Real32 step, UInt32 style, const String& preferenceName,
                                     const String& label)
{
    if (!RenderControl::Initialize())
        return false;
    Setup(parent, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    SetTransparent(true);

    m_label = new RenderControl(m_manager);
    if (!m_label->Initialize())
        return false;
    m_label->Setup(this, 0, 7, Rect(Vector2(0.0f, 0.0f), Vector2()), 0, false);
    m_label->SetText(label);
    m_label->SetFont(LDDApplication::GetInstance()->GetFont(3));
    m_label->ResizeHeightToFitText(0);
    m_label->SetTransparent(true);

    m_slider = new LDDSliderControl(m_manager, minimum, maximum, step, minimum);
    if (!m_slider->Create(style, preferenceName, this, 0))
        return false;
    m_slider->SetAlignment(0x23, 0x23, 0.0f, 0.0f, 0);
    SliderControlObserver::Attach(m_slider.Get());
    FitToChildren(true, true, true, true);
    return true;
}

// Mac 0x299d70, Win 0x68a91c
void PreferenceControlSlider::Revert()
{
    m_slider->SetValue(m_value);
}

// Mac 0x299da0, Win 0x68a932
bool PreferenceControlSlider::IsChanged() const
{
    return m_slider->GetValue() != m_value;
}

// Mac 0x299de0, Win 0x68a957
void PreferenceControlSlider::Store()
{
    RefPointer<Preferences> preferences = Preferences::GetInstance();
    const String& key = m_slider->GetPreferenceName();
    Real32 value = m_slider->GetValue();
    String text;
    ToString(value, text);
    preferences->SetValue(key, text);
}

// Mac 0x299ec0, Win 0x68a9b7
void PreferenceControlSlider::Remember()
{
    m_value = m_slider->GetValue();
}

// Mac 0x299ef0, Win 0x68a9cd
void PreferenceControlSlider::ResetToDefault()
{
    Preferences::GetInstance()->RemoveValue(m_slider->GetPreferenceName());
    Real32 value = 0.0f;   // NOTE: uninitialised in both builds when there is no default
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        String text;
        if (preferences->GetValue(m_slider->GetPreferenceName(), text))
            FromString(value, text);
    }
    m_slider->SetValue(value);
}

// Mac 0x29a030, Win 0x68aa6e
void PreferenceControlSlider::Enable(bool enabled)
{
    if (!enabled) {
        m_label->SetOpacity(0.25f);
        m_slider->SetOpacity(0.25f);
    } else {
        m_label->SetOpacity(1.0f);
        m_slider->SetOpacity(1.0f);
    }
    m_slider->SetEnabled(enabled);
}

// Mac 0x29a0b0, Win 0x68aad2
bool PreferenceControlSlider::IsOn() const
{
    return m_slider->GetStatus().m_enabledInTree;
}

// Mac 0x29a930 (thunk 0x29a940); Win 0x65a12a (ICF)
void PreferenceControlSlider::OnAttached(SliderControl*)
{
}

// Mac 0x29a950 (thunk 0x29a970), Win 0x68a90b
void PreferenceControlSlider::OnValueChanged(SliderControl*, Real32, Real32)
{
    Store();
}

// Mac 0x29a6b0
void PreferenceControlSlider::OnFinalRelease()
{
    SliderControlObserver::OnFinalRelease();
    RenderControl::OnFinalRelease();
}

} // namespace LEGO
