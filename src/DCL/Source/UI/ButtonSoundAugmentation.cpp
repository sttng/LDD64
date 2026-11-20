// LEGO Digital Designer — restored source
// TU 153 "ButtonSoundAugmentation.cpp".
#include "ButtonSoundAugmentation.h"

namespace LEGO {

// Mac 0x215c20 / 0x215c80 (body 0x216060), Win 0x45be90 (sdd 0x45bf40)
ButtonSoundAugmentation::~ButtonSoundAugmentation()
{
}

// Mac 0x215ab0, Win 0x515c97
void ButtonSoundAugmentation::OnMousePressed(Control* control, const Vector2& /*position*/,
                                             UInt32 /*button*/)
{
    if (GetControl() == control && m_playSound) {
        m_sound->SetVolume(25);
        m_sound->Play();
    }
}

// Mac 0x215b40, Win 0x515cc3
void ButtonSoundAugmentation::OnPreferenceChanged(const String& key)
{
    if (key == String("PLAYSOUND")) {
        // Mac: GetValue(key, String) + FromString (the inlined bool GetValue), Win 0x41c700
        PreferencesObserver::GetSubject()->GetValue(key, m_playSound);
    }
}

// Mac 0x215cf0, Win 0x45c5a0
void ButtonSoundAugmentation::OnFinalRelease()
{
    PreferencesObserver::OnFinalRelease();
    ControlAugmentationControlObserver::OnFinalRelease();
}

} // namespace LEGO
