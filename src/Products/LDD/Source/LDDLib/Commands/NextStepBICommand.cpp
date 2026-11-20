// LEGO Digital Designer — restored source
// TU 244 "NextStepBICommand.cpp": Mac 0x2724b0-0x2729eb, Win 0x65f0ca-0x65f230.
#include "NextStepBICommand.h"

#include <iostream>

#include "BGMode.h"
#include "BGApplication.h"

namespace LEGO {

// Mac 0x2724b0, Win 0x65f156
NextStepBICommand::NextStepBICommand(BGMode* mode)
    : Command(true, false), m_mode(mode)
{
    SetEnable(false);
}

// Mac 0x2726b0 (D1) / 0x272720 (D0), Win sdd 0x65f230
NextStepBICommand::~NextStepBICommand()
{
}

// Mac 0x2725b0, Win 0x65f105
void NextStepBICommand::Do()
{
    BasicArray<BIBrick*> bricks;
    m_mode->NextStep(true, true, bricks, false);
}

// Mac 0x272650, Win 0x65f0ca
void NextStepBICommand::UpdateEnable()
{
    SetEnable(!m_mode->GetApplication()->IsAtEnd() && !m_mode->IsPlaying());
}

} // namespace LEGO
