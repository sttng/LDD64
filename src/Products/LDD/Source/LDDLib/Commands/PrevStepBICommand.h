// LEGO Digital Designer — restored source
// LEGO::PrevStepBICommand — one step back (BGMode::PrevStep, animated); "prevStep", key 0xc9 (Page
//   Up). Enabled while the guide is not at its start and the mode does not auto-play.
//
// TU 250 "PrevStepBICommand.cpp": Mac 0x274550-0x274a1b, Win 0x65ee6a-0x65ef8f. The TU also emits
// NotifierT<Command, CommandObserver> construction-vtable thunks; the static initialiser is
// <iostream> + a header static.
// RTTI: si {Command}, virtual base Object. Mac vtable 0x739f2c {D1 0x2746e0, D0 0x274750,
//   OnFinalRelease (NotifierT), Do 0x274650, SetEnable, GetEnable, UpdateEnable 0x274680}; Win
//   0x9d7248 {Do 0x65ee6a, SetEnable, GetEnable, UpdateEnable 0x65ee7a} (Win UpdateEnable folded
//   with the other Prev* commands').
// Layout (Mac): Command +0x00..+0x28, m_mode +0x28, Object +0x2c. Win: m_mode +0x2c.
// Names: class RTTI; members inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"

namespace LEGO {

class BGMode;

class PrevStepBICommand : public Command {
public:
    // Disabled at first. Mac 0x274550, Win 0x65eeb5
    explicit PrevStepBICommand(BGMode* mode);
    virtual ~PrevStepBICommand();                                                  // Mac 0x2746e0/0x274750, Win sdd 0x65ef8f

    virtual void Do();                                                      // 3/0 Mac 0x274650, Win 0x65ee6a
    virtual void UpdateEnable();                                            // 6/3 Mac 0x274680, Win 0x65ee7a

private:
    BGMode* m_mode;       // Mac +0x28, Win +0x2c
};

} // namespace LEGO
