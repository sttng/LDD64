// LEGO Digital Designer — restored source
// The geometries of the Vortex physics (BrickPhysics slots 9-14): a box, sphere,
// capsule and cylinder (one Vx shape each, kept referenced), a configurable compound geometry
// (per "configuration" a list of placed geometries; BrickConnect's knob shapes) and a sampled
// height field (the terrain).
// VortexPhysicsGeometryConfigable.cpp (Mac TU 431, 0x3a4b20-0x3a861f; Win 0x51e850-0x51ffcf,
// 0x5207f0-0x520930 thunks). Mac 0x3a4e50 / 0x3a6c20 / 0x3a72b0 are the C1 forwarders of the box /
// configurable / height-field constructors (sphere / capsule / cylinder: C1 only).

// RTTI (every class: PhysicsGeometryXxx (+0), VortexPhysicsGeometry (+4)):
//   VortexPhysicsGeometryBox      Mac 0x758678 / 0x7586a8, Win 0xa2936c / 0xa29384. sizeof 0x1c.
//   VortexPhysicsGeometrySphere   Mac 0x7586d0 / 0x758700, Win 0xa293a4 / 0xa293bc. sizeof 0x14.
//   VortexPhysicsGeometryCapsule  Mac 0x758728 / 0x75875c, Win 0xa293dc / 0xa293f8. sizeof 0x18.
//   VortexPhysicsGeometryCylinder Mac 0x758784 / 0x7587b8, Win 0xa29418 / 0xa29434. sizeof 0x18.
//   VortexPhysicsGeometryConfigable Mac 0x7587e0 / 0x758814, Win 0xa29454 / 0xa29478. sizeof 0x1c.
//   VortexPhysicsGeometrySampledHeightField Mac 0x75883c / 0x758870, Win 0xa294a8 / 0xa294cc.
//     sizeof 0x20.
//   struct VortexPhysicsGeometryConfigable::PlacedGeometry (0x44; BasicArray<PlacedGeometry> Mac
//     0x778c9c {D1 0x3a7a30, D0 0x3a7a90}, Win 0xa29498 {sdd 0x51ff70}) and ::Configuration (0x10;
//     BasicArray<Configuration> Mac 0x778cd0 {D1 0x3a6b70, D0 0x3a8520}, Win 0xa294a0 {sdd 0x51ff90}).
// The shapes are 0.007 larger than asked (VortexPhysics' contact margin); the getters subtract
// it again (box, sphere). Every name is inferred unless noted.
#pragma once
#include "LEGO/Types.h"
#include "Geometry.h"
#include "BasicArray.h"
#include "BrickPhysics.h"
#include "VortexPhysicsGeometry.h"

namespace Vx {
class VxBox;
class VxSphere;
class VxCapsule;
class VxCylinder;
class VxHeightField;
}

namespace LEGO {

// QueryInterface of the six geometries: VortexPhysicsGeometry::GetIID() gives the Vortex base,
// the class's own id and PhysicsGeometry::GetIID() the primary one. (the shape of Mac 0x3a4b20 ..)
#define LEGO_VORTEX_GEOMETRY_IUNKNOWN()                                                            \
    virtual Int32  LEGO_COM_CALL QueryInterface(const InterfaceID& id, void** object);           \
    virtual UInt32 LEGO_COM_CALL AddRef();                                                       \
    virtual UInt32 LEGO_COM_CALL Release()

class VortexPhysicsGeometryBox : public PhysicsGeometryBox, public VortexPhysicsGeometry {
public:
    // A box of half size `halfSize` (+ 0.007). Mac 0x3a4d10 (C1 0x3a4e50)
    explicit VortexPhysicsGeometryBox(const Vector3& halfSize);
    LEGO_VORTEX_GEOMETRY_IUNKNOWN();                      // Mac 0x3a4b20 / 0x3a7430 / 0x3a7450, Win 0x51e900 (AddRef) / 0x51eb00
    virtual UInt32 GetKind() const;                       // Mac 0x3a7490, Win 0x51ec40: 1
    virtual Vector3 GetHalfSize() const;                  // Mac 0x3a4c60
    virtual void* GetNativeGeometry() const;          // Mac 0x3a74a0 / 0x3a7510
    virtual void Inflate(Real32 amount);                  // Mac 0x3a4fd0 / 0x3a5060
    virtual void ResetSize();                             // Mac 0x3a50f0 / 0x3a5130
    virtual ~VortexPhysicsGeometryBox();                  // Mac D1 0x3a4e60 / D0 0x3a4f00, Win sdd 0x51e9b0

