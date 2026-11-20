// LEGO Digital Designer — restored source
// LEGO::VortexPhysicsSceneryElement (Mac TU 437). See VortexPhysicsSceneryElement.h.
#include "VortexPhysicsSceneryElement.h"
#include "VortexPhysicsScene.h"
#include "VortexPhysicsGeometry.h"
#include "VortexConversions.h"
#include "BaseBrickPhysicsAtomic.h"

#include "Vx/VxPart.h"
#include "Vx/VxCollisionGeometry.h"

#include <string.h>

namespace LEGO {

// Mac 0x3b4af0 (C1 0x3b4f60)
VortexPhysicsSceneryElement::VortexPhysicsSceneryElement(const PhysicsPlacedConfiguredGeometry* geometries, UInt32 count,
                                                         const PhysicsMassProperties& properties)
    : VortexPhysicsSceneElement(KindScenery), m_properties(properties), m_scene(0), m_userData(0), m_flags(1),
      m_dynamic(false), m_refCount(1)
{
    m_geometries.Reserve(count);
    for (UInt32 i = 0; i < count; ++i) {
        VortexPhysicsGeometry* geometry = 0;
        geometries->m_geometry->QueryInterface(VortexPhysicsGeometry::GetIID(), reinterpret_cast<void**>(&geometry));
        VortexPhysicsPlacedGeometry placed(VortexShapeUserData::KindBrick, this, geometry, geometries->m_transform,
                                           geometries->m_type);
        m_geometries.PushBack(placed);
        geometry->Release();
        ++geometries;
    }
}

// Mac 0x3b4f70, Win 0x529720
Int32 LEGO_COM_CALL VortexPhysicsSceneryElement::QueryInterface(const InterfaceID& id, void** object)
{
    if (memcmp(&id, &PhysicsSceneryElement::GetIID(), sizeof(InterfaceID)) != 0 &&
        memcmp(&id, &PhysicsSceneElement::GetIID(), sizeof(InterfaceID)) != 0) {
        *object = 0;
        return kPhysicsNoInterface;
    }
    *object = static_cast<PhysicsSceneryElement*>(this);
    AddRef();
    return 0;
}

// Mac 0x3b5040, Win 0x529780
UInt32 LEGO_COM_CALL VortexPhysicsSceneryElement::AddRef()
{
    return (UInt32)PhysicsAtomicIncrement(m_refCount);
}

// Mac 0x3b5060, Win 0x5297a0
UInt32 LEGO_COM_CALL VortexPhysicsSceneryElement::Release()
{
    Int32 count = PhysicsAtomicDecrement(m_refCount);
    if (count == 0)
        delete this;
    return (UInt32)count;
}

// Mac 0x3b50a0, Win 0x5297d0
void VortexPhysicsSceneryElement::GetTransform(Matrix3D& transform) const
{
    const Vx::VxTransform& tm = *m_part->mTransformPtr;
    for (int i = 0; i < 16; ++i)
        transform.m[i] = (Real32)tm.m[i / 4][i % 4];
}

// Mac 0x3b5120, Win 0x5297f0
const PhysicsMassProperties& VortexPhysicsSceneryElement::GetMassProperties() const
{
    if (m_properties.m_mass < 0.0f) {
        m_properties.m_inertia.SetIdentity();
        m_properties.m_mass = -m_properties.m_mass;
    }
    return m_properties;
}

// Mac 0x3b55c0, Win 0x5294e0
void* VortexPhysicsSceneryElement::GetUserData() const
{
    return m_userData;
}

// Mac 0x3b55d0, Win 0x5294f0
void VortexPhysicsSceneryElement::SetUserData(void* data)
{
    m_userData = data;
}

// Mac 0x3b5170, Win 0x529830
UInt32 VortexPhysicsSceneryElement::GetGeometryCount() const
{
    return m_geometries.GetSize();
}

// Mac 0x3b5190, Win 0x529850
bool VortexPhysicsSceneryElement::GetGeometry(UInt32 index, PhysicsPlacedConfiguredGeometry& geometry) const
{
    if (index >= m_geometries.GetSize())
        return false;
    const VortexPhysicsPlacedGeometry& placed = m_geometries[index];
    placed.m_geometry->QueryInterface(PhysicsGeometry::GetIID(), reinterpret_cast<void**>(&geometry.m_geometry));
    if (geometry.m_geometry == 0)
        return false;
    geometry.m_transform = placed.m_transform;
    geometry.m_type = placed.m_configuration;
    return true;
}

// Mac 0x3b5280, Win 0x5298d0
void VortexPhysicsSceneryElement::SetDynamic(bool dynamic)
{
    if (m_properties.m_mass != 0.0f && m_dynamic != dynamic) {   // (NaN counts as a mass)
        m_dynamic = dynamic;
        if (m_part != 0)
            m_part->freeze(!dynamic);
    }
}

// Mac 0x3b55f0, Win 0x529500
bool VortexPhysicsSceneryElement::IsDynamic() const
{
    return m_dynamic;
}

// Mac 0x3b52e0, Win 0x529910
void VortexPhysicsSceneryElement::SetLinearVelocity(const Vector3& impulse)
{
    m_part->addImpulse(ToVxVector3(impulse));
}

// Mac 0x3b5320, Win 0x529950
void VortexPhysicsSceneryElement::AddImpulseAtPosition(const Vector3& impulse, const Vector3& position)
{
    m_part->addImpulseAtPosition(ToVxVector3(impulse), ToVxVector3(position));
}

// Mac 0x3b5380
void VortexPhysicsSceneryElement::AddToScene(VortexPhysicsScene* scene, const Matrix3D& transform, UInt32 flags)
{
    m_scene = scene;
    m_kind = 5 - ((flags >> 1) & 1);
    m_flags = flags;
    CreatePart(scene->m_mechanism);
    Vx::VxPart* part = m_part;
    part->mUserInertia = true;
    part->mUserCOM = true;
    part->mUserMass = true;
    for (VortexPhysicsPlacedGeometry* it = m_geometries.Begin(); it != m_geometries.End(); ++it)
        it->m_kind = flags;
    for (VortexPhysicsPlacedGeometry* it = m_geometries.Begin(); it != m_geometries.End(); ++it)
        AddGeometry(this, 0, it, false);
    if (m_part->getCollisionGeometryCount() > 0) {
        m_part->updateCollisionGeometryTransforms();
        m_part->updateMassPropertiesFromGeometry();
    }
    ScaleMass(0.2f);
    m_part->mUserInertia = true;
    m_part->mUserCOM = true;
    m_part->mUserMass = true;
    Vx::VxTransform tm;
    ToVxTransform(transform, tm);
    m_part->setTransform(tm);
    if (!m_dynamic)
        m_part->freeze(true);
    else
        m_part->updateCollisionGeometries();
}

// Mac 0x3b5590
void VortexPhysicsSceneryElement::RemoveFromScene()
{
    DestroyPart();
    m_scene = 0;
}

} // namespace LEGO
