// LEGO Digital Designer — restored source
// LEGO::VortexPhysicsPlacedGeometry / VortexPhysicsBrickPlacedGeometry / VortexCollisionGeometryData
// (Mac TU 432). See VortexPhysicsPlacedGeometry.h.
#include "VortexPhysicsPlacedGeometry.h"
#include "VortexPhysicsGeometry.h"
#include "VortexPhysicsGeometryConfigable.h"
#include "VortexPhysicsPrimitiveElement.h"
#include "LDDVortexCollisionFilter.h"
#include "VortexConversions.h"

#include "Vx/VxCollisionGeometry.h"
#include "Vx/VxEntity.h"
#include "Vx/VxGeometry.h"
#include "Vx/VxHeightField.h"
#include "Vx/VxMessage.h"
#include "Me/VinPool.h"
#include <new>

namespace LEGO {

namespace {

// The data of the collision geometries (17 per brick) as structs of a pool growing by blocks of
// 4096, instead of a heap block each (24 bytes, and the heap's own bookkeeping on top). Used next
// to the Vortex calls that create and delete the geometries, so one thread at a time, as
// Vortex's own pools. (LDD64)
VinPool g_geometryDataPool;
bool    g_geometryDataPoolReady = false;

// The intersect filter of the collision models (VxCollisionGeometry::setIntersectFilter, the
// former unk_437f26, Mac 0x437f26): the LEGO filter, or none.
inline Vx::VxIntersectFilter* FilterValue(bool on)
{
    return on ? &g_lddVortexCollisionFilter : 0;
}

inline bool IsComposite(Vx::VxCollisionGeometry* geometry)
{
    return geometry->isOfClassType(Vx::VxCompositeCollisionGeometry::getStaticClassType());
}

inline VortexCollisionGeometryData* GetData(Vx::VxCollisionGeometry* geometry)
{
    return static_cast<VortexCollisionGeometryData*>(geometry->getUserData());
}

} // namespace

// =============================================================================================
// VortexCollisionGeometryData
// =============================================================================================

// Mac 0x3a9240, Win 0x522d80
void VortexCollisionGeometryData::Create(Vx::VxCollisionGeometry* geometry)
{
    if (!g_geometryDataPoolReady) {
        g_geometryDataPool.name = "VortexCollisionGeometryData";
        VinPoolInit(&g_geometryDataPool, 4096, (int)sizeof(VortexCollisionGeometryData), 4096, 0.0f, 0);
        g_geometryDataPoolReady = true;
    }
    VortexCollisionGeometryData* data =
        new (VinPoolGetStruct(&g_geometryDataPool)) VortexCollisionGeometryData;
    data->m_configuration = 0;
    data->m_filter = 0;
    data->m_shape = 0;
    geometry->setUserData(data);
}

// Mac 0x3a9470
void VortexCollisionGeometryData::Destroy(Vx::VxCollisionGeometry* geometry)
{
    VortexCollisionGeometryData* data = GetData(geometry);
    geometry->setUserData(0);
    if (data != 0)
        VinPoolPutStruct(&g_geometryDataPool, data);
}

// Mac 0x3a8a70, Win 0x52df30
void VortexCollisionGeometryData::DestroyAll(Vx::VxCollisionGeometry* geometry)
{
    Destroy(geometry);
    if (IsComposite(geometry)) {
        Vx::VxCompositeCollisionGeometry* composite = static_cast<Vx::VxCompositeCollisionGeometry*>(geometry);
        for (int i = 0; i < composite->getCollisionGeometryCount(); ++i)
            DestroyAll(composite->getCollisionGeometry(i));
    }
}

// Mac 0x3a94a0, Win 0x52e730
void VortexCollisionGeometryData::DestroyAll(Vx::VxCollisionSensor* sensor)
{
    for (int i = 0; i < sensor->getCollisionGeometryCount(); ++i)
        DestroyAll(sensor->getCollisionGeometry(i));
}

// Mac 0x3a94f0
VortexShapeCollisionFilter* VortexCollisionGeometryData::GetFilter(Vx::VxCollisionGeometry* geometry)
{
    VortexCollisionGeometryData* data = GetData(geometry);
    if (data != 0)
        return data->m_filter;
    return 0;
}

// Mac 0x3a95a0
VortexShapeUserData* VortexCollisionGeometryData::GetShape(Vx::VxCollisionGeometry* geometry)
{
    VortexCollisionGeometryData* data = GetData(geometry);
    if (data != 0)
        return data->m_shape;
    return 0;
}

// Mac 0x3a9280
void VortexCollisionGeometryData::SetShape(Vx::VxCollisionGeometry* geometry, VortexShapeUserData* shape)
{
    GetData(geometry)->m_shape = shape;
    geometry->setIntersectFilter(FilterValue(shape != 0));
}

// Mac 0x3a9510
void VortexCollisionGeometryData::SetFilter(Vx::VxCollisionGeometry* geometry, VortexShapeCollisionFilter* filter)
{
    GetData(geometry)->m_filter = filter;
    geometry->setIntersectFilter(FilterValue(filter != 0));
    if (IsComposite(geometry)) {
        Vx::VxCompositeCollisionGeometry* composite = static_cast<Vx::VxCompositeCollisionGeometry*>(geometry);
        for (int i = 0; i < composite->getCollisionGeometryCount(); ++i)
            SetFilter(composite->getCollisionGeometry(i), filter);
    }
}

// =============================================================================================
// VortexPhysicsPlacedGeometry
// =============================================================================================

// Mac 0x3a8620
VortexPhysicsPlacedGeometry::VortexPhysicsPlacedGeometry(UInt32 kind, VortexShapeCollisionFilter* filter,
                                                         VortexPhysicsGeometry* geometry, const Matrix3D& transform,
                                                         UInt32 configuration)
    : VortexShapeUserData(kind, filter), m_transform(transform), m_configuration(configuration),
      m_collisionGeometry(0), m_geometry(geometry)
{
    m_geometry->AddRef();
}

// Mac 0x3a8680
VortexPhysicsPlacedGeometry::VortexPhysicsPlacedGeometry(const VortexPhysicsPlacedGeometry& o)
    : VortexShapeUserData(o), m_transform(o.m_transform), m_configuration(o.m_configuration),
      m_collisionGeometry(0), m_geometry(o.m_geometry)
{
    m_geometry->AddRef();
}

// Mac D1 0x3a8810 / D0 0x3a8860, Win sdd 0x52de70
VortexPhysicsPlacedGeometry::~VortexPhysicsPlacedGeometry()
{
    m_geometry->Release();
    if (m_collisionGeometry != 0) {
        VortexCollisionGeometryData::DestroyAll(m_collisionGeometry);
        delete m_collisionGeometry;
        m_collisionGeometry = 0;
    }
}

// Win 0x52d740 (Mac inlined)
Vx::VxCollisionGeometry* VortexPhysicsPlacedGeometry::UpdateCollisionGeometry()
{
    if (m_collisionGeometry == 0) {
        m_collisionGeometry = CreateCollisionGeometry(this, m_geometry, m_transform);
    } else {
        Vx::VxTransform transform;
        ToVxTransform(m_transform, transform);
        m_collisionGeometry->setTransformRelative(&transform);
    }
    return m_collisionGeometry;
}

// Mac 0x3a86f0 (Win inlined into VortexPhysicsPrimitiveElement 0x52b190)
void VortexPhysicsPlacedGeometry::SetEnabled(bool enabled)
{
    m_enabled = enabled;
    Vx::VxCollisionGeometry* geometry = UpdateCollisionGeometry();
    if (geometry != 0)
        geometry->setCollisionMode(enabled ? 0 : 1);
}

// Mac 0x3a88d0, Win 0x52dfc0
void VortexPhysicsPlacedGeometry::SetConfiguration(UInt32 configuration)
{
    if (m_geometry->GetNativeGeometry() != 0) {
        Vx::VxWarning(0, "VortexPhysicsPlacedGeometry::setConfiguration: was called on a non-configable geometry.\n");
        return;
    }
    VortexPhysicsGeometryConfigable* configable = static_cast<VortexPhysicsGeometryConfigable*>(m_geometry);
    m_configuration = configuration;
    Vx::VxCompositeCollisionGeometry* composite =
        static_cast<Vx::VxCompositeCollisionGeometry*>(m_collisionGeometry);
    if (composite == 0)
        return;
    for (int i = composite->getCollisionGeometryCount(); i > 0; --i) {
        Vx::VxCollisionGeometry* child = composite->getCollisionGeometry(i - 1);
        composite->removeCollisionGeometry(child);
        VortexCollisionGeometryData::DestroyAll(child);
        delete child;
    }
    if (m_configuration != 0) {
        Int32 count = (Int32)configable->GetGeometryCount(m_configuration);
        for (Int32 i = 0; i < count; ++i) {
            Matrix3D transform;
            VortexPhysicsGeometry* part = configable->GetPlacedGeometry(m_configuration, (UInt32)i, transform);
            Vx::VxCollisionGeometry* child = CreateCollisionGeometry(this, part, transform);
            composite->addCollisionGeometry(child);
            GetData(child)->m_configuration = m_configuration;
        }
    }
    if (composite->getCollisionGeometryCount() == 0) {
        composite->setCollisionMode(2);
    } else {
        composite->setCollisionMode(0);
        Vx::VxCollisionGeometry* top = composite;
        while (top->getParent() != 0)
            top = top->getParent();
        top->updateCollisionModel();
        top->update();
    }
}

// Mac 0x3a8af0, Win 0x52e150
Vx::VxCollisionGeometry* VortexPhysicsPlacedGeometry::CreateCollisionGeometry(VortexPhysicsPlacedGeometry* placed,
                                                                              VortexPhysicsGeometry* geometry,
                                                                              const Matrix3D& transform)
{
    if (geometry->GetNativeGeometry() == 0) {
        // a configurable geometry: a composite of the configuration's parts
        VortexPhysicsGeometryConfigable* configable = static_cast<VortexPhysicsGeometryConfigable*>(geometry);
        Int32 count = (Int32)configable->GetGeometryCount(placed->m_configuration);
        Vx::VxCompositeCollisionGeometry* composite = new Vx::VxCompositeCollisionGeometry(count > 1 ? count : 2);
        VortexCollisionGeometryData::Create(composite);
        for (Int32 i = 0; i < count; ++i) {
            Matrix3D partTransform;
            VortexPhysicsGeometry* part =
                configable->GetPlacedGeometry(placed->m_configuration, (UInt32)i, partTransform);
            Vx::VxCollisionGeometry* child = CreateCollisionGeometry(placed, part, partTransform);
            composite->addCollisionGeometry(child);
            GetData(child)->m_configuration = placed->m_configuration;
        }
        Vx::VxTransform relative;
        ToVxTransform(transform, relative);
        composite->setTransformRelative(&relative);
        GetData(composite)->m_shape = placed;
        // NOTE: unconditional here (the primitive branch tests this)
        composite->setIntersectFilter(FilterValue(true));
        composite->setCollisionMode(composite->getCollisionGeometryCount() == 0 ? 2 : 0);
        return composite;
    }

    Vx::VxCollisionGeometry* collision =
        new Vx::VxCollisionGeometry(static_cast<Vx::VxGeometry*>(geometry->GetNativeGeometry()));
    VortexCollisionGeometryData::Create(collision);
    Vx::VxTransform relative;
    ToVxTransform(transform, relative);
    if (collision->getGeometry()->isOfClassType(Vx::VxHeightField::getStaticClassType())) {
        // a Vx height field lies in its x / y plane around its centre: turn it into the x / z
        // plane and move its corner to the origin (NOTE: the placement's translation is dropped)
        Vx::VxTransform rotation;
        rotation.makeRotation(1.5707963267948966, 0.0, 0.0, 0x20);
        relative.postMultiply(rotation);
        rotation = Vx::VxTransform();
        rotation.makeRotation(0.0, 1.5707963267948966, 0.0, 0x20);
        relative.postMultiply(rotation);
        const Vx::VxHeightField* field = static_cast<const Vx::VxHeightField*>(collision->getGeometry());
        relative.m[3][0] = (Vx::VxReal)field->mXCells * (field->mXSpacing * 0.5);
        relative.m[3][1] = 0.0;
        relative.m[3][2] = (Vx::VxReal)field->mYCells * (0.5 * field->mYSpacing);
    }
    collision->setTransformRelative(&relative);
    collision->setMaterial(0);
    GetData(collision)->m_shape = placed;
    collision->setIntersectFilter(FilterValue(placed != 0));
    return collision;
}

// Mac 0x3a9170
void VortexPhysicsPlacedGeometry::SetCollisionTransform(const Matrix3D& transform)
{
    Vx::VxTransform relative;
    ToVxTransform(transform, relative);
    m_collisionGeometry->setTransformRelative(&relative);
}

// =============================================================================================
// VortexPhysicsBrickPlacedGeometry
// =============================================================================================

// Mac 0x3a92b0
VortexPhysicsBrickPlacedGeometry::VortexPhysicsBrickPlacedGeometry(UInt32 kind, VortexPhysicsGeometry* geometry,
                                                                   const Matrix3D& transform, UInt32 configuration,
                                                                   UInt32 index,
                                                                   VortexPhysicsPrimitiveElement& element)
    : VortexPhysicsPlacedGeometry(kind, &element, geometry, transform, configuration)
{
    m_identifier.m_element = &element;
    m_identifier.m_feature = index;
}

// Mac 0x3a9330
VortexPhysicsBrickPlacedGeometry::VortexPhysicsBrickPlacedGeometry(const VortexPhysicsBrickPlacedGeometry& o)
    : VortexPhysicsPlacedGeometry(o), m_identifier(o.m_identifier)
{
}

} // namespace LEGO
