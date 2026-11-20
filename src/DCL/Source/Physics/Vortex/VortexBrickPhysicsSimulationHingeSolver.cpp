// LEGO Digital Designer — restored source
// LEGO::VortexBrickPhysicsSimulationHingeSolver (Mac TU 424, Win 0x531d00-0x5320ff).
// See VortexBrickPhysicsSimulationHingeSolver.h.
#include "VortexBrickPhysicsSimulationHingeSolver.h"
#include "VortexPhysicsScene.h"
#include "VortexAnchorSolver.h"
#include "BaseBrickPhysicsRigidElement.h"
#include "BaseBrickPhysicsConnectionHelper.h"
#include "SystemTime.h"

#include "Vx/VxUniverse.h"
#include "Vx/VxMessage.h"

namespace LEGO {

namespace {

const Real32 kStepTime = 1.0f / 60.0f;            // 0x3c888889
const Int64  kTimeLimit = 1000;                   // ms
const UInt32 kMaxIterations = 1000;

// The joint tolerances of the solve (Mac __const 0x63f310, the same values inline in the other
// Vortex solvers): squared distances 0.0009 / 0.0625, |cos| 0.9999 / 0.9848, no break count.
inline PhysicsSeparationLimits SolveLimits()
{
    PhysicsSeparationLimits limits;
    limits.m_distance = 0.0009f;
    limits.m_maxDistance = 0.0625f;
    limits.m_alignment = 0.9999f;
    limits.m_minAlignment = 0.9848f;
    limits.m_breakCount = 0;
    return limits;
}

// The first non-zero CheckSeparation of `connections` (0 when all hold).
inline UInt32 CheckSeparations(const BasicArray<BaseBrickPhysicsConnectionHelper*>& connections,
                               const PhysicsSeparationLimits& limits)
{
    for (BaseBrickPhysicsConnectionHelper* const* it = connections.Begin(); it != connections.End(); ++it) {
        UInt32 result = (*it)->CheckSeparation(limits);
        if (result != 0)
            return result;
    }
    return 0;
}

} // namespace

// Mac 0x39f590 (Win inlined)
VortexBrickPhysicsSimulationHingeSolver::VortexBrickPhysicsSimulationHingeSolver(
    VortexPhysicsScene* scene, BaseBrickPhysicsConnectionHelper* hinge, bool first,
    const BasicArray<ActiveRigidElement>& elements, const BasicArray<BaseBrickPhysicsConnectionHelper*>& connections)
    : BaseBrickPhysicsSimulationHingeSolver(scene, hinge, first, elements, connections), m_vortexScene(scene),
      m_snapshot(0)
{
}

// Mac D1 0x39f6e0 (body 0x39f620) / D0 0x39f730, Win 0x531d20 / sdd 0x531d00
VortexBrickPhysicsSimulationHingeSolver::~VortexBrickPhysicsSimulationHingeSolver()
{
    if (m_snapshot != 0)
        m_snapshot->Release();
}

// Mac 0x39f7f0, Win 0x531db0
void VortexBrickPhysicsSimulationHingeSolver::Start()
{
    m_vortexScene->LockDirty();
    BasicArray<PhysicsRigidElement*> bodies;
    for (ActiveRigidElement* e = m_elements.Begin(); e != m_elements.End(); ++e) {
        m_vortexScene->Unknown39(e->m_element);
        m_vortexScene->EnableManualMotion(e->m_element);
        bodies.PushBack(e->m_element);
        for (TransformedElement* c = e->m_children.Begin(); c != e->m_children.End(); ++c) {
            bodies.PushBack(c->m_element);
            m_vortexScene->Unknown37(c->m_element);
            BasicArray<BaseBrickPhysicsConnectionHelper*>& joints = c->m_element->m_connections;
            for (BaseBrickPhysicsConnectionHelper** j = joints.Begin(); j != joints.End(); ++j)
                (*j)->DestroyConstraint();
        }
    }
    m_vortexScene->ResetSeparationCounts(m_connections);
    m_vortexScene->BuildConnectionSpringMirrors(1, m_connections, m_hinges, 0);
    m_vortexScene->UnlockDirty(false);
    m_snapshot = m_vortexScene->CreateSnapshot(bodies.IsEmpty() ? 0 : bodies.Begin(), bodies.GetSize());
}

// Mac 0x39fe00, Win 0x531f70
void VortexBrickPhysicsSimulationHingeSolver::Stop(bool& /*result*/)
{
    m_vortexScene->LockDirty();
    for (ActiveRigidElement* e = m_elements.Begin(); e != m_elements.End(); ++e) {
        m_vortexScene->Unknown38(e->m_element);
        m_vortexScene->DisableSimulation(e->m_element);
        for (TransformedElement* c = e->m_children.Begin(); c != e->m_children.End(); ++c) {
            m_vortexScene->DisableSimulation(c->m_element);
            BasicArray<BaseBrickPhysicsConnectionHelper*>& joints = c->m_element->m_connections;
            for (BaseBrickPhysicsConnectionHelper** j = joints.Begin(); j != joints.End(); ++j)
                if (!(*j)->HasNativeConstraint())
                    (*j)->UpdateConstraint();
        }
    }
    m_vortexScene->DestroyConnectionSpringMirrors(m_connections, m_hinges);
    m_vortexScene->UnlockDirty(false);
    m_elements.Clear();
    m_connections.Clear();
    m_hinges.Clear();
    m_snapshot->Release();
    m_snapshot = 0;
}

// Mac 0x39ff60, Win 0x5320a0
void VortexBrickPhysicsSimulationHingeSolver::OnSnapshotRestored()
{
    m_snapshot->Update();
}

// Mac 0x39ff80, Win 0x5320b0
bool VortexBrickPhysicsSimulationHingeSolver::Solve(Real32 /*angle*/, const Vector3& /*axis*/,
                                                    const Vector3& /*center*/,
                                                    BaseBrickPhysicsConnectionHelper* detach)
{
    VortexHingeSolverHinge* hinge = static_cast<VortexHingeSolverHinge*>(detach);
    Vx::VxUniverse* universe = static_cast<Vx::VxUniverse*>(m_vortexScene->GetNativeScene());
    universe->setSleepingPairsIntersect(true);   // (the scene's SleepingPairsIntersectScope)
    m_vortexScene->LockDirty();
    static_cast<BaseBrickPhysicsSimulationCallback*>(hinge)->Start();
    m_vortexScene->UnlockDirty(false);
    bool result = false;
    m_vortexScene->m_doCollide = false;
    Int64 deadline = GetMilliseconds() + kTimeLimit;
    UInt32 iterations = 0;
    for (;;) {
        m_vortexScene->StepPhysics(kStepTime);
        PhysicsSeparationLimits limits = SolveLimits();
        UInt32 separation = CheckSeparations(m_hinges, limits);
        if (separation == 0)
            separation = CheckSeparations(m_connections, limits);
        if (separation == 0) {
            separation = hinge->CheckSeparation(limits);
            if (separation == 0 && HasNoDeepContacts(m_vortexScene)) {
                // NOTE: the original builds and drops String("atRest result ok.") here.
                Vx::VxInfo(0, "VortexBrickPhysicsSimulationHingeSolver Simulation success");
                result = true;
                if (!m_vortexScene->IsAtRest(m_elements))
                    m_vortexScene->ResetForces(m_elements);
                break;
            }
        } else if (separation == 2) {
            // NOTE: String("dislocated.") built and dropped
            break;
        }
        if (deadline < GetMilliseconds()) {
            result = false;
            // NOTE: String("Simulation time constraint reached - rejected result.") built and dropped
            Vx::VxInfo(0, "VortexBrickPhysicsSimulationHingeSolver Simulation stop: time expired");
            break;
        }
        hinge->Step(kStepTime);
        if (++iterations > kMaxIterations - 1)
            break;
    }
    Vx::VxInfo(0, "VortexBrickPhysicsSimulationHingeSolver::Change: stepped %d iterations", iterations);
    // NOTE: String("VortexBrickPhysicsSimulationHingeSolver: Step ") << iterations << " iterations"
    // is built and dropped as well.
    m_vortexScene->m_doCollide = true;
    m_vortexScene->LockDirty();
    hinge->Detach();
    for (ActiveRigidElement* e = m_elements.Begin(); e != m_elements.End(); ++e) {
        BaseBrickPhysicsRigidElement* body = e->m_element;
        body->GetEngineTransform(body->m_transform);
        body->NoteTransformChanged();
        m_vortexScene->Unknown38(body);
        m_vortexScene->MoveChildren(*e);
    }
    if (result)
        result = !m_vortexScene->TestElementCollisions(m_elements);
    static_cast<BaseBrickPhysicsSimulationCallback*>(hinge)->Stop(result);
    for (ActiveRigidElement* e = m_elements.Begin(); e != m_elements.End(); ++e)
        m_vortexScene->Unknown39(e->m_element);
    m_vortexScene->UnlockDirty(false);
    if (!result)
        m_vortexScene->RestoreSnapshotImpl(m_snapshot);
    else
        m_snapshot->Update();
    PhysicsSeparationLimits limits = SolveLimits();
    BasicArray<BaseBrickPhysicsConnectionHelper*>& all = m_vortexScene->m_connections;
    for (BaseBrickPhysicsConnectionHelper** it = all.Begin(); it != all.End(); ++it) {
        if ((*it)->CheckSeparation(limits) != 0) {
            m_vortexScene->RestoreSnapshotImpl(m_snapshot);
            break;
        }
    }
    universe->setSleepingPairsIntersect(false);
    return result;
}

} // namespace LEGO
