// LEGO Digital Designer — restored source
// TU 361 "BGSubstepFilterMinifig.cpp" (Mac 0x344740-0x344d5f): LEGO::BGSubstepFilterMinifig.
#include "BGSubstepFilterMinifig.h"
#include "BIBrickSubstep.h"
#include "BIBrickRepository.h"

namespace LEGO {

// Mac 0x344740
BGSubstepFilterMinifig::BGSubstepFilterMinifig()
{
}

// Mac 0x344b00 / 0x344c20 (thunks 0x344b80 / 0x344cb0), Win sdd 0x5a1ab0
BGSubstepFilterMinifig::~BGSubstepFilterMinifig()
{
}

// Mac 0x3447a0, Win 0x5a1b20
bool BGSubstepFilterMinifig::Accept(const BIBrick* substep)
{
    // BISubstepFilterProgram::Apply only passes substeps.
    if (static_cast<const BIBrickSubstep*>(substep)->GetAttribute(1))
        return true;
    for (BasicArray<BIBrickSubstep*>::Iterator it = m_minifigs.Begin(); it != m_minifigs.End(); ++it) {
        if (substep->Intersects(static_cast<const BIBrick*>(*it)))
            return false;
    }
    return true;
}

// Mac 0x344810, Win 0x5a1bc0
void BGSubstepFilterMinifig::Begin()
{
    BasicArray<BIBrickSubstep*> substeps;
    BIBrickRepository::GetInstance().GetFreeSubsteps(substeps);
    for (BasicArray<BIBrickSubstep*>::Iterator it = substeps.Begin(); it != substeps.End(); ++it) {
        if ((*it)->GetAttribute(1))
            m_minifigs.PushBack(*it);
    }
}

// Mac 0x344cd0 (thunk 0x344cf0), Win 0x5a1d60
void BGSubstepFilterMinifig::OnFinalRelease()
{
    Object::OnFinalRelease();
}

} // namespace LEGO
