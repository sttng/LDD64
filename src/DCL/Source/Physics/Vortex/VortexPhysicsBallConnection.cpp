// LEGO Digital Designer — restored source
// LEGO::VortexPhysicsBallConnection (Mac TU 426). See VortexPhysicsBallConnection.h.
#include "VortexPhysicsBallConnection.h"
#include "VortexPhysicsConnectionCommon.h"
#include "VortexConversions.h"
#include "Object.h"

#include <math.h>

#include "Vx/VxHinge.h"
#include "Vx/VxMotorConstraint.h"
#include "Vx/VxConeConstraint.h"
#include "Vx/VxAssemblyBase.h"

namespace LEGO {

namespace {
const Real32 kHalfPi = 1.5707964f;       // Mac 0x63c440 / -0x63c340
const Real32 kSmallDelta = 1e-05f;       // Mac 0x62f294
}

// Mac 0x3a2b80, Win 0x51d530
void* VortexPhysicsBallConnection::AddConstraint(const ConnectionField::Attributes& attributes)
{
    return m_attributeDatas.Add(this, attributes);
}

// Mac 0x3a2bb0, Win 0x52cb10
void VortexPhysicsBallConnection::RemoveConstraint(void* constraint)
{
    m_attributeDatas.Remove(this, constraint);
}

// Mac 0x3a1f40, Win 0x42e9e0
bool VortexPhysicsBallConnection::Unknown10()
{
    return false;
}

// Mac 0x3a1f50, Win 0x52c0b0
bool VortexPhysicsBallConnection::Rotate(bool interactive, Real32 delta, const Vector3& axis)
{
    if (-kHalfPi > delta || delta > kHalfPi)
        return false;
    if (kSmallDelta > (Real32)fabs(delta))
        return true;
    BaseBrickPhysicsRigidElement* body = interactive ? m_body : m_otherBody;
    BaseBrickPhysicsRigidElement* otherBody = interactive ? m_otherBody : m_body;
    Matrix3D transform;
    Matrix3D inverse;
    Matrix3D rotation;
    rotation.SetRotation(axis, delta);
    otherBody->GetTransform(transform);
    inverse.SetInverse(transform);
    body->GetTransform(transform);
    Vector3 perpendicular = axis.GetPerpendicular();
    perpendicular.Normalize();
    Vector3 normal(perpendicular);
    // the frame of the turned body expressed in the other body
    Vector3 otherAxis = LEGO::Rotate(LEGO::Rotate(axis, transform), inverse);
    Vector3 otherNormal = LEGO::Rotate(LEGO::Rotate(LEGO::Rotate(normal, rotation), transform), inverse);
    VortexPhysicsScene* scene = GetVortexScene(this);
    BaseBrickPhysicsBaseHingeSolver* solver = scene->GetHingeSolver(this, interactive);
    BasicArray<BaseBrickPhysicsConnectionHelper*>* hinges = GetSolverHinges(solver);
    const Vector3& position = interactive ? m_position : m_otherPosition;
    const Vector3& otherPosition = interactive ? m_otherPosition : m_position;
    RefPointer<VortexHingeSolverHinge> hinge = scene->BuildHingeSolverHinge(
        true, body, axis, normal, position, otherBody, otherAxis, otherNormal, otherPosition, hinges);
    // NOTE: no NULL check of the solver (the original's)
    return solver->Solve(delta, axis, position, hinge.Get());
}

// Mac 0x3a2250, Win 0x52c3a0
bool VortexPhysicsBallConnection::RotateSimulated(bool interactive, Real32 delta, const Vector3& axis)
{
    if (-kHalfPi > delta || delta > kHalfPi)
        return false;
    BaseBrickPhysicsRigidElement* body = interactive ? m_body : m_otherBody;
    BaseBrickPhysicsRigidElement* otherBody = interactive ? m_otherBody : m_body;
    Matrix3D transform;
    Matrix3D inverse;
    Matrix3D rotation;
    rotation.SetRotation(axis, delta);
    otherBody->GetTransform(transform);
    inverse.SetInverse(transform);
    body->GetTransform(transform);
    Vector3 perpendicular = axis.GetPerpendicular();
    perpendicular.Normalize();
    Vector3 normal(perpendicular);
    Vector3 otherAxis = LEGO::Rotate(LEGO::Rotate(axis, transform), inverse);
    Vector3 otherNormal = LEGO::Rotate(LEGO::Rotate(LEGO::Rotate(normal, rotation), transform), inverse);
    VortexPhysicsScene* scene = GetVortexScene(this);
    BaseBrickPhysicsBaseHingeSolver* solver = scene->GetHingeSimulateSolver(this, interactive);
    BasicArray<BaseBrickPhysicsConnectionHelper*>* hinges = GetSolverHinges(solver);
    const Vector3& position = interactive ? m_position : m_otherPosition;
    const Vector3& otherPosition = interactive ? m_otherPosition : m_position;
    RefPointer<VortexHingeSolverHinge> hinge = scene->BuildHingeSolverHinge(
        true, body, axis, normal, position, otherBody, otherAxis, otherNormal, otherPosition, hinges);
    return solver->Solve(delta, axis, position, hinge.Get());
}

// Mac 0x3a2cc0 / 0x3a2d30, Win 0x52a700
void* VortexPhysicsBallConnection::GetNativeConstraint()
{
    if (m_constraint != 0)
        return m_constraint->getUserData();
    return 0;
}

// Mac 0x3a2ce0 / 0x3a2d50, Win 0x51cf20
bool VortexPhysicsBallConnection::HasNativeConstraint()
{
    return m_constraint != 0;
}

// Mac 0x3a2cf0 / 0x3a2d60, Win 0x51d240
bool VortexPhysicsBallConnection::Unknown3()
{
    return m_attributeDatas.HasMotors();
}

// Mac 0x3a2d10, Win 0x4aede0
bool VortexPhysicsBallConnection::Unknown20()
{
    return m_attributeDatas.Unknown3b2430();
}

// Mac 0x3a2520 / 0x3a2aa0, Win 0x52c660
bool VortexPhysicsBallConnection::UpdateConstraint()
{
    Vx::VxVector3 position = ToVxVector3(m_position);
    Vx::VxVector3 otherPosition = ToVxVector3(m_otherPosition);
    Vx::VxVector3 axis = ToVxVector3(m_axis);
    Vx::VxVector3 otherAxis = ToVxVector3(m_otherAxis);
    Vx::VxVector3 normal = ToVxVector3(m_normal);
    Vx::VxVector3 otherNormal = ToVxVector3(m_otherNormal);
    Vx::VxPart* part = GetVortexPart(m_body);
    Vx::VxPart* otherPart = GetVortexPart(m_otherBody);
    Vx::VxHinge* hinge = new Vx::VxHinge();
    m_constraint = hinge;
    hinge->setPositionEquationReferencePart(1);
    m_constraint->setPartAndAttachmentRel(0, part, position, axis, normal);
    m_constraint->setPartAndAttachmentRel(1, otherPart, otherPosition, otherAxis, otherNormal);
    m_constraint->setUserData(static_cast<BaseBrickPhysicsConnectionHelper*>(this));
    Vx::VxMechanism* mechanism = GetVortexScene(this)->m_mechanism;
    mechanism->addConstraint(m_constraint);

    const Vx::VxReal scale = (Vx::VxReal)g_vortexConstraintStiffnessScale;
    Vx::VxReal stiffness = 1.0;
    for (VortexConnectionAttributeDatas::AttributesData* it = m_attributeDatas.m_datas.Begin();
         it != m_attributeDatas.m_datas.End(); ++it) {
        const ConnectionField::Attributes& a = it->m_attributes;
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
            Vx::VxReal k = (Vx::VxReal)a.m_linearLimits.y * scale;
            stiffness = k > stiffness ? k : stiffness;
            if (0.0f <= a.m_twist && (Vx::VxReal)a.m_twist < 3.141592653589793) {
                Vx::VxConeConstraint* cone = new Vx::VxConeConstraint();
                mechanism->addConstraint(cone);
                it->m_constraint2 = cone;
                cone->setPartAndAttachmentRel(0, part, position, axis, normal);
                cone->setPartAndAttachmentRel(1, otherPart, otherPosition, otherAxis, otherNormal);
                cone->setConeAngle(a.m_twist);
            }
        }
        // the two angular rows of the hinge become springs (every entry, the stiffest so far)
        hinge->setRelaxationParameters(4, stiffness, stiffness + stiffness, 0.0, true);
        hinge->setRelaxationForceLimits(4, -1e10, 1e10);
        hinge->setRelaxationParameters(3, stiffness, stiffness + stiffness, 0.0, true);
        hinge->setRelaxationForceLimits(3, -1e10, 1e10);
    }
    return true;
}

// Mac 0x3a2ac0 / 0x3a2b20, Win 0x52cac0
void VortexPhysicsBallConnection::DestroyConstraint()
{
    if (m_constraint != 0) {
        Vx::VxMechanism* mechanism = GetVortexScene(this)->m_mechanism;
        mechanism->removeConstraint(m_constraint);
        m_attributeDatas.DestroyConstraints(mechanism);
        delete m_constraint;
        m_constraint = 0;
    }
}

} // namespace LEGO