    Vx::VxBox*     m_geometry;   // +0x08 referenced
    volatile Int32 m_refCount;   // +0x0c 1
    Vector3        m_size;       // +0x10 full size (2 * (half + 0.007))
};

class VortexPhysicsGeometrySphere : public PhysicsGeometrySphere, public VortexPhysicsGeometry {
public:
    explicit VortexPhysicsGeometrySphere(Real32 radius);  // Mac 0x3a52f0, Win 0x51f2a0
    LEGO_VORTEX_GEOMETRY_IUNKNOWN();                      // Mac 0x3a5170 / 0x3a7520 / 0x3a7540, Win - / 0x51ec70 / 0x51eea0
    virtual UInt32 GetKind() const;                       // Mac 0x3a7580, Win 0x51eee0: 0
    virtual Real32 GetRadius() const;                     // Mac 0x3a52b0 (- 0.007)
    virtual void* GetNativeGeometry() const;          // Mac 0x3a7590 / 0x3a7600
    virtual void Inflate(Real32 amount);                  // Mac 0x3a5500 / 0x3a5550
    virtual void ResetSize();                             // Mac 0x3a55a0 / 0x3a55d0, Win 0x51ed20
    virtual ~VortexPhysicsGeometrySphere();               // Mac D1 0x3a5390 / D0 0x3a5430

    Vx::VxSphere*  m_geometry;   // +0x08 referenced
    volatile Int32 m_refCount;   // +0x0c 1
    Real32         m_radius;     // +0x10 radius + 0.007
};

class VortexPhysicsGeometryCapsule : public PhysicsGeometryCapsule, public VortexPhysicsGeometry {
public:
    VortexPhysicsGeometryCapsule(Real32 radius, Real32 length);   // Mac 0x3a57a0, Win 0x51ed40
    LEGO_VORTEX_GEOMETRY_IUNKNOWN();                      // Mac 0x3a5600 / 0x3a7610 / 0x3a7630, Win - / 0x51ef00 / 0x51e7b0
    virtual UInt32 GetKind() const;                       // Mac 0x3a7670: 2
    virtual Real32 GetRadius() const;                     // Mac 0x3a5740
    virtual Real32 GetLength() const;                     // Mac 0x3a5770
    virtual void* GetNativeGeometry() const;          // Mac 0x3a7680 / 0x3a76f0
    // The radius only. Mac 0x3a59c0 / 0x3a5a10
    virtual void Inflate(Real32 amount);
    virtual void ResetSize();                             // Mac 0x3a5a60 / 0x3a5a90, Win 0x51efd0 (radius only)
    virtual ~VortexPhysicsGeometryCapsule();              // Mac D1 0x3a5850 / D0 0x3a58f0

    Vx::VxCapsule* m_geometry;   // +0x08 referenced
    volatile Int32 m_refCount;   // +0x0c 1
    Real32         m_radius;     // +0x10
    Real32         m_length;     // +0x14
};

class VortexPhysicsGeometryCylinder : public PhysicsGeometryCylinder, public VortexPhysicsGeometry {
public:
    VortexPhysicsGeometryCylinder(Real32 radius, Real32 length);  // Mac 0x3a5c60, Win 0x51ea10
    LEGO_VORTEX_GEOMETRY_IUNKNOWN();                      // Mac 0x3a5ac0 / 0x3a7700 / 0x3a7720, Win - / 0x51f1b0 / 0x51e7b0
    virtual UInt32 GetKind() const;                       // Mac 0x3a7760: 3
    virtual Real32 GetRadius() const;                     // Mac 0x3a5c00
    virtual Real32 GetLength() const;                     // Mac 0x3a5c30
    virtual void* GetNativeGeometry() const;          // Mac 0x3a7770 / 0x3a77e0
    virtual void Inflate(Real32 amount);                  // Mac 0x3a5e80 / 0x3a5f10
    virtual void ResetSize();                             // Mac 0x3a5fa0 / 0x3a5ff0
    virtual ~VortexPhysicsGeometryCylinder();             // Mac D1 0x3a5d10 / D0 0x3a5db0, Win sdd 0x51f280

    Vx::VxCylinder* m_geometry;  // +0x08 referenced
    volatile Int32  m_refCount;  // +0x0c 1
    Real32          m_radius;    // +0x10
    Real32          m_length;    // +0x14
};

class VortexPhysicsGeometryConfigable : public PhysicsGeometryConfigable, public VortexPhysicsGeometry {
public:
    // One part of a configuration: a geometry (referenced) and its placement.
    struct PlacedGeometry {
        PlacedGeometry(VortexPhysicsGeometry* geometry, const Matrix3D& transform)
            : m_geometry(geometry), m_transform(transform)
        {
            m_geometry->AddRef();
        }
        PlacedGeometry(const PlacedGeometry& o) : m_geometry(o.m_geometry), m_transform(o.m_transform)
        {
            m_geometry->AddRef();
        }
        ~PlacedGeometry() { m_geometry->Release(); }
        // (Mac 0x3a8260 inlines it: the new reference first)
        PlacedGeometry& operator=(const PlacedGeometry& o)
        {
            VortexPhysicsGeometry* old = m_geometry;
            m_geometry = o.m_geometry;
            m_geometry->AddRef();
            old->Release();
            m_transform = o.m_transform;
            return *this;
        }
        VortexPhysicsGeometry* m_geometry;   // +0x00
        Matrix3D               m_transform;  // +0x04
    };
    struct Configuration {
        BasicArray<PlacedGeometry> m_geometries;   // +0x00
    };

