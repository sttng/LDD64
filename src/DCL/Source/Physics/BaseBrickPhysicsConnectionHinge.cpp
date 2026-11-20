// LEGO Digital Designer — restored source
// LEGO::BaseBrickPhysicsConnectionHinge (Mac TU 206). See BaseBrickPhysicsConnectionHinge.h.
#include "BaseBrickPhysicsConnectionHinge.h"
#include "BaseBrickPhysicsRigidElement.h"
#include "BaseBrickPhysicsAtomic.h"
#include "MathUtil.h"

#include <math.h>
#include <string.h>

namespace LEGO {

// Mac 0x2531e0, Win 0x5c4630
BaseBrickPhysicsConnectionHinge::BaseBrickPhysicsConnectionHinge(
    BaseBrickPhysicsRigidElement* body, const Vector3& axis, const Vector3& normal, const Vector3& position,
    BaseBrickPhysicsRigidElement* otherBody, const Vector3& otherAxis, const Vector3& otherNormal,
    const Vector3& otherPosition)
    : BaseBrickPhysicsConnectionHelper(body, axis, normal, position, otherBody, otherAxis, otherNormal, otherPosition),
      m_refCount(1)
{
}

// Mac 0x252d00, Win 0x5c4340
Int32 BaseBrickPhysicsConnectionHinge::QueryInterface(const InterfaceID& id, void** object)
{
    if (memcmp(&id, &PhysicsHingeConnection::GetIID(), sizeof id) != 0 &&
        memcmp(&id, &PhysicsConnection::GetIID(), sizeof id) != 0) {
        *object = 0;
        return kPhysicsNoInterface;
    }
    *object = static_cast<PhysicsHingeConnection*>(this);
    AddRef();
    return 0;
}

// Mac 0x252dd0, Win 0x5c43a0
UInt32 BaseBrickPhysicsConnectionHinge::AddRef()
{
    return (UInt32)PhysicsAtomicIncrement(m_refCount);
}

// Mac 0x252df0, Win 0x5c43c0
UInt32 BaseBrickPhysicsConnectionHinge::Release()
{
    Int32 count = PhysicsAtomicDecrement(m_refCount);
    if (count == 0) {
        Detach();
        delete this;
    }
    return (UInt32)count;
}

// Mac 0x252e40 / 0x252e50, Win 0x51e780
UInt32 BaseBrickPhysicsConnectionHinge::GetType() const
{
    return 1;
}

// Mac 0x252e60, Win 0x5c48d0
PhysicsRigidElement* BaseBrickPhysicsConnectionHinge::GetRigidElement(bool first) const
{
    return first ? m_body : m_otherBody;
}

// Mac 0x252e80, Win 0x5c4400
void BaseBrickPhysicsConnectionHinge::GetLocalFrame(bool first, Vector3* axis, Vector3* normal,
                                                    Vector3* position) const
{
    BaseBrickPhysicsConnectionHelper::GetLocalFrame(first, axis, normal, position);
}

// Mac 0x252eb0, Win 0x51d030
void BaseBrickPhysicsConnectionHinge::GetWorldFrame(Vector3* axis, Vector3* normal, Vector3* position) const
{
    BaseBrickPhysicsConnectionHelper::GetWorldFrame(axis, normal, position);
}

// Mac 0x252ed0, Win 0x51d2f0
void BaseBrickPhysicsConnectionHinge::SetFrames(PhysicsRigidElement* body, const Vector3& axis,
                                                const Vector3& normal, const Vector3& position,
                                                PhysicsRigidElement* otherBody, const Vector3& otherAxis,
                                                const Vector3& otherNormal, const Vector3& otherPosition)
{
    BaseBrickPhysicsConnectionHelper::SetFrames(static_cast<BaseBrickPhysicsRigidElement*>(body), axis, normal,
                                                position, static_cast<BaseBrickPhysicsRigidElement*>(otherBody),
                                                otherAxis, otherNormal, otherPosition);
}

// Mac 0x252f10, Win 0x5c4430
bool BaseBrickPhysicsConnectionHinge::SetAngle(bool interactive, Real32 angle)
{
    return Rotate(interactive, angle - GetAngle());
}

// The angle from the first normal to the second about the first axis (cached transforms):
// acos of the normals' dot product (0 above 0.99999, PI below -0.99999), negative when
// (normal x otherNormal) . axis < 0. Mac 0x252f60, Win 0x5c4470
Real32 BaseBrickPhysicsConnectionHinge::GetAngle()
{
    const Matrix3D& transform = m_body->m_transform;
    const Matrix3D& otherTransform = m_otherBody->m_transform;
    Vector3 axis = LEGO::Rotate(m_axis, transform);
    Vector3 normal = LEGO::Rotate(m_normal, transform);
    Vector3 otherNormal = LEGO::Rotate(m_otherNormal, otherTransform);
    Vector3 cross = normal ^ otherNormal;
    Real32 cosine = normal * otherNormal;
    Real32 angle = 0.0f;
    if (!(cosine > 0.99999f)) {
        if (-0.99999f > cosine)
            angle = PI;
        else
            angle = (Real32)acos((double)cosine);
    }
    if (cross * axis < 0.0f)
        angle = -angle;
    return angle;
}

// Mac 0x253070 (helper thunk 0x2531c0), Win 0x5255e0
UInt32 BaseBrickPhysicsConnectionHinge::CheckSeparation(const PhysicsSeparationLimits& limits)
{
    Matrix3D transform;
    Matrix3D otherTransform;
    m_body->GetEngineTransform(transform);
    m_otherBody->GetEngineTransform(otherTransform);
    Vector3 axis = LEGO::Rotate(m_axis, transform);
    Vector3 otherAxis = LEGO::Rotate(m_otherAxis, otherTransform);
    Vector3 position = m_position * transform;
    Vector3 otherPosition = m_otherPosition * otherTransform;
    Real32 alignment = (Real32)fabs(axis * otherAxis);
    Real32 distance = position.GetDistanceSquared(otherPosition);
    if (alignment > limits.m_alignment && limits.m_distance > distance) {
        m_separationCount = 0;
        return 0;
    }
    if (alignment > limits.m_minAlignment && limits.m_maxDistance > distance) {
        m_separationCount = 0;
        return 1;
    }
    ++m_separationCount;
    return (m_separationCount >= limits.m_breakCount) + 1;
}

} // namespace LEGO
