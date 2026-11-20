// LEGO Digital Designer — restored source
// LEGO::ReplayPartBICommand — shows the last part again: BGMode::PrevPart then NextPart (both
//   without substeps); "replayPart" of LDD.commands.bgAutomaticMode, key 0x39 (Space). Enabled
//   while the guide is not at its start and the mode does not auto-play.
//
// TU 263 "ReplayPartBICommand.cpp": Mac 0x27abc0-0x27b09b, Win 0x65ec38-0x65ed31. The TU also
// emits NotifierT<Command, CommandObserver> construction-vtable thunks; the static initialiser is
// <iostream> + a header static.
// RTTI: si {Command}, virtual base Object. Mac vtable 0x73bcac {D1 0x27ad60, D0 0x27add0,
//   OnFinalRelease (NotifierT), Do 0x27acc0, SetEnable, GetEnable, UpdateEnable 0x27ad00};
//   Win 0x9d71c8 {Do 0x65ec38, SetEnable, GetEnable, UpdateEnable 0x65ee7a (folded with the
//   Prev* commands'; the own copy 0x65ecdb is unreferenced)}.
// Layout (Mac): Command +0x00..+0x28, m_mode +0x28, Object +0x2c. Win: m_mode +0x2c.
// Names: class RTTI; members inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"

namespace LEGO {

class BGMode;

class ReplayPartBICommand : public Command {
public:
    // Disabled at first. Mac 0x27abc0, Win 0x65ec57
    explicit ReplayPartBICommand(BGMode* mode);
    virtual ~ReplayPartBICommand();                                         // Mac 0x27ad60/0x27add0, Win sdd 0x65ed31

    virtual void Do();                                                      // 3/0 Mac 0x27acc0, Win 0x65ec38
    virtual void UpdateEnable();                                            // 6/3 Mac 0x27ad00, Win 0x65ecdb

private:
    BGMode* m_mode;       // Mac +0x28, Win +0x2c
};

} // namespace LEGO
