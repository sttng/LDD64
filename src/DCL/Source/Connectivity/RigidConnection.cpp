// LEGO Digital Designer — restored source
// LEGO::RigidConnection (see RigidConnection.h).
//
// Original object file: Scene.cpp (Mac TU 025); this is sub-TU 025h, Mac 0x9b2a0-0x9cc30 (+
// RemoveConnection 0x998d0 and Update 0x99f00 from 025g), Win 0x474720-0x475230. The Mac range also
// holds the BasicMap erase-by-key instance 0x9b680, BasicArray instances (0x9ba30 unordered erase,
// 0x9bda0 range insert) and the destructor thunks 0x9ca90 / 0x9cb90.
#include "RigidConnection.h"
#include "PrimitiveConnection.h"
#include "PrimitiveRigidConnection.h"
#include "PrimitiveRigidElement.h"
#include "RigidElement.h"
#include "HingedElement.h"
#include "Scene.h"
#include "SceneGraphUtil.h"
#include "MathUtil.h"

namespace LEGO {

namespace {
const Real32 kLineEpsilon     = 0.01f;
const Real32 kParallelEpsilon = 0.001f;
const Real32 kBallDistance    = 0.4f;

// The primitive rigid element of `connection` that belongs to `body` (the other one otherwise).
// (inline)
inline PrimitiveRigidElement* SideOf(const PrimitiveRigidConnection* connection, const RigidElement* body)
{
    return connection->m_element->m_rigidElement == body ? connection->m_element
                                                         : connection->m_otherElement;
}
} // namespace

// Mac 0x9b2a0, Win 0x474580
RigidConnection::RigidConnection(PrimitiveRigidConnection* connection)
    : m_element(connection->m_element->m_rigidElement),
      m_otherElement(connection->m_otherElement->m_rigidElement), m_joint(0), m_flag68(false)
{
    m_element->m_connections[m_otherElement] = this;
    m_otherElement->m_connections[m_element] = this;
    m_connections.PushBack(connection);
    connection->m_rigidConnection = this;
    Update();
}

// Mac 0x9ca20 (complete) / 0x9cb10 (deleting), thunks 0x9ca90 / 0x9cb90; Win 0x474ba0 /
// sdd 0x474720
RigidConnection::~RigidConnection()
{
}

// Mac 0x9b7b0, Win 0x474860
bool RigidConnection::AddConnection(PrimitiveRigidConnection* connection)
{
    if (connection->m_rigidConnection == 0) {
        m_connections.PushBack(connection);
        connection->m_rigidConnection = this;
    }
    UInt32 type = GetJointType();
    Update();
    return type != GetJointType() || !connection->m_attributes.IsEmpty();
}

// Win 0x474750 (Mac inlined into RemoveConnection and Merge)
void RigidConnection::Clear()
{
    for (BasicArray<PrimitiveRigidConnection*>::Iterator it = m_connections.Begin();
         it != m_connections.End(); ++it)
        (*it)->m_rigidConnection = 0;
    m_connections.Clear();
    if (m_joint != 0) {
        // NOTE: the constraint handles are dropped without RemoveConstraint (the joint goes).
        m_joint->Release();
        m_joint = 0;
        m_constraints.Clear();
    }
    RigidElement* element = m_element;
    RigidElement* otherElement = m_otherElement;
    m_otherElement = 0;
    m_element = 0;
    // NOTE: the second erase may release the last reference to this connection.
    element->m_connections.Erase(otherElement);
    otherElement->m_connections.Erase(element);
}

// Mac 0x998d0 (025g), Win 0x4748d0
bool RigidConnection::RemoveConnection(PrimitiveRigidConnection* connection)
{
    connection->m_rigidConnection = 0;
    SceneGraph::EraseValueUnordered(m_connections, connection);
    if (m_connections.IsEmpty()) {
        Clear();
        return true;
    }
    Update();
    return false;
}

// Mac 0x9bc30, Win 0x474920
bool RigidConnection::Merge(RigidConnection* other)
{
    SceneGraph::Append(m_connections, other->m_connections);
    for (BasicArray<PrimitiveRigidConnection*>::Iterator it = other->m_connections.Begin();
         it != other->m_connections.End(); ++it)
        (*it)->m_rigidConnection = this;
    other->m_connections.Clear();
    other->Clear();

    UInt32 type = GetJointType();
    Update();
    return type != GetJointType();
}

// Mac 0x9c550, Win 0x474a70
void RigidConnection::SetRigidElements(RigidElement* element, RigidElement* otherElement)
{
    RigidElement* oldElement = m_element;
    RigidElement* oldOtherElement = m_otherElement;
    m_element = element;
    m_otherElement = otherElement;
    element->m_connections[otherElement] = this;
    otherElement->m_connections[element] = this;
    oldElement->m_connections.Erase(oldOtherElement);
    oldOtherElement->m_connections.Erase(oldElement);

    if (m_joint != 0 && m_joint->GetType() != 0) {
        Update();
        // NOTE: Update may have released the joint (rigid now) — both builds then call
        // through a null pointer; the rebuilt or kept joint gets the new frames.
        m_joint->SetFrames(m_element->m_body, m_axis, m_normal, m_position,
                           m_otherElement->m_body, m_otherAxis, m_otherNormal, m_otherPosition);
        UpdateConstraints();
    }
}

// Win 0x4749a0 (Mac inlined)
void RigidConnection::GetFrame(const RigidElement* element, Vector3& axis, Vector3& normal,
                               Vector3& position) const
{
    if (m_element == element) {
        axis = m_axis;
        normal = m_normal;
        position = m_position;
    } else {
        axis = m_otherAxis;
        normal = m_otherNormal;
        position = m_otherPosition;
    }
}

// Mac 0x99f00 (025g), Win 0x474c30
void RigidConnection::Update()
{
    PrimitiveRigidConnection** it = m_connections.Begin();
    PrimitiveRigidConnection* connection = *it;
    UInt32 type = connection->m_type;
    if (type != PrimitiveConnection::JointRigid) {
        connection->GetFrame(SideOf(connection, m_element), m_axis, m_normal, m_position);
        connection->GetFrame(SideOf(connection, m_otherElement), m_otherAxis, m_otherNormal,
                             m_otherPosition);
        for (++it; type != PrimitiveConnection::JointRigid && it != m_connections.End(); ++it) {
            connection = *it;
            UInt32 next = connection->m_type;
            if (next == PrimitiveConnection::JointRigid) {
                type = PrimitiveConnection::JointRigid;
                break;
            }
            Vector3 axis;
            Vector3 normal;
            Vector3 position;
            connection->GetFrame(SideOf(connection, m_element), axis, normal, position);
            switch (type) {
            case PrimitiveConnection::JointBall:
                if (next == PrimitiveConnection::JointHinge) {
                    if (!position.IsOnLine(axis, m_position, kLineEpsilon)) {
                        type = PrimitiveConnection::JointRigid;
                    } else {
                        type = PrimitiveConnection::JointHinge;
                        m_axis = axis;
                        m_normal = normal;
                        m_position = position;
                        connection->GetFrame(SideOf(connection, m_otherElement), m_otherAxis,
                                             m_otherNormal, m_otherPosition);
                    }
                } else if (next == PrimitiveConnection::JointBall) {
                    if (Distance(m_position, position) >= kBallDistance) {
                        type = PrimitiveConnection::JointHinge;
                        m_axis = position - m_position;
                        m_axis.Normalize();
                        m_normal = m_axis.GetPerpendicular();
                        m_normal.Normalize();
                        connection->GetFrame(SideOf(connection, m_otherElement), axis, normal, position);
                        m_otherAxis = position - m_otherPosition;
                        m_otherAxis.Normalize();
                        m_otherNormal = m_otherAxis.GetPerpendicular();
                        m_otherNormal.Normalize();
                    }
                } else if (next == PrimitiveConnection::JointSlide) {
                    type = PrimitiveConnection::JointRigid;
                }
                break;
            case PrimitiveConnection::JointSlide:
                if (next == PrimitiveConnection::JointSlide) {
                    // NOTE: the two sides' axes of this connection (see PrimitiveConnection::Update)
                    Real32 d = m_axis * m_otherAxis;
                    if (!IsEqual(d, 1.0f, kParallelEpsilon) && !IsEqual(d, -1.0f, kParallelEpsilon))
                        type = PrimitiveConnection::JointRigid;
                } else if (next == PrimitiveConnection::JointBall || next == PrimitiveConnection::JointHinge) {
                    type = PrimitiveConnection::JointRigid;
                }
                break;
            case PrimitiveConnection::JointHinge:
                if (next == PrimitiveConnection::JointHinge) {
                    if (!m_position.IsSameLine(m_axis, position, axis, kLineEpsilon))
                        type = PrimitiveConnection::JointRigid;
                } else if (next == PrimitiveConnection::JointBall) {
                    if (!m_position.IsOnLine(m_axis, position, kLineEpsilon))
                        type = PrimitiveConnection::JointRigid;
                } else if (next == PrimitiveConnection::JointSlide) {
                    type = PrimitiveConnection::JointRigid;
                }
                break;
            }
        }
    }
    m_axis.Normalize();
    m_normal.Normalize();
    m_otherAxis.Normalize();
    m_otherNormal.Normalize();

    if (type != GetJointType()) {
        if (m_joint != 0) {
            m_joint->Release();
            m_joint = 0;
            m_constraints.Clear();
        }
        BrickPhysics* physics = m_element->m_hinged->GetScene()->GetBrickPhysics();
        PhysicsRigidElement* body = m_element->m_body;
        PhysicsRigidElement* otherBody = m_otherElement->m_body;
        switch (type) {
        case PrimitiveConnection::JointHinge:
            m_joint = physics->CreateHingeConnection(body, m_axis, m_normal, m_position, otherBody,
                                                     m_otherAxis, m_otherNormal, m_otherPosition);
            break;
        case PrimitiveConnection::JointSlide:
            m_joint = physics->CreateSlideHingeConnection(body, m_axis, m_normal, m_position,
                                                          otherBody, m_otherAxis, m_otherNormal,
                                                          m_otherPosition);
            break;
        case PrimitiveConnection::JointBall:
            m_joint = physics->CreateBallConnection(body, m_axis, m_normal, m_position, otherBody,
                                                    m_otherAxis, m_otherNormal, m_otherPosition);
            break;
        case PrimitiveConnection::JointFixed:
            m_joint = physics->CreateFixedConnection(body, otherBody);
            break;
        default:
            return;
        }
    }
    if (m_joint != 0)
        UpdateConstraints();
}

// Mac 0x9c6e0, Win 0x475140
void RigidConnection::UpdateConstraints()
{
    for (void** it = m_constraints.Begin(); it != m_constraints.End(); ++it)
        m_joint->RemoveConstraint(*it);
    m_constraints.Clear();

    for (PrimitiveRigidConnection** it = m_connections.Begin(); it != m_connections.End(); ++it) {
        PrimitiveRigidConnection* connection = *it;
        for (const ConnectionField::Attributes* attributes = connection->m_attributes.Begin();
             attributes != connection->m_attributes.End(); ++attributes) {
            ConnectionField::Attributes transformed;
            attributes->TransformFrames(connection->m_element->m_rigidElement != m_element,
                                        connection->m_element->m_transform,
                                        connection->m_otherElement->m_transform, transformed);
            m_constraints.PushBack(m_joint->AddConstraint(transformed));
        }
    }
}

} // namespace LEGO