    // Room for 8 configurations. Mac 0x3a6880 (C1 0x3a6c20), Win 0x51f4a0 area
    VortexPhysicsGeometryConfigable();
    LEGO_VORTEX_GEOMETRY_IUNKNOWN();                      // Mac 0x3a6040 / 0x3a77f0 / 0x3a7810
    virtual UInt32 GetKind() const;                       // Mac 0x3a7850, Win 0x51e840: 4
    // The parts of `configuration` (each geometry referenced). false for configuration 0 or one
    // that has parts already. Mac 0x3a6180, Win 0x51f550
    virtual bool   SetGeometries(UInt32 configuration, const PhysicsPlacedGeometry* geometries, UInt32 count);
    virtual UInt32 GetConfigurationCount();               // Mac 0x3a6660, Win 0x51f740
    virtual UInt32 GetGeometryCount(UInt32 configuration);  // Mac 0x3a6680, Win 0x51f750 (0 out of range)
    // Part `index` (its geometry queried for PhysicsGeometry: one reference for the caller).
    // NOTE: m_type of the result is left as it was. Mac 0x3a66c0, Win 0x51f790
    virtual bool   GetGeometry(UInt32 configuration, UInt32 index, PhysicsPlacedConfiguredGeometry& geometry);
    virtual void* GetNativeGeometry() const;          // Mac 0x3a7860 / 0x3a78d0: NULL
    virtual ~VortexPhysicsGeometryConfigable();           // Mac D1 0x3a6c30 / D0 0x3a6db0 (thunks 0x3a6cf0 / 0x3a6e80)

    // Part `index` of `configuration`: its geometry (not referenced) and `transform`; NULL (and
    // `transform` untouched) out of range. Mac 0x3a67d0
    VortexPhysicsGeometry* GetPlacedGeometry(UInt32 configuration, UInt32 index, Matrix3D& transform);

    volatile Int32            m_refCount;         // +0x08 1
    BasicArray<Configuration> m_configurations;   // +0x0c
};

class VortexPhysicsGeometrySampledHeightField : public PhysicsGeometrySampledHeightField, public VortexPhysicsGeometry {
public:
    // A Vx::VxHeightField of (sizeX - 1) x (sizeY - 1) cells, spacing scale.x / scale.z, heights
    // `heights` (sizeX rows of sizeY) * scale.y. NOTE: the rows are copied with stride sizeY
    // while the Vx grid's row stride is sizeX (the same for the square terrains LDD makes).
    // Mac 0x3a7080 (C1 0x3a72b0), Win 0x51f8b0
    VortexPhysicsGeometrySampledHeightField(UInt32 sizeX, UInt32 sizeY, const Vector3& scale, bool flag,
                                            const Real32* heights);
    LEGO_VORTEX_GEOMETRY_IUNKNOWN();                      // Mac 0x3a6ea0 / 0x3a7900 / 0x3a7920, Win - / 0x51f9f0 / 0x4d43e0
    virtual UInt32 GetKind() const;                       // Mac 0x3a7960, Win 0x51e850: 6
    // NOTE: the Vx cell counts (size - 1), not the sample counts. Mac 0x3a6fe0
    virtual UIntVector2 GetSize() const;
    virtual Vector3 GetScale() const;                     // Mac 0x3a7970, Win 0x51faa0
    virtual bool GetFlag() const;                         // Mac 0x3a79a0
    // height[mXCount * x + y] / scale.y (VxArray range check). Mac 0x3a7010, Win 0x51e890
    virtual Real32 GetHeight(const UIntVector2& sample) const;
    // Both PhysicsGeometrySampledHeightField slot 8 and VortexPhysicsGeometry slot 3. Mac 0x3a79b0
    // (thunk 0x3a7a20)
    virtual void* GetNativeGeometry() const;
    virtual ~VortexPhysicsGeometrySampledHeightField();   // Mac D1 0x3a72c0 / D0 0x3a7360, Win sdd 0x51fac0

    Vx::VxHeightField* m_geometry;   // +0x08 referenced
    Vector3            m_scale;      // +0x0c
    volatile Int32     m_refCount;   // +0x18 1
    bool               m_flag;       // +0x1c
};

} // namespace LEGO
