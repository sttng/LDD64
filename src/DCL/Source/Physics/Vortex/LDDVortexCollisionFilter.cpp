// LEGO Digital Designer — restored source
// The second object of the rigid element sources (Mac TU 436 "LDDVortexCollisionFilter.cpp", file
// name inferred; 0x3b4350-0x3b4aef; Win 0x52d850-0x52dd6f): the VortexPhysicsSceneElement methods,
// LDDVortexCollisionFilter and its global instance. VortexShapeCollisionFilter::Add / Remove of
// the same object are in VortexShapeCollisionFilter.cpp.
#include "LDDVortexCollisionFilter.h"
#include "VortexPhysicsRigidElement.h"
#include "VortexPhysicsPlacedGeometry.h"
#include "VortexConversions.h"

#include "Vx/VxPart.h"
#include "Vx/VxAssemblyBase.h"
#include "Vx/VxCollisionGeometry.h"
#include "Vx/VxMessage.h"

namespace LEGO {

// The global instance. Mac 0x77cdbc (initialiser 0x3b4a90: vtable, +4 = 0, +8 = 0), Win 0xb43f98
// (initialiser 0x52d7e0).
LDDVortexCollisionFilter g_lddVortexCollisionFilter;

// =============================================================================================
// VortexPhysicsSceneElement
// =============================================================================================

// Mac 0x3b43c0
void VortexPhysicsSceneElement::CreatePart(Vx::VxMechanism* mechanism)
{
    Vx::VxPart* part = new Vx::VxPart();
    m_part = part;
    part->setUserData(static_cast<VortexRigidBodyUserData*>(this));
    part->setControl(Vx::VxEntity::kControlDynamic);
    mechanism->addPart(m_part);
}

// Mac 0x3b4440, Win 0x52da40
void VortexPhysicsSceneElement::DestroyPart()
{
    if (m_part == 0)
        return;
    if (m_part->mAssembly != 0)
        static_cast<Vx::VxMechanism*>(m_part->mAssembly)->removePart(m_part);
    bool composites = m_useComposites;
    int count = m_part->getCollisionGeometryCount();
    if (!composites) {
        while (count > 0) {
            m_part->removeCollisionGeometry(m_part->getCollisionGeometryCount() - 1);
            count = m_part->getCollisionGeometryCount();
        }
    } else {
        for (int i = 0; i < count; ++i) {
            Vx::VxCompositeCollisionGeometry* composite =
                static_cast<Vx::VxCompositeCollisionGeometry*>(m_part->getCollisionGeometry(i));
            while (composite->getCollisionGeometryCount() > 0)
                composite->removeCollisionGeometry(composite->getCollisionGeometryCount() - 1);
            VortexCollisionGeometryData::Destroy(composite);
        }
    }
    delete m_part;
    m_part = 0;
}

// Mac 0x3b4550
void VortexPhysicsSceneElement::ScaleMass(Real32 scale)
{
    Vx::VxPart* part = m_part;
    if (part != 0 && part->mControl == Vx::VxEntity::kControlDynamic)
        part->setMass(((double)scale * part->getMass()) / (double)m_massUnit);
}

// Mac 0x3b45b0
Vx::VxCollisionGeometry* VortexPhysicsSceneElement::GetComposite(Vx::VxCollisionGeometry* last,
                                                                 VortexShapeCollisionFilter* filter)
{
    if (last == 0) {
        if (m_part->getCollisionGeometryCount() < 1) {
            Vx::VxCompositeCollisionGeometry* composite = new Vx::VxCompositeCollisionGeometry((int)m_compositeSize);
            m_part->addCollisionGeometry(composite);
            VortexCollisionGeometryData::Create(composite);
            VortexCollisionGeometryData::SetFilter(composite, filter);
            return composite;
        }
        last = m_part->getCollisionGeometry(m_part->getCollisionGeometryCount() - 1);
    }
    // NOTE: `last` is taken as a composite unchecked (the original's)
    if ((UInt32)static_cast<Vx::VxCompositeCollisionGeometry*>(last)->getCollisionGeometryCount() == m_compositeSize) {
        Vx::VxCompositeCollisionGeometry* composite = new Vx::VxCompositeCollisionGeometry((int)m_compositeSize);
        VortexCollisionGeometryData::Create(composite);
        VortexCollisionGeometryData::SetFilter(composite, filter);
        m_part->addCollisionGeometry(composite);
        return composite;
    }
    return last;
}

// Mac 0x3b46c0, Win 0x52dc60
void VortexPhysicsSceneElement::AddGeometry(VortexShapeCollisionFilter* filter, const Matrix3D* transform,
                                            VortexPhysicsPlacedGeometry* placed, bool update)
{
    Vx::VxCollisionGeometry* geometry;
    if (placed->m_collisionGeometry == 0) {
        geometry = VortexPhysicsPlacedGeometry::CreateCollisionGeometry(placed, placed->m_geometry, placed->m_transform);
        placed->m_collisionGeometry = geometry;
    } else {
        placed->SetCollisionTransform(placed->m_transform);
        geometry = placed->m_collisionGeometry;
    }
    if (geometry->mEntity == m_part) {
        Vx::VxWarning(0, "VortexPhysicsRigidElement::addRigidElements: cg alreadty added.\n");
        return;
    }
    if (geometry->mEntity != 0) {
        Vx::VxWarning(0, "VortexPhysicsRigidElement::addRigidElements: cg already added to another body.\n");
        return;
    }
    if (transform != 0) {
        Vx::VxTransform relative;
        geometry->getTransformRelative(relative);
        Vx::VxTransform tm;
        ToVxTransform(*transform, tm);
        tm.preMultiply(relative);
        geometry->setTransformRelative(&tm);
    }
    Vx::VxCollisionGeometry* updated = geometry;
    if (!m_useComposites) {
        VortexCollisionGeometryData::SetFilter(geometry, filter);
        m_part->addCollisionGeometry(geometry);
        if (!update)
            return;
        geometry->updateCollisionModel();
    } else {
        Vx::VxCompositeCollisionGeometry* composite =
            static_cast<Vx::VxCompositeCollisionGeometry*>(GetComposite(0, filter));
        composite->addCollisionGeometry(geometry);
        if (!update)
            return;
        composite->updateCollisionModel();
        updated = composite;
    }
    updated->update();
}

// Mac 0x3b4900
void VortexPhysicsSceneElement::RemoveGeometry(VortexPhysicsPlacedGeometry* placed)
{
    if (placed->m_collisionGeometry != 0 && placed->m_collisionGeometry->mEntity == m_part)
        RemoveCollisionGeometry(placed->m_collisionGeometry);
}

// Mac 0x3b4920, Win 0x52dd70
void VortexPhysicsSceneElement::RemoveCollisionGeometry(Vx::VxCollisionGeometry* geometry)
{
    Vx::VxCompositeCollisionGeometry* parent = static_cast<Vx::VxCompositeCollisionGeometry*>(geometry->getParent());
    if (parent == 0) {
        geometry->mEntity->removeCollisionGeometry(geometry);
        return;
    }
    parent->removeCollisionGeometry(geometry);
    if (parent->getCollisionGeometryCount() == 0) {
        RemoveCollisionGeometry(parent);
        VortexCollisionGeometryData::Destroy(parent);
        delete parent;
    }
}

// =============================================================================================
// LDDVortexCollisionFilter
// =============================================================================================

// Mac 0x3b49a0, Win 0x52d850
bool LDDVortexCollisionFilter::accept(Vx::VxCollisionGeometry* a, Vx::VxCollisionGeometry* b)
{
    if (m_mode == ModeCallback) {
        VortexShapeUserData* shape = VortexCollisionGeometryData::GetShape(a);
        VortexShapeUserData* otherShape = VortexCollisionGeometryData::GetShape(b);
        if (shape != 0 && otherShape != 0 && shape->m_kind == VortexShapeUserData::KindBrick &&
            otherShape->m_kind == VortexShapeUserData::KindBrick) {
            const PhysicsGeometryIdentifier& id = static_cast<VortexPhysicsBrickPlacedGeometry*>(shape)->m_identifier;
            const PhysicsGeometryIdentifier& otherId =
                static_cast<VortexPhysicsBrickPlacedGeometry*>(otherShape)->m_identifier;
            if (id.m_element == otherId.m_element)
                return false;
            if (m_callback != 0 && m_callback->OnCollision(id, otherId) != PhysicsCollisionCallback::Collision)
                return false;
        }
    } else if (m_mode == ModeTerrain) {
        VortexShapeUserData* shape = VortexCollisionGeometryData::GetShape(a);
        VortexShapeUserData* otherShape = VortexCollisionGeometryData::GetShape(b);
        if (shape != 0 && shape->m_kind == VortexShapeUserData::KindTerrain)
            return true;
        return otherShape != 0 && otherShape->m_kind == VortexShapeUserData::KindTerrain;
    }
    // the flexible connections of the elements
    VortexShapeCollisionFilter* filter = VortexCollisionGeometryData::GetFilter(a);
    VortexShapeCollisionFilter* otherFilter = VortexCollisionGeometryData::GetFilter(b);
    if (filter == 0 || otherFilter == 0)
        return true;
    for (UInt32 i = 0; i < filter->m_count; ++i)
        if (filter->m_filters[i] == otherFilter)
            return false;
    return true;
}

} // namespace LEGO
