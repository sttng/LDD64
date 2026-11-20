// LEGO Digital Designer — restored source
// LEGO::PrevPartBICommand — one part back (BGMode::PrevPart); "prevPart", key 0xcb (Left).
//   Enabled while the guide is not at its start and the mode does not auto-play.
//
// TU 264 "PrevPartBICommand.cpp": Mac 0x27b0a0-0x27b58b, Win 0x65ed58-0x65ee4b. The TU also emits
// NotifierT<Command, CommandObserver> construction-vtable thunks; the static initialiser is
// <iostream> + a header static.
// RTTI: si {Command}, virtual base Object. Mac vtable 0x73bd6c {D1 0x27b250, D0 0x27b2c0,
//   OnFinalRelease (NotifierT), Do 0x27b1c0, SetEnable, GetEnable, UpdateEnable 0x27b1f0}; Win
//   0x9d7208 {Do 0x65ed58, SetEnable, GetEnable, UpdateEnable 0x65ee7a} (Win UpdateEnable folded
//   with the other Prev* commands').
// Layout (Mac): Command +0x00..+0x28, m_mode +0x28, m_substeps +0x2c, Object +0x30. Win: m_mode
// +0x2c.
// Names: class RTTI; members inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"

namespace LEGO {

class BGMode;

class PrevPartBICommand : public Command {
public:
    // Disabled at first. Mac 0x27b0a0, Win 0x65ed6b
    PrevPartBICommand(BGMode* mode, bool substeps);
    virtual ~PrevPartBICommand();                                                  // Mac 0x27b250/0x27b2c0, Win sdd 0x65ee4b

    virtual void Do();                                                      // 3/0 Mac 0x27b1c0, Win 0x65ed58
    virtual void UpdateEnable();                                            // 6/3 Mac 0x27b1f0, Win 0x65ee7a

    // The flag NextPart / PrevPart get (BGAutomaticMode::Enter sets it). Mac 0x27b1b0
    void SetSubsteps(bool substeps);

private:
    BGMode* m_mode;       // Mac +0x28, Win +0x2c
    bool    m_substeps;   // Mac +0x2c
};

} // namespace LEGO
