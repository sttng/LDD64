// LEGO Digital Designer — restored source
// TU 265 "NextPartBICommand.cpp": Mac 0x27b590-0x27ba7b, Win 0x65efb8-0x65f0ab.
#include "NextPartBICommand.h"

#include <iostream>

#include "BGMode.h"
#include "BGApplication.h"

namespace LEGO {

// Mac 0x27b590, Win 0x65efcb
NextPartBICommand::NextPartBICommand(BGMode* mode, bool substeps)
    : Command(true, false), m_mode(mode)
{
    SetEnable(false);
    m_substeps = substeps;
}

// Mac 0x27b740 (D1) / 0x27b7b0 (D0), Win sdd 0x65f0ab
NextPartBICommand::~NextPartBICommand()
{
}

// Mac 0x27b6b0, Win 0x65efb8
void NextPartBICommand::Do()
{
    m_mode->NextPart(m_substeps);
}

// Mac 0x27b6e0, Win 0x65f0ca
void NextPartBICommand::UpdateEnable()
{
    SetEnable(!m_mode->GetApplication()->IsAtEnd() && !m_mode->IsPlaying());
}

// Mac 0x27b6a0
void NextPartBICommand::SetSubsteps(bool substeps)
{
    m_substeps = substeps;
}

} // namespace LEGO
