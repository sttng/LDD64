// LEGO Digital Designer — restored source
// LEGO::VortexAnchorSolver, VortexAnchorSolver::ActiveElement, VortexAnchorSolver::Anchor and the
// shared at-rest helper (Mac TU 421, Win 0x52ec30-0x5319bf). See VortexAnchorSolver.h.
#include "VortexAnchorSolver.h"
#include "VortexPhysicsScene.h"
#include "VortexPhysicsRigidElement.h"
#include "VortexPhysicsPrimitiveElement.h"
#include "VortexConversions.h"
#include "BaseBrickPhysicsConnectionHelper.h"
#include "BaseBrickPhysicsAtomic.h"

#include "Vx/VxUniverse.h"
#include "Vx/VxMechanism.h"
#include "Vx/VxAssemblyBase.h"
#include "Vx/VxPart.h"
#include "Vx/VxIntersect.h"
#include "Vx/VxBallAndSocket.h"
#include "Vx/VxCylindrical.h"
#include "Vx/VxRPRO.h"
#include "Vx/VxAngular1Position3.h"

#include <math.h>
#include <string.h>

namespace LEGO {

Real32 VortexAnchorSolver::s_linearDampingFactor = 10.0f;    // Mac 0x778a68
Real32 VortexAnchorSolver::s_angularDampingFactor = 10.0f;   // Mac 0x778a6c
Real32 VortexAnchorSolver::s_maxForce = 10000.0f;            // Mac 0x778a60
Real32 VortexAnchorSolver::s_maxAngularForce = 10000.0f;     // Mac 0x778a64

namespace {

const Real32 kStepTime = 1.0f / 30.0f;          // 0x3d088889
const int    kStepsPerSolve = 3;
const Real32 kOnTarget = 0.001f;                // 0x3a83126f
const Real32 kAxisOnTarget = 0.1f;              // 0x3dcccccd
const double kQuarter = 0.25;                   // Mac __const 0x63f358
const double kGrowth = (double)1.2f;            // Mac __const 0x63f360
const Real32 kGrowthF = 1.2f;                   // Mac __const 0x63f368
const double kUnitTolerance = (double)1e-8f;    // Mac __const 0x63a580
const double kBallForce = 100.0;                // 0x40590000
const double kCylinderAngularForce = 10.0;      // 0x40240000
const double kAngularForce = 1000.0;            // 0x408f4000
const double kDeepContact = (double)0.0139f;    // Mac __const 0x63f350

// The joint tolerances of Stop and of the anchors' state (Mac __const 0x63f310)
inline PhysicsSeparationLimits HoldLimits()
{
    PhysicsSeparationLimits limits;
    limits.m_distance = 0.0009f;
    limits.m_maxDistance = 0.0625f;
    limits.m_alignment = 0.9999f;
    limits.m_minAlignment = 0.9848f;
    limits.m_breakCount = 0;
    return limits;
}

// minsd: the second operand unless the first is less
inline double MinSd(double a, double b)
{
    return a < b ? a : b;
}

inline Real32 MinSs(Real32 a, Real32 b)
{
    return a < b ? a : b;
}

inline Vx::VxPart* PartOf(VortexPhysicsRigidElement* body)
{
    return body->m_part;
}

inline Vx::VxMechanism* MechanismOf(VortexAnchorSolver* solver)
{
    return solver->m_vortexScene->m_mechanism;
}

// A Vx constraint of the anchor out of the mechanism and deleted.
template <class T>
inline void DestroyConstraint(Vx::VxMechanism* mechanism, T*& constraint)
{
    if (constraint != 0) {
        mechanism->removeConstraint(constraint);
        delete constraint;
        constraint = 0;
    }
}

// The configuration of an anchor's Angular1Position3: angular and first linear coordinate free,
// the other two motorised (1000; the second one at -offset), relaxation of the first two
// equations within +-1000. Mac 0x39c150, Win 0x52fef0
void ConfigureAngular(Vx::VxConstraint* constraint, Real32 offset)
{
    constraint->setControl(Vx::VxAngular1Position3::kAngularCoordinate, Vx::VxConstraint::kControlFree);
    constraint->setControl(Vx::VxAngular1Position3::kLinear0Coordinate, Vx::VxConstraint::kControlFree);
    int linear1 = Vx::VxAngular1Position3::kLinear1Coordinate;
    constraint->setControl(linear1, Vx::VxConstraint::kControlMotorized);
    constraint->setMotorMaximumForce(linear1, kAngularForce);
    constraint->setMotorDesiredVelocity(linear1, (double)-offset);
    int linear2 = Vx::VxAngular1Position3::kLinear2Coordinate;
    constraint->setControl(linear2, Vx::VxConstraint::kControlMotorized);
    constraint->setMotorMaximumForce(linear2, kAngularForce);
    constraint->setMotorDesiredVelocity(linear2, 0.0);
    constraint->setRelaxationActive(0, true);
    constraint->setRelaxationActive(1, true);
    constraint->setRelaxationForceLimits(0, -kAngularForce, kAngularForce);
    constraint->setRelaxationForceLimits(1, -kAngularForce, kAngularForce);
}

// RefPointer array: `it` replaced by the last element, which goes (the order is not kept)
template <class T>
void EraseBySwap(BasicArray<RefPointer<T> >& array, RefPointer<T>* it)
{
    RefPointer<T>* last = array.End() - 1;
    if (last != it)
        *it = *last;
    array.PopBack();
}

inline void PushUnique(BasicArray<BaseBrickPhysicsConnectionHelper*>& array, BaseBrickPhysicsConnectionHelper* value)
{
    BaseBrickPhysicsConnectionHelper** it = array.Begin();
    while (it != array.End() && *it != value)
        ++it;
    if (it == array.End())
        array.PushBack(value);
}

} // namespace

// Mac 0x399f50 (Win inlined)
bool HasNoDeepContacts(VortexPhysicsScene* scene)
{
    Vx::VxUniverse* universe = scene->m_universe;
    Vx::VxUniverse::DynamicsContactIterator it = universe->dynamicsContactBegin();
    const double limit = kDeepContact;
    while (it != universe->dynamicsContactEnd()) {
        if ((*it)->getPenetration() > limit)
            return false;
        ++it;
    }
    return true;
}

// =============================================================================================
// VortexAnchorSolver::ActiveElement
// =============================================================================================

// Mac 0x39a690, Win 0x52f060
VortexAnchorSolver::ActiveElement::ActiveElement(VortexPhysicsRigidElement* element, const Real32& linearFactor,
                                                 const Real32& angularFactor)
    : m_element(element), m_anchorCount(1)
{
    Vx::VxPart* part = PartOf(element);
    m_linearDamping = part->getLinearVelocityDamping();
    m_angularDamping = part->getAngularVelocityDamping();
    double mass = PartOf(m_element)->getMass();
    PartOf(m_element)->setLinearVelocityDamping((double)linearFactor * mass);
    PartOf(m_element)->setAngularVelocityDamping(mass * (double)angularFactor);
}

// Mac D1 0x39a790 / D0 0x39a810, Win 0x52f150 / sdd 0x52f130
VortexAnchorSolver::ActiveElement::~ActiveElement()
{
    PartOf(m_element)->setLinearVelocityDamping(m_linearDamping);
    PartOf(m_element)->setAngularVelocityDamping(m_angularDamping);
}

// =============================================================================================
// VortexAnchorSolver::Anchor
// =============================================================================================

// Mac 0x39a8a0 (Win inlined)
VortexAnchorSolver::Anchor::Anchor(VortexAnchorSolver* solver, VortexPhysicsRigidElement* element,
                                   const Matrix3D& local, const Matrix3D& target, UInt32 count,
                                   PhysicsRigidElement* const* bodies)
    : m_solver(solver), m_userData(0), m_element(element), m_local(local), m_target(target), m_ballAndSocket(0),
      m_ballPart(0), m_cylindrical(0), m_orientation(0), m_angular(0), m_angularReversed(0), m_attraction(0)
{
    for (UInt32 i = 0; i < count; ++i)
        m_bodies.PushBack(static_cast<VortexPhysicsRigidElement*>(static_cast<BaseBrickPhysicsRigidElement*>(bodies[i])));
    m_solver->AddRef();
}

// Mac D1 0x39d820 / D0 0x39d870, Win 0x531080 / sdd 0x52f430
VortexAnchorSolver::Anchor::~Anchor()
{
}

// NOTE: an anchor whose last reference goes while it is still in its solver's list re-enters
// Remove from here (as the original: RemoveAnchor takes and drops a reference). Mac 0x39d8e0,
// Win 0x531100
void VortexAnchorSolver::Anchor::OnFinalRelease()
{
    Remove();
    Object::OnFinalRelease();
}

// Mac 0x39ad80 (thunk 0x39ae00), Win 0x52f450
Int32 VortexAnchorSolver::Anchor::QueryInterface(const InterfaceID& id, void** object)
{
    if (memcmp(&id, &AnchorPhysicsSolverAnchor::GetIID(), sizeof id) == 0) {
        *object = static_cast<AnchorPhysicsSolverAnchor*>(this);
        AddRef();
        return 0;
    }
    *object = 0;
    return kPhysicsNoInterface;
}

// Mac 0x39ae90 (thunk 0x39aeb0), Win 0x5c57e0
UInt32 VortexAnchorSolver::Anchor::AddRef()
{
    Object::AddRef();
    return 0xffffffff;
}

// Mac 0x39aed0 (thunk 0x39aef0), Win 0x52cca0
UInt32 VortexAnchorSolver::Anchor::Release()
{
    Object::Release();
    return 0xffffffff;
}

// Mac 0x39af10 (thunk 0x39af20), Win 0x52f4a0
void VortexAnchorSolver::Anchor::SetUserData(void* data)
{
    m_userData = data;
}

// Mac 0x39af30 (thunk 0x39af40), Win 0x52f4b0
void* VortexAnchorSolver::Anchor::GetUserData()
{
    return m_userData;
}

// Mac 0x39af50 (thunk 0x39af60), Win 0x46ee40
PhysicsRigidElement* VortexAnchorSolver::Anchor::GetElement()
{
    return m_element;
}

// Mac 0x39af70 (thunk 0x39af80), Win 0x52f4c0
const Matrix3D& VortexAnchorSolver::Anchor::GetLocalTransform()
{
    return m_local;
}

// Mac 0x39af90 (thunk 0x39afa0), Win 0x52f4d0
const Matrix3D& VortexAnchorSolver::Anchor::GetTargetTransform()
{
    return m_target;
}

// Mac 0x39afb0 (thunk 0x39afd0), Win 0x52f4e0
UInt32 VortexAnchorSolver::Anchor::GetElementCount()
{
    return m_bodies.GetSize();
}

// Mac 0x39aff0 (thunk 0x39b000), Win 0x52f4f0
PhysicsRigidElement* const* VortexAnchorSolver::Anchor::GetElements()
{
    return reinterpret_cast<PhysicsRigidElement* const*>(m_bodies.Begin());
}

// Mac 0x39b010 (thunk 0x39b0d0), Win 0x52f500
void VortexAnchorSolver::Anchor::SetTargetTransform(const Matrix3D& target)
{
    if (m_solver == 0)
        return;
    m_target = target;
    AnchorPhysicsSolverCallback* callback = m_solver->m_anchorCallback;
    if (callback == 0)
        SetPointAttraction(m_local.GetTranslation(), const_cast<Matrix3D&>(target).GetTranslation());
    else
        callback->OnAnchorChanged(this);
}

// Mac 0x39b190 (thunk 0x39b3d0), Win 0x52f560
void VortexAnchorSolver::Anchor::Remove()
{
    if (m_solver != 0) {
        m_solver->RemoveAnchor(this);
        static_cast<Object*>(m_solver)->Release();
        m_solver = 0;
    }
}

// Mac 0x39b410 (thunk 0x39b470), Win 0x52f590
void VortexAnchorSolver::Anchor::ClearPointAttraction()
{
    if (m_attraction != 0) {
        MechanismOf(m_solver)->removeUpdateSubscriber(m_attraction);
        m_attraction->release();
        m_attraction = 0;
    }
}

// Mac 0x39b4d0 (thunk 0x39b630), Win 0x52f5e0
void VortexAnchorSolver::Anchor::SetPointAttraction(const Vector3& local, const Vector3& target)
{
    if (m_solver == 0)
        return;
    if (m_attraction != 0) {
        m_attraction->m_point = local;
        m_attraction->m_target = target;
        return;
    }
    VortexPhysicsScene* scene = m_solver->m_vortexScene;
    m_attraction = new PointGravitationAction(m_element, local, target, 1.0);
    m_attraction->addRef();
    m_attraction->m_part = PartOf(m_element);
    scene->m_mechanism->addUpdateSubscriber(m_attraction, false);
}

// NOTE: the original reads the solver's mechanism first, also without a solver (a crash after
// Remove); fixed: nothing is done then. Mac 0x39b650 (thunk 0x39b6e0), Win 0x52f6f0
void VortexAnchorSolver::Anchor::ClearBallAndSocket()
{
    if (m_solver == 0)
        return;
    Vx::VxMechanism* mechanism = MechanismOf(m_solver);
    DestroyConstraint(mechanism, m_ballAndSocket);
    if (m_ballPart != 0) {
        mechanism->removePart(m_ballPart);
        delete m_ballPart;
        m_ballPart = 0;
    }
}

// Mac 0x39b770 (thunk 0x39ba70), Win 0x52f770
void VortexAnchorSolver::Anchor::SetBallAndSocket(const Vector3& local, const Vector3& target)
{
    if (m_solver == 0)
        return;
    ClearBallAndSocket();
    m_ballLocal = local;
    m_ballTarget = target;
    Vx::VxMechanism* mechanism = MechanismOf(m_solver);
    Vx::VxVector3 localPosition = ToVxVector3(local);
    Vx::VxVector3 targetPosition = ToVxVector3(target);
    Vx::VxBallAndSocket* joint = new Vx::VxBallAndSocket();
    m_ballAndSocket = joint;
    Vx::VxVector3 primary(1.0, 0.0, 0.0);
    Vx::VxVector3 secondary(0.0, 1.0, 0.0);
    joint->setPartAndAttachment(0, 0, targetPosition, primary, &secondary);
    Vx::VxVector3 primaryRel(1.0, 0.0, 0.0);
    Vx::VxVector3 secondaryRel(0.0, 1.0, 0.0);
    joint->setPartAndAttachmentRel(1, PartOf(m_element), localPosition, primaryRel, secondaryRel);
    joint->setRelaxationActive(0, true);
    joint->setRelaxationActive(1, true);
    joint->setRelaxationActive(2, true);
    joint->setRelaxationForceLimits(0, -kBallForce, kBallForce);
    joint->setRelaxationForceLimits(1, -kBallForce, kBallForce);
    joint->setRelaxationForceLimits(2, -kBallForce, kBallForce);
    mechanism->addConstraint(joint);
}

// Mac 0x39ba90 (thunk 0x39bae0), Win 0x52f9d0
void VortexAnchorSolver::Anchor::ClearCylindrical()
{
    if (m_cylindrical != 0)
        DestroyConstraint(MechanismOf(m_solver), m_cylindrical);
}

// Mac 0x39bb30 (thunk 0x39beb0), Win 0x52fa10
void VortexAnchorSolver::Anchor::SetCylindrical(const Vector3& position, const Vector3& axis,
                                                const Vector3& targetPosition, const Vector3& targetAxis)
{
    if (m_solver == 0)
        return;
    ClearCylindrical();
    m_cylinderPosition = position;
    m_cylinderAxis = axis;
    m_cylinderTargetPosition = targetPosition;
    m_cylinderTargetAxis = targetAxis;
    Vx::VxVector3 vPosition = ToVxVector3(position);
    Vx::VxVector3 vAxis = ToVxVector3(axis);
    VortexPhysicsScene* scene = m_solver->m_vortexScene;
    Vx::VxVector3 vTargetPosition = ToVxVector3(targetPosition);
    Vx::VxVector3 vTargetAxis = ToVxVector3(targetAxis);
    Vx::VxCylindrical* joint = new Vx::VxCylindrical();
    m_cylindrical = joint;
    joint->setPartAndAttachment(0, 0, vTargetPosition, vTargetAxis, 0);
    Vector3 perpendicular = axis.GetPerpendicular();
    joint->setPartAndAttachmentRel(1, PartOf(m_element), vPosition, vAxis, ToVxVector3(perpendicular));
    joint->setControl(Vx::VxCylindrical::kAngularCoordinate, Vx::VxConstraint::kControlFree);
    joint->setControl(Vx::VxCylindrical::kLinearCoordinate, Vx::VxConstraint::kControlFree);
    joint->setRelaxationActive(3, true);
    joint->setRelaxationActive(2, true);
    joint->setRelaxationActive(0, true);
    joint->setRelaxationActive(1, true);
    joint->setRelaxationForceLimits(0, -kBallForce, kBallForce);
    joint->setRelaxationForceLimits(1, -kBallForce, kBallForce);
    joint->setRelaxationForceLimits(3, -kCylinderAngularForce, kCylinderAngularForce);
    joint->setRelaxationForceLimits(2, -kCylinderAngularForce, kCylinderAngularForce);
    joint->mUserData = static_cast<Object*>(this);
    scene->m_mechanism->addConstraint(joint);
}

// Mac 0x39bee0 (thunk 0x39bf30), Win 0x52fd70
void VortexAnchorSolver::Anchor::ClearOrientation()
{
    if (m_orientation != 0)
        DestroyConstraint(MechanismOf(m_solver), m_orientation);
}

// Mac 0x39bf80 (thunk 0x39c090), Win 0x52fdb0
void VortexAnchorSolver::Anchor::SetOrientation(const Real32& tolerance)
{
    if (m_solver == 0)
        return;
    ClearOrientation();
    VortexPhysicsScene* scene = m_solver->m_vortexScene;
    const Vx::VxTransform& tm = *PartOf(m_element)->mTransformPtr;
    Vx::VxVector3 position(tm.m[3][0], tm.m[3][1], tm.m[3][2]);
    Vx::VxRPRO* joint = new Vx::VxRPRO(PartOf(m_element), 0, position);
    joint->setLinearStrength(Vx::VxVector3(0.0, 0.0, 0.0));
    double strength = (double)tolerance;
    joint->setAngularStrength(Vx::VxVector3(strength, strength, strength));
    m_orientation = joint;
    scene->m_mechanism->addConstraint(joint);
}

// Mac 0x39c0b0 (thunk 0x39c100), Win 0x52feb0
void VortexAnchorSolver::Anchor::ClearAngular()
{
    if (m_angular != 0)
        DestroyConstraint(MechanismOf(m_solver), m_angular);
}

// Mac 0x39c270 (thunk 0x39c4f0), Win 0x52fff0
void VortexAnchorSolver::Anchor::SetAngular(const Vector3& position, const Vector3& axis, const Vector3& targetPosition,
                                            const Vector3& targetAxis, const Vector3& reference, const Real32& offset)
{
    if (m_solver == 0)
        return;
    ClearAngular();
    m_angularPosition = position;
    m_angularAxis = axis;
    m_angularTargetPosition = targetPosition;
    m_angularTargetAxis = targetAxis;
    m_angularReference = reference;
    m_angularOffsetTarget = targetPosition + reference * offset;
    Vx::VxMechanism* mechanism = MechanismOf(m_solver);
    Vx::VxPart* part = PartOf(m_element);
    Vector3 perpendicular = axis.GetPerpendicular();
    Vx::VxAngular1Position3* joint = new Vx::VxAngular1Position3();
    m_angular = joint;
    mechanism->addConstraint(joint);
    Vx::VxVector3 vReference = ToVxVector3(reference);
    joint->setPartAndAttachment(0, 0, ToVxVector3(targetPosition), ToVxVector3(targetAxis), &vReference);
    joint->setPartAndAttachmentRel(1, part, ToVxVector3(position), ToVxVector3(axis), ToVxVector3(perpendicular));
    ConfigureAngular(joint, offset);
}

// NOTE (guard): as ClearBallAndSocket, nothing without a solver. Mac 0x39c520 (thunk 0x39c610),
// Win 0x530300
void VortexAnchorSolver::Anchor::ClearAngularReversed()
{
    if (m_solver == 0)
        return;
    VortexAnchorSolver* solver = m_solver;
    Vx::VxMechanism* mechanism = MechanismOf(solver);
    // NOTE: the original looks the body up in the active elements and does not use the result.
    double mass = PartOf(m_element)->getMass();
    PartOf(m_element)->setLinearVelocityDamping((double)solver->m_linearFactor * mass);
    PartOf(m_element)->setAngularVelocityDamping((double)solver->m_angularFactor * mass);
    DestroyConstraint(mechanism, m_angularReversed);
}

// Mac 0x39c630 (thunk 0x39c890), Win 0x5303c0
void VortexAnchorSolver::Anchor::SetAngularReversed(const Vector3& position, const Vector3& axis,
                                                    const Vector3& targetPosition, const Vector3& targetAxis,
                                                    const Vector3& reference, const Real32& offset)
{
    if (m_solver == 0)
        return;
    ClearAngularReversed();
    m_reversedPosition = position;
    m_reversedAxis = axis;
    m_reversedTargetPosition = targetPosition;
    m_reversedTargetAxis = targetAxis;
    m_reversedReference = reference;
    Vx::VxMechanism* mechanism = MechanismOf(m_solver);
    Vx::VxPart* part = PartOf(m_element);
    Vector3 perpendicular = axis.GetPerpendicular();
    Vector3 targetPerpendicular = targetAxis.GetPerpendicular();
    Vx::VxAngular1Position3* joint = new Vx::VxAngular1Position3();
    m_angularReversed = joint;
    mechanism->addConstraint(joint);
    joint->setPartAndAttachmentRel(0, part, ToVxVector3(position), ToVxVector3(axis), ToVxVector3(perpendicular));
    Vx::VxVector3 vSecondary = ToVxVector3(targetPerpendicular);
    joint->setPartAndAttachment(1, 0, ToVxVector3(targetPosition), ToVxVector3(targetAxis), &vSecondary);
    ConfigureAngular(joint, offset);
}

// Mac 0x39c8c0 (thunk 0x39c8d0), Win 0x459790
void VortexAnchorSolver::Anchor::Unknown24()
{
}

// Mac 0x39c8e0 (thunk 0x39c8f0), Win 0x424ec0
void VortexAnchorSolver::Anchor::Unknown25(UInt32, UInt32)
{
}

// 0 when one of the bodies moves, else 1 (inlined everywhere in GetState)
UInt32 VortexAnchorSolver::Anchor::MovingState() const
{
    for (VortexPhysicsRigidElement* const* it = m_bodies.Begin(); it != m_bodies.End(); ++it)
        if ((*it)->IsMoving())
            return 0;
    return 1;
}

// Mac 0x39c900 (thunk 0x39cdf0), Win 0x530680
UInt32 VortexAnchorSolver::Anchor::GetState()
{
    if (!HasNoDeepContacts(m_solver->m_vortexScene))
        return MovingState();
    Matrix3D tm;
    GetElement()->GetTransform(tm);
    if (m_cylindrical != 0) {
        if (m_cylindrical->isRelaxationActive(3))
            return MovingState();
        Vector3 position = m_cylinderPosition * tm;
        Vector3 axis = Rotate(m_cylinderAxis, tm);
        if (!m_cylinderTargetPosition.IsSameLine(m_cylinderTargetAxis, position, axis, kOnTarget))
            return MovingState();
    }
    if (m_angular != 0) {
        if (m_angular->isRelaxationActive(0))
            return MovingState();
        Vector3 axis = Rotate(m_angularAxis, tm);
        if (!m_angularTargetPosition.IsSameLine(m_angularTargetAxis, m_angularTargetPosition, axis, kAxisOnTarget))
            return MovingState();
    }
    if (m_angularReversed != 0) {
        if (m_angularReversed->isRelaxationActive(0))
            return MovingState();
        Vector3 axis = Rotate(m_reversedAxis, tm);
        if (!m_reversedTargetPosition.IsSameLine(m_reversedTargetAxis, m_reversedTargetPosition, axis, kAxisOnTarget))
            return MovingState();
    }
    if (m_ballAndSocket != 0) {
        if (m_ballAndSocket->isRelaxationActive(2))
            return MovingState();
        Vector3 position = m_ballLocal * tm;
        if (!m_ballTarget.IsEqual(position, kOnTarget))
            return MovingState();
    }
    PhysicsSeparationLimits limits = HoldLimits();
    BasicArray<BaseBrickPhysicsConnectionHelper*>& connections = m_solver->m_connections;
    for (BaseBrickPhysicsConnectionHelper** it = connections.Begin(); it != connections.End(); ++it)
        if ((*it)->CheckSeparation(limits) != 0)
            return MovingState();
    return 2;
}

// Mac 0x39ce10 (thunk 0x39ce50), Win 0x5308b0
void VortexAnchorSolver::Anchor::EnableCollisions(const PhysicsGeometryIdentifier* features, UInt32 count)
{
    for (const PhysicsGeometryIdentifier* f = features; (Int32)count > 0 && f < features + count; ++f)
        static_cast<VortexPhysicsPrimitiveElement*>(f->m_element)->SetGeometryEnabled((Int32)f->m_feature, true);
}

// Mac 0x39ce90 (thunk 0x39ced0), Win 0x5308e0
void VortexAnchorSolver::Anchor::DisableCollisions(const PhysicsGeometryIdentifier* features, UInt32 count)
{
    for (const PhysicsGeometryIdentifier* f = features; (Int32)count > 0 && f < features + count; ++f)
        static_cast<VortexPhysicsPrimitiveElement*>(f->m_element)->SetGeometryEnabled((Int32)f->m_feature, false);
}

// Mac 0x39cf10, Win 0x530910
void VortexAnchorSolver::Anchor::Update(Vx::VxUniverse* /*universe*/, UInt32 separation)
{
    if (m_solver == 0)
        return;
    if (m_attraction != 0) {
        double strength = m_attraction->m_strength;
        if (separation == 2) {
            m_attraction->m_strength = strength * kQuarter;
        } else if (!(kUnitTolerance > fabs(-1.0 + strength))) {
            Real32 grown = (Real32)strength * kGrowthF;
            m_attraction->m_strength = (double)MinSs(grown, 1.0f);
        }
    }
    if (m_cylindrical != 0) {
        double low0, high0, low3, high3;
        m_cylindrical->getRelaxationForceLimits(0, &low0, &high0);
        m_cylindrical->getRelaxationForceLimits(3, &low3, &high3);
        if (separation == 2) {
            // NOTE: only the linear limit drops; the angular one stays
            high0 = high0 * kQuarter;
        } else {
            Matrix3D tm;
            GetElement()->GetTransform(tm);
            Vector3 position = m_cylinderPosition * tm;
            Vector3 axis = Rotate(m_cylinderAxis, tm);
            if (m_cylinderTargetPosition.IsSameLine(m_cylinderTargetAxis, position, axis, kOnTarget)) {
                m_cylindrical->setRelaxationActive(3, false);
                m_cylindrical->setRelaxationActive(2, false);
                m_cylindrical->setRelaxationActive(0, false);
                m_cylindrical->setRelaxationActive(1, false);
                high0 = (double)s_maxForce;
                high3 = (double)s_maxAngularForce;
            } else {
                high0 = MinSd(high0 * kGrowth, (double)s_maxForce);
                high3 = MinSd(kGrowth * high3, (double)s_maxAngularForce);
            }
        }
        m_cylindrical->setRelaxationForceLimits(3, -high3, high3);
        m_cylindrical->setRelaxationForceLimits(2, -high3, high3);
        m_cylindrical->setRelaxationForceLimits(0, -high0, high0);
        m_cylindrical->setRelaxationForceLimits(1, -high0, high0);
    }
    if (m_angular != 0) {
        int linear2 = Vx::VxAngular1Position3::kLinear2Coordinate;
        double force = m_angular->getMotorMaximumForce(linear2);
        double low, high;
        m_angular->getRelaxationForceLimits(0, &low, &high);
        if (separation == 2) {
            force = force * kQuarter;
            high = kQuarter * high;
        } else {
            Matrix3D tm;
            GetElement()->GetTransform(tm);
            Vector3 position = m_angularPosition * tm;
            Vector3 axis = Rotate(m_angularAxis, tm);
            if (m_angularOffsetTarget.IsSameLine(m_angularTargetAxis, position, axis, kOnTarget)) {
                m_angular->setRelaxationActive(0, false);
                m_angular->setRelaxationActive(1, false);
                high = (double)s_maxAngularForce;
            } else {
                force = MinSd(force * kGrowth, (double)s_maxForce);
                high = MinSd(kGrowth * high, (double)s_maxAngularForce);
            }
        }
        m_angular->setMotorMaximumForce(Vx::VxAngular1Position3::kLinear1Coordinate, force);
        m_angular->setMotorMaximumForce(linear2, force);
        m_angular->setRelaxationForceLimits(0, -high, high);
        m_angular->setRelaxationForceLimits(1, -high, high);
    }
    if (m_angularReversed != 0) {
        int linear2 = Vx::VxAngular1Position3::kLinear2Coordinate;
        double force = m_angularReversed->getMotorMaximumForce(linear2);
        double low, high;
        m_angularReversed->getRelaxationForceLimits(0, &low, &high);
        if (separation == 2) {
            force = force * kQuarter;
            high = kQuarter * high;
        } else {
            Matrix3D tm;
            GetElement()->GetTransform(tm);
            Vector3 position = m_reversedPosition * tm;
            Vector3 axis = Rotate(m_reversedAxis, tm);
            if (m_reversedTargetPosition.IsSameLine(m_reversedTargetAxis, position, axis, kOnTarget)) {
                m_angularReversed->setRelaxationActive(0, false);
                m_angularReversed->setRelaxationActive(1, false);
                high = (double)s_maxAngularForce;
            } else {
                force = MinSd(force * kGrowth, (double)s_maxForce);
                high = MinSd(kGrowth * high, (double)s_maxAngularForce);
            }
        }
        m_angularReversed->setMotorMaximumForce(Vx::VxAngular1Position3::kLinear1Coordinate, force);
        m_angularReversed->setMotorMaximumForce(linear2, force);
        m_angularReversed->setRelaxationForceLimits(0, -high, high);
        m_angularReversed->setRelaxationForceLimits(1, -high, high);
    }
    if (m_ballAndSocket != 0) {
        double low, high;
        m_ballAndSocket->getRelaxationForceLimits(0, &low, &high);
        if (separation == 2) {
            high = high * kQuarter;
        } else {
            Matrix3D tm;
            GetElement()->GetTransform(tm);
            Vector3 position = m_ballLocal * tm;
            if (m_ballTarget.IsEqual(position, kOnTarget)) {
                high = (double)s_maxForce;
                m_ballAndSocket->setRelaxationActive(0, false);
                m_ballAndSocket->setRelaxationActive(1, false);
                m_ballAndSocket->setRelaxationActive(2, false);
            } else {
                Real32 grown = (Real32)high * kGrowthF;
                high = (double)MinSs(grown, s_maxForce);
            }
        }
        m_ballAndSocket->setRelaxationForceLimits(0, -high, high);
        m_ballAndSocket->setRelaxationForceLimits(1, -high, high);
        m_ballAndSocket->setRelaxationForceLimits(2, -high, high);
    }
}

// =============================================================================================
// VortexAnchorSolver
// =============================================================================================

// Mac 0x399830 (C1 0x399b90), Win inlined
VortexAnchorSolver::VortexAnchorSolver(VortexPhysicsScene* scene, PhysicsSolverCallback* callback)
    : BaseBrickPhysicsSimulationSolver(scene, callback), m_vortexScene(scene),
      m_anchorCallback(static_cast<AnchorPhysicsSolverCallback*>(callback)), m_linearFactor(s_linearDampingFactor),
      m_angularFactor(s_angularDampingFactor), m_snapshot(0)
{
    // NOTE: the callback is taken as an AnchorPhysicsSolverCallback without a QueryInterface
    if (callback != 0)
        callback->AddRef();
}

// Mac D1 0x39e480 (body 0x39e1b0) / D0 0x39e4b0, Win 0x531550 / sdd 0x52ec30
VortexAnchorSolver::~VortexAnchorSolver()
{
    if (m_anchorCallback != 0)
        m_anchorCallback->Release();
}

// Mac 0x399d40, Win 0x459790
void VortexAnchorSolver::Start()
{
}

// Mac 0x399d50, Win 0x52ecf0
void VortexAnchorSolver::Stop(bool& /*result*/)
{
    PhysicsSeparationLimits limits = HoldLimits();
    for (BaseBrickPhysicsConnectionHelper** it = m_connections.Begin(); it != m_connections.End(); ++it) {
        if ((*it)->CheckSeparation(limits) != 0) {
            m_vortexScene->RestoreSnapshotImpl(m_snapshot);
            break;
        }
    }
    while (!m_anchors.IsEmpty())
        m_anchors.Begin()->Get()->Remove();
    if (m_snapshot != 0) {
        m_snapshot->Release();
        m_snapshot = 0;
    }
}

// Mac 0x399df0, Win 0x52eda0
UInt32 VortexAnchorSolver::Step(Real32 /*time*/)
{
    if (m_snapshot == 0)
        return 0;
    PhysicsSeparationLimits limits;
    limits.m_distance = 0.0009f;
    limits.m_maxDistance = 0.005625f;   // 0x3bb851ec
    limits.m_alignment = 0.9999f;
    limits.m_minAlignment = 0.9848f;
    limits.m_breakCount = 0;
    int restores = 0;
    for (int i = 0; i < kStepsPerSolve; ++i) {
        m_vortexScene->StepPhysics(kStepTime);
        UInt32 separation = 0;
        for (BaseBrickPhysicsConnectionHelper** it = m_connections.Begin(); separation == 0 && it != m_connections.End();
             ++it)
            separation = (*it)->CheckSeparation(limits);
        if (separation == 0) {
            if (HasNoDeepContacts(m_vortexScene))
                m_snapshot->Update();
        } else if (separation == 2) {
            m_vortexScene->RestoreSnapshotImpl(m_snapshot);
            if (++restores > 1)
                break;
            --i;
        }
        for (RefPointer<Anchor>* it = m_anchors.Begin(); it != m_anchors.End(); ++it)
            (*it)->Update(m_vortexScene->m_universe, separation);
    }
    for (RefPointer<ActiveElement>* it = m_activeElements.Begin(); it != m_activeElements.End(); ++it) {
        VortexPhysicsRigidElement* body = (*it)->m_element;
        body->GetEngineTransform(body->m_transform);
        body->NoteTransformChanged();
    }
    return 0;
}

// Mac 0x39a000, Win 0x52eee0
void VortexAnchorSolver::OnBeginUpdate()
{
    m_vortexScene->StopSolver(false);
}

// Mac 0x399ba0 (thunks 0x399c20 / 0x399cb0), Win 0x52ec50
Int32 VortexAnchorSolver::QueryInterface(const InterfaceID& id, void** object)
{
    if (memcmp(&id, &AnchorPhysicsSolver::GetIID(), sizeof id) == 0) {
        *object = static_cast<AnchorPhysicsSolver*>(this);
        AddRef();
        return 0;
    }
    return BaseBrickPhysicsSimulationSolver::QueryInterface(id, object);
}

// Mac 0x39e530 (thunks 0x39e560 / 0x39e5c0)
UInt32 VortexAnchorSolver::AddRef()
{
    return BaseBrickPhysicsSimulationSolver::AddRef();
}

// Mac 0x39e540 (thunks 0x39e580 / 0x39e5e0)
UInt32 VortexAnchorSolver::Release()
{
    return BaseBrickPhysicsSimulationSolver::Release();
}

// Mac 0x39e550 (thunks 0x39e5a0 / 0x39e600)
PhysicsSolverCallback* VortexAnchorSolver::GetCallback()
{
    return BaseBrickPhysicsSimulationSolver::GetCallback();
}

// Mac 0x39a020 (thunk 0x39a660), Win 0x52eef0
AnchorPhysicsSolverAnchor* VortexAnchorSolver::CreateAnchor(PhysicsRigidElement* body, const Matrix3D& local,
                                                            const Matrix3D& target, UInt32 count,
                                                            PhysicsRigidElement* const* bodies)
{
    Anchor* anchor = new Anchor(this, static_cast<VortexPhysicsRigidElement*>(static_cast<BaseBrickPhysicsRigidElement*>(body)),
                                local, target, count, bodies);
    {
        RefPointer<Anchor> held(anchor);
        m_anchors.PushBack(held);
    }
    AddActiveElements(anchor);
    if (m_anchorCallback != 0)
        m_anchorCallback->OnAnchorAdded(anchor);
    anchor->AddRef();
    return anchor;
}

// Mac 0x39a230 (Win inlined into CreateAnchor)
void VortexAnchorSolver::AddActiveElements(Anchor* anchor)
{
    bool added = false;
    for (VortexPhysicsRigidElement** body = anchor->m_bodies.Begin(); body != anchor->m_bodies.End(); ++body) {
        RefPointer<ActiveElement>* it = m_activeElements.Begin();
        while (it != m_activeElements.End() && (*it)->m_element != *body)
            ++it;
        if (it != m_activeElements.End()) {
            ++(*it)->m_anchorCount;
            continue;
        }
        {
            RefPointer<ActiveElement> element(new ActiveElement(*body, m_linearFactor, m_angularFactor));
            m_activeElements.PushBack(element);
        }
        VortexPhysicsRigidElement* last = m_activeElements.Back()->m_element;
        m_vortexScene->EnableManualMotion(*body);
        added = true;
        for (BaseBrickPhysicsConnectionHelper** j = last->m_connections.Begin(); j != last->m_connections.End(); ++j)
            PushUnique(m_connections, *j);
    }
    if (!added)
        return;
    if (m_snapshot != 0)
        m_snapshot->Release();
    BasicArray<PhysicsRigidElement*> bodies;
    for (RefPointer<ActiveElement>* it = m_activeElements.Begin(); it != m_activeElements.End(); ++it)
        bodies.PushBack((*it)->m_element);
    m_snapshot = m_vortexScene->CreateSnapshot(bodies.IsEmpty() ? 0 : bodies.Begin(), bodies.GetSize());
}

// Mac 0x39dd20 (Win inlined)
void VortexAnchorSolver::RemoveActiveElements(Anchor* anchor)
{
    bool removed = false;
    for (VortexPhysicsRigidElement** body = anchor->m_bodies.Begin(); body != anchor->m_bodies.End(); ++body) {
        RefPointer<ActiveElement>* it = m_activeElements.Begin();
        while (it != m_activeElements.End() && (*it)->m_element != *body)
            ++it;
        // (the body is always there: AddActiveElements made it)
        PartOf((*it)->m_element)->freeze(false);
        if (--(*it)->m_anchorCount != 0)
            continue;
        m_vortexScene->DisableSimulation((*it)->m_element);
        removed = true;
        EraseBySwap(m_activeElements, it);
    }
    if (!removed)
        return;
    m_connections.Clear();
    BasicArray<PhysicsRigidElement*> bodies;
    for (RefPointer<ActiveElement>* it = m_activeElements.Begin(); it != m_activeElements.End(); ++it) {
        VortexPhysicsRigidElement* element = (*it)->m_element;
        bodies.PushBack(element);
        for (BaseBrickPhysicsConnectionHelper** j = element->m_connections.Begin(); j != element->m_connections.End(); ++j)
            PushUnique(m_connections, *j);
    }
    if (m_snapshot != 0)
        m_snapshot->Release();
    m_snapshot = bodies.IsEmpty() ? 0 : m_vortexScene->CreateSnapshot(bodies.Begin(), bodies.GetSize());
}

// Mac 0x39b1d0, Win 0x531630
void VortexAnchorSolver::RemoveAnchor(Anchor* anchor)
{
    anchor->AddRef();
    RefPointer<Anchor>* it = m_anchors.Begin();
    while (it != m_anchors.End() && it->Get() != anchor)
        ++it;
    static_cast<Object*>(anchor)->Release();
    if (it == m_anchors.End())
        return;
    for (Vx::VxMechanism** m = m_mechanisms.Begin(); m != m_mechanisms.End(); ++m) {
        m_vortexScene->m_universe->removeAssembly(*m);
        delete *m;
    }
    m_mechanisms.Clear();
    RemoveActiveElements(it->Get());
    anchor->ClearPointAttraction();
    anchor->ClearBallAndSocket();
    anchor->ClearCylindrical();
    anchor->ClearOrientation();
    anchor->ClearAngular();
    anchor->ClearAngularReversed();
    if (m_anchorCallback != 0)
        m_anchorCallback->OnAnchorRemoved(anchor);
    EraseBySwap(m_anchors, it);
}

} // namespace LEGO
