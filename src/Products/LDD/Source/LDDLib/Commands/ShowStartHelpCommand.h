// LEGO Digital Designer — restored source
// LEGO::ShowStartHelpCommand — shows the edit mode's start help (the StartHelp tabs).
//
// TU 258 "ShowStartHelpCommand.cpp" (Mac 0x277200-0x27757b; Win 0x66a27e, 0x62a9ea/0x62c194).
// The constructor is header-inline (not in the TU; signature inferred).
// Mac layout: Command +0x00..+0x28, edit mode +0x28 (not referenced), Object +0x2c (sizeof
// 0x34). Win: edit mode +0x2c, Object +0x34.
// Vtables: Mac 0x73a94c {D1 0x277220, D0 0x277290, OnFinalRelease 0x277310 (NotifierT body), Do
// 0x277200, SetEnable, GetEnable, UpdateEnable 0x277360}; Win 0x9d1568 {Do 0x66a27e, …, 0x630465}.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"

namespace LEGO {

class EditMode;

class ShowStartHelpCommand : public Command {
public:
    explicit ShowStartHelpCommand(EditMode* editMode) : Command(true), m_editMode(editMode) {}
    virtual ~ShowStartHelpCommand();                                        // Mac 0x277220/0x277290, Win 0x62a9ea (sdd 0x62c194)

    virtual void Do();                                                      // Mac 0x277200, Win 0x66a27e
    virtual void UpdateEnable();                                            // Mac 0x277360 (SetEnable(true)), Win 0x630465

protected:
    EditMode* m_editMode;   // Mac +0x28, Win +0x2c
};

} // namespace LEGO
