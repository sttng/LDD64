// LEGO Digital Designer — restored source
// TU 263 "ReplayPartBICommand.cpp": Mac 0x27abc0-0x27b09b, Win 0x65ec38-0x65ed31.
#include "ReplayPartBICommand.h"

#include <iostream>

#include "BGMode.h"
#include "BGApplication.h"

namespace LEGO {

// Mac 0x27abc0, Win 0x65ec57
ReplayPartBICommand::ReplayPartBICommand(BGMode* mode)
    : Command(true, false), m_mode(mode)
{
    SetEnable(false);
}

// Mac 0x27ad60 (D1) / 0x27add0 (D0), Win 0x65ed1e / sdd 0x65ed31
ReplayPartBICommand::~ReplayPartBICommand()
{
}

// Mac 0x27acc0, Win 0x65ec38
void ReplayPartBICommand::Do()
{
    m_mode->PrevPart(false);
    m_mode->NextPart(false);
}

// Mac 0x27ad00, Win 0x65ecdb
void ReplayPartBICommand::UpdateEnable()
{
    SetEnable(!m_mode->GetApplication()->IsAtStart() && !m_mode->IsPlaying());
}

} // namespace LEGO
