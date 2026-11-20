// LEGO Digital Designer — restored source
// LEGO::PrimitiveRigidConnection (see PrimitiveRigidConnection.h).
//
// Original object file: Scene.cpp (Mac TU 025); this is sub-TU 025g, Mac 0x994f0-0x9b2a0 (+ 0x97500
// / 0x98fa0 from 025f), Win 0x473c40-0x474720. The Mac range also holds
// RigidConnection::RemoveConnection (0x998d0) and RigidConnection::Update (0x99f00) (in
// RigidConnection.cpp), the BasicMap erase-by-key instance 0x99980, BasicArray instances (0x99d00
// erase, 0x9a6c0 range insert, 0x9b240 / 0x9b270 destructors) and the destructor thunks 0x9b0a0 /
// 0x9b1a0.
#include "PrimitiveRigidConnection.h"
#include "PrimitiveConnection.h"
#include "PrimitiveElement.h"
#include "PrimitiveRigidElement.h"
#include "RigidConnection.h"
#include "SceneGraphUtil.h"
#include "MathUtil.h"

namespace LEGO {

namespace {
const Real32 kLineEpsilon     = 0.01f;
const Real32 kParallelEpsilon = 0.001f;
const Real32 kBallDistance    = 0.4f;
} // namespace

// Mac 0x994f0, Win 0x473aa0
PrimitiveRigidConnection::PrimitiveRigidConnection(PrimitiveConnection* connection)
    : m_element(connection->m_element->m_rigid),
      m_otherElement(connection->m_otherElement->m_rigid), m_rigidConnection(0),
      m_type(PrimitiveConnection::JointRigid)
{
    // NOTE: m_type is left uninitialised by the original constructor (Update sets it).
    m_element->m_connections[m_otherElement] = this;
    m_otherElement->m_connections[m_element] = this;
    m_connections.PushBack(connection);
    connection->m_rigidConnection = this;
    Update();
}

// Mac 0x9b030 (complete) / 0x9b120 (deleting), thunks 0x9b0a0 / 0x9b1a0; Win 0x4744f0 /
// sdd 0x473c40
PrimitiveRigidConnection::~PrimitiveRigidConnection()
{
}

// Mac 0x99ab0 (Win inlined into 0x477de0)
bool PrimitiveRigidConnection::AddConnection(PrimitiveConnection* connection)
{
    m_connections.PushBack(connection);
    connection->m_rigidConnection = this;
    UInt32 type = m_type;
    Update();
    return type != m_type || !connection->m_attributes.IsEmpty();
}

// Win 0x473c70 (Mac inlined into RemoveConnection and the merge / split code)
void PrimitiveRigidConnection::Clear()
{
    for (BasicArray<PrimitiveConnection*>::Iterator it = m_connections.Begin();
         it != m_connections.End(); ++it)
        (*it)->m_rigidConnection = 0;
    m_connections.Clear();
    if (m_rigidConnection != 0) {
        m_rigidConnection->RemoveConnection(this);
        m_rigidConnection = 0;
    }
    PrimitiveRigidElement* element = m_element;
    PrimitiveRigidElement* otherElement = m_otherElement;
    m_otherElement = 0;
    m_element = 0;
    // NOTE: the second erase may release the last reference to this connection.
    element->m_connections.Erase(otherElement);
    otherElement->m_connections.Erase(element);
}

// Mac 0x97500 (025f), Win 0x473d80
bool PrimitiveRigidConnection::RemoveConnection(PrimitiveConnection* connection)
{
    connection->m_rigidConnection = 0;
    SceneGraph::EraseValueUnordered(m_connections, connection);
    if (m_connections.IsEmpty()) {
        Clear();
        return true;
    }
    UInt32 type = m_type;
    Update();
    if (m_rigidConnection != 0 && (type != m_type || !connection->m_attributes.IsEmpty()))
        m_rigidConnection->Update();
    return false;
}

// Mac 0x9a560, Win 0x473e00
bool PrimitiveRigidConnection::Merge(PrimitiveRigidConnection* other)
{
    SceneGraph::Append(m_connections, other->m_connections);
    for (BasicArray<PrimitiveConnection*>::Iterator it = other->m_connections.Begin();
         it != other->m_connections.End(); ++it)
        (*it)->m_rigidConnection = this;
    // NOTE: the original reads other->m_attributes after other->Clear(), which may have
    // released the last reference to `other` (use after free in both builds); the value is
    // taken before here.
    bool otherHasAttributes = !other->m_attributes.IsEmpty();
    other->m_connections.Clear();
    other->Clear();

    UInt32 type = m_type;
    Update();
    if (m_rigidConnection != 0 && (type != m_type || otherHasAttributes))
        m_rigidConnection->Update();
    return type != m_type;
}

// Mac 0x9af30, Win 0x473fd0
void PrimitiveRigidConnection::SetRigidElements(PrimitiveRigidElement* element,
                                                PrimitiveRigidElement* otherElement)
{
    PrimitiveRigidElement* oldElement = m_element;
    PrimitiveRigidElement* oldOtherElement = m_otherElement;
    m_element = element;
    m_otherElement = otherElement;
    element->m_connections[otherElement] = this;
    otherElement->m_connections[element] = this;
    oldElement->m_connections.Erase(oldOtherElement);
    oldOtherElement->m_connections.Erase(oldElement);
    Update();
}

// Mac 0x9ae70, Win 0x473ec0
void PrimitiveRigidConnection::GetFrame(const PrimitiveRigidElement* element, Vector3& axis,
                                        Vector3& normal, Vector3& position) const
{
    const Matrix3D& transform = element->m_transform;
    if (m_element == element) {
        axis = Rotate(m_axis, transform);
        normal = Rotate(m_normal, transform);
        position = m_position * transform;
    } else {
        axis = Rotate(m_otherAxis, transform);
        normal = Rotate(m_otherNormal, transform);
        position = m_otherPosition * transform;
    }
}

// The element of `connection` that belongs to `rigid` (the other one otherwise). (inline)
static inline PrimitiveElement* SideOf(const PrimitiveConnection* connection,
                                       const PrimitiveRigidElement* rigid)
{
    return connection->m_element->m_rigid == rigid ? connection->m_element : connection->m_otherElement;
}

// Mac 0x98fa0 (025f), Win 0x4740b0
void PrimitiveRigidConnection::Update()
{
    PrimitiveConnection** it = m_connections.Begin();
    PrimitiveConnection* connection = *it;
    m_attributes.Clear();
    m_type = connection->m_type;
    if (m_type == PrimitiveConnection::JointRigid)
        return;

    connection->GetFrame(SideOf(connection, m_element), m_axis, m_normal, m_position);
    connection->GetFrame(SideOf(connection, m_otherElement), m_otherAxis, m_otherNormal, m_otherPosition);
    for (++it; it != m_connections.End(); ++it) {
        connection = *it;
        UInt32 type = connection->m_type;
        if (type == PrimitiveConnection::JointRigid) {
            m_type = PrimitiveConnection::JointRigid;
            return;
        }
        Vector3 axis;
        Vector3 normal;
        Vector3 position;
        connection->GetFrame(SideOf(connection, m_element), axis, normal, position);
        bool valid = true;
        switch (m_type) {
        case PrimitiveConnection::JointBall:
            if (type == PrimitiveConnection::JointHinge) {
                valid = position.IsOnLine(axis, m_position, kLineEpsilon);
                if (valid) {
                    m_type = PrimitiveConnection::JointHinge;
                    m_axis = axis;
                    m_normal = normal;
                    m_position = position;
                    connection->GetFrame(SideOf(connection, m_otherElement), m_otherAxis,
                                         m_otherNormal, m_otherPosition);
                }
            } else if (type == PrimitiveConnection::JointBall) {
                if (Distance(m_position, position) >= kBallDistance) {
                    m_type = PrimitiveConnection::JointHinge;
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
            } else if (type == PrimitiveConnection::JointSlide) {
                valid = false;
            }
            break;
        case PrimitiveConnection::JointSlide:
            if (type == PrimitiveConnection::JointSlide) {
                // NOTE: the two sides' axes of this connection (see PrimitiveConnection::Update)
                Real32 d = m_axis * m_otherAxis;
                valid = IsEqual(d, 1.0f, kParallelEpsilon) || IsEqual(d, -1.0f, kParallelEpsilon);
            } else if (type == PrimitiveConnection::JointBall || type == PrimitiveConnection::JointHinge) {
                valid = false;
            }
            break;
        case PrimitiveConnection::JointHinge:
            if (type == PrimitiveConnection::JointHinge)
                valid = m_position.IsSameLine(m_axis, position, axis, kLineEpsilon);
            else if (type == PrimitiveConnection::JointBall)
                valid = m_position.IsOnLine(m_axis, position, kLineEpsilon);
            else if (type == PrimitiveConnection::JointSlide)
                valid = false;
            break;
        }
        if (!valid) {
            m_type = PrimitiveConnection::JointRigid;
            return;
        }
    }

    // the joint attributes of every field connection, frames in the rigid elements
    for (it = m_connections.Begin(); it != m_connections.End(); ++it) {
        connection = *it;
        for (const ConnectionField::Attributes* attributes = connection->m_attributes.Begin();
             attributes != connection->m_attributes.End(); ++attributes) {
            ConnectionField::Attributes transformed;
            transformed.m_isFlexible = false;
            attributes->Transform(connection->m_element->m_rigid != m_element,
                                  connection->m_element->m_transform,
                                  connection->m_otherElement->m_transform, transformed);
            m_attributes.PushBack(transformed);
        }
    }
}

} // namespace LEGO
