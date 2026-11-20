// LEGO Digital Designer — restored source
// LEGO::NextPartBICommand — one part forward (BGMode::NextPart, into substeps when the flag is
//   set); "nextPart", key 0xcd (Right). Enabled while the guide is not at its end and the mode does
//   not auto-play.
//
// TU 265 "NextPartBICommand.cpp": Mac 0x27b590-0x27ba7b, Win 0x65efb8-0x65f0ab. The TU also emits
// NotifierT<Command, CommandObserver> construction-vtable thunks; the static initialiser is
// <iostream> + a header static.
// RTTI: si {Command}, virtual base Object. Mac vtable 0x73be2c {D1 0x27b740, D0 0x27b7b0,
//   OnFinalRelease (NotifierT), Do 0x27b6b0, SetEnable, GetEnable, UpdateEnable 0x27b6e0}; Win
//   0x9d7288 {Do 0x65efb8, SetEnable, GetEnable, UpdateEnable 0x65f0ca} (Win UpdateEnable folded
//   with the other Next* commands').
// Layout (Mac): Command +0x00..+0x28, m_mode +0x28, m_substeps +0x2c, Object +0x30. Win: m_mode
// +0x2c.
// Names: class RTTI; members inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"

namespace LEGO {

class BGMode;

class NextPartBICommand : public Command {
public:
    // Disabled at first. Mac 0x27b590, Win 0x65efcb
    NextPartBICommand(BGMode* mode, bool substeps);
    virtual ~NextPartBICommand();                                                  // Mac 0x27b740/0x27b7b0, Win sdd 0x65f0ab

    virtual void Do();                                                      // 3/0 Mac 0x27b6b0, Win 0x65efb8
    virtual void UpdateEnable();                                            // 6/3 Mac 0x27b6e0, Win 0x65f0ca

    // The flag NextPart / PrevPart get (BGAutomaticMode::Enter sets it). Mac 0x27b6a0
    void SetSubsteps(bool substeps);

private:
    BGMode* m_mode;       // Mac +0x28, Win +0x2c
    bool    m_substeps;   // Mac +0x2c
};

} // namespace LEGO
