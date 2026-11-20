// LEGO Digital Designer — restored source
// Inline pieces the four Vortex joints (Mac TUs 426-429) share: the bodies' VxParts, the scene's
// VxMechanism and the hinge-solver plumbing of Rotate. (file and function names
// inferred - the originals inline these expressions in every joint.)
#pragma once
#include "LEGO/Types.h"
#include "BaseBrickPhysicsConnectionHelper.h"
#include "BaseBrickPhysicsRigidElement.h"
#include "BaseBrickPhysicsScene.h"
#include "VortexPhysicsScene.h"
#include "VortexPhysicsRigidElement.h"
#include "VortexMoveTogetherHingeData.h"

namespace LEGO {

// "*(body + 0xd4)" (Mac 0x3a2520, Win 0x52c660)
inline Vx::VxPart* GetVortexPart(BaseBrickPhysicsRigidElement* body)
{
    return static_cast<VortexPhysicsRigidElement*>(body)->m_part;
}

// The scene of the joint's first body ("*(*(joint + 0x10) + 0xc)"): every joint reaches the
// mechanism and the hinge solvers through it.
inline VortexPhysicsScene* GetVortexScene(const BaseBrickPhysicsConnectionHelper* helper)
{
    return static_cast<VortexPhysicsScene*>(helper->m_body->m_scene);
}

// The hinge array of a Vortex hinge solver (dynamic_cast to its VortexMoveTogetherHingeData
// base, Mac typeinfo 0x778fc0, then +4). NOTE: like the original, a solver that is not a Vortex
// one gives the address 4 (the cast result 0 plus the member offset), not NULL.
inline BasicArray<BaseBrickPhysicsConnectionHelper*>* GetSolverHinges(BaseBrickPhysicsBaseHingeSolver* solver)
{
    VortexMoveTogetherHingeData* data = solver != 0 ? dynamic_cast<VortexMoveTogetherHingeData*>(solver) : 0;
    if (data == 0)
        return reinterpret_cast<BasicArray<BaseBrickPhysicsConnectionHelper*>*>(
            static_cast<size_t>(sizeof(void*)));
    return &data->m_hinges;
}

} // namespace LEGO
