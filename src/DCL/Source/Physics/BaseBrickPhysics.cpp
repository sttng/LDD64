// LEGO Digital Designer — restored source
// LEGO::BaseBrickPhysics (Mac TU 203, Win 0x5c4cb0-0x5c4d4f). See BaseBrickPhysics.h.
#include "BaseBrickPhysics.h"
#include "BaseBrickPhysicsAtomic.h"

#include <string.h>

namespace LEGO {

// Mac 0x251020, Win 0x5c4cb0
Int32 BaseBrickPhysics::QueryInterface(const InterfaceID& id, void** object)
{
    if (memcmp(&id, &BrickPhysics::GetIID(), sizeof id) == 0) {
        *object = this;
        AddRef();
        return 0;
    }
    *object = 0;
    return kPhysicsNoInterface;
}

// Mac 0x2510a0, Win 0x5c7750
UInt32 BaseBrickPhysics::AddRef()
{
    return (UInt32)PhysicsAtomicIncrement(m_refCount);
}

// Mac 0x2510c0, Win 0x5c4cf0
UInt32 BaseBrickPhysics::Release()
{
    Int32 count = PhysicsAtomicDecrement(m_refCount);
    if (count == 0)
        delete this;
    return (UInt32)count;
}

} // namespace LEGO
