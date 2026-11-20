// LEGO Digital Designer — restored source
// LEGO::VortexPhysicsPrimitiveElement (Mac TU 433). See VortexPhysicsPrimitiveElement.h.
#include "VortexPhysicsPrimitiveElement.h"
#include "VortexPhysicsGeometry.h"
#include "VortexConversions.h"

#include "Vx/VxCollisionGeometry.h"

namespace LEGO {

// Mac 0x3a9600 (C1 0x3a99c0; PushBack 0x3a9860)
VortexPhysicsPrimitiveElement::VortexPhysicsPrimitiveElement(const PhysicsPlacedConfiguredGeometry* geometries,
                                                             UInt32 count, const PhysicsMassProperties& properties)
    : BaseBrickPhysicsPrimitiveElement(properties), m_transform()
{
    if (count != 0) {
        m_geometries.Reserve(count);
        for (UInt32 i = 0; i < count; ++i) {
            VortexPhysicsGeometry* geometry;
            geometries[i].m_geometry->QueryInterface(VortexPhysicsGeometry::GetIID(), (void**)&geometry);
            VortexPhysicsBrickPlacedGeometry placed(VortexShapeUserData::KindBrick, geometry, geometries[i].m_transform,
                                                    geometries[i].m_type, i, *this);
            m_geometries.PushBack(placed);
            geometry->Release();
        }
    }
}

// Mac 0x3a99d0, Win 0x52b140
void VortexPhysicsPrimitiveElement::SetFeatureType(UInt32 feature, UInt32 type)
{
    if (m_geometries[feature].m_configuration != type)
        m_geometries[feature].SetConfiguration(type);
}

// Mac 0x3a9a10, Win 0x52b170
UInt32 VortexPhysicsPrimitiveElement::GetGeometryCount() const
{
    return m_geometries.GetSize();
}

// Mac 0x3a9a90, Win 0x52b260
bool VortexPhysicsPrimitiveElement::GetGeometry(UInt32 index, PhysicsPlacedConfiguredGeometry& geometry) const
{
    if (index >= m_geometries.GetSize())
        return false;
    const VortexPhysicsBrickPlacedGeometry& placed = m_geometries[index];
    if (placed.m_geometry->QueryInterface(PhysicsGeometry::GetIID(), (void**)&geometry.m_geometry) < 0)
        return false;
    geometry.m_transform = placed.m_transform;
    geometry.m_type = placed.m_configuration;
    return true;
}

// Mac 0x3a9b90, Win 0x52b2e0
void VortexPhysicsPrimitiveElement::AddFlexibleConnection(PhysicsPrimitiveElement* other)
{
    VortexPhysicsPrimitiveElement* element = static_cast<VortexPhysicsPrimitiveElement*>(other);
    VortexShapeCollisionFilter::Add(element);
    element->VortexShapeCollisionFilter::Add(this);
}

// Mac 0x3a9bc0, Win 0x52b320
void VortexPhysicsPrimitiveElement::RemoveFlexibleConnection(PhysicsPrimitiveElement* other)
{
    VortexPhysicsPrimitiveElement* element = static_cast<VortexPhysicsPrimitiveElement*>(other);
    VortexShapeCollisionFilter::Remove(element);
    element->VortexShapeCollisionFilter::Remove(this);
}

// Mac 0x3a9bf0, Win 0x52b3a0
void VortexPhysicsPrimitiveElement::SetTransform(const Matrix3D& transform)
{
    m_transform = transform;
}

// Mac 0x3a9a30, Win 0x52b190
void VortexPhysicsPrimitiveElement::SetGeometryEnabled(Int32 index, bool enabled)
{
    if (index == -1) {
        for (VortexPhysicsBrickPlacedGeometry* it = m_geometries.Begin(); it != m_geometries.End(); ++it)
            it->SetEnabled(enabled);
    } else {
        m_geometries[(UInt32)index].SetEnabled(enabled);
    }
}

// Mac 0x3a9c70, Win 0x52b3c0
Vx::VxCollisionGeometry* VortexPhysicsPrimitiveElement::CreateCollisionGeometry(bool inBody)
{
    if (GetGeometryCount() == 0)
        return 0;
    Vx::VxCompositeCollisionGeometry* composite = new Vx::VxCompositeCollisionGeometry((int)GetGeometryCount());
    for (VortexPhysicsBrickPlacedGeometry* it = m_geometries.Begin(); it != m_geometries.End(); ++it) {
        if (!it->m_enabled)
            continue;
        Vx::VxCollisionGeometry* geometry =
            VortexPhysicsPlacedGeometry::CreateCollisionGeometry(it, it->m_geometry, it->m_transform);
        if (geometry != 0)
            composite->addCollisionGeometry(geometry);
    }
    VortexCollisionGeometryData::Create(composite);
    VortexCollisionGeometryData::SetFilter(composite, this);
    if (composite->getCollisionGeometryCount() == 0) {
        delete composite;
        return 0;
    }
    if (!inBody) {
        Vx::VxTransform relative;
        ToVxTransform(m_transform, relative);
        if (!relative.isIdentity(1e-14))
            composite->setTransformRelative(&relative);
    }
    return composite;
}

} // namespace LEGO
