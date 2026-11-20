// LEGO Digital Designer — restored source
// LEGO::BaseBrickPhysicsConnectionSlideHinge (Mac TU 207). See BaseBrickPhysicsConnectionSlideHinge.h.
#include "BaseBrickPhysicsConnectionSlideHinge.h"
#include "BaseBrickPhysicsRigidElement.h"
#include "BaseBrickPhysicsAtomic.h"

#include <string.h>

namespace LEGO {

// Mac 0x2533d0, Win 0x5c4b80
Int32 BaseBrickPhysicsConnectionSlideHinge::QueryInterface(const InterfaceID& id, void** object)
{
    if (memcmp(&id, &PhysicsSlideHingeConnection::GetIID(), sizeof id) != 0 &&
        memcmp(&id, &PhysicsConnection::GetIID(), sizeof id) != 0) {
        *object = 0;
        return kPhysicsNoInterface;
    }
    *object = static_cast<PhysicsSlideHingeConnection*>(this);
    AddRef();
    return 0;
}

// Mac 0x2534a0, Win 0x5c43a0
UInt32 BaseBrickPhysicsConnectionSlideHinge::AddRef()
{
    return (UInt32)PhysicsAtomicIncrement(m_refCount);
}

// Mac 0x2534c0, Win 0x5c43c0
UInt32 BaseBrickPhysicsConnectionSlideHinge::Release()
{
    Int32 count = PhysicsAtomicDecrement(m_refCount);
    if (count == 0) {
        Detach();
        delete this;
    }
    return (UInt32)count;
}

// Mac 0x253510 / 0x253660, Win 0x51e840. NOTE: 4, not RigidConnection's slide type 2
UInt32 BaseBrickPhysicsConnectionSlideHinge::GetType() const
{
    return 4;
}

// Mac 0x253520, Win 0x5c48d0
PhysicsRigidElement* BaseBrickPhysicsConnectionSlideHinge::GetRigidElement(bool first) const
{
    return first ? m_body : m_otherBody;
}

// Mac 0x253540, Win 0x5c4400
void BaseBrickPhysicsConnectionSlideHinge::GetLocalFrame(bool first, Vector3* axis, Vector3* normal,
                                                         Vector3* position) const
{
    BaseBrickPhysicsConnectionHelper::GetLocalFrame(first, axis, normal, position);
}

// Mac 0x253570, Win 0x51d030
void BaseBrickPhysicsConnectionSlideHinge::GetWorldFrame(Vector3* axis, Vector3* normal, Vector3* position) const
{
    BaseBrickPhysicsConnectionHelper::GetWorldFrame(axis, normal, position);
}

// Mac 0x253590, Win 0x51d2f0
void BaseBrickPhysicsConnectionSlideHinge::SetFrames(PhysicsRigidElement* body, const Vector3& axis,
                                                     const Vector3& normal, const Vector3& position,
                                                     PhysicsRigidElement* otherBody, const Vector3& otherAxis,
                                                     const Vector3& otherNormal, const Vector3& otherPosition)
{
    BaseBrickPhysicsConnectionHelper::SetFrames(static_cast<BaseBrickPhysicsRigidElement*>(body), axis, normal,
                                                position, static_cast<BaseBrickPhysicsRigidElement*>(otherBody),
                                                otherAxis, otherNormal, otherPosition);
}

// Mac 0x2535d0 / 0x253670, Win 0x4f9670
UInt32 BaseBrickPhysicsConnectionSlideHinge::CheckSeparation(const PhysicsSeparationLimits& /*limits*/)
{
    return 0;
}

} // namespace LEGO
