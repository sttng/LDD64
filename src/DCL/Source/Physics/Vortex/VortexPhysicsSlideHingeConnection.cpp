// LEGO Digital Designer — restored source
// LEGO::VortexPhysicsSlideHingeConnection (Mac TU 429). See VortexPhysicsSlideHingeConnection.h.
#include "VortexPhysicsSlideHingeConnection.h"
#include "VortexPhysicsConnectionCommon.h"
#include "VortexConversions.h"

#include "Vx/VxCylindrical.h"
#include "Vx/VxMotorConstraint.h"
#include "Vx/VxAssemblyBase.h"

namespace LEGO {

// Mac 0x3a4820, Win 0x51d530
void* VortexPhysicsSlideHingeConnection::AddConstraint(const ConnectionField::Attributes& attributes)
{
    return m_attributeDatas.Add(this, attributes);
}

// Mac 0x3a4850, Win 0x52cb10
void VortexPhysicsSlideHingeConnection::RemoveConstraint(void* constraint)
{
    m_attributeDatas.Remove(this, constraint);
}

// Mac 0x3a4230 / 0x3a4250, Win 0x52a700
void* VortexPhysicsSlideHingeConnection::GetNativeConstraint()
{
    if (m_constraint != 0)
        return m_constraint->getUserData();
    return 0;
}

// Mac 0x3a4960 / 0x3a4990, Win 0x51cf20
bool VortexPhysicsSlideHingeConnection::HasNativeConstraint()
{
    return m_constraint != 0;
}

// Mac 0x3a4970 / 0x3a49a0, Win 0x51d240
bool VortexPhysicsSlideHingeConnection::Unknown3()
{
    return m_attributeDatas.HasMotors();
}

// Mac 0x3a4270 / 0x3a4740, Win 0x52aad0
bool VortexPhysicsSlideHingeConnection::UpdateConstraint()
{
    Vx::VxVector3 position = ToVxVector3(m_position);
    Vx::VxVector3 otherPosition = ToVxVector3(m_otherPosition);
    Vx::VxVector3 axis = ToVxVector3(m_axis);
    Vx::VxVector3 otherAxis = ToVxVector3(m_otherAxis);
    Vx::VxVector3 normal = ToVxVector3(m_normal);
    Vx::VxVector3 otherNormal = ToVxVector3(m_otherNormal);
    Vx::VxPart* part = GetVortexPart(m_body);
    Vx::VxPart* otherPart = GetVortexPart(m_otherBody);
    m_constraint = new Vx::VxCylindrical();
    m_constraint->setPartAndAttachmentRel(0, part, position, axis, normal);
    m_constraint->setPartAndAttachmentRel(1, otherPart, otherPosition, otherAxis, otherNormal);
    m_constraint->setUserData(static_cast<BaseBrickPhysicsConnectionHelper*>(this));
    Vx::VxMechanism* mechanism = GetVortexScene(this)->m_mechanism;
    mechanism->addConstraint(m_constraint);

    const Vx::VxReal scale = (Vx::VxReal)g_vortexConstraintStiffnessScale;
    for (VortexConnectionAttributeDatas::AttributesData* it = m_attributeDatas.m_datas.Begin();
         it != m_attributeDatas.m_datas.End(); ++it) {
        const ConnectionField::Attributes& a = it->m_attributes;
        // the rotation about the axle
        if (a.m_hasLinear || a.m_hasLimits) {
            Vx::VxMotorConstraint* motor = new Vx::VxMotorConstraint();
            mechanism->addConstraint(motor);
            it->m_constraint = motor;
            motor->setPartAndAttachmentRel(0, part, position, axis, normal);
            motor->setPartAndAttachmentRel(1, otherPart, otherPosition, otherAxis, otherNormal);
            if (a.m_hasLinear) {
                motor->setControl(0, Vx::VxConstraint::kControlMotorized);
                Vx::VxReal k = (Vx::VxReal)a.m_linearLimits.x * scale;
                motor->setMotorParameters(0, 0.0, 1e10, k, k * (Vx::VxReal)0.1f, 0.0);
            }
            if (a.m_hasLimits && a.m_lowerLimit.x < a.m_upperLimit.x)
                motor->setLimitPositions(0, a.m_lowerLimit.x, a.m_upperLimit.x);
        }
        // the slide along it (a linear motor)
        if (a.m_hasAngles || a.m_hasSpring) {
            Vx::VxMotorConstraint* motor = new Vx::VxMotorConstraint();
            motor->setAngular(0, false);
            mechanism->addConstraint(motor);
            it->m_constraint2 = motor;
            motor->setPartAndAttachmentRel(0, part, position, axis, normal);
            motor->setPartAndAttachmentRel(1, otherPart, otherPosition, otherAxis, otherNormal);
            // NOTE: no lower < upper test here (the original's)
            if (a.m_hasAngles)
                motor->setLimitPositions(0, a.m_lowerAngle.x, a.m_upperAngle.x);
            if (a.m_hasSpring) {
                motor->setControl(0, Vx::VxConstraint::kControlMotorized);
                Vx::VxReal k = (Vx::VxReal)a.m_damping.x * scale;
                motor->setMotorParameters(0, 0.0, 1e10, k, k * (Vx::VxReal)0.1f, 0.0);
            }
        }
    }
    return true;
}

// Mac 0x3a4760 / 0x3a47c0, Win 0x52aa00
void VortexPhysicsSlideHingeConnection::DestroyConstraint()
{
    if (m_constraint != 0) {
        Vx::VxMechanism* mechanism = GetVortexScene(this)->m_mechanism;
        mechanism->removeConstraint(m_constraint);
        delete m_constraint;
        m_attributeDatas.DestroyConstraints(mechanism);
        m_constraint = 0;
    }
}

} // namespace LEGO
