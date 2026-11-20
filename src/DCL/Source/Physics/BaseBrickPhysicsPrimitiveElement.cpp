// LEGO Digital Designer — restored source
// LEGO::BaseBrickPhysicsPrimitiveElement (Mac TU 209). See BaseBrickPhysicsPrimitiveElement.h.
#include "BaseBrickPhysicsPrimitiveElement.h"
#include "BaseBrickPhysicsAtomic.h"

#include <string.h>

namespace LEGO {

// Mac 0x253790, Win 0x5c7710
Int32 BaseBrickPhysicsPrimitiveElement::QueryInterface(const InterfaceID& id, void** object)
{
    if (memcmp(&id, &PhysicsPrimitiveElement::GetIID(), sizeof id) == 0) {
        *object = this;
        AddRef();
        return 0;
    }
    *object = 0;
    return kPhysicsNoInterface;
}

// Mac 0x253810, Win 0x5c7750
UInt32 BaseBrickPhysicsPrimitiveElement::AddRef()
{
    return (UInt32)PhysicsAtomicIncrement(m_refCount);
}

// Mac 0x253830, Win 0x5c7770
UInt32 BaseBrickPhysicsPrimitiveElement::Release()
{
    Int32 count = PhysicsAtomicDecrement(m_refCount);
    if (count == 0)
        delete this;
    return (UInt32)count;
}

// Mac 0x253870, Win 0x52f4c0
const PhysicsMassProperties& BaseBrickPhysicsPrimitiveElement::GetMassProperties() const
{
    return m_massProperties;
}

// Mac 0x253880, Win 0x46ee40
PrimitiveElement* BaseBrickPhysicsPrimitiveElement::GetUserData() const
{
    return m_userData;
}

// Mac 0x253890, Win 0x52aee0
void BaseBrickPhysicsPrimitiveElement::SetUserData(PrimitiveElement* element)
{
    m_userData = element;
}

} // namespace LEGO
