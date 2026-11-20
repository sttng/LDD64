// LEGO Digital Designer — restored source
// TU 264 "PrevPartBICommand.cpp": Mac 0x27b0a0-0x27b58b, Win 0x65ed58-0x65ee4b.
#include "PrevPartBICommand.h"

#include <iostream>

#include "BGMode.h"
#include "BGApplication.h"

namespace LEGO {

// Mac 0x27b0a0, Win 0x65ed6b
PrevPartBICommand::PrevPartBICommand(BGMode* mode, bool substeps)
    : Command(true, false), m_mode(mode), m_substeps(substeps)
{
    SetEnable(false);
}

// Mac 0x27b250 (D1) / 0x27b2c0 (D0), Win sdd 0x65ee4b
PrevPartBICommand::~PrevPartBICommand()
{
}

// Mac 0x27b1c0, Win 0x65ed58
void PrevPartBICommand::Do()
{
    m_mode->PrevPart(m_substeps);
}

// Mac 0x27b1f0, Win 0x65ee7a
void PrevPartBICommand::UpdateEnable()
{
    SetEnable(!m_mode->GetApplication()->IsAtStart() && !m_mode->IsPlaying());
}

// Mac 0x27b1b0
void PrevPartBICommand::SetSubsteps(bool substeps)
{
    m_substeps = substeps;
}

} // namespace LEGO
