// LEGO Digital Designer — restored source
// LEGO::VortexMoveTogetherHingeData — the second base of the Vortex solvers that make solver
// hinges (VortexBrickPhysicsSimulationHingeSolver / VortexBrickPhysicsManualHingeSolver at +0x40,
// VortexMoveTogetherSolver at +0x18): the spring mirrors of the group's joints while the solver
// runs (VortexPhysicsScene::BuildConnectionSpringMirrors fills it; BuildHingeSolverHinge and the
// MoveTogether hinge keep a pointer to it).
// Header only; the inline destructor is emitted (weak) in VortexBrickPhysicsSimulationHingeSolver.cpp
// (Mac TU 424: D1 0x3a0610 / D0 0x3a0650; Win 0x521500 / sdd 0x521560).

// RTTI: class LEGO::VortexMoveTogetherHingeData (no base). Mac typeinfo 0x778fc0, vtable 0x778bd0
//   {D1 0x3a0610, D0 0x3a0650}; Win 0xa2a110 {sdd 0x521560}. sizeof 0x14. (names inferred)
#pragma once
#include "LEGO/Types.h"
#include "BasicArray.h"

namespace LEGO {

class BaseBrickPhysicsConnectionHelper;

class VortexMoveTogetherHingeData {
public:
    VortexMoveTogetherHingeData() {}
    virtual ~VortexMoveTogetherHingeData() {}             // Mac D1 0x3a0610 / D0 0x3a0650, Win 0x521500 / sdd 0x521560

    BasicArray<BaseBrickPhysicsConnectionHelper*> m_hinges;   // +0x04 the spring mirrors
};

} // namespace LEGO
