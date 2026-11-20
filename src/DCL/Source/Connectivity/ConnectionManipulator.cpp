// LEGO Digital Designer — restored source
// LEGO::ConnectionManipulator, PrimitiveConnectionManipulator, RigidConnectionManipulator (see
// ConnectionManipulator.h) and PrimitiveElement::CreateManipulator / RigidElement::
// CreateManipulator.
//
// Original object file: Scene.cpp (Mac TU 025); this is sub-TU 025i, Mac 0x9cc30-0x9d050 (0x9d050
// itself is the Scene function CollectManipulatedElements), the manipulator inline virtuals and
// helpers of 025s (0xc40b0-0xc4200, 0xc53a0-0xc5e90, 0xc74a0-0xc79c0) and the CreateManipulator
// functions (Mac 0x9dea0 in the 025j range, 0xa2840 in the 025l range). Win 0x470330-0x470460,
// 0x475230-0x475ed0.
#include "ConnectionManipulator.h"
#include "PrimitiveElement.h"
#include "PrimitiveConnection.h"
#include "PrimitiveRigidConnection.h"
#include "PrimitiveRigidElement.h"
#include "RigidConnection.h"
#include "RigidElement.h"
#include "Scene.h"

namespace LEGO {

namespace {

// Field kinds from 999000 on are not real connections (not manipulated).
const UInt32 KIND_LIMIT = 999000;

// The first field connection of `connection` with a real field kind. (inline: Mac 0xc5940)
bool HasRealFieldConnection(const PrimitiveConnection* connection)
{
    const BasicArray<PrimitiveConnection::Connection>& list = connection->m_connections;
    for (UInt32 i = 0; i < list.GetSize(); ++i)
        if (list[i].m_field->GetKind() < KIND_LIMIT)
            return true;
    return false;
}

// Win 0x473e70 (Mac inlined into 0xa2840 / 0xc77c0)
bool HasRealFieldConnection(const PrimitiveRigidConnection* connection)
{
    for (UInt32 i = 0; i < connection->m_connections.GetSize(); ++i)
        if (HasRealFieldConnection(connection->m_connections[i]))
            return true;
    return false;
}

} // namespace

// ---------------------------------------------------------------------------------------------
// ConnectionManipulator
// ---------------------------------------------------------------------------------------------

// Win 0x470330 (Mac inlined)
ConnectionManipulator::ConnectionManipulator(Scene* scene)
    : m_connection(0), m_fixed(0), m_scene(scene), m_position(0.0)
{
}

// Mac 0xc40b0 / 0xc4150, Win 0x4703c0 / sdd 0x470460
ConnectionManipulator::~ConnectionManipulator()
{
}

// Mac 0x9cef0, Win 0x4753c0
void ConnectionManipulator::OnFinalRelease()
{
    Detach();
    Object::OnFinalRelease();
}

// Mac 0x9cc30, Win 0x475230
bool ConnectionManipulator::Rotate(bool interactive, Real32 delta)
{
    PhysicsHingeConnection* hinge = 0;
    if (m_connection->m_joint->QueryInterface(PhysicsHingeConnection::GetIID(), (void**)&hinge) < 0)
        return false;
    bool moved = hinge->Rotate(interactive, delta);
    if (moved) {
#if defined(_WIN32)
        m_position = (Real64)(delta + (Real32)m_position);
#else
        m_position = (Real64)delta + m_position;
#endif
    }
    if (m_scene)
        m_scene->NotifyObservers5();
    hinge->Release();
    return moved;
}

// Mac 0x9cd00, Win 0x4752b0
Real32 ConnectionManipulator::GetAngle() const
{
    PhysicsHingeConnection* hinge = 0;
    if (m_connection->m_joint->QueryInterface(PhysicsHingeConnection::GetIID(), (void**)&hinge) < 0)
        return 0.0f;
    Real32 angle = hinge->GetAngle();
    hinge->Release();
    return angle;
}

// Mac 0x9cda0, Win 0x475300
bool ConnectionManipulator::Slide(bool interactive, Real32 delta, const Vector3* axis)
{
    PhysicsBallConnection* slide = 0;
    if (m_connection->m_joint->QueryInterface(PhysicsBallConnection::GetIID(), (void**)&slide) < 0)
        return false;
    bool moved = slide->Rotate(interactive, delta, *axis);
    if (m_scene)
        m_scene->NotifyObservers5();
    slide->Release();
    return moved;
}

// Mac 0x9ce60, Win 0x475370
bool ConnectionManipulator::SlideUnknown(Real32* a, Real32* b, Real32* c)
{
    PhysicsBallConnection* slide = 0;
    if (m_connection->m_joint->QueryInterface(PhysicsBallConnection::GetIID(), (void**)&slide) >= 0) {
        slide->GetAngles(*a, *b, *c);
        slide->Release();
    }
    return false;                               // NOTE: false in both builds
}

// Mac 0x9cf10, Win 0x4753d0
void ConnectionManipulator::SetConnection(RigidConnection* connection, RigidElement* fixed)
{
    m_connection = connection;
    m_fixed = fixed;
    if (connection && connection->m_joint->GetType() == PrimitiveConnection::JointHinge)
        m_position = GetAngle();
    m_connections.Clear();
    m_elements.Clear();
    if (m_connection && m_fixed)
        m_scene->CollectManipulatedElements(m_connection, m_fixed, m_connections, m_elements);
}

// Win 0x475cc0 (Mac inlined into 0xc5810, 0xc7610, 0xc77c0)
void ConnectionManipulator::ResetConnection()
{
    if (m_connection == 0)
        return;
    m_connection = 0;
    m_fixed = 0;
    m_connections.Clear();
    m_elements.Clear();
    // (the original repeats SetConnection's collection here, never reached)
}

// Mac 0xc5570 / 0xc7700 without the element fields, Win 0x4758c0
void ConnectionManipulator::ResetAll()
{
    m_scene = 0;
    m_connection = 0;
    m_fixed = 0;
    m_connections.Clear();
    m_elements.Clear();
}

// ---------------------------------------------------------------------------------------------
// PrimitiveConnectionManipulator
// ---------------------------------------------------------------------------------------------

// Mac inlined into 0x9dea0
PrimitiveConnectionManipulator::PrimitiveConnectionManipulator(PrimitiveElement* element)
    : ConnectionManipulator(element->GetScene()), m_element(element),
      m_current(element->m_connections.Begin()->second),
      m_suspended(0),                           // NOTE: left uninitialised by the original
      m_released(false), m_changed(false)
{
}

// Mac 0xc53a0 / 0xc5440, Win sdd 0x475ba0
PrimitiveConnectionManipulator::~PrimitiveConnectionManipulator()
{
}

// Mac 0xc54f0, Win 0x475440
bool PrimitiveConnectionManipulator::SelectNext(bool* wrapped)
{
    if (m_connection == 0)
        return false;
    return Next(wrapped);
}

// Mac 0xc5510, Win 0x475870
void PrimitiveConnectionManipulator::Detach()
{
    if (m_scene == 0 || m_element == 0)
        return;
    Restore();
    if (m_scene && m_scene->m_manipulator == this)
        m_scene->m_manipulator = 0;
    if (m_changed)
        m_scene->NotifyRigidElementChanges();
    m_element = 0;
    m_current = 0;
}

// Mac 0xc5570, Win 0x4758c0
void PrimitiveConnectionManipulator::Reset()
{
    ResetAll();
    m_element = 0;
    m_current = 0;
}

// Mac 0xc5720
bool PrimitiveConnectionManipulator::Next(bool* wrapped)
{
    PrimitiveElement* element = m_element;
    PrimitiveConnection* start = m_current;
    if (element->m_connections.GetSize() != 1) {
        PrimitiveElement::ConnectionMap::Iterator it =
            element->m_connections.Find(start->GetOther(element));
        Restore();
        bool selected;
        do {
            ++it;
            if (it == m_element->m_connections.End())
                it = m_element->m_connections.Begin();
            m_current = it->second;
            selected = TrySelect();
        } while (!selected && start != m_current);
    }
    if (m_changed) {
        m_changed = false;
        m_scene->NotifyRigidElementChanges();
    }
    if (wrapped)
        *wrapped = start != m_current;
    return m_connection != 0;
}

// Mac 0xc5940
bool PrimitiveConnectionManipulator::TrySelect()
{
    PrimitiveConnection* connection = m_current;
    if (!HasRealFieldConnection(connection))
        return false;
    RigidElement* body = m_element->m_rigid->m_rigidElement;
    if (connection->m_rigidConnection == 0) {
        // a connection inside a primitive rigid element: only one held rigid by the
        // suspension state 2 can be released
        if (connection->m_suspended != 2)
            return false;
        m_suspended = 2;
        connection->SetSuspended(0);
        m_changed = true;
        m_scene->GetPhysicsScene()->Unknown6();
        Split();
        m_scene->GetPhysicsScene()->Unknown7();
        connection = m_current;
        if (connection->m_rigidConnection && connection->m_rigidConnection->m_rigidConnection) {
            SetConnection(connection->m_rigidConnection->m_rigidConnection,
                          m_element->m_rigid->m_rigidElement);
            m_released = true;
            return true;
        }
        connection->SetSuspended(m_suspended);
        m_scene->GetPhysicsScene()->Unknown6();
        Merge();
        m_scene->GetPhysicsScene()->Unknown7();
        return false;
    }
    RigidConnection* rigid = connection->m_rigidConnection->m_rigidConnection;
    if (rigid == 0)
        return false;
    if (rigid->m_joint->GetType() == PrimitiveConnection::JointFixed)
        return false;
    SetConnection(rigid, body);
    return true;
}

// Mac 0xc5810
void PrimitiveConnectionManipulator::Restore()
{
    ResetConnection();
    if (!m_released)
        return;
    m_scene->GetPhysicsScene()->Unknown6();
    m_current->m_rigidConnection->RemoveConnection(m_current);
    m_released = false;
    m_current->SetSuspended(m_suspended);
    Merge();
    m_scene->GetPhysicsScene()->Unknown7();
    m_changed = true;
}

// Mac 0xc5aa0, Win 0x475a70
void PrimitiveConnectionManipulator::Merge()
{
    BasicArray<PrimitiveRigidConnection*> changed;
    BasicArray<RigidConnection*> changedRigid;
    // NOTE: the original passes BasicArray<RefPointer<…>> instances here (RTTI of the locals);
    // the restored PrimitiveRigidElement::Merge takes plain pointer arrays.
    m_current->m_element->m_rigid->Merge(m_current->m_otherElement->m_rigid, changed, changedRigid);
    BasicArray<PrimitiveConnection*> connections;
    connections.Reserve(4);
    connections.PushBack(m_current);
    BasicArray<RefPointer<PrimitiveRigidConnection> > changedRefs;
    for (UInt32 i = 0; i < changed.GetSize(); ++i)
        changedRefs.PushBack(RefPointer<PrimitiveRigidConnection>(changed[i]));
    BasicArray<RefPointer<RigidConnection> > changedRigidRefs;
    for (UInt32 i = 0; i < changedRigid.GetSize(); ++i)
        changedRigidRefs.PushBack(RefPointer<RigidConnection>(changedRigid[i]));
    m_scene->UpdateConnections(connections, changedRefs, changedRigidRefs);
}

// Mac 0xc5d10, Win 0x475920
void PrimitiveConnectionManipulator::Split()
{
    BasicArray<PrimitiveRigidElement*> elements;
    elements.Reserve(4);
    elements.PushBack(m_current->m_element->m_rigid);
    PrimitiveRigidElement* other = m_current->m_otherElement->m_rigid;
    bool known = false;
    for (UInt32 i = 0; i < elements.GetSize(); ++i)
        if (elements[i] == other)
            known = true;
    if (!known)
        elements.PushBack(other);
    BasicArray<RigidElement*> bodies;
    BasicArray<HingedElement*> hinged;
    m_scene->SplitElements(elements, bodies, hinged);
}

// ---------------------------------------------------------------------------------------------
// RigidConnectionManipulator
// ---------------------------------------------------------------------------------------------

// Mac inlined into 0xa2840, Win 0x478d10
RigidConnectionManipulator::RigidConnectionManipulator(RigidElement* element)
    : ConnectionManipulator(element->GetScene()), m_element(element),
      m_current(element->m_connections.Begin()->second)
{
}

// Mac 0xc74a0 / 0xc7540, Win 0x475e20 / sdd 0x475eb0
RigidConnectionManipulator::~RigidConnectionManipulator()
{
}

// Mac 0xc75f0, Win 0x475bc0
bool RigidConnectionManipulator::SelectNext(bool* wrapped)
{
    if (m_connection == 0)
        return false;
    return Next(wrapped);
}

// Mac 0xc7610, Win 0x475e70
void RigidConnectionManipulator::Detach()
{
    if (m_scene == 0 || m_element == 0)
        return;
    if (m_scene->m_manipulator == this)
        m_scene->m_manipulator = 0;
    ResetConnection();
    m_element = 0;
    m_current = 0;
}

// Mac 0xc7700
void RigidConnectionManipulator::Reset()
{
    ResetAll();
    m_element = 0;
    m_current = 0;
}

// Win 0x475c70 (Mac inlined into 0xa2840 / 0xc77c0)
bool RigidConnectionManipulator::TrySelect()
{
    RigidConnection* connection = m_current;
    for (UInt32 i = 0; i < connection->m_connections.GetSize(); ++i) {
        if (!HasRealFieldConnection(connection->m_connections[i]))
            continue;
        if (connection->m_joint->GetType() == PrimitiveConnection::JointFixed)
            return false;
        SetConnection(connection, m_element);
        return true;
    }
    return false;
}

// Mac 0xc77c0, Win 0x475d20
bool RigidConnectionManipulator::Next(bool* wrapped)
{
    RigidElement* element = m_element;
    RigidConnection* start = m_current;
    if (element->m_connections.GetSize() != 1) {
        RigidElement* other = start->m_element == element ? start->m_otherElement : start->m_element;
        RigidElement::ConnectionMap::Iterator it = element->m_connections.Find(other);
        ResetConnection();
        bool selected;
        do {
            ++it;
            if (it == m_element->m_connections.End())
                it = m_element->m_connections.Begin();
            m_current = it->second;
            selected = TrySelect();
        } while (!selected && start != m_current);
    }
    if (wrapped)
        *wrapped = start != m_current;
    return m_connection != 0;
}

// ---------------------------------------------------------------------------------------------
// CreateManipulator
// ---------------------------------------------------------------------------------------------

// Mac 0x9dea0 (025j range)
ConnectionManipulator* PrimitiveElement::CreateManipulator()
{
    Scene* scene = GetScene();
    if (scene->m_manipulator)
        scene->m_manipulator->Detach();
    scene->m_manipulator = 0;
    if (m_connections.IsEmpty())
        return 0;
    PrimitiveConnectionManipulator* manipulator = new PrimitiveConnectionManipulator(this);
    if (scene->m_manipulator)
        scene->m_manipulator->Detach();
    scene->m_manipulator = manipulator;
    if (manipulator->TrySelect()) {
        if (manipulator->m_changed) {
            scene->NotifyRigidElementChanges();
            manipulator->m_changed = false;
        }
    } else if (!manipulator->Next(0)) {
        if (scene->m_manipulator)
            scene->m_manipulator->Detach();
        scene->m_manipulator = 0;
        manipulator->Release();                 // not referenced yet: deletes it
        return 0;
    }
    return manipulator;
}

// Mac 0xa2840 (025l range), Win 0x478d10 (+ 0x475c70)
ConnectionManipulator* RigidElement::CreateManipulator()
{
    Scene* scene = GetScene();
    if (scene->m_manipulator)
        scene->m_manipulator->Detach();
    scene->m_manipulator = 0;
    if (m_connections.IsEmpty())
        return 0;
    RigidConnectionManipulator* manipulator = new RigidConnectionManipulator(this);
    if (scene->m_manipulator)
        scene->m_manipulator->Detach();
    scene->m_manipulator = manipulator;
    if (manipulator->TrySelect())
        return manipulator;
    if (!manipulator->Next(0)) {
        if (scene->m_manipulator)
            scene->m_manipulator->Detach();
        scene->m_manipulator = 0;
        manipulator->Release();
        return 0;
    }
    return manipulator;
}

} // namespace LEGO
