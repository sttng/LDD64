// LEGO Digital Designer — restored source
// The geometries of the Vortex physics (Mac TU 431). See VortexPhysicsGeometryConfigable.h.
#include "VortexPhysicsGeometryConfigable.h"
#include "BaseBrickPhysicsAtomic.h"

#include "Vx/VxGeometry.h"
#include "Vx/VxHeightField.h"

#include <string.h>

namespace LEGO {

namespace {

const Real32 kMargin = 0.007f;   // Mac 0x63f7d4 / -0x63f7d0

inline bool SameID(const InterfaceID& a, const InterfaceID& b)
{
    return memcmp(&a, &b, sizeof a) == 0;
}

} // namespace

// The IUnknown of the six geometries (identical but for the class). Release deletes at 0.
#define LEGO_VORTEX_GEOMETRY_IMPLEMENT_IUNKNOWN(Class, Interface)                                  \
    Int32 Class::QueryInterface(const InterfaceID& id, void** object)                             \
    {                                                                                              \
        if (SameID(id, VortexPhysicsGeometry::GetIID())) {                                         \
            *object = static_cast<VortexPhysicsGeometry*>(this);                                   \
        } else if (SameID(id, Interface::GetIID()) || SameID(id, PhysicsGeometry::GetIID())) {      \
            *object = static_cast<Interface*>(this);                                               \
        } else {                                                                                   \
            *object = 0;                                                                           \
            return kPhysicsNoInterface;                                                            \
        }                                                                                          \
        AddRef();                                                                                  \
        return 0;                                                                                  \
    }                                                                                              \
    UInt32 Class::AddRef()                                                                         \
    {                                                                                              \
        return (UInt32)PhysicsAtomicIncrement(m_refCount);                                         \
    }                                                                                              \
    UInt32 Class::Release()                                                                        \
    {                                                                                              \
        Int32 count = PhysicsAtomicDecrement(m_refCount);                                          \
        if (count == 0)                                                                            \
            delete this;                                                                           \
        return (UInt32)count;                                                                      \
    }

// =============================================================================================
// box
// =============================================================================================

// Mac 0x3a4b20 (thunk 0x3a4c40), 0x3a7430 (0x3a74b0), 0x3a7450 (0x3a74d0); Win 0x51e900, 0x51eb00
LEGO_VORTEX_GEOMETRY_IMPLEMENT_IUNKNOWN(VortexPhysicsGeometryBox, PhysicsGeometryBox)

// Mac 0x3a4d10 (C1 0x3a4e50)
VortexPhysicsGeometryBox::VortexPhysicsGeometryBox(const Vector3& halfSize) : m_refCount(1), m_size()
{
    Real32 x = halfSize.x + kMargin;
    m_size.x = x + x;
    Real32 y = halfSize.y + kMargin;
    m_size.y = y + y;
    Real32 z = halfSize.z + kMargin;
    m_size.z = z + z;
    m_geometry = new Vx::VxBox(Vx::VxVector3(m_size.x, m_size.y, m_size.z));
    m_geometry->addRef();
}

// Mac D1 0x3a4e60 / D0 0x3a4f00 (thunks 0x3a4eb0 / 0x3a4f60), Win sdd 0x51e9b0
VortexPhysicsGeometryBox::~VortexPhysicsGeometryBox()
{
    m_geometry->release();
}

// Mac 0x3a7490, Win 0x51ec40
UInt32 VortexPhysicsGeometryBox::GetKind() const
{
    return KindBox;
}

// Mac 0x3a4c60
Vector3 VortexPhysicsGeometryBox::GetHalfSize() const
{
    Vx::VxVector3 d;
    m_geometry->getDimensions(d);
    return Vector3((Real32)d[0] * 0.5f - kMargin, (Real32)d[1] * 0.5f - kMargin, (Real32)d[2] * 0.5f - kMargin);
}

// Mac 0x3a74a0 / 0x3a7510
void* VortexPhysicsGeometryBox::GetNativeGeometry() const
{
    return m_geometry;
}

// Mac 0x3a4fd0 / 0x3a5060
void VortexPhysicsGeometryBox::Inflate(Real32 amount)
{
    Vx::VxVector3 d(0.0, 0.0, 0.0);
    m_geometry->getDimensions(d);
    Vx::VxReal a = (Vx::VxReal)amount;
    if (0.0 < d[0] + a)
        d[0] = d[0] + a;
    if (0.0 < d[1] + a)
        d[1] = d[1] + a;
    if (0.0 < a + d[2])
        d[2] = a + d[2];
    m_geometry->setDimensions(d);
}

// Mac 0x3a50f0 / 0x3a5130
void VortexPhysicsGeometryBox::ResetSize()
{
    Vx::VxVector3 d(m_size.x, m_size.y, m_size.z);
    m_geometry->setDimensions(d);
}

// =============================================================================================
// sphere
// =============================================================================================

// Mac 0x3a5170 (0x3a5290), 0x3a7520 (0x3a75a0), 0x3a7540 (0x3a75c0); Win 0x51ec70, 0x51eea0
LEGO_VORTEX_GEOMETRY_IMPLEMENT_IUNKNOWN(VortexPhysicsGeometrySphere, PhysicsGeometrySphere)

// Mac 0x3a52f0, Win 0x51f2a0
VortexPhysicsGeometrySphere::VortexPhysicsGeometrySphere(Real32 radius) : m_refCount(1)
{
    m_radius = radius + kMargin;
    m_geometry = new Vx::VxSphere((Vx::VxReal)m_radius);
    m_geometry->addRef();
}

// Mac D1 0x3a5390 / D0 0x3a5430 (thunks 0x3a53e0 / 0x3a5490)
VortexPhysicsGeometrySphere::~VortexPhysicsGeometrySphere()
{
    m_geometry->release();
}

// Mac 0x3a7580, Win 0x51eee0
UInt32 VortexPhysicsGeometrySphere::GetKind() const
{
    return KindSphere;
}

// Mac 0x3a52b0
Real32 VortexPhysicsGeometrySphere::GetRadius() const
{
    return (Real32)m_geometry->getRadius() - kMargin;
}

// Mac 0x3a7590 / 0x3a7600
void* VortexPhysicsGeometrySphere::GetNativeGeometry() const
{
    return m_geometry;
}

// Mac 0x3a5500 / 0x3a5550
void VortexPhysicsGeometrySphere::Inflate(Real32 amount)
{
    Vx::VxReal r = m_geometry->getRadius();
    if (0.0 < (Vx::VxReal)amount + r)
        m_geometry->setRadius((Vx::VxReal)amount + r);
}

// Mac 0x3a55a0 / 0x3a55d0, Win 0x51ed20
void VortexPhysicsGeometrySphere::ResetSize()
{
    m_geometry->setRadius((Vx::VxReal)m_radius);
}

// =============================================================================================
// capsule
// =============================================================================================

// Mac 0x3a5600 (0x3a5720), 0x3a7610 (0x3a7690), 0x3a7630 (0x3a76b0); Win 0x51ef00, 0x51e7b0
LEGO_VORTEX_GEOMETRY_IMPLEMENT_IUNKNOWN(VortexPhysicsGeometryCapsule, PhysicsGeometryCapsule)

// Mac 0x3a57a0, Win 0x51ed40
VortexPhysicsGeometryCapsule::VortexPhysicsGeometryCapsule(Real32 radius, Real32 length) : m_refCount(1)
{
    m_geometry = new Vx::VxCapsule((Vx::VxReal)radius, (Vx::VxReal)length);
    m_geometry->addRef();
    m_radius = radius;
    m_length = length;
}

// Mac D1 0x3a5850 / D0 0x3a58f0 (thunks 0x3a58a0 / 0x3a5950)
VortexPhysicsGeometryCapsule::~VortexPhysicsGeometryCapsule()
{
    m_geometry->release();
}

// Mac 0x3a7670
UInt32 VortexPhysicsGeometryCapsule::GetKind() const
{
    return KindCapsule;
}

// Mac 0x3a5740
Real32 VortexPhysicsGeometryCapsule::GetRadius() const
{
    return (Real32)m_geometry->getRadius();
}

// Mac 0x3a5770
Real32 VortexPhysicsGeometryCapsule::GetLength() const
{
    return (Real32)m_geometry->getHeight();
}

// Mac 0x3a7680 / 0x3a76f0
void* VortexPhysicsGeometryCapsule::GetNativeGeometry() const
{
    return m_geometry;
}

// Mac 0x3a59c0 / 0x3a5a10
void VortexPhysicsGeometryCapsule::Inflate(Real32 amount)
{
    Vx::VxReal r = m_geometry->getRadius();
    if (0.0 < (Vx::VxReal)amount + r)
        m_geometry->setRadius((Vx::VxReal)amount + r);
}

// Mac 0x3a5a60 / 0x3a5a90, Win 0x51efd0
void VortexPhysicsGeometryCapsule::ResetSize()
{
    m_geometry->setRadius((Vx::VxReal)m_radius);
}

// =============================================================================================
// cylinder
// =============================================================================================

// Mac 0x3a5ac0 (0x3a5be0), 0x3a7700 (0x3a7780), 0x3a7720 (0x3a77a0); Win 0x51f1b0, 0x51e7b0
LEGO_VORTEX_GEOMETRY_IMPLEMENT_IUNKNOWN(VortexPhysicsGeometryCylinder, PhysicsGeometryCylinder)

// Mac 0x3a5c60, Win 0x51ea10
VortexPhysicsGeometryCylinder::VortexPhysicsGeometryCylinder(Real32 radius, Real32 length) : m_refCount(1)
{
    m_geometry = new Vx::VxCylinder((Vx::VxReal)radius, (Vx::VxReal)length);
    m_geometry->addRef();
    m_radius = radius;
    m_length = length;
}

// Mac D1 0x3a5d10 / D0 0x3a5db0 (thunks 0x3a5d60 / 0x3a5e10), Win sdd 0x51f280
VortexPhysicsGeometryCylinder::~VortexPhysicsGeometryCylinder()
{
    m_geometry->release();
}

// Mac 0x3a7760
UInt32 VortexPhysicsGeometryCylinder::GetKind() const
{
    return KindCylinder;
}

// Mac 0x3a5c00
Real32 VortexPhysicsGeometryCylinder::GetRadius() const
{
    return (Real32)m_geometry->getRadius();
}

// Mac 0x3a5c30
Real32 VortexPhysicsGeometryCylinder::GetLength() const
{
    return (Real32)m_geometry->getHeight();
}

// Mac 0x3a7770 / 0x3a77e0
void* VortexPhysicsGeometryCylinder::GetNativeGeometry() const
{
    return m_geometry;
}

// Mac 0x3a5e80 / 0x3a5f10
void VortexPhysicsGeometryCylinder::Inflate(Real32 amount)
{
    Vx::VxReal r = m_geometry->getRadius() + (Vx::VxReal)amount;
    if (0.0 < r)
        m_geometry->setRadius(r);
    Vx::VxReal h = (Vx::VxReal)amount + m_geometry->getHeight();
    if (0.0 < h)
        m_geometry->setHeight(h);
}

// Mac 0x3a5fa0 / 0x3a5ff0
void VortexPhysicsGeometryCylinder::ResetSize()
{
    m_geometry->setRadius((Vx::VxReal)m_radius);
    m_geometry->setHeight((Vx::VxReal)m_length);
}

// =============================================================================================
// configurable
// =============================================================================================

// Mac 0x3a6040 (0x3a6160), 0x3a77f0 (0x3a7870), 0x3a7810 (0x3a7890); Win 0x51f4a0, 0x5c78e0, 0x51e810
LEGO_VORTEX_GEOMETRY_IMPLEMENT_IUNKNOWN(VortexPhysicsGeometryConfigable, PhysicsGeometryConfigable)

// Mac 0x3a6880 (C1 0x3a6c20; Reserve 0x3a6980)
VortexPhysicsGeometryConfigable::VortexPhysicsGeometryConfigable() : m_refCount(1)
{
    m_configurations.Reserve(8);
}

// Mac D1 0x3a6c30 / D0 0x3a6db0 (thunks 0x3a6cf0 / 0x3a6e80)
VortexPhysicsGeometryConfigable::~VortexPhysicsGeometryConfigable()
{
}

// Mac 0x3a7850, Win 0x51e840
UInt32 VortexPhysicsGeometryConfigable::GetKind() const
{
    return KindConfigurable;
}

// Mac 0x3a6180 (Resize: 0x3a7b10 insert, 0x3a8260 assign; Reserve 0x3a6430, PushBack 0x3a6520),
// Win 0x51f550
bool VortexPhysicsGeometryConfigable::SetGeometries(UInt32 configuration, const PhysicsPlacedGeometry* geometries,
                                                    UInt32 count)
{
    if (configuration == 0)
        return false;
    if (configuration >= m_configurations.GetSize())
        m_configurations.Resize(configuration + 1, Configuration());
    BasicArray<PlacedGeometry>& parts = m_configurations[configuration].m_geometries;
    if (!parts.IsEmpty())
        return false;
    parts.Reserve(count);
    for (UInt32 i = 0; i < count; ++i) {
        VortexPhysicsGeometry* geometry;
        geometries[i].m_geometry->QueryInterface(VortexPhysicsGeometry::GetIID(), (void**)&geometry);
        PlacedGeometry part(geometry, geometries[i].m_transform);
        parts.PushBack(part);
        geometry->Release();
    }
    return true;
}

// Mac 0x3a6660, Win 0x51f740
UInt32 VortexPhysicsGeometryConfigable::GetConfigurationCount()
{
    return m_configurations.GetSize();
}

// Mac 0x3a6680, Win 0x51f750
UInt32 VortexPhysicsGeometryConfigable::GetGeometryCount(UInt32 configuration)
{
    if (configuration < m_configurations.GetSize())
        return m_configurations[configuration].m_geometries.GetSize();
    return 0;
}

// Mac 0x3a66c0, Win 0x51f790
bool VortexPhysicsGeometryConfigable::GetGeometry(UInt32 configuration, UInt32 index,
                                                  PhysicsPlacedConfiguredGeometry& geometry)
{
    if (configuration >= m_configurations.GetSize())
        return false;
    BasicArray<PlacedGeometry>& parts = m_configurations[configuration].m_geometries;
    if (index >= parts.GetSize())
        return false;
    if (parts[index].m_geometry->QueryInterface(PhysicsGeometry::GetIID(), (void**)&geometry.m_geometry) < 0)
        return false;
    geometry.m_transform = parts[index].m_transform;
    return true;
}

// Mac 0x3a7860 / 0x3a78d0
void* VortexPhysicsGeometryConfigable::GetNativeGeometry() const
{
    return 0;
}

// Mac 0x3a67d0
VortexPhysicsGeometry* VortexPhysicsGeometryConfigable::GetPlacedGeometry(UInt32 configuration, UInt32 index,
                                                                          Matrix3D& transform)
{
    if (configuration >= m_configurations.GetSize())
        return 0;
    BasicArray<PlacedGeometry>& parts = m_configurations[configuration].m_geometries;
    if (index >= parts.GetSize())
        return 0;
    transform = parts[index].m_transform;
    return parts[index].m_geometry;
}

// =============================================================================================
// sampled height field
// =============================================================================================

// Mac 0x3a6ea0 (0x3a6fc0), 0x3a7900 (0x3a79c0), 0x3a7920 (0x3a79e0); Win 0x51f9f0, 0x4d43e0
LEGO_VORTEX_GEOMETRY_IMPLEMENT_IUNKNOWN(VortexPhysicsGeometrySampledHeightField, PhysicsGeometrySampledHeightField)

// Mac 0x3a7080 (C1 0x3a72b0), Win 0x51f8b0
VortexPhysicsGeometrySampledHeightField::VortexPhysicsGeometrySampledHeightField(UInt32 sizeX, UInt32 sizeY,
                                                                                 const Vector3& scale, bool flag,
                                                                                 const Real32* heights)
    : m_scale(), m_refCount(1)
{
    m_scale = scale;
    Vx::VxHeightField* field =
        new Vx::VxHeightField((int)sizeX - 1, (int)sizeY - 1, (Vx::VxReal)m_scale.x, (Vx::VxReal)m_scale.z, 0.0);
    m_geometry = field;
    field->addRef();
    m_flag = flag;
    if (sizeX != 0 && sizeY != 0) {
        Vx::VxReal heightScale = (Vx::VxReal)m_scale.y;
        Vx::VxReal* out = field->mHeights.begin();
        for (UInt32 x = 0; x < sizeX; ++x) {
            for (UInt32 y = 0; y < sizeY; ++y)
                out[y] = (Vx::VxReal)heights[y] * heightScale;
            out += sizeY;
            heights += sizeY;
        }
    }
    field->build();
}

// Mac D1 0x3a72c0 / D0 0x3a7360 (thunks 0x3a7310 / 0x3a73c0), Win sdd 0x51fac0
VortexPhysicsGeometrySampledHeightField::~VortexPhysicsGeometrySampledHeightField()
{
    m_geometry->release();
}

// Mac 0x3a7960, Win 0x51e850
UInt32 VortexPhysicsGeometrySampledHeightField::GetKind() const
{
    return KindSampledHeightField;
}

// Mac 0x3a6fe0
UIntVector2 VortexPhysicsGeometrySampledHeightField::GetSize() const
{
    return UIntVector2((UInt32)m_geometry->mXCells, (UInt32)m_geometry->mYCells);
}

// Mac 0x3a7970, Win 0x51faa0
Vector3 VortexPhysicsGeometrySampledHeightField::GetScale() const
{
    return Vector3(m_scale);
}

// Mac 0x3a79a0
bool VortexPhysicsGeometrySampledHeightField::GetFlag() const
{
    return m_flag;
}

// Mac 0x3a7010, Win 0x51e890
Real32 VortexPhysicsGeometrySampledHeightField::GetHeight(const UIntVector2& sample) const
{
    const Vx::VxHeightField* field = m_geometry;
    int index = field->mXCount * (int)sample.x + (int)sample.y;
    return (Real32)field->mHeights[index] / m_scale.y;
}

// Mac 0x3a79b0 (thunk 0x3a7a20)
void* VortexPhysicsGeometrySampledHeightField::GetNativeGeometry() const
{
    return m_geometry;
}

} // namespace LEGO
