// LEGO Digital Designer — restored source
// LEGO::VortexPhysicsFreeConnection (Mac TU 427). See VortexPhysicsFreeConnection.h.
#include "VortexPhysicsFreeConnection.h"
#include "VortexPhysicsConnectionCommon.h"

#include "Vx/VxConstraint.h"
#include "Vx/VxAssemblyBase.h"

namespace LEGO {

// Mac 0x3a2ee0, Win 0x51d530
void* VortexPhysicsFreeConnection::AddConstraint(const ConnectionField::Attributes& attributes)
{
    return m_attributeDatas.Add(this, attributes);
}

// Mac 0x3a2f10, Win 0x52cb10
void VortexPhysicsFreeConnection::RemoveConstraint(void* constraint)
{
    m_attributeDatas.Remove(this, constraint);
}

// Mac 0x3a2ea0 / 0x3a2ec0, Win 0x52a700
void* VortexPhysicsFreeConnection::GetNativeConstraint()
{
    if (m_constraint != 0)
        return m_constraint->getUserData();
    return 0;
}

// Mac 0x3a3040 / 0x3a3160, Win 0x4aede0
bool VortexPhysicsFreeConnection::HasNativeConstraint()
{
    return false;
}

// Mac 0x3a3050 / 0x3a3170, Win 0x51d240
bool VortexPhysicsFreeConnection::Unknown3()
{
    return m_attributeDatas.HasMotors();
}

// Mac 0x3a2f40 / 0x3a2f60, Win 0x52cb40
bool VortexPhysicsFreeConnection::UpdateConstraint()
{
    DestroyConstraint();
    return true;
}

// Mac 0x3a2f80 / 0x3a2fe0, Win 0x52aa00
void VortexPhysicsFreeConnection::DestroyConstraint()
{
    if (m_constraint != 0) {
        Vx::VxMechanism* mechanism = GetVortexScene(this)->m_mechanism;
        mechanism->removeConstraint(m_constraint);
        delete m_constraint;
        m_attributeDatas.DestroyConstraints(mechanism);
        m_constraint = 0;
    }
}

// Mac 0x3a3140, Win 0x51d680
Real32 VortexPhysicsFreeConnection::Unknown18()
{
    return m_attributeDatas.Unknown3b2440();
}

} // namespace LEGO
