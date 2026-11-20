// LEGO Digital Designer — restored source
// LEGO::ButtonSoundAugmentation — plays a click sound when the mouse is pressed on its control,
// while preference "PlaySound" is on. Lua: LuaButtonSoundAugmentation (TU 307).
// TU 153 "ButtonSoundAugmentation.cpp": Mac 0x215ab0-0x21617b, Win 0x45be90-0x45bf6f (+ 0x515c97,
// 0x515cc3); the inline ctor (Win 0x45bd00 / Mac 0x2b5ed0) is emitted with TU 307.
//
// RTTI (Mac): vmi, bases ControlAugmentationControlObserver (+0x00), PreferencesObserver
//   (+0x2c, private), virtual base Object.
//   Primary vtable 0x72c8bc {GetControl 0x113da0, Attach 0x1638c0, Detach 0x163740, D1 0x215c20,
//   D0 0x215c80, OnFinalRelease 0x215cf0, OnMousePressed 0x215ab0, OnPreferenceChanged 0x215b40}
//   (the last two: Mac-only primary entries of the secondary-base overriders); ControlObserver-in
//   0x72c8e8 (slot 20 → thunk 0x215af0); PreferencesObserver-in 0x72c97c (slot 5 → thunk
//   0x215c00); Object-in 0x72c9ac {0x215f60, 0x215fc0, 0x216040, AddRef, Release}.
//   Layout Mac: CACO +0x00..+0x2c, PreferencesObserver +0x2c (subject +0x30), SimpleSound* +0x34,
//   bool +0x38, Object +0x3c (sizeof 0x44).
//   Win: vtable 0x9cdd6c {GetControl, Attach 0x45a400, Detach 0x45a430}, ControlObserver-in
//   0x9cdcec (slot 17 = 0x515c97), PreferencesObserver-in 0x9cdcd8 at +0x28 (slot 2 = 0x515cc3),
//   Object-in 0x9cdcc4 at +0x40 {sdd 0x45c590, OnFinalRelease 0x45c5a0}; sound +0x34, flag +0x38.
// Method and member names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Preferences.h"
#include "SimpleSound.h"
#include "ControlAugmentationControlObserver.h"

namespace LEGO {

class ButtonSoundAugmentation : public ControlAugmentationControlObserver,
                                private PreferencesObserver {
public:
    // Keeps `sound`, observes the Preferences and reads "PlaySound".
    // NOTE: m_playSound stays uninitialised when the preference is missing (original quirk;
    // the shipped preferences define PlaySound); fixed: true then, as almost any garbage byte
    // (low).
    // Inline: Mac 0x2b5ed0 (C2, emitted in TU 307), Win 0x45bd00
    explicit ButtonSoundAugmentation(SimpleSound* sound)
        : m_sound(sound)
    {
        PreferencesObserver::Attach(Preferences::GetInstance().Get());
        m_playSound = true;
        Preferences::GetInstance()->GetValue(String("PlaySound"), m_playSound);
    }
    virtual ~ButtonSoundAugmentation();

    // The augmentation's Attach / Detach (the PreferencesObserver ones are internal).
    using ControlAugmentationControlObserver::Attach;
    using ControlAugmentationControlObserver::Detach;                                     // Mac 0x215c20/0x215c80, Win 0x45be90 (sdd 0x45bf40)

    // `control` is the augmented one and sounds are on: volume 25, play. Mac 0x215ab0 (thunk
    // 0x215af0), Win 0x515c97
    virtual void OnMousePressed(Control* control, const Vector2& position, UInt32 button);

    bool IsPlaySoundEnabled() const { return m_playSound; }

protected:
    // PreferencesObserver::OnFinalRelease, then ControlAugmentationControlObserver's
    // (inlined). Mac 0x215cf0 (thunks 0x215e50, 0x215f40, 0x216040), Win 0x45c5a0
    virtual void OnFinalRelease();

private:
    // key == "PLAYSOUND": re-read the flag. Mac 0x215b40 (thunk 0x215c00), Win 0x515cc3
    virtual void OnPreferenceChanged(const String& key);

    RefPointer<SimpleSound> m_sound;       // Mac +0x34, Win +0x34
    bool                    m_playSound;   // Mac +0x38, Win +0x38
};

} // namespace LEGO
