// LEGO Digital Designer — restored source
// LEGO::AnimateShowHideControlCommand — slides a control in or out through its
//   "SlidingHideAugmentation" (preference ShowHideSlideDuration, 500 ms by default); disabled
//   while the slide runs.
//
// TU 290 "AnimateShowHideControlCommand.cpp" (Mac 0x29e950-0x29ef1b; Win 0x66d8d0-0x66da92).
// Mac layout (constructor 0x29e950): Command +0x00..+0x28, augmentation +0x28, duration
// (UInt64 ms) +0x2c, Object +0x34 (sizeof 0x3c). Win (ctor 0x66d906): augmentation +0x2c,
// duration +0x30, Object +0x3c.
// Vtables: Mac 0x7424fc {D1 0x29ecf0, D0 0x29edb0, OnFinalRelease 0x1414d0, Do 0x29ebb0,
// SetEnable 0x29ebd0, GetEnable, UpdateEnable 0x3730c0}; Win 0x9d83d0 {Do 0x66d8d0, SetEnable
// 0x66d8e1, …}.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"
#include "SlidingHideAugmentation.h"

namespace LEGO {

class Control;

class AnimateShowHideControlCommand : public Command {
public:
    // Enabled; no augmentation yet; duration 500 or the preference. Mac 0x29e950, Win 0x66d906
    AnimateShowHideControlCommand();
    virtual ~AnimateShowHideControlCommand();                               // Mac 0x29ecf0/0x29edb0, Win 0x66d9e7 (sdd 0x66da92)

    // The control to slide: its "SlidingHideAugmentation" (null when it has none); true when
    // it has one. Mac 0x29eb30, Win 0x66da4b (name inferred)
    bool SetControl(Control* control);

    // SlidingHideAugmentation::Start(duration). NOTE: no null check (original).
    virtual void Do();                                                      // Mac 0x29ebb0, Win 0x66d8d0
    // NOTE: ignores `enable`: enabled = there is an augmentation that is not sliding.
    virtual void SetEnable(bool enable);                                    // Mac 0x29ebd0, Win 0x66d8e1

protected:
    RefPointer<SlidingHideAugmentation> m_augmentation;   // Mac +0x28, Win +0x2c
    UInt64                              m_duration;       // Mac +0x2c, Win +0x30 (milliseconds)
};

} // namespace LEGO
