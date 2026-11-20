// LEGO Digital Designer — restored source
// TU 258 "ShowStartHelpCommand.cpp": Mac 0x277200-0x27757b, Win 0x66a27e / 0x62a9ea / 0x62c194.
// Not code: the NotifierT<Command, …> OnFinalRelease instance (Mac 0x277310 and thunks),
// Mac 0x277500 = <iostream> + a header static.
#include "ShowStartHelpCommand.h"
#include "EditMode.h"

namespace LEGO {

// Mac 0x277220 (D1), 0x277290 (D0), Win 0x62a9ea (sdd 0x62c194)
ShowStartHelpCommand::~ShowStartHelpCommand()
{
}

// Mac 0x277200, Win 0x66a27e
void ShowStartHelpCommand::Do()
{
    m_editMode->ShowStartHelp();
}

// Mac 0x277360, Win 0x630465
void ShowStartHelpCommand::UpdateEnable()
{
    SetEnable(true);
}

} // namespace LEGO
