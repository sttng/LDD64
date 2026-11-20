// LEGO Digital Designer — restored source
// LEGO::SaveAsCommand — saves the model under a new name ("saveAs" in LDD.commands.editMode):
//   Application::SaveDocumentAs.
//
// TU 254 "SaveAsCommand.cpp": Mac 0x2752e0-0x27556f, Win 0x66a286 / 0x62c1b3. The constructor is
// inline (Mac: inlined in EditMode::Initialize; Win copy 0x62a9fd). Mac 0x275530 = <iostream> +
// a header static (not code).
// RTTI: SaveAsCommand : Command. Mac sizeof 0x28 + Object, vtable 0x73a30c {D1 0x275300, D0
//   0x275370, OnFinalRelease (NotifierT's), Do 0x2752e0, SetEnable, GetEnable, UpdateEnable
//   0x2753f0}; Win 0x9d1590 {Do 0x66a286, …, UpdateEnable 0x630465}, Object-in 0x9d157c {sdd
//   0x62c1b3}.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"

namespace LEGO {

class SaveAsCommand : public Command {
public:
    SaveAsCommand() : Command(true) {}                                      // Win 0x62a9fd
    virtual ~SaveAsCommand();                                               // Mac 0x275300/0x275370, Win sdd 0x62c1b3

    // Application::SaveDocumentAs (slot 14).
    virtual void Do();                                                      // 3/0 Mac 0x2752e0, Win 0x66a286
    // Always enabled. (own copy of Command's)
    virtual void UpdateEnable();                                            // 6/3 Mac 0x2753f0, Win 0x630465
};

} // namespace LEGO
