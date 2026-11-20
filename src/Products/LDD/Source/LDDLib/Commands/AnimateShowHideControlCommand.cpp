// LEGO Digital Designer — restored source
// TU 290 "AnimateShowHideControlCommand.cpp": Mac 0x29e950-0x29ef1b, Win 0x66d8d0-0x66da92.
// Not code: construction-vtable thunks, Mac 0x29eea0 = <iostream> + a header static.
#include "AnimateShowHideControlCommand.h"
#include "Control.h"
#include "ControlAugmentation.h"
#include "Preferences.h"
#include "StringStream.h"

namespace LEGO {

// Mac 0x29e950, Win 0x66d906
AnimateShowHideControlCommand::AnimateShowHideControlCommand()
    : Command(true), m_augmentation(0), m_duration(500)
{
    RefPointer<Preferences> preferences = Preferences::GetInstance();
    String value;
    if (preferences->GetValue(String("ShowHideSlideDuration"), value))
        FromString(m_duration, value);
}

// Mac 0x29ecf0 (D1, body 0x29ec10), 0x29edb0 (D0), Win 0x66d9e7 (sdd 0x66da92)
AnimateShowHideControlCommand::~AnimateShowHideControlCommand()
{
}

// Mac 0x29eb30 (via the GetAugmentation<T> instance 0x29b120 of TU 287, Win 0x66d4b5)
bool AnimateShowHideControlCommand::SetControl(Control* control)
{
    RefPointer<ControlAugmentation> augmentation;
    control->GetAugmentation(String("SlidingHideAugmentation"), augmentation);
    m_augmentation = augmentation ? dynamic_cast<SlidingHideAugmentation*>(augmentation.Get()) : 0;
    return !m_augmentation.IsNull();
}

// Mac 0x29ebb0, Win 0x66d8d0
void AnimateShowHideControlCommand::Do()
{
    m_augmentation->Start(m_duration);
}

// Mac 0x29ebd0, Win 0x66d8e1
void AnimateShowHideControlCommand::SetEnable(bool)
{
    Command::SetEnable(m_augmentation && !m_augmentation->IsSliding());
}

} // namespace LEGO
