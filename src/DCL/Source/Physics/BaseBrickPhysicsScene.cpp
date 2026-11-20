// LEGO Digital Designer — restored source
// LEGO::BaseBrickPhysicsScene and the solvers of the BaseBrickPhysics layer (Mac TU 211). See
// BaseBrickPhysicsScene.h.

// Template instances in this TU: BasicArray<T*>::PushBack (Mac 0x256210 / 0x256440 /
// inlined; Win 0x49c250), PhysicsArrayRemove (Mac 0x255530, 0x25a380; Win 0x53e6c0), the
// ActiveRigidElement / TransformedElement / BasicArray<BaseBrickPhysicsConnectionHelper*> array
// operations (Mac 0x258620, 0x258cb0, 0x258f70, 0x259060 (std::search), 0x259e00, 0x25a5b0,
// 0x25a870, 0x25a980; Win 0x5c7240, 0x5c72c0, 0x5c7350, 0x5c7470, 0x4a11d0) and the array
// destructors (Mac 0x254e90 ..., Win 0x528970-0x528a30).
#include "BaseBrickPhysicsScene.h"
#include "BaseBrickPhysics.h"
#include "BaseBrickPhysicsRigidElement.h"
#include "BaseBrickPhysicsConnectionHelper.h"
#include "BaseBrickPhysicsAtomic.h"

#include <string.h>
#include <algorithm>

