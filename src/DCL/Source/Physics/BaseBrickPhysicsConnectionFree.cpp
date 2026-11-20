// LEGO Digital Designer — restored source
// LEGO::BaseBrickPhysicsConnectionFree (Mac TU 438). See BaseBrickPhysicsConnectionFree.h.
#include "BaseBrickPhysicsConnectionFree.h"
#include "BaseBrickPhysicsRigidElement.h"
#include "BaseBrickPhysicsAtomic.h"

#include <string.h>

namespace LEGO {

// Mac 0x3b5b30 (Win inlined into VortexBrickPhysics slot 17)
BaseBrickPhysicsConnectionFree::BaseBrickPhysicsConnectionFree(BaseBrickPhysicsRigidElement* body,
                                                               BaseBrickPhysicsRigidElement* otherBody)
    : BaseBrickPhysicsConnectionHelper(body, Vector3(), Vector3(), Vector3(), otherBody, Vector3(), Vector3(),
                                       Vector3()),
      m_refCount(1)
{
}

// Mac 0x3b5900, Win 0x5c4870
Int32 BaseBrickPhysicsConnectionFree::QueryInterface(const InterfaceID& id, void** object)
{
    if (memcmp(&id, &PhysicsFreeConnection::GetIID(), sizeof id) != 0 &&
        memcmp(&id, &PhysicsConnection::GetIID(), sizeof id) != 0) {
        *object = 0;
        return kPhysicsNoInterface;
    }
    *object = static_cast<PhysicsFreeConnection*>(this);
    AddRef();
    return 0;
}

// Mac 0x3b59d0, Win 0x5c43a0
UInt32 BaseBrickPhysicsConnectionFree::AddRef()
{
    return (UInt32)PhysicsAtomicIncrement(m_refCount);
}

// Mac 0x3b59f0, Win 0x5c43c0
UInt32 BaseBrickPhysicsConnectionFree::Release()
{
    Int32 count = PhysicsAtomicDecrement(m_refCount);
    if (count == 0) {
        Detach();
        delete this;
    }
    return (UInt32)count;
}

// Mac 0x3b5a40 / 0x3b5a50, Win 0x51e880
UInt32 BaseBrickPhysicsConnectionFree::GetType() const
{
    return 6;
}

// Mac 0x3b5a60, Win 0x5c48d0
PhysicsRigidElement* BaseBrickPhysicsConnectionFree::GetRigidElement(bool first) const
{
    return first ? m_body : m_otherBody;
}

// Mac 0x3b5a80, Win 0x5c4400
void BaseBrickPhysicsConnectionFree::GetLocalFrame(bool first, Vector3* axis, Vector3* normal,
                                                   Vector3* position) const
{
    BaseBrickPhysicsConnectionHelper::GetLocalFrame(first, axis, normal, position);
}

// Mac 0x3b5ab0, Win 0x51d030
void BaseBrickPhysicsConnectionFree::GetWorldFrame(Vector3* axis, Vector3* normal, Vector3* position) const
{
    BaseBrickPhysicsConnectionHelper::GetWorldFrame(axis, normal, position);
}

// Mac 0x3b5ad0, Win 0x51d2f0
void BaseBrickPhysicsConnectionFree::SetFrames(PhysicsRigidElement* body, const Vector3& axis,
                                               const Vector3& normal, const Vector3& position,
                                               PhysicsRigidElement* otherBody, const Vector3& otherAxis,
                                               const Vector3& otherNormal, const Vector3& otherPosition)
{
    BaseBrickPhysicsConnectionHelper::SetFrames(static_cast<BaseBrickPhysicsRigidElement*>(body), axis, normal,
                                                position, static_cast<BaseBrickPhysicsRigidElement*>(otherBody),
                                                otherAxis, otherNormal, otherPosition);
}

// Mac 0x3b5b10, Win 0x4f9670
void* BaseBrickPhysicsConnectionFree::AddConstraint(const ConnectionField::Attributes& /*attributes*/)
{
    return 0;
}

// Mac 0x3b5b20, Win 0x40be80
void BaseBrickPhysicsConnectionFree::RemoveConstraint(void* /*constraint*/)
{
}

// Mac 0x3b5d30 / 0x3b5d80, Win 0x51e7a0
void* BaseBrickPhysicsConnectionFree::GetNativeConstraint()
{
    return 0;
}

// Mac 0x3b5d40 / 0x3b5d90, Win 0x4aede0
bool BaseBrickPhysicsConnectionFree::HasNativeConstraint()
{
    return false;
}

// Mac 0x3b5d50 / 0x3b5da0, Win 0x4aede0
bool BaseBrickPhysicsConnectionFree::Unknown3()
{
    return false;
}

// Mac 0x3b5d60 / 0x3b5db0, Win 0x4f5b70
bool BaseBrickPhysicsConnectionFree::UpdateConstraint()
{
    return true;
}

// Mac 0x3b5d70 / 0x3b5dc0, Win 0x459790
void BaseBrickPhysicsConnectionFree::DestroyConstraint()
{
}

} // namespace LEGO
