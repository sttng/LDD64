// LEGO Digital Designer — restored source
// LEGO::VortexPhysicsHingeConnection (Mac TU 428). See VortexPhysicsHingeConnection.h.
#include "VortexPhysicsHingeConnection.h"
#include "VortexPhysicsConnectionCommon.h"
#include "VortexConversions.h"
#include "Object.h"

#include "Vx/VxHinge.h"
#include "Vx/VxMotorConstraint.h"
#include "Vx/VxAssemblyBase.h"
#include "Vx/VxMessage.h"

#include <math.h>

namespace LEGO {

namespace {

const Real32 kPi = 3.1415927f;           // Mac 0x62f298 / -0x62f29c
const Real32 kTwoPi = 6.2831855f;        // Mac 0x630014 / -0x63f720 (fmodf argument 0x40c90fdb)
const Real32 kSmallDelta = 1e-05f;       // Mac 0x62f294
const Real32 kParallel = 0.99999f;       // Mac 0x62ffe8 / -0x632c70

// The angle wrapped into [-PI, PI] (fmodf by 2 PI, then one period). Inlined (Mac), Win 0x529dd0.
inline Real32 WrapAngle(Real32 angle)
{
    Real32 a = (Real32)fmodf(angle, kTwoPi);
    if (a > kPi)
        a += -kTwoPi;
    else if (-kPi > a)
        a += kTwoPi;
    return a;
}

// The signed angle between two normals about `axis` (acos of the cosine, the sign of
// (normal ^ other) . axis); 0 / PI beyond +-0.99999. Inlined twice in Rotate.
inline Real32 SignedAngle(const Vector3& normal, const Vector3& other, const Vector3& axis)
{
    Real32 side = (normal ^ other) * axis;
    Real32 c = normal * other;
    Real32 angle;
    if (c > kParallel)
        angle = 0.0f;
    else if (-kParallel > c)
        angle = kPi;
    else
        angle = (Real32)acos((double)c);
    return side < 0.0f ? -angle : angle;
}

} // namespace

// Mac 0x3a3fb0, Win 0x51d530
void* VortexPhysicsHingeConnection::AddConstraint(const ConnectionField::Attributes& attributes)
{
    return m_attributeDatas.Add(this, attributes);
}

// Mac 0x3a3fe0, Win 0x52cb10
void VortexPhysicsHingeConnection::RemoveConstraint(void* constraint)
{
    m_attributeDatas.Remove(this, constraint);
}

// Mac 0x3a32a0, Win 0x529e30
bool VortexPhysicsHingeConnection::Rotate(bool interactive, Real32 delta)
{
    const Int32 kIterations = 1;   // the "%d iteration" of the log (Win keeps a loop of one pass)
    Real32 d = WrapAngle(delta);
    Vx::VxInfo(0, "VortexPhysicsHingeConnection::Change: Desired delta=%.3f, %d iteration of %.3f", (double)d,
               kIterations, (double)d);
    BaseBrickPhysicsRigidElement* const* moving = interactive ? &m_body : &m_otherBody;
    Real32 current = GetAngle();
    Real32 target = WrapAngle(d + current);
    // the joint limits of the attributes (their frames relative to the joint's)
    for (const VortexConnectionAttributeDatas::AttributesData* it = m_attributeDatas.m_datas.Begin();
         it != m_attributeDatas.m_datas.End(); ++it) {
        const ConnectionField::Attributes& a = it->m_attributes;
        if (!a.m_hasLimits)
            continue;
        Real32 angle = SignedAngle(m_normal, a.m_normal, m_axis);
        Real32 otherAngle = SignedAngle(m_otherNormal, a.m_otherNormal, m_otherAxis);
        Real32 lower = (a.m_lowerLimit.x - angle) - otherAngle;
        Real32 upper = (a.m_upperLimit.x - angle) - otherAngle;
        Real32 t = lower > target ? lower : target;   // maxss
        target = upper < t ? upper : t;               // minss
    }
    Vector3 axis;
    Vector3 position;
    GetLocalFrame(interactive, &axis, 0, &position);
    Real32 step = target - current;
    if (interactive)
        step = -step;
    if (kSmallDelta > (Real32)fabs(step))
        return true;
    m_scene->EnableManualMotion(*moving);
    Matrix3D rotation;
    rotation.SetRotation(m_axis, target);
    VortexPhysicsScene* scene = GetVortexScene(this);
    BaseBrickPhysicsBaseHingeSolver* solver = scene->GetHingeSolver(this, interactive);
    BasicArray<BaseBrickPhysicsConnectionHelper*>* hinges = GetSolverHinges(solver);
    Vector3 normal = LEGO::Rotate(m_normal, rotation);
    RefPointer<VortexHingeSolverHinge> hinge = scene->BuildHingeSolverHinge(
        interactive, m_body, m_axis, normal, m_position, m_otherBody, m_otherAxis, m_otherNormal, m_otherPosition,
        hinges);
    bool moved = solver->Solve(step, axis, position, hinge.Get());
    if (!moved)
        Vx::VxInfo(0, "Failed at iteration %d", 0);
    return moved;
}

// Mac 0x3a3840, Win 0x52a4b0
bool VortexPhysicsHingeConnection::RotateUnlimited(bool interactive, Real32 delta)
{
    Real32 current = GetAngle();
    Real32 d = WrapAngle(delta);
    Real32 target = WrapAngle(current + d);
    Vector3 axis;
    Vector3 position;
    GetLocalFrame(interactive, &axis, 0, &position);
    if (interactive)
        d = -d;
    if (kSmallDelta > (Real32)fabs(d))
        return true;
    Matrix3D rotation;
    rotation.SetRotation(m_axis, target);
    VortexPhysicsScene* scene = GetVortexScene(this);
    BaseBrickPhysicsBaseHingeSolver* solver = scene->GetHingeSolver(this, interactive);
    BasicArray<BaseBrickPhysicsConnectionHelper*>* hinges = GetSolverHinges(solver);
    Vector3 normal = LEGO::Rotate(m_normal, rotation);
    RefPointer<VortexHingeSolverHinge> hinge = scene->BuildHingeSolverHinge(
        interactive, m_body, m_axis, normal, m_position, m_otherBody, m_otherAxis, m_otherNormal, m_otherPosition,
        hinges);
    return solver->Solve(d, axis, position, hinge.Get());
}

// Mac 0x3a3b20 / 0x3a3b40, Win 0x52a700
void* VortexPhysicsHingeConnection::GetNativeConstraint()
{
    if (m_constraint != 0)
        return m_constraint->getUserData();
    return 0;
}

// Mac 0x3a40d0 / 0x3a4100, Win 0x51cf20
bool VortexPhysicsHingeConnection::HasNativeConstraint()
{
    return m_constraint != 0;
}

// Mac 0x3a40e0 / 0x3a4110, Win 0x51d240
bool VortexPhysicsHingeConnection::Unknown3()
{
    return m_attributeDatas.HasMotors();
}

// Mac 0x3a3b60 / 0x3a3ed0, Win 0x52a710
bool VortexPhysicsHingeConnection::UpdateConstraint()
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
    for (VortexConnectionAttributeDatas::AttributesData* it = m_attributeDatas.m_datas.Begin();
         it != m_attributeDatas.m_datas.End(); ++it) {
        const ConnectionField::Attributes& a = it->m_attributes;
        if (!a.m_hasLinear && !a.m_hasLimits)
            continue;
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
    return true;
}

// Mac 0x3a3ef0 / 0x3a3f50, Win 0x52aa00 (shared with the slide hinge and the free joint)
void VortexPhysicsHingeConnection::DestroyConstraint()
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
