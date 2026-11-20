// LEGO Digital Designer — restored source
// LEGO::BaseBrickPhysicsConnectionBall (Mac TU 205). See BaseBrickPhysicsConnectionBall.h.
#include "BaseBrickPhysicsConnectionBall.h"
#include "BaseBrickPhysicsRigidElement.h"
#include "BaseBrickPhysicsAtomic.h"

#include <string.h>

namespace LEGO {

// Mac 0x2524e0, Win 0x5c3f90
Int32 BaseBrickPhysicsConnectionBall::QueryInterface(const InterfaceID& id, void** object)
{
    if (memcmp(&id, &PhysicsBallConnection::GetIID(), sizeof id) != 0 &&
        memcmp(&id, &PhysicsConnection::GetIID(), sizeof id) != 0) {
        *object = 0;
        return kPhysicsNoInterface;
    }
    *object = static_cast<PhysicsBallConnection*>(this);
    AddRef();
    return 0;
}

// Mac 0x2525b0, Win 0x5c43a0
UInt32 BaseBrickPhysicsConnectionBall::AddRef()
{
    return (UInt32)PhysicsAtomicIncrement(m_refCount);
}

// Mac 0x2525d0, Win 0x5c43c0
UInt32 BaseBrickPhysicsConnectionBall::Release()
{
    Int32 count = PhysicsAtomicDecrement(m_refCount);
    if (count == 0) {
        Detach();
        delete this;
    }
    return (UInt32)count;
}

// Mac 0x252ae0 / 0x252c20, Win 0x475ed0
UInt32 BaseBrickPhysicsConnectionBall::GetType() const
{
    return 5;
}

// Mac 0x252af0, Win 0x5c48d0
PhysicsRigidElement* BaseBrickPhysicsConnectionBall::GetRigidElement(bool first) const
{
    return first ? m_body : m_otherBody;
}

// Mac 0x252b10, Win 0x5c4400
void BaseBrickPhysicsConnectionBall::GetLocalFrame(bool first, Vector3* axis, Vector3* normal,
                                                   Vector3* position) const
{
    BaseBrickPhysicsConnectionHelper::GetLocalFrame(first, axis, normal, position);
}

// Mac 0x252b40, Win 0x51d030
void BaseBrickPhysicsConnectionBall::GetWorldFrame(Vector3* axis, Vector3* normal, Vector3* position) const
{
    BaseBrickPhysicsConnectionHelper::GetWorldFrame(axis, normal, position);
}

// Mac 0x252b60, Win 0x51d2f0
void BaseBrickPhysicsConnectionBall::SetFrames(PhysicsRigidElement* body, const Vector3& axis,
                                               const Vector3& normal, const Vector3& position,
                                               PhysicsRigidElement* otherBody, const Vector3& otherAxis,
                                               const Vector3& otherNormal, const Vector3& otherPosition)
{
    BaseBrickPhysicsConnectionHelper::SetFrames(static_cast<BaseBrickPhysicsRigidElement*>(body), axis, normal,
                                                position, static_cast<BaseBrickPhysicsRigidElement*>(otherBody),
                                                otherAxis, otherNormal, otherPosition);
}

// The rotation from the first body's joint frame to the second's, as Euler angles. Each frame
// is the matrix with the rows normal x axis, axis, normal (world space, the bodies' cached
// transforms). Mac 0x252620, Win 0x5c3ff0 (fully inlined matrix arithmetic)
void BaseBrickPhysicsConnectionBall::GetAngles(Real32& pitch, Real32& yaw, Real32& roll)
{
    const Matrix3D& transform = m_body->m_transform;
    const Matrix3D& otherTransform = m_otherBody->m_transform;

    Vector3 axis = LEGO::Rotate(m_axis, transform);
    Vector3 normal = LEGO::Rotate(m_normal, transform);
    Vector3 side = normal ^ axis;
    Matrix3D frame(side.x, axis.x, normal.x, 0.0f,
                   side.y, axis.y, normal.y, 0.0f,
                   side.z, axis.z, normal.z, 0.0f,
                   0.0f, 0.0f, 0.0f, 1.0f);

    Vector3 otherAxis = LEGO::Rotate(m_otherAxis, otherTransform);
    Vector3 otherNormal = LEGO::Rotate(m_otherNormal, otherTransform);
    Vector3 otherSide = otherNormal ^ otherAxis;
    Matrix3D otherFrame(otherSide.x, otherAxis.x, otherNormal.x, 0.0f,
                        otherSide.y, otherAxis.y, otherNormal.y, 0.0f,
                        otherSide.z, otherAxis.z, otherNormal.z, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f);

    Matrix3D inverse;
    Matrix3D unused0, unused1, unused2;   // NOTE: three more identity matrices, never used (Mac)
    inverse.SetInverse(frame);
    otherFrame.Multiply(inverse);
    otherFrame.GetEulerAngles(pitch, yaw, roll);
}

// Mac 0x2529e0 (helper slot 4: thunk 0x252ac0), Win 0x526060
UInt32 BaseBrickPhysicsConnectionBall::CheckSeparation(const PhysicsSeparationLimits& limits)
{
    Matrix3D transform;
    Matrix3D otherTransform;
    m_body->GetEngineTransform(transform);
    m_otherBody->GetEngineTransform(otherTransform);
    Vector3 position = m_position * transform;
    Vector3 otherPosition = m_otherPosition * otherTransform;
    Real32 distance = position.GetDistanceSquared(otherPosition);
    if (limits.m_distance > distance) {
        m_separationCount = 0;
        return 0;
    }
    if (limits.m_maxDistance > distance) {
        m_separationCount = 0;
        return 1;
    }
    ++m_separationCount;
    return (m_separationCount >= limits.m_breakCount) + 1;
}

} // namespace LEGO
