// LEGO Digital Designer — restored source
// LEGO::BaseBrickPhysicsConnectionHelper (Mac TU 204). See BaseBrickPhysicsConnectionHelper.h.
#include "BaseBrickPhysicsConnectionHelper.h"
#include "BaseBrickPhysicsRigidElement.h"
#include "BaseBrickPhysicsScene.h"
#include "BaseBrickPhysics.h"

namespace LEGO {

// Mac 0x251890, Win 0x5c5160
BaseBrickPhysicsConnectionHelper::BaseBrickPhysicsConnectionHelper(
    BaseBrickPhysicsRigidElement* body, const Vector3& axis, const Vector3& normal,
    const Vector3& position, BaseBrickPhysicsRigidElement* otherBody, const Vector3& otherAxis,
    const Vector3& otherNormal, const Vector3& otherPosition)
    : m_mark(0), m_separationCount(0), m_body(body), m_otherBody(otherBody), m_scene(body->m_scene),
      m_axis(axis), m_otherAxis(otherAxis), m_position(position), m_otherPosition(otherPosition),
      m_normal(normal), m_otherNormal(otherNormal), m_dirty(false)
{
    m_scene->AddConnection(this);
    m_body->m_connections.PushBack(this);
    m_otherBody->m_connections.PushBack(this);
}

// Mac 0x251d70 (Win inlined into the Vortex spring mirrors 0x525f80 / 0x526d30)
BaseBrickPhysicsConnectionHelper::BaseBrickPhysicsConnectionHelper(const BaseBrickPhysicsConnectionHelper& other)
    : m_mark(0), m_separationCount(0), m_body(other.m_body), m_otherBody(other.m_otherBody),
      m_scene(other.m_body->m_scene), m_axis(other.m_axis), m_otherAxis(other.m_otherAxis),
      m_position(other.m_position), m_otherPosition(other.m_otherPosition), m_normal(other.m_normal),
      m_otherNormal(other.m_otherNormal), m_dirty(false)
{
    m_scene->AddConnection(this);
    m_body->m_connections.PushBack(this);
    m_otherBody->m_connections.PushBack(this);
}

// Mac 0x2511b0 (Win inlined into the joints' Release, e.g. 0x5c43c0)
void BaseBrickPhysicsConnectionHelper::Detach()
{
    DestroyConstraint();
    PhysicsArrayRemove(m_body->m_connections, this);
    PhysicsArrayRemove(m_otherBody->m_connections, this);
    m_scene->RemoveConnection(this);
    m_otherBody = 0;
    m_body = 0;
    m_scene = 0;
}

// Mac 0x251220 (Win 0x5c4400 via the joints' slot 5)
void BaseBrickPhysicsConnectionHelper::GetLocalFrame(bool first, Vector3* axis, Vector3* normal,
                                                     Vector3* position) const
{
    if (axis != 0)
        *axis = first ? m_axis : m_otherAxis;
    if (normal != 0)
        *normal = first ? m_normal : m_otherNormal;
    if (position != 0)
        *position = first ? m_position : m_otherPosition;
}

// Mac 0x2512a0 (Win 0x51d030 via the joints' slot 6)
void BaseBrickPhysicsConnectionHelper::GetWorldFrame(Vector3* axis, Vector3* normal, Vector3* position) const
{
    Matrix3D transform;
    m_body->GetTransform(transform);
    if (axis != 0)
        *axis = LEGO::Rotate(m_axis, transform);
    if (normal != 0)
        *normal = LEGO::Rotate(m_normal, transform);
    if (position != 0)
        *position = m_position * transform;
}

// Mac 0x251350 (Win 0x51d2f0 via the joints' slot 7)
void BaseBrickPhysicsConnectionHelper::SetFrames(BaseBrickPhysicsRigidElement* body, const Vector3& axis,
                                                 const Vector3& normal, const Vector3& position,
                                                 BaseBrickPhysicsRigidElement* otherBody,
                                                 const Vector3& otherAxis, const Vector3& otherNormal,
                                                 const Vector3& otherPosition)
{
    DestroyConstraint();
    PhysicsArrayRemove(m_body->m_connections, this);
    PhysicsArrayRemove(m_otherBody->m_connections, this);
    m_body = body;
    m_otherBody = otherBody;
    m_axis = axis;
    m_otherAxis = otherAxis;
    m_position = position;
    m_otherPosition = otherPosition;
    m_normal = normal;
    m_otherNormal = otherNormal;
    m_body->m_connections.PushBack(this);
    m_otherBody->m_connections.PushBack(this);
    MarkDirty();
}

// Mac 0x251860, Win inlined (0x5c5160)
void BaseBrickPhysicsConnectionHelper::MarkDirty()
{
    if (!m_dirty) {
        m_scene->MarkConnectionDirty(this);
        m_dirty = true;
    }
}

} // namespace LEGO
