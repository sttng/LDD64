// LEGO Digital Designer — restored source
// LEGO::PrimitiveConnection (see PrimitiveConnection.h).
//
// Original object file: Scene.cpp (Mac TU 025); this is sub-TU 025f, Mac 0x97310-0x994f0, Win
// 0x472ef0-0x473c40. The Mac range also holds PrimitiveElement::DisconnectFields (0x97310, in
// PrimitiveElement.cpp), PrimitiveRigidConnection::RemoveConnection (0x97500) / Update (0x98fa0)
// (in PrimitiveRigidConnection.cpp), the BasicMap<PrimitiveElement*,
// RefPointer<PrimitiveConnection>> erase-by-key instance (0x975d0) and BasicArray instances
// (0x97ad0 PushBack<Connection>, 0x98ea0 PushBack<Attributes>, 0x987f0 / 0x98820 / 0x994c0
// destructors).
#include "PrimitiveConnection.h"
#include "PrimitiveElement.h"
#include "PrimitiveRigidConnection.h"
#include "ConnectionFieldAxle.h"
#include "MathUtil.h"

namespace LEGO {

namespace {
const Real32 kLineEpsilon     = 0.01f;    // IsOnLine / IsSameLine (0x3c23d70a)
const Real32 kParallelEpsilon = 0.001f;   // Mac 0x62fff4
const Real32 kBallDistance    = 0.4f;     // Mac 0x62fff0: two balls this far apart form a hinge

bool HasJointAttributes(const ConnectionField::Attributes& attributes)
{
    return attributes.m_hasLinear || attributes.m_hasSpring || attributes.m_hasLimits ||
           attributes.m_hasAngles;
}
} // namespace

// Mac 0x977c0
PrimitiveConnection::PrimitiveConnection(const PrimitiveConnectionInfo& info,
                                         const ConnectionField::Attributes& attributes)
    : m_element(info.m_element), m_otherElement(info.m_otherElement), m_rigidConnection(0),
      m_type(JointRigid), m_suspended(0)
{
    // NOTE: m_type is left uninitialised by the original constructor (Update sets it).
    m_element->m_connections[m_otherElement] = this;
    m_otherElement->m_connections[m_element] = this;

    Connection connection;
    connection.m_field = info.m_field;
    connection.m_otherField = info.m_otherField;
    connection.m_indices = info.m_indices;
    connection.m_attributes = attributes;
    // Almost every connection of two elements has one field connection: room for that one
    // instead of PushBack's first 4 (0xd0 bytes each; 720 bytes saved per connection in x64)
    m_connections.Reserve(1);
    m_connections.PushBack(connection);
    Update();
}

// Mac 0x98890 (complete) / 0x98930 (deleting), Win 0x4731e0 / sdd 0x472ef0
PrimitiveConnection::~PrimitiveConnection()
{
}

// Mac 0x989f0, Win 0x473280
bool PrimitiveConnection::AddConnection(const PrimitiveConnectionInfo& info,
                                        const ConnectionField::Attributes& attributes)
{
    m_connections.PushBack(Connection());
    Connection& connection = m_connections.Back();
    connection.m_indices = info.m_indices;
    if (info.m_element == m_element) {
        connection.m_field = info.m_field;
        connection.m_otherField = info.m_otherField;
        connection.m_attributes = attributes;
    } else {
        connection.m_field = info.m_otherField;
        connection.m_otherField = info.m_field;
        for (ConnectionField::ConnectionIndices::Iterator it = connection.m_indices.Begin();
             it != connection.m_indices.End(); ++it) {
            UInt32 index = it->m_index;
            it->m_index = it->m_otherIndex;
            it->m_otherIndex = index;
        }
        Matrix3D identity;
        Matrix3D otherIdentity;
        attributes.Transform(true, identity, otherIdentity, connection.m_attributes);
    }
    UInt32 type = m_type;
    Update();
    return type != m_type || HasJointAttributes(attributes);
}

// Inlined (Mac 0x982c0 tables 0x987b4 / 0x987cc, Win 0x472ad0)
UInt32 PrimitiveConnection::GetJointType(const Connection& connection)
{
    switch (connection.m_field->GetType()) {
    case ConnectionField::TYPE_PLANAR:
    case ConnectionField::TYPE_AXLE:
        return connection.m_field->IsRigidConnection(*connection.m_otherField, connection.m_indices)
                   ? JointRigid : JointHinge;
    case ConnectionField::TYPE_HINGE:
        return JointHinge;
    case ConnectionField::TYPE_BALL:
        return JointBall;
    case ConnectionField::TYPE_RAIL:
        return JointSlide;
    default:    // fixed, slider
        return JointRigid;
    }
}

// Mac 0x982c0, Win 0x473420
void PrimitiveConnection::Update()
{
    if (m_suspended != 0)
        return;
    m_attributes.Clear();

    Connection* connection = m_connections.Begin();
    m_type = GetJointType(*connection);
    if (m_type == JointRigid)
        return;
    connection->m_field->GetFrame(true, *connection->m_otherField, connection->m_indices,
                                  m_axis, m_normal, m_position);
    UpdateFrame(*connection);

    for (++connection; connection != m_connections.End(); ++connection) {
        UInt32 type = GetJointType(*connection);
        if (type == JointRigid) {
            m_type = JointRigid;
            return;
        }
        Vector3 axis;
        Vector3 normal;
        Vector3 position;
        connection->m_field->GetFrame(true, *connection->m_otherField, connection->m_indices,
                                      axis, normal, position);
        bool valid = true;
        switch (m_type) {
        case JointBall:
            if (type == JointHinge) {
                // a hinge through the ball centre
                valid = position.IsOnLine(axis, m_position, kLineEpsilon);
                if (valid) {
                    m_type = JointHinge;
                    m_axis = axis;
                    m_normal = normal;
                    m_position = position;
                    UpdateFrame(*connection);
                }
            } else if (type == JointBall) {
                // two balls far enough apart: a hinge through both centres
                if (Distance(m_position, position) >= kBallDistance) {
                    m_type = JointHinge;
                    m_axis = position - m_position;
                    m_axis.Normalize();
                    m_normal = m_axis.GetPerpendicular();
                    m_normal.Normalize();
                    connection->m_field->GetFrame(false, *connection->m_otherField,
                                                  connection->m_indices, axis, normal, position);
                    m_otherAxis = position - m_otherPosition;
                    m_otherAxis.Normalize();
                    m_otherNormal = m_otherAxis.GetPerpendicular();
                    m_otherNormal.Normalize();
                }
            } else if (type == JointSlide) {
                valid = false;
            }
            break;
        case JointSlide:
            if (type == JointSlide) {
                // NOTE: compares the two sides' axes of this connection, not the new
                // connection's axis (both builds).
                Real32 d = m_axis * m_otherAxis;
                valid = IsEqual(d, 1.0f, kParallelEpsilon) || IsEqual(d, -1.0f, kParallelEpsilon);
            } else if (type == JointBall || type == JointHinge) {
                valid = false;
            }
            break;
        case JointHinge:
            if (type == JointHinge)
                valid = m_position.IsSameLine(m_axis, position, axis, kLineEpsilon);
            else if (type == JointBall)
                valid = m_position.IsOnLine(m_axis, position, kLineEpsilon);
            else if (type == JointSlide)
                valid = false;
            break;
        }
        if (!valid) {
            m_type = JointRigid;
            return;
        }
    }

    for (Connection* c = m_connections.Begin(); c != m_connections.End(); ++c)
        if (HasJointAttributes(c->m_attributes))
            m_attributes.PushBack(c->m_attributes);
}

// Mac 0x98c80, Win 0x4737e0
void PrimitiveConnection::UpdateFrame(const Connection& connection)
{
    connection.m_field->GetFrame(false, *connection.m_otherField, connection.m_indices,
                                 m_otherAxis, m_otherNormal, m_otherPosition);
    if (connection.m_field->GetType() != ConnectionField::TYPE_AXLE)
        return;

    // axles: the joint sits at the closest points of the two axle segments
    Matrix3D transform;
    Matrix3D otherTransform;
    m_element->GetTransform(transform);
    m_otherElement->GetTransform(otherTransform);
    transform.Multiply(connection.m_field->GetTransform());
    otherTransform.Multiply(connection.m_otherField->GetTransform());

    const Vector3 up(0.0f, 1.0f, 0.0f);
    const Vector3& origin = transform.GetTranslation();
    Vector3 direction = Rotate(up, transform);
    Real32 length = static_cast<const ConnectionFieldLineHelper*>(connection.m_field)->GetLength();
    const Vector3& otherOrigin = otherTransform.GetTranslation();
    Vector3 otherDirection = Rotate(up, otherTransform);
    Real32 otherLength = static_cast<const ConnectionFieldLineHelper*>(connection.m_otherField)->GetLength();
    Real32 distance = 0.0f;
    Real32 otherDistance = 0.0f;
    SegmentSegmentClosest(origin, direction, length, otherOrigin, otherDirection, otherLength,
                          distance, otherDistance);
    m_position = (up * distance) * connection.m_field->GetTransform();
    m_otherPosition = (up * otherDistance) * connection.m_otherField->GetTransform();
}

// Mac 0x97700
void PrimitiveConnection::GetFrame(const PrimitiveElement* element, Vector3& axis, Vector3& normal,
                                   Vector3& position) const
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

// Win 0x473a70 (Mac inlined)
void PrimitiveConnection::SetSuspended(UInt32 suspended)
{
    if (m_suspended == suspended)
        return;
    m_suspended = suspended;
    if (suspended != 0) {
        m_type = JointRigid;
        m_attributes.Clear();
    } else {
        Update();
    }
}

} // namespace LEGO
