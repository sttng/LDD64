// LEGO Digital Designer — restored source
// LEGO::OpenCommand — opens a model ("open" in LDD.commands.editMode): Application slot 11
//   (LDDApplication: the open panel, Mac 0x1742e0).
//
// TU 245 "OpenCommand.cpp": Mac 0x2729f0-0x272d2f, Win 0x66a239 / 0x62c156 / 0x62c712. The
// constructor is inline (Mac: inlined in EditMode::Initialize; Win copy 0x62a786). Mac 0x272cf0
// = <iostream> + a header static (not code).
// RTTI: OpenCommand : Command. Mac sizeof 0x28 + Object, vtable 0x7395ec {D1 0x272a10, D0
//   0x272a80, OnFinalRelease 0x272b00, Do 0x2729f0, SetEnable, GetEnable, UpdateEnable 0x272b50};
//   Win 0x9d1518 {Do 0x66a239, …, UpdateEnable 0x630465}, Object-in 0x9d1504 {sdd 0x62c156,
//   OnFinalRelease 0x62c712}.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"

namespace LEGO {

class OpenCommand : public Command {
public:
    OpenCommand() : Command(true) {}                                        // Win 0x62a786
    virtual ~OpenCommand();                                                 // Mac 0x272a10/0x272a80, Win sdd 0x62c156

    // Application::NewDocument (slot 11: LDD's open panel).
    virtual void Do();                                                      // 3/0 Mac 0x2729f0, Win 0x66a239
    // Always enabled. (own copy of Command's)
    virtual void UpdateEnable();                                            // 6/3 Mac 0x272b50, Win 0x630465

protected:
    // NotifierT<Command>'s (own copy of the inherited body). Mac 0x272b00, Win 0x62c712
    virtual void OnFinalRelease();
};

} // namespace LEGO
