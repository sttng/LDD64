// LEGO Digital Designer — restored source
// LEGO::VortexBrickPhysicsSimulationHingeSolver — the hinge solver for a hinge group with loops
// (VortexPhysicsScene::CreateHingeSimulateSolver): the group's joints become soft spring mirrors
// and the bodies move in the simulation while Solve steps the world.
// VortexBrickPhysicsSimulationHingeSolver.cpp (Mac TU 424, 0x39f590-0x3a06cf; Win 0x531d00-
// 0x5320ff); also emits VortexMoveTogetherHingeData's destructors (Mac 0x3a0610 / 0x3a0650, Win
// 0x521500 / 0x521560).

// RTTI: class LEGO::VortexBrickPhysicsSimulationHingeSolver : BaseBrickPhysicsSimulationHingeSolver
//   (+0), VortexMoveTogetherHingeData (+0x40). sizeof 0x5c.
//   Mac vtable 0x758278 {0x39f7f0, 0x39fe00, base Step 0x254430, D1 0x39f6e0, D0 0x39f730,
//     base 0x254440, 0x259440 .. 0x259480, 0x39ff60, base QI / AddRef / Release / GetCallback,
//     IsFor 0x2595f0, 0x39ff80} (+ 0x7582c8 Object thunks 0x39f6f0 / 0x39f770, 0x7582e4,
//     0x7582fc hinge-data thunks 0x39f710 / 0x39f7b0).
//   Win vtable 0xa2ac88 {0x531db0, 0x531f70, 0x4f9670, sdd 0x531d00, 0x5c5800, .., 0x5320a0,
//     0x520bb0, 0x5320b0}.
// Every name is inferred.
#pragma once
#include "LEGO/Types.h"
#include "BaseBrickPhysicsScene.h"
#include "VortexMoveTogetherHingeData.h"

namespace LEGO {

class VortexPhysicsScene;

class VortexBrickPhysicsSimulationHingeSolver : public BaseBrickPhysicsSimulationHingeSolver,
                                                public VortexMoveTogetherHingeData {
public:
    // Mac 0x39f590 (Win inlined into CreateHingeSimulateSolver 0x525340)
    VortexBrickPhysicsSimulationHingeSolver(VortexPhysicsScene* scene, BaseBrickPhysicsConnectionHelper* hinge,
                                            bool first, const BasicArray<ActiveRigidElement>& elements,
                                            const BasicArray<BaseBrickPhysicsConnectionHelper*>& connections);
    // Releases the snapshot. Mac D1 0x39f6e0 (body 0x39f620) / D0 0x39f730, Win 0x531d20 / sdd 0x531d00
    virtual ~VortexBrickPhysicsSimulationHingeSolver();

    // The moving bodies join the simulation (their children keep still, without their joints),
    // the group's joints become spring mirrors (strength row 1), a snapshot of all of them.
    // Mac 0x39f7f0, Win 0x531db0
    virtual void   Start();
    // Undoes Start (engine joints back, mirrors gone) and forgets the bodies; `result` unused.
    // Mac 0x39fe00, Win 0x531f70
    virtual void   Stop(bool& result);
    // The snapshot takes the restored transforms. Mac 0x39ff60, Win 0x5320a0
    virtual void   OnSnapshotRestored();
    // Steps the world (1/60 s, no collision response) with `detach` (the VortexHingeSolverHinge
    // turned to the target angle) until every joint holds and no contact is deeper than 0.0139
    // (true), a joint breaks or the time (1 s) / 1000 steps run out (false; the snapshot is put
    // back). `angle` / `axis` / `center` are not used here (the solver hinge carries the target).
    // Mac 0x39ff80, Win 0x5320b0
    virtual bool   Solve(Real32 angle, const Vector3& axis, const Vector3& center,
                         BaseBrickPhysicsConnectionHelper* detach);

    VortexPhysicsScene* m_vortexScene;                    // +0x54
    PhysicsSnapshot*    m_snapshot;                       // +0x58 Start .. Stop (one reference)

private:
    VortexBrickPhysicsSimulationHingeSolver(const VortexBrickPhysicsSimulationHingeSolver&);
    VortexBrickPhysicsSimulationHingeSolver& operator=(const VortexBrickPhysicsSimulationHingeSolver&);
};

} // namespace LEGO