namespace LEGO {

namespace {

// Adds `element` unless an element with the same body is there. Mac 0x2579e0 (Win inlined)
void AddActiveRigidElement(BasicArray<ActiveRigidElement>& elements, const ActiveRigidElement& element)
{
    for (ActiveRigidElement* it = elements.Begin(); it != elements.End(); ++it)
        if (it->m_element == element.m_element)
            return;
    elements.PushBack(element);
}

// Removes the element of `body` (the last element takes its place). Mac 0x258140 (erase
// 0x25a870), Win inlined into 0x5c6aa0 / 0x5c6ca0
void RemoveActiveRigidElement(BasicArray<ActiveRigidElement>& elements, BaseBrickPhysicsRigidElement* body)
{
    ActiveRigidElement* it = elements.Begin();
    while (it != elements.End() && it->m_element != body)
        ++it;
    if (it == elements.End())
        return;
    ActiveRigidElement* last = elements.End() - 1;
    if (last != it)
        *it = *last;
    elements.PopBack();
}

bool Contains(const BasicArray<BaseBrickPhysicsConnectionHelper*>& array, BaseBrickPhysicsConnectionHelper* value)
{
    for (BaseBrickPhysicsConnectionHelper* const* it = array.Begin(); it != array.End(); ++it)
        if (*it == value)
            return true;
    return false;
}

} // namespace

// =============================================================================================
// TransformedElement
// =============================================================================================

// Mac 0x254240
TransformedElement::TransformedElement(BaseBrickPhysicsRigidElement* element, bool getTransform)
    : m_element(element)
{
    if (getTransform && m_element != 0)
        m_element->GetTransform(m_transform);
}

// =============================================================================================
// BaseBrickPhysicsSimulationSolver
// =============================================================================================

// Win 0x5209d0 (Mac inlined)
BaseBrickPhysicsSimulationSolver::BaseBrickPhysicsSimulationSolver(BaseBrickPhysicsScene* scene,
                                                                   PhysicsSolverCallback* callback)
    : BaseBrickPhysicsSimulationCallback(scene), m_callback(callback)
{
    if (m_callback != 0)
        m_callback->AddRef();
}

// Mac 0x259370 / 0x2593f0, Win 0x520a70 / 0x520af0
BaseBrickPhysicsSimulationSolver::~BaseBrickPhysicsSimulationSolver()
{
    if (m_callback != 0)
        m_callback->Release();
}

// Mac 0x254280 (thunk 0x254300), Win 0x5c5790
Int32 BaseBrickPhysicsSimulationSolver::QueryInterface(const InterfaceID& id, void** object)
{
    if (memcmp(&id, &PhysicsSolver::GetIID(), sizeof id) == 0) {
        *object = static_cast<PhysicsSolver*>(this);
        AddRef();
        return 0;
    }
    *object = 0;
    return kPhysicsNoInterface;
}

// Mac 0x254390 (thunk 0x2543b0), Win 0x5c57e0
UInt32 BaseBrickPhysicsSimulationSolver::AddRef()
{
    Object::AddRef();
    return 0xffffffff;
}

// Mac 0x2543d0 (thunk 0x2543f0), Win 0x52cca0
UInt32 BaseBrickPhysicsSimulationSolver::Release()
{
    Object::Release();
    return 0xffffffff;
}

// Mac 0x254410 (thunk 0x254420), Win 0x51e790
PhysicsSolverCallback* BaseBrickPhysicsSimulationSolver::GetCallback()
{
    return m_callback;
}

// =============================================================================================
// BaseBrickPhysicsAnimatingSolver
// =============================================================================================

// Mac 0x254d40, Win 0x5c59b0
BaseBrickPhysicsAnimatingSolver::BaseBrickPhysicsAnimatingSolver(BaseBrickPhysicsScene* scene,
                                                                 BasicArray<BaseBrickPhysicsRigidElement*>& bodies)
    : BaseBrickPhysicsSimulationSolver(scene, 0), m_gravity(0.0f, -9.82f, 0.0f)
{
    m_bodies.Swap(bodies);
}

// Mac 0x255060, Win 0x5c5b70
void BaseBrickPhysicsAnimatingSolver::Start()
{
    m_scene->SetGravity(m_gravity);
    for (UInt32 i = 0; i < m_bodies.GetSize(); ++i)
        m_scene->EnableSimulation(m_bodies[i]);
}

// Mac 0x2550c0, Win 0x5c5bb0
void BaseBrickPhysicsAnimatingSolver::Stop(bool& /*result*/)
{
    m_scene->SetGravity(Vector3(0.0f, 0.0f, 0.0f));
    for (UInt32 i = 0; i < m_bodies.GetSize(); ++i)
        m_scene->DisableSimulation(m_bodies[i]);
    m_bodies.Clear();
}

// Mac 0x255140, Win 0x5c5c10
UInt32 BaseBrickPhysicsAnimatingSolver::Step(Real32 time)
{
    m_scene->StepPhysics(time);
    for (UInt32 i = 0; i < m_bodies.GetSize(); ++i) {
        BaseBrickPhysicsRigidElement* body = m_bodies[i];
        body->GetEngineTransform(body->m_transform);
        body->NoteTransformChanged();
    }
    return 0;
}

// The changed bodies of this animation take part again. Mac 0x2551a0, Win 0x5c5c50
void BaseBrickPhysicsAnimatingSolver::OnEndUpdate(BasicArray<BaseBrickPhysicsRigidElement*>& bodies,
                                                  BasicArray<BaseBrickPhysicsConnectionHelper*>& /*connections*/)
{
    for (UInt32 i = 0; i < bodies.GetSize(); ++i) {
        BaseBrickPhysicsRigidElement* body = bodies[i];
        if (std::find(m_bodies.Begin(), m_bodies.End(), body) != m_bodies.End())
            m_scene->EnableSimulation(body);
    }
}

// Mac 0x2552c0, Win 0x5c5ca0
void BaseBrickPhysicsAnimatingSolver::OnAddRigidElement(BaseBrickPhysicsRigidElement* body,
                                                        const Matrix3D& /*transform*/)
{
    m_bodies.PushBack(body);
}

// Mac 0x2554f0, Win 0x5c5cc0
void BaseBrickPhysicsAnimatingSolver::OnRemoveRigidElement(BaseBrickPhysicsRigidElement* body)
{
    m_scene->DisableSimulation(body);
    PhysicsArrayRemove(m_bodies, body);
}

// Mac 0x254ec0, Win 0x5c5a80
Int32 BaseBrickPhysicsAnimatingSolver::QueryInterface(const InterfaceID& id, void** object)
{
    if (memcmp(&id, &AnimatePhysicsSolver::GetIID(), sizeof id) == 0) {
        *object = static_cast<AnimatePhysicsSolver*>(this);
    } else if (memcmp(&id, &PhysicsSolver::GetIID(), sizeof id) == 0) {
        *object = static_cast<PhysicsSolver*>(static_cast<BaseBrickPhysicsSimulationSolver*>(this));
    } else {
        *object = 0;
        return kPhysicsNoInterface;
    }
    AddRef();
    return 0;
}

// Mac 0x259820 (thunks 0x2598d0, 0x259920), Win 0x5c57e0
UInt32 BaseBrickPhysicsAnimatingSolver::AddRef()
{
    Object::AddRef();
    return 0xffffffff;
}

// Mac 0x259840 (thunks 0x2598f0, 0x259940), Win 0x52cca0
UInt32 BaseBrickPhysicsAnimatingSolver::Release()
{
    Object::Release();
    return 0xffffffff;
}

// Mac 0x259860 (thunks 0x259910, 0x259960), Win 0x51e790
PhysicsSolverCallback* BaseBrickPhysicsAnimatingSolver::GetCallback()
{
    return m_callback;
}

// Mac 0x254fe0 (thunk 0x255020), Win 0x5c5b20
void BaseBrickPhysicsAnimatingSolver::SetGravity(const Vector3* gravity)
{
    if (gravity == 0)
        return;
    m_gravity = *gravity;
    if (m_scene->m_solver == static_cast<BaseBrickPhysicsSimulationSolver*>(this))
        m_scene->SetGravity(m_gravity);
}

// =============================================================================================
// BaseBrickPhysicsBaseHingeSolver / BaseBrickPhysicsManualHingeSolver
// =============================================================================================

// Mac 0x259af0, Win 0x520b10
BaseBrickPhysicsBaseHingeSolver::BaseBrickPhysicsBaseHingeSolver(
    BaseBrickPhysicsScene* scene, BaseBrickPhysicsConnectionHelper* hinge, bool first,
    const BasicArray<ActiveRigidElement>& elements, const BasicArray<BaseBrickPhysicsConnectionHelper*>& connections)
    : BaseBrickPhysicsSimulationSolver(scene, 0), m_hinge(hinge), m_first(first), m_elements(elements),
      m_connections(connections)
{
}

// Mac 0x254430, Win 0x4f9670
UInt32 BaseBrickPhysicsBaseHingeSolver::Step(Real32 /*time*/)
{
    return 0;
}

// Mac 0x254440, Win 0x5c5800
void BaseBrickPhysicsBaseHingeSolver::OnBeginUpdate()
{
    // NOTE: drops this solver (the scene's reference) while it is being called
    m_scene->StopSolver(false);
}

// Mac 0x2595f0, Win 0x520bb0
bool BaseBrickPhysicsBaseHingeSolver::IsFor(BaseBrickPhysicsConnectionHelper* hinge, bool first)
{
    return m_hinge == hinge && first == m_first;
}

// Turns the moving side of the hinge (its body m_hinge->m_body when m_first, else
// m_otherBody) by `angle` about `axis` through `center`; the attached bodies follow; a
// collision puts everything back. Mac 0x254460, Win 0x5c5810
bool BaseBrickPhysicsManualHingeSolver::Solve(Real32 angle, const Vector3& axis, const Vector3& center,
                                              BaseBrickPhysicsConnectionHelper* detach)
{
    m_scene->LockDirty();
    detach->Detach();
    m_scene->UnlockDirty(false);

    BaseBrickPhysicsRigidElement* body = m_first ? m_hinge->m_body : m_hinge->m_otherBody;
    Matrix3D rotation;
    Vector3 translation;
    rotation.SetRotation(axis, angle, translation, center);
    Matrix3D transform(body->m_transform, rotation);

    PhysicsSnapshot* snapshot = m_scene->CreateElementSnapshot(m_elements);
    for (ActiveRigidElement* it = m_elements.Begin(); it != m_elements.End(); ++it) {
        m_scene->EnableManualMotion(it->m_element);
        for (TransformedElement* c = it->m_children.Begin(); c != it->m_children.End(); ++c)
            m_scene->EnableManualMotion(c->m_element);
    }
    body->SetEngineTransform(transform);
    m_scene->MoveChildren(m_elements);
    bool collision = m_scene->TestElementCollisions(m_elements);
    if (collision)
        m_scene->RestoreSnapshotImpl(snapshot);
    for (ActiveRigidElement* it = m_elements.Begin(); it != m_elements.End(); ++it) {
        m_scene->DisableSimulation(it->m_element);
        for (TransformedElement* c = it->m_children.Begin(); c != it->m_children.End(); ++c)
            m_scene->DisableSimulation(c->m_element);
    }
    snapshot->Release();
    return !collision;
}

// =============================================================================================
// BaseBrickPhysicsSnapshot
// =============================================================================================

// (Mac inlined into 0x3ad500)
BaseBrickPhysicsSnapshot::BaseBrickPhysicsSnapshot(BaseBrickPhysicsScene* scene)
    : m_refCount(1), m_scene(scene)
{
    m_scene->m_snapshots.PushBack(this);
}

// Mac 0x2599d0 / 0x259a10, Win 0x520ff0 / 0x521020
BaseBrickPhysicsSnapshot::~BaseBrickPhysicsSnapshot()
{
    PhysicsArrayRemove(m_scene->m_snapshots, this);
}

// Mac 0x259290, Win 0x5c71b0
Int32 BaseBrickPhysicsSnapshot::QueryInterface(const InterfaceID& id, void** object)
{
    if (memcmp(&id, &PhysicsSnapshot::GetIID(), sizeof id) == 0) {
        *object = this;
        AddRef();
        return 0;
    }
    *object = 0;
    return kPhysicsNoInterface;
}

// Mac 0x259310, Win 0x5c7750
UInt32 BaseBrickPhysicsSnapshot::AddRef()
{
    return (UInt32)PhysicsAtomicIncrement(m_refCount);
}

// Mac 0x259330, Win 0x5c71f0
UInt32 BaseBrickPhysicsSnapshot::Release()
{
    Int32 count = PhysicsAtomicDecrement(m_refCount);
    if (count == 0)
        delete this;
    return (UInt32)count;
}

// =============================================================================================
// BaseBrickPhysicsScene
// =============================================================================================

// Win 0x520db0
BaseBrickPhysicsScene::BaseBrickPhysicsScene(BaseBrickPhysics* physics)
    : m_refCount(1), m_physics(physics), m_solver(0), m_stepCount(0), m_unknown54(false), m_updateDepth(0),
      m_dirtyLock(0)
{
}

// Mac D2 0x25a0a0 (D1 0x259980, D0 0x259990), Win 0x520eb0 / 0x520fd0
BaseBrickPhysicsScene::~BaseBrickPhysicsScene()
{
    if (m_solver != 0)
        m_solver->Object::Release();
}

// Mac 0x255730, Win 0x5c5d00
Int32 BaseBrickPhysicsScene::QueryInterface(const InterfaceID& id, void** object)
{
    if (memcmp(&id, &PhysicsScene::GetIID(), sizeof id) == 0) {
        *object = this;
        AddRef();
        return 0;
    }
    *object = 0;
    return kPhysicsNoInterface;
}

// Mac 0x2557b0, Win 0x5c5d40
UInt32 BaseBrickPhysicsScene::AddRef()
{
    return (UInt32)PhysicsAtomicIncrement(m_refCount);
}

// Mac 0x2557d0, Win 0x5c5d60
UInt32 BaseBrickPhysicsScene::Release()
{
    Int32 count = PhysicsAtomicDecrement(m_refCount);
    if (count == 0)
        delete this;
    return (UInt32)count;
}

// Mac 0x256120, Win 0x5c6070
void BaseBrickPhysicsScene::Unknown6()
{
    if (m_updateDepth == 0 && m_solver != 0)
        m_solver->OnBeginUpdate();
    for (UInt32 i = 0; i < m_snapshots.GetSize(); ++i)
        m_snapshots[i]->Clear();
    ++m_updateDepth;
    ++m_dirtyLock;
}

// Mac 0x256180, Win 0x5c60c0
void BaseBrickPhysicsScene::Unknown7()
{
    --m_updateDepth;
    UnlockDirty(true);
}

// Mac 0x2546a0 (Win inlined)
bool BaseBrickPhysicsScene::LockDirty()
{
    ++m_dirtyLock;
    return true;
}

// Mac 0x2546b0, Win 0x5c60d0
bool BaseBrickPhysicsScene::UnlockDirty(bool notifySolver)
{
    if (--m_dirtyLock == 0) {
        for (UInt32 i = 0; i < m_dirtyRigidElements.GetSize(); ++i)
            m_dirtyRigidElements[i]->Update();
        for (UInt32 i = 0; i < m_dirtyConnections.GetSize(); ++i) {
            BaseBrickPhysicsConnectionHelper* connection = m_dirtyConnections[i];
            connection->m_dirty = false;
            connection->UpdateConstraint();
        }
        if (notifySolver && m_solver != 0)
            m_solver->OnEndUpdate(m_dirtyRigidElements, m_dirtyConnections);
        m_dirtyRigidElements.Clear();
        m_dirtyConnections.Clear();
    }
    return true;
}

// Mac 0x255810, Win 0x5c5d90
void BaseBrickPhysicsScene::AddRigidElement(PhysicsRigidElement* body, const Matrix3D& transform)
{
    BaseBrickPhysicsRigidElement* element = static_cast<BaseBrickPhysicsRigidElement*>(body);
    m_rigidElements.PushBack(element);
    element->AddToScene(this, transform);
    if (m_solver != 0)
        m_solver->OnAddRigidElement(element, transform);
}

// Mac 0x255a70, Win 0x5c5e00
void BaseBrickPhysicsScene::RemoveRigidElement(PhysicsRigidElement* body)
{
    BaseBrickPhysicsRigidElement* element = static_cast<BaseBrickPhysicsRigidElement*>(body);
    if (m_solver != 0)
        m_solver->OnRemoveRigidElement(element);
    element->RemoveFromScene();
    PhysicsArrayRemove(m_dirtyRigidElements, element);
    PhysicsArrayRemove(m_rigidElements, element);
}

// Mac 0x255ad0, Win 0x40c160
void BaseBrickPhysicsScene::RestoreSnapshot(PhysicsSnapshot* snapshot)
{
    RestoreSnapshotImpl(snapshot);
}

// Mac 0x255af0, Win 0x5c5e80
AnimatePhysicsSolver* BaseBrickPhysicsScene::CreateAnimatingSolver(PhysicsRigidElement* const* bodies, UInt32 count,
                                                                   const Vector3* gravity)
{
    StopSolver(false);
    BasicArray<BaseBrickPhysicsRigidElement*> list;
    if (count != 0) {
        list.Reserve(count);
        for (UInt32 i = 0; i < count; ++i)
            list.PushBack(static_cast<BaseBrickPhysicsRigidElement*>(bodies[i]));
    }
    BaseBrickPhysicsAnimatingSolver* solver = new BaseBrickPhysicsAnimatingSolver(this, list);
    solver->AddRef();                       // the local reference
    solver->SetGravity(gravity);
    SetSolver(solver);
    solver->AddRef();                       // the caller's reference
    solver->Object::Release();
    return solver;
}

// Mac 0x255f50 (Win 0x5c6010 + 0x5c6960)
UInt32 BaseBrickPhysicsScene::Step(Real32 time)
{
    if (m_solver == 0)
        return 3;
    UInt32 result = m_solver->Step(time);
    if (result == 1) {
        bool success = true;
        StopRunningSolver(success);
        return success ? 1 : 2;
    }
    if (result == 2) {
        bool success = false;
        StopRunningSolver(success);
        return 2;
    }
    if (m_solver->m_callback != 0)
        m_solver->m_callback->OnStep(time);
    return 0;
}

// Mac 0x255ff0, Win 0x5c6030
bool BaseBrickPhysicsScene::StopSolver(bool result)
{
    if (m_solver == 0)
        return true;
    StopRunningSolver(result);
    return result;
}

// Mac 0x256100, Win 0x5c6060
PhysicsSolver* BaseBrickPhysicsScene::GetSolver()
{
    return m_solver != 0 ? static_cast<PhysicsSolver*>(m_solver) : 0;
}

// Mac 0x255ee0, Win 0x5c6900
void BaseBrickPhysicsScene::SetSolver(BaseBrickPhysicsSimulationSolver* solver)
{
    BaseBrickPhysicsSimulationSolver* old = m_solver;
    if (old != solver) {
        m_solver = solver;
        if (solver != 0)                    // (Win checks; Mac does not)
            solver->AddRef();
        if (old != 0)
            old->Object::Release();
    }
    m_stepCount = 0;
    m_solver->Start();
    if (m_solver->m_callback != 0)
        m_solver->m_callback->OnBegin(static_cast<PhysicsSolver*>(m_solver));
}

// Mac 0x256030, Win 0x5c69e0
void BaseBrickPhysicsScene::StopRunningSolver(bool& result)
{
    BaseBrickPhysicsSimulationSolver* solver = m_solver;
    if (solver->m_callback != 0)
        solver->m_callback->OnEnd(result);
    solver->Stop(result);
    BaseBrickPhysicsSimulationSolver* hold = m_solver;
    if (hold != 0)
        hold->AddRef();
    if (m_solver != 0) {
        BaseBrickPhysicsSimulationSolver* running = m_solver;
        m_solver = 0;
        running->Object::Release();
    }
    m_stepCount = 0;
    if (hold->m_callback != 0)
        hold->m_callback->OnResult(result);
    hold->Object::Release();
}

// Mac 0x259970, Win 0x520da0
void BaseBrickPhysicsScene::StepPhysics(Real32 /*time*/)
{
    ++m_stepCount;
}

// Mac 0x258590, Win 0x5c6e80
BaseBrickPhysicsBaseHingeSolver* BaseBrickPhysicsScene::CreateHingeManualSolver(
    BaseBrickPhysicsConnectionHelper* hinge, bool first, const BasicArray<ActiveRigidElement>& elements,
    const BasicArray<BaseBrickPhysicsConnectionHelper*>& connections)
{
    return new BaseBrickPhysicsManualHingeSolver(this, hinge, first, elements, connections);
}

// Mac 0x256670 (Win inlined into the helper constructor 0x5c5160)
void BaseBrickPhysicsScene::AddConnection(BaseBrickPhysicsConnectionHelper* connection)
{
    m_connections.PushBack(connection);
    connection->MarkDirty();
}

// Mac 0x2568a0, Win 0x5c6150
void BaseBrickPhysicsScene::RemoveConnection(BaseBrickPhysicsConnectionHelper* connection)
{
    if (connection->m_dirty)
        PhysicsArrayRemove(m_dirtyConnections, connection);
    PhysicsArrayRemove(m_connections, connection);
}

// Mac 0x256210
void BaseBrickPhysicsScene::MarkRigidElementDirty(BaseBrickPhysicsRigidElement* body)
{
    m_dirtyRigidElements.PushBack(body);
}

// Mac 0x256440
void BaseBrickPhysicsScene::MarkConnectionDirty(BaseBrickPhysicsConnectionHelper* connection)
{
    m_dirtyConnections.PushBack(connection);
}

// ---- hinge groups ------------------------------------------------------------------------------

// Mac 0x257e30, Win 0x5c6aa0
BaseBrickPhysicsBaseHingeSolver* BaseBrickPhysicsScene::GetHingeSolver(BaseBrickPhysicsConnectionHelper* hinge,
                                                                       bool first)
{
    if (m_solver != 0) {
        BaseBrickPhysicsBaseHingeSolver* solver = dynamic_cast<BaseBrickPhysicsBaseHingeSolver*>(m_solver);
        if (solver != 0 && solver->IsFor(hinge, first))
            return solver;
        StopSolver(false);
    }
    BaseBrickPhysicsRigidElement* body = first ? hinge->m_body : hinge->m_otherBody;
    BaseBrickPhysicsRigidElement* fixed = first ? hinge->m_otherBody : hinge->m_body;
    BasicArray<BaseBrickPhysicsConnectionHelper*> connections;
    BasicArray<ActiveRigidElement> elements;
    CollectHingeGroup(hinge, body, connections, elements);
    RemoveActiveRigidElement(elements, fixed);
    BaseBrickPhysicsBaseHingeSolver* solver;
    if (connections.GetSize() == 1)
        solver = CreateHingeManualSolver(hinge, first, elements, connections);
    else
        solver = CreateHingeSimulateSolver(hinge, first, elements, connections);
    SetSolver(solver);
    return solver;
}

// Mac 0x2582c0, Win 0x5c6ca0
BaseBrickPhysicsBaseHingeSolver* BaseBrickPhysicsScene::GetHingeSimulateSolver(BaseBrickPhysicsConnectionHelper* hinge,
                                                                               bool first)
{
    if (m_solver != 0) {
        BaseBrickPhysicsBaseHingeSolver* solver = dynamic_cast<BaseBrickPhysicsSimulationHingeSolver*>(m_solver);
        if (solver != 0 && solver->IsFor(hinge, first))
            return solver;
        StopSolver(false);
    }
    BaseBrickPhysicsRigidElement* body = first ? hinge->m_body : hinge->m_otherBody;
    BaseBrickPhysicsRigidElement* fixed = first ? hinge->m_otherBody : hinge->m_body;
    BasicArray<BaseBrickPhysicsConnectionHelper*> connections;
    BasicArray<ActiveRigidElement> elements;
    CollectHingeGroup(hinge, body, connections, elements);
    RemoveActiveRigidElement(elements, fixed);
    BaseBrickPhysicsBaseHingeSolver* solver = CreateHingeSimulateSolver(hinge, first, elements, connections);
    SetSolver(solver);
    return solver;
}

// Mac 0x2568f0, Win 0x5c61a0
void BaseBrickPhysicsScene::CollectHingeGroup(BaseBrickPhysicsConnectionHelper* hinge, BaseBrickPhysicsRigidElement* body,
                                              BasicArray<BaseBrickPhysicsConnectionHelper*>& connections,
                                              BasicArray<ActiveRigidElement>& elements)
{
    // no marks (Win 0x5c6460)
    for (UInt32 i = 0; i < m_rigidElements.GetSize(); ++i) {
        BaseBrickPhysicsRigidElement* element = m_rigidElements[i];
        element->m_mark = 0;
        for (UInt32 j = 0; j < element->m_connections.GetSize(); ++j)
            element->m_connections[j]->m_mark = 0;
    }
    m_stack.Clear();
    m_loops.Clear();
    FindLoops(hinge, body);
    connections.PushBack(hinge);
    RemoveContainedLoops();

    // every loop through a hinge of the group joins the group (the loop is consumed)
    if (!m_loops.IsEmpty()) {
        for (UInt32 i = 0; i < connections.GetSize(); ++i) {
            for (BasicArray<BaseBrickPhysicsConnectionHelper*>* loop = m_loops.Begin(); loop != m_loops.End(); ++loop) {
                if (!Contains(*loop, connections[i]))
                    continue;
                for (UInt32 j = 0; j < loop->GetSize(); ++j)
                    if (!Contains(connections, (*loop)[j]))
                        connections.PushBack((*loop)[j]);
                loop->Clear();
            }
        }
    }

    // the bodies of the hinges, then what hangs on them
    for (UInt32 i = 0; i < connections.GetSize(); ++i) {
        AddActiveRigidElement(elements, ActiveRigidElement(connections[i]->m_body));
        AddActiveRigidElement(elements, ActiveRigidElement(connections[i]->m_otherBody));
    }
    for (ActiveRigidElement* it = elements.Begin(); it != elements.End(); ++it) {
        it->m_element->m_mark = 1;
        if (it->m_element != 0)
            it->m_element->GetTransform(it->m_transform);
    }
    for (ActiveRigidElement* it = elements.Begin(); it != elements.End(); ++it)
        CollectChildren(*it, it->m_element);
}

// Mac 0x2571a0, Win 0x5c6f00
void BaseBrickPhysicsScene::FindLoops(BaseBrickPhysicsConnectionHelper* connection, BaseBrickPhysicsRigidElement* body)
{
    connection->m_mark = 1;
    m_stack.PushBack(connection);
    for (UInt32 i = 0; i < body->m_connections.GetSize(); ++i) {
        BaseBrickPhysicsConnectionHelper* next = body->m_connections[i];
        if (next->m_mark != 0)
            continue;
        BaseBrickPhysicsRigidElement* other = next->m_body == body ? next->m_otherBody : next->m_body;
        // the newest joint on the path that `other` also has closes a loop
        for (BaseBrickPhysicsConnectionHelper** p = m_stack.End(); p != m_stack.Begin();) {
            --p;
            if (!Contains(other->m_connections, *p))
                continue;
            m_loops.PushBack(BasicArray<BaseBrickPhysicsConnectionHelper*>());
            BasicArray<BaseBrickPhysicsConnectionHelper*>& loop = m_loops.Back();
            for (BaseBrickPhysicsConnectionHelper** q = p; q != m_stack.End(); ++q)
                loop.PushBack(*q);
            loop.PushBack(next);
            break;
        }
        FindLoops(next, other);
    }
    m_stack.PopBack();
}

// Mac 0x257740, Win 0x5c7120
void BaseBrickPhysicsScene::RemoveContainedLoops()
{
    for (BasicArray<BaseBrickPhysicsConnectionHelper*>* a = m_loops.Begin(); a != m_loops.End(); ++a) {
        for (BasicArray<BaseBrickPhysicsConnectionHelper*>* b = m_loops.Begin(); b != m_loops.End(); ++b) {
            if (a == b)
                continue;
            BaseBrickPhysicsConnectionHelper** found = std::search(b->Begin(), b->End(), a->Begin(), a->End());
            UInt32 count = a->GetSize();
            if (found == b->End() || count == 0)
                continue;
            BaseBrickPhysicsConnectionHelper** to = found;
            for (BaseBrickPhysicsConnectionHelper** from = found + count; from != b->End(); ++from)
                *to++ = *from;
            while (b->End() != to)
                b->PopBack();
        }
    }
}

// Mac 0x257ae0, Win 0x5c7090
void BaseBrickPhysicsScene::CollectChildren(ActiveRigidElement& element, BaseBrickPhysicsRigidElement* body)
{
    body->m_mark = 1;
    for (UInt32 i = 0; i < body->m_connections.GetSize(); ++i) {
        BaseBrickPhysicsConnectionHelper* connection = body->m_connections[i];
        BaseBrickPhysicsRigidElement* other =
            connection->m_body == body ? connection->m_otherBody : connection->m_body;
        if (other->m_mark != 0)
            continue;
        element.m_children.PushBack(TransformedElement(other, true));
        CollectChildren(element, other);
    }
}

// Mac 0x254740, Win 0x5c64a0
PhysicsSnapshot* BaseBrickPhysicsScene::CreateElementSnapshot(const BasicArray<ActiveRigidElement>& elements)
{
    BasicArray<PhysicsRigidElement*> bodies;
    for (const ActiveRigidElement* it = elements.Begin(); it != elements.End(); ++it) {
        bodies.PushBack(it->m_element);
        for (const TransformedElement* c = it->m_children.Begin(); c != it->m_children.End(); ++c)
            bodies.PushBack(c->m_element);
    }
    return CreateSnapshot(bodies.Begin(), bodies.GetSize());
}

// Mac 0x254c70, Win 0x5c6670
void BaseBrickPhysicsScene::MoveChildren(const BasicArray<ActiveRigidElement>& elements)
{
    for (const ActiveRigidElement* it = elements.Begin(); it != elements.End(); ++it)
        MoveChildren(*it);
}

// Mac 0x257cc0 (inlined in 0x254c70)
void BaseBrickPhysicsScene::MoveChildren(const ActiveRigidElement& element)
{
    Matrix3D inverse;
    inverse.SetInverse(element.m_transform);
    Matrix3D relative(element.m_element->m_transform, inverse);
    for (const TransformedElement* c = element.m_children.Begin(); c != element.m_children.End(); ++c)
        c->m_element->SetEngineTransform(Matrix3D(relative, c->m_transform));
}

// Mac 0x257c30, Win 0x5c6590
void BaseBrickPhysicsScene::UpdateTransforms(BasicArray<ActiveRigidElement> elements)
{
    for (ActiveRigidElement* it = elements.Begin(); it != elements.End(); ++it) {
        it->m_element->GetEngineTransform(it->m_element->m_transform);
        it->m_element->NoteTransformChanged();
        for (TransformedElement* c = it->m_children.Begin(); c != it->m_children.End(); ++c) {
            Matrix3D unused(c->m_transform);   // NOTE: a copy that is never used (both builds)
            (void)unused;
            c->m_element->GetEngineTransform(c->m_element->m_transform);
            c->m_element->NoteTransformChanged();
        }
    }
}

// Mac 0x257d60, Win 0x5c6720
bool BaseBrickPhysicsScene::IsAtRest(BasicArray<ActiveRigidElement> elements)
{
    for (ActiveRigidElement* it = elements.Begin(); it != elements.End(); ++it)
        if (it->m_element->IsMoving())
            return false;
    return true;
}

// Mac 0x257db0, Win 0x5c6800
bool BaseBrickPhysicsScene::IsAtRestWithChildren(BasicArray<ActiveRigidElement> elements)
{
    for (ActiveRigidElement* it = elements.Begin(); it != elements.End(); ++it) {
        if (it->m_element->IsMoving())
            return false;
        for (TransformedElement* c = it->m_children.Begin(); c != it->m_children.End(); ++c)
            if (c->m_element->IsMoving())
                return false;
    }
    return true;
}

// Mac 0x257b80
void BaseBrickPhysicsScene::ResetSeparationCounts(const BasicArray<BaseBrickPhysicsConnectionHelper*>& connections)
{
    for (BaseBrickPhysicsConnectionHelper* const* it = connections.Begin(); it != connections.End(); ++it)
        (*it)->m_separationCount = 0;
}

} // namespace LEGO
