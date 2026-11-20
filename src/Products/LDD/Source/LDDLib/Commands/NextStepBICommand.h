// LEGO Digital Designer — restored source
// LEGO::NextStepBICommand — one step of the building guide forward (BGMode::NextStep: animated,
//   camera of the step not taken); "nextStep" of LDD.commands.bgAutomaticMode, key 0xd1 (Page
//   Down). Enabled while the guide is not at its end and the mode does not auto-play.
//
// TU 244 "NextStepBICommand.cpp": Mac 0x2724b0-0x2729eb, Win 0x65f0ca-0x65f230. The TU also emits
// NotifierT<Command, CommandObserver> construction-vtable thunks; the static initialiser is
// <iostream> + a header static.
// RTTI: si {Command}, virtual base Object. Mac vtable 0x73952c {D1 0x2726b0, D0 0x272720,
//   OnFinalRelease (NotifierT), Do 0x2725b0, SetEnable, GetEnable, UpdateEnable 0x272650}; Win
//   0x9d72d0 {Do 0x65f105, SetEnable, GetEnable, UpdateEnable 0x65f0ca} (Win UpdateEnable folded
//   with the other Next* commands').
// Layout (Mac): Command +0x00..+0x28, m_mode +0x28, Object +0x2c. Win: m_mode +0x2c.
// Names: class RTTI; members inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"

namespace LEGO {

class BGMode;

class NextStepBICommand : public Command {
public:
    // Disabled at first. Mac 0x2724b0, Win 0x65f156
    explicit NextStepBICommand(BGMode* mode);
    virtual ~NextStepBICommand();                                                  // Mac 0x2726b0/0x272720, Win sdd 0x65f230

    virtual void Do();                                                      // 3/0 Mac 0x2725b0, Win 0x65f105
    virtual void UpdateEnable();                                            // 6/3 Mac 0x272650, Win 0x65f0ca

private:
    BGMode* m_mode;       // Mac +0x28, Win +0x2c
};

} // namespace LEGO
