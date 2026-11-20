// LEGO Digital Designer — restored source
// TU 250 "PrevStepBICommand.cpp": Mac 0x274550-0x274a1b, Win 0x65ee6a-0x65ef8f.
#include "PrevStepBICommand.h"

#include <iostream>

#include "BGMode.h"
#include "BGApplication.h"

namespace LEGO {

// Mac 0x274550, Win 0x65eeb5
PrevStepBICommand::PrevStepBICommand(BGMode* mode)
    : Command(true, false), m_mode(mode)
{
    SetEnable(false);
}

// Mac 0x2746e0 (D1) / 0x274750 (D0), Win sdd 0x65ef8f
PrevStepBICommand::~PrevStepBICommand()
{
}

// Mac 0x274650, Win 0x65ee6a
void PrevStepBICommand::Do()
{
    m_mode->PrevStep(true, true);
}

// Mac 0x274680, Win 0x65ee7a
void PrevStepBICommand::UpdateEnable()
{
    SetEnable(!m_mode->GetApplication()->IsAtStart() && !m_mode->IsPlaying());
}

} // namespace LEGO
