// LEGO Digital Designer — restored source
// LEGO::VortexMoveTogetherSolver (Mac TU 425, Win 0x52ccd0-0x52d73f). See VortexMoveTogetherSolver.h.
#include "VortexMoveTogetherSolver.h"
#include "VortexPhysicsScene.h"
#include "VortexPhysicsRigidElement.h"
#include "VortexAnchorSolver.h"
#include "BaseBrickPhysicsConnectionHelper.h"

#include "Vx/VxUniverse.h"
#include "Vx/VxPart.h"
#include "Vx/VxMessage.h"

#include <string.h>

namespace LEGO {

namespace {

const Real32 kStepTime = 1.0f / 60.0f;      // 0x3c888889
const double kStartDamping = 5.0;           // linear velocity damping while moving (0x40140000 00000000)
const double kDampingDecay = 0.95;          // Mac __const 0x63f640 (attached: per step)
const double kMinDamping = 0.01;            // Mac __const 0x63f648
const Int32  kMaxRestSteps = 50;

inline Vx::VxPart* PartOf(BaseBrickPhysicsRigidElement* body)
{
    return static_cast<VortexPhysicsRigidElement*>(body)->m_part;
}

// The joint tolerances (inline constants, the same as Mac __const 0x63f310)
inline PhysicsSeparationLimits StepLimits()
{
    PhysicsSeparationLimits limits;
    limits.m_distance = 0.0009f;
    limits.m_maxDistance = 0.0625f;
    limits.m_alignment = 0.9999f;
    limits.m_minAlignment = 0.9848f;
    limits.m_breakCount = 0;
    return limits;
}

// maxsd: the second operand unless the first is greater
inline double MaxSd(double a, double b)
{
    return a > b ? a : b;
}

inline void DecayDamping(Vx::VxPart* part)
{
    part->setLinearVelocityDamping(MaxSd(part->getLinearVelocityDamping() * kDampingDecay, kMinDamping));
}

} // namespace

// Mac 0x3a06d0 (C1 0x3a07f0), Win inlined
VortexMoveTogetherSolver::VortexMoveTogetherSolver(VortexPhysicsScene* scene, PhysicsSolverCallback* callback)
    : BaseBrickPhysicsSimulationSolver(scene, callback), m_vortexScene(scene), m_snapshot(0)
{
    // NOTE: +0x5c / +0x60 are first set by Start (zero here)
    m_stepCount = 0;
    m_restCount = 0;
}

// Mac D1 0x3a1ce0 (body 0x3a1a60) / D0 0x3a1d30, Win 0x52d5d0 / sdd 0x52cdb0
VortexMoveTogetherSolver::~VortexMoveTogetherSolver()
{
    if (m_snapshot != 0)
        m_snapshot->Release();
}

// Mac 0x3a0800 (thunks 0x3a0880 / 0x3a0910), Win 0x52cdd0
Int32 VortexMoveTogetherSolver::QueryInterface(const InterfaceID& id, void** object)
{
    if (memcmp(&id, &MoveTogetherPhysicsSolver::GetIID(), sizeof id) == 0) {
        *object = static_cast<MoveTogetherPhysicsSolver*>(this);
        AddRef();
        return 0;
    }
    return BaseBrickPhysicsSimulationSolver::QueryInterface(id, object);
}

// Mac 0x3a1e10 (thunks 0x3a1e40 / 0x3a1ea0)
UInt32 VortexMoveTogetherSolver::AddRef()
{
    return BaseBrickPhysicsSimulationSolver::AddRef();
}

// Mac 0x3a1e20 (thunks 0x3a1e60 / 0x3a1ec0)
UInt32 VortexMoveTogetherSolver::Release()
{
    return BaseBrickPhysicsSimulationSolver::Release();
}

// Mac 0x3a1e30 (thunks 0x3a1e80 / 0x3a1ee0)
PhysicsSolverCallback* VortexMoveTogetherSolver::GetCallback()
{
    return BaseBrickPhysicsSimulationSolver::GetCallback();
}

// Mac 0x3a1df0, Win 0x52ccd0
void VortexMoveTogetherSolver::OnBeginUpdate()
{
    m_vortexScene->StopSolver(false);
}

// Mac 0x3a09a0, Win 0x52ce70
void VortexMoveTogetherSolver::Start()
{
    m_vortexScene->LockDirty();
    BasicArray<PhysicsRigidElement*> bodies;
    for (ActiveRigidElement* e = m_elements.Begin(); e != m_elements.End(); ++e) {
        m_vortexScene->EnableManualMotion(e->m_element);
        PartOf(e->m_element)->setLinearVelocityDamping(kStartDamping);
        bodies.PushBack(e->m_element);
        for (TransformedElement* c = e->m_children.Begin(); c != e->m_children.End(); ++c) {
            m_vortexScene->EnableManualMotion(c->m_element);
            PartOf(c->m_element)->setLinearVelocityDamping(kStartDamping);
            bodies.PushBack(c->m_element);
            BasicArray<BaseBrickPhysicsConnectionHelper*>& joints = c->m_element->m_connections;
            for (BaseBrickPhysicsConnectionHelper** j = joints.Begin(); j != joints.End(); ++j) {
                BaseBrickPhysicsConnectionHelper** it = m_connections.Begin();
                while (it != m_connections.End() && *it != *j)
                    ++it;
                if (it == m_connections.End())
                    m_connections.PushBack(*j);
            }
        }
    }
    m_vortexScene->ResetSeparationCounts(m_connections);
    m_vortexScene->BuildConnectionSpringMirrors(0, m_connections, m_hinges, m_hinge.Get());
    m_vortexScene->m_universe->setAutoDisable(true);
    m_hinge->Start();
    m_stepCount = 0;
    m_restCount = 0;
    m_vortexScene->UnlockDirty(false);
    m_snapshot = m_vortexScene->CreateSnapshot(bodies.IsEmpty() ? 0 : bodies.Begin(), bodies.GetSize());
}

// Mac 0x3a1320, Win 0x52d0c0
void VortexMoveTogetherSolver::Stop(bool& result)
{
    m_vortexScene->LockDirty();
    m_hinge->Detach();
    m_vortexScene->UpdateTransforms(m_elements);
    if (result && m_vortexScene->TestElementCollisions(m_elements))
        result = false;
    m_hinge->Stop(result);
    m_vortexScene->DestroyConnectionSpringMirrors(m_connections, m_hinges);
    m_vortexScene->m_universe->setAutoDisable(false);
    for (ActiveRigidElement* e = m_elements.Begin(); e != m_elements.End(); ++e) {
        e->m_element->Stop();
        m_vortexScene->DisableSimulation(e->m_element);
        for (TransformedElement* c = e->m_children.Begin(); c != e->m_children.End(); ++c) {
            c->m_element->Stop();
            m_vortexScene->DisableSimulation(c->m_element);
        }
    }
    m_vortexScene->UnlockDirty(false);
    if (!result) {
        m_vortexScene->RestoreSnapshotImpl(m_snapshot);
        // NOTE: the original builds and drops String("VortexPhysicsScene::SimulateUntilConstraints
        // AreFulfilled Restored Transformed Elements") here.
    }
    m_hinge = 0;
    m_elements.Clear();
    m_connections.Clear();
    m_hinges.Clear();
    m_snapshot->Release();
    m_snapshot = 0;
}

// Mac 0x3a15d0, Win 0x52d2b0
UInt32 VortexMoveTogetherSolver::Step(Real32 /*time*/)
{
    Vx::VxUniverse* universe = static_cast<Vx::VxUniverse*>(m_vortexScene->GetNativeScene());
    universe->setSleepingPairsIntersect(true);   // (the scene's SleepingPairsIntersectScope)
    m_vortexScene->StepPhysics(kStepTime);
    PhysicsSeparationLimits limits = StepLimits();
    UInt32 separation = 0;
    for (BaseBrickPhysicsConnectionHelper** it = m_hinges.Begin(); separation == 0 && it != m_hinges.End(); ++it)
        separation = (*it)->CheckSeparation(limits);
    for (BaseBrickPhysicsConnectionHelper** it = m_connections.Begin(); separation == 0 && it != m_connections.End();
         ++it)
        separation = (*it)->CheckSeparation(limits);
    if (separation == 2) {
        universe->setSleepingPairsIntersect(false);
        return 2;
    }
    bool atRest = m_vortexScene->IsAtRestWithChildren(m_elements) && m_stepCount > 1;
    if (separation == 0 && m_hinge->CheckSeparation(limits) == 0 && HasNoDeepContacts(m_vortexScene)) {
        if (atRest) {
            Vx::VxInfo(0, "VortexMoveTogetherSolver::StepSimulation Simulation succeeded");
            universe->setSleepingPairsIntersect(false);
            return 1;
        }
        Vx::VxInfo(0, "VortexMoveTogetherSolver::StepSimulation Simulation satisfied but not at rest");
        m_restCount = 0;
    } else if (atRest) {
        if (m_restCount++ >= kMaxRestSteps) {
            Vx::VxInfo(0, "VortexMoveTogetherSolver::StepSimulation Simulation failed");
            universe->setSleepingPairsIntersect(false);
            return 2;
        }
    } else {
        Vx::VxInfo(0, "VortexMoveTogetherSolver::StepSimulation Simulation not satisfied and not at rest");
        m_restCount = 0;
    }
    m_hinge->Step(kStepTime);
    if (m_hinge->m_attached) {
        for (ActiveRigidElement* e = m_elements.Begin(); e != m_elements.End(); ++e) {
            DecayDamping(PartOf(e->m_element));
            for (TransformedElement* c = e->m_children.Begin(); c != e->m_children.End(); ++c)
                DecayDamping(PartOf(c->m_element));
        }
    }
    m_vortexScene->UpdateTransforms(m_elements);
    ++m_stepCount;
    universe->setSleepingPairsIntersect(false);
    return 0;
}

} // namespace LEGO
