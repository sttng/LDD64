// LEGO Digital Designer — restored source
// LEGO::VortexPhysicsRigidElement (Mac TU 434). See VortexPhysicsRigidElement.h.
#include "VortexPhysicsRigidElement.h"
#include "VortexPhysicsPrimitiveElement.h"
#include "VortexPhysicsScene.h"
#include "VortexConversions.h"

#include "Vx/VxPart.h"
#include "Vx/VxAssemblyBase.h"
#include "Vx/VxCollisionGeometry.h"
#include "Vx/VxMessage.h"

namespace LEGO {

// Mac 0x3aa4f0, Win 0x52bc50
void VortexPhysicsRigidElement::AddPrimitiveElement(PhysicsPrimitiveElement* element, const Matrix3D& transform)
{
    int first = 0;
    if (m_part != 0) {
        first = m_part->getCollisionGeometryCount();
        if (first > 0)
            --first;   // the last composite may receive geometries too
    }
    BaseBrickPhysicsRigidElement::AddPrimitiveElement(element, transform);
    if (m_part == 0)
        return;
    for (int i = first; i < m_part->getCollisionGeometryCount(); ++i) {
        m_part->getCollisionGeometry(i)->updateCollisionModel();
        m_part->getCollisionGeometry(i)->update();
    }
}

// Mac 0x3aa340, Win 0x52bb30
void VortexPhysicsRigidElement::GetEngineTransform(Matrix3D& transform) const
{
    const Vx::VxTransform& tm = *m_part->mTransformPtr;
    for (int i = 0; i < 16; ++i)
        transform.m[i] = (Real32)tm.m[i / 4][i % 4];
}

// Mac 0x3aa1c0, Win 0x52baa0
void VortexPhysicsRigidElement::SetEngineTransform(const Matrix3D& transform)
{
    Vx::VxTransform tm;
    ToVxTransform(transform, tm);
    m_part->setTransform(tm);
    if (m_part->mInternalControl != Vx::VxEntity::kControlFrozen)
        m_part->updateCollisionGeometries();
    m_part->resetForces();
    m_transform = transform;
    NoteTransformChanged();
}

// Mac 0x3aa3c0, Win 0x52bb50
bool VortexPhysicsRigidElement::IsMoving() const
{
    Vx::VxVector3 linear(0.0, 0.0, 0.0);
    Vx::VxVector3 angular(0.0, 0.0, 0.0);
    m_part->getLinearVelocity(linear);
    m_part->getAngularVelocity(angular);
    // NOTE: both must exceed the threshold (the original's)
    return 1e-06 < angular[2] * angular[2] + angular[1] * angular[1] + angular[0] * angular[0] &&
           1e-06 < linear[2] * linear[2] + linear[1] * linear[1] + linear[0] * linear[0];
}

// Mac 0x3aa490, Win 0x52bc20
void VortexPhysicsRigidElement::Stop()
{
    m_part->setLinearVelocityDamping(0.0);
    m_part->setAngularVelocityDamping(0.0);
}

// Mac 0x3aa4d0
void VortexPhysicsRigidElement::ResetForces()
{
    m_part->resetForces();
}

// Mac 0x3aa5a0, Win 0x52bcf0
void VortexPhysicsRigidElement::OnAddPrimitiveElement(PhysicsPrimitiveElement* element)
{
    if (m_part == 0 || element->GetGeometryCount() == 0)
        return;
    VortexPhysicsPrimitiveElement* primitive = dynamic_cast<VortexPhysicsPrimitiveElement*>(element);
    if (primitive == 0) {
        Vx::VxWarning(0, "VortexPhysicsRigidElement::OnAddPrimitiveElement: rigidBody not a VortexPhysicsPrimitiveElement.\n");
        return;
    }
    primitive->GetGeometryCount();   // NOTE: called again, result unused (the original's)
    for (VortexPhysicsBrickPlacedGeometry* it = primitive->m_geometries.Begin(); it != primitive->m_geometries.End(); ++it)
        AddGeometry(primitive, &primitive->m_transform, it, false);
}

// Mac 0x3aa680, Win 0x52bda0
void VortexPhysicsRigidElement::OnRemovePrimitiveElement(PhysicsPrimitiveElement* element)
{
    if (m_part == 0 || m_part->getCollisionGeometryCount() == 0)
        return;
    VortexPhysicsPrimitiveElement* primitive = dynamic_cast<VortexPhysicsPrimitiveElement*>(element);
    if (primitive == 0) {
        Vx::VxWarning(0, "VortexPhysicsRigidElement::OnRemovePrimitiveElement: rigidBody not a VortexPhysicsPrimitiveElement.\n");
        return;
    }
    for (VortexPhysicsBrickPlacedGeometry* it = primitive->m_geometries.Begin(); it != primitive->m_geometries.End(); ++it)
        RemoveGeometry(it);
}

// Mac 0x3aa730, Win 0x52be30
void VortexPhysicsRigidElement::Update()
{
    if (m_primitives.Begin() == m_primitives.End())
        ReleaseEngineBody();
    else if (m_part == 0)
        CreateBody();
    m_dirty = false;
}

// Mac 0x3aa770, Win 0x52bed0 (+ 0x52be70)
void VortexPhysicsRigidElement::CreateBody()
{
    Vx::VxTransform tm;
    ToVxTransform(m_transform, tm);
    m_part = new Vx::VxPart();
    m_part->setTransform(tm);
    // by the body kind (rigid elements: always KindRigid; Mac +0xd0, Win switch on +0xd0)
    UInt32 kind = m_kind;
    if (kind - 1 < 3) {
        // dynamic: mass and inertia from the collision geometries, then mass 1
        m_part->setControl(Vx::VxEntity::kControlDynamic);
        m_part->mUserInertia = true;
        m_part->mUserCOM = true;
        m_part->mUserMass = true;
        for (BaseBrickPhysicsPrimitiveElement** it = m_primitives.Begin(); it != m_primitives.End(); ++it)
            OnAddPrimitiveElement(*it);
        m_part->updateCollisionGeometryTransforms();
        m_part->updateCollisionGeometries();
        m_part->updateMassPropertiesFromGeometry();
        m_part->setMass(1.0);
        m_part->mUserInertia = true;
        m_part->mUserCOM = true;
        m_part->mUserMass = true;
    } else if (kind - 4 < 2) {
        for (BaseBrickPhysicsPrimitiveElement** it = m_primitives.Begin(); it != m_primitives.End(); ++it)
            OnAddPrimitiveElement(*it);
        m_part->updateCollisionGeometryTransforms();
        m_part->updateCollisionGeometries();
    }
    m_part->freeze(true);
    // NOTE: the element itself (its PhysicsRigidElement), not its VortexRigidBodyUserData
    // sub-object as the other scene elements (CreatePart) store (Mac 0x3aa770 +0x1c = this,
    // Win 0x52bed0 +0x34 = ECX)
    m_part->setUserData(static_cast<PhysicsRigidElement*>(this));
    static_cast<VortexPhysicsScene*>(m_scene)->m_mechanism->addPart(m_part);
}

// Mac 0x3aaa00, Win 0x52bec0
void VortexPhysicsRigidElement::ReleaseEngineBody()
{
    DestroyPart();
}

} // namespace LEGO
