// LEGO Digital Designer — restored source
// LEGO::VortexMoveTogetherSolver — the solver of PhysicsScene::SolveMoveTogether (HingeAlignTool:
// two bodies pulled together until the given frames meet) and its interface
// LEGO::MoveTogetherPhysicsSolver. The bodies move in the simulation (damped), the joints of the
// group are spring mirrors, the VortexMoveTogetherHinge pulls and then locks.
// VortexMoveTogetherSolver.cpp (Mac TU 425, 0x3a06d0-0x3a1f3f; Win 0x52ccd0-0x52d73f).

// RTTI: class LEGO::VortexMoveTogetherSolver : BaseBrickPhysicsSimulationSolver (+0),
//   VortexMoveTogetherHingeData (+0x18), MoveTogetherPhysicsSolver (+0x2c). sizeof 0x64.
//   Mac vtable 0x758338 {0x3a09a0, 0x3a1320, 0x3a15d0, D1 0x3a1ce0, D0 0x3a1d30, 0x3a1df0,
//     0x259440 .. 0x259490, QI 0x3a0800, 0x3a1e10, 0x3a1e20, 0x3a1e30} + 0x758380 (Object:
//     0x3a1cf0 / 0x3a1d70), 0x75839c (PhysicsSolver: 0x3a0880, 0x3a1e40 ..), 0x7583b4 (hinge data:
//     0x3a1d10 / 0x3a1db0), 0x7583c4 (MoveTogetherPhysicsSolver: 0x3a0910, 0x3a1ea0 ..).
//   Win vtable 0xa2a6cc {0x52ce70, 0x52d0c0, 0x52d2b0, sdd 0x52cdb0, 0x52ccd0, ..} + 0xa2a6fc,
//     0xa2a710 (QI 0x52cdd0), 0xa2a724, 0xa2a72c (thunks 0x52d710 / 0x52d6e0 / 0x52d730 / 0x52d6f0).
// RTTI: class LEGO::MoveTogetherPhysicsSolver : PhysicsSolver (no slots of its own: the Mac
//   secondary vtable 0x7583c4 has QI / AddRef / Release / GetCallback; Win 0xa2a6b8 4 pure).
// Every name is inferred.
#pragma once
#include "LEGO/Types.h"
#include "BasicArray.h"
#include "Object.h"
#include "BaseBrickPhysicsScene.h"
#include "VortexMoveTogetherHingeData.h"

namespace LEGO {

class VortexMoveTogetherHinge;
class VortexPhysicsScene;

class MoveTogetherPhysicsSolver : public PhysicsSolver {
public:
    // QueryInterface id (VortexMoveTogetherSolver Mac 0x3a0800: function-local static Mac 0x778be8)
    static const InterfaceID& GetIID()
    {
        static const InterfaceID id = {{0xf1548a86, 0x4b4fc067, 0x717fefa8, 0xcaa1f07b}};
        return id;
    }
};

class VortexMoveTogetherSolver : public BaseBrickPhysicsSimulationSolver,
                                 public VortexMoveTogetherHingeData,
                                 public MoveTogetherPhysicsSolver {
public:
    // `callback` (the MoveTogetherSolverContext) is referenced. Mac 0x3a06d0 (C1 0x3a07f0), Win
    // inlined into SolveMoveTogether 0x524410
    VortexMoveTogetherSolver(VortexPhysicsScene* scene, PhysicsSolverCallback* callback);
    // Releases the snapshot and the hinge. Mac D1 0x3a1ce0 (body 0x3a1a60) / D0 0x3a1d30,
    // Win 0x52d5d0 / sdd 0x52cdb0
    virtual ~VortexMoveTogetherSolver();
    // The bodies join the simulation (linear damping 5), the joints of their children join the
    // group, the group becomes spring mirrors (row 0, the hinge weak), auto-disable on, the
    // hinge starts, a snapshot of the bodies. Mac 0x3a09a0, Win 0x52ce70
    virtual void   Start();
    // `result` cleared when a moved body collides; the hinge stops, mirrors / engine joints
    // back, the bodies leave the simulation, the snapshot is put back on failure. Mac 0x3a1320,
    // Win 0x52d0c0
    virtual void   Stop(bool& result);
    // One world step of 1/60 s: 1 done (frames met, joints hold, contacts shallow, at rest after
    // two steps), 2 failed (a joint broke, or 50 steps at rest without meeting), else 0.
    // Mac 0x3a15d0, Win 0x52d2b0
    virtual UInt32 Step(Real32 time);
    // A scene update stops the solver. Mac 0x3a1df0, Win 0x52ccd0
    virtual void   OnBeginUpdate();
    // Answers MoveTogetherPhysicsSolver::GetIID(), then the base's. Mac 0x3a0800 (thunks
    // 0x3a0880 / 0x3a0910), Win 0x52cdd0
    virtual Int32  LEGO_COM_CALL QueryInterface(const InterfaceID& id, void** object);
    virtual UInt32 LEGO_COM_CALL AddRef();                // Mac 0x3a1e10 (the base's)
    virtual UInt32 LEGO_COM_CALL Release();               // Mac 0x3a1e20 (the base's)
    virtual PhysicsSolverCallback* GetCallback();         // Mac 0x3a1e30 (the base's)
    using Object::AddRef;
    using Object::Release;

    VortexPhysicsScene*                           m_vortexScene;   // +0x30
    BasicArray<BaseBrickPhysicsConnectionHelper*> m_connections;   // +0x34 the hinge group (SolveMoveTogether)
    BasicArray<ActiveRigidElement>                m_elements;      // +0x44 the bodies that move
    RefPointer<VortexMoveTogetherHinge>           m_hinge;         // +0x54 the temporary joint
    PhysicsSnapshot*                              m_snapshot;      // +0x58 Start .. Stop (0 at construction)
    Int32                                         m_stepCount;     // +0x5c steps since Start
    Int32                                         m_restCount;     // +0x60 steps at rest without meeting

private:
    VortexMoveTogetherSolver(const VortexMoveTogetherSolver&);
    VortexMoveTogetherSolver& operator=(const VortexMoveTogetherSolver&);
};

} // namespace LEGO
