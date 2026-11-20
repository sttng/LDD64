// LEGO Digital Designer — restored source
// LEGO::VortexPhysicsPlacedGeometry — one geometry of an element of the Vortex physics with its
// placement and "configuration" (for a configurable geometry: which set of parts), and the
// Vx::VxCollisionGeometry made from it; LEGO::VortexPhysicsBrickPlacedGeometry adds the
// (primitive element, feature) pair the collision callbacks report. Also the user data the glue
// hangs on every collision geometry (VortexCollisionGeometryData).
// VortexPhysicsPlacedGeometry.cpp (Mac TU 432, 0x3a8620-0x3a95ff; Win 0x52de70-0x52e76f, 0x522d80,
// 0x52d740). The constructors are out of line on Mac only.

// RTTI: class LEGO::VortexPhysicsPlacedGeometry : VortexShapeUserData. sizeof 0x5c. Mac vtable
//   0x758958 {D1 0x3a8810, D0 0x3a8860}; Win 0xa2a850 {sdd 0x52de70}.
// RTTI: class LEGO::VortexPhysicsBrickPlacedGeometry : VortexPhysicsPlacedGeometry. sizeof 0x64.
//   Mac vtable 0x758968 {D1 0x3a93b0, D0 0x3a9400}; Win 0xa2a858 {sdd 0x52e580}.
//   BasicArray<VortexPhysicsBrickPlacedGeometry>: Mac 0x778d20 {D1 0x3a9960, D0 0x3a9fa0}, Win
//   0xa2a3b4 {sdd 0x52b670} (its PushBack Mac 0x3a9860 is emitted in TU 433).
// Every name is inferred.
#pragma once
#include "LEGO/Types.h"
#include "Geometry.h"
#include "VortexShapeUserData.h"
#include "PrimitiveElement.h"

namespace Vx {
class VxCollisionGeometry;
class VxCollisionSensor;
}

namespace LEGO {

class VortexPhysicsGeometry;
class VortexPhysicsPrimitiveElement;

// The user data of every collision geometry the glue makes (VxBaseUserData::mUserData, 0xc
// bytes, operator new). (name inferred)
struct VortexCollisionGeometryData {
    UInt32                      m_configuration;   // +0x00 the configuration of a configurable part
    VortexShapeCollisionFilter* m_filter;          // +0x04 the element's filter
    VortexShapeUserData*        m_shape;           // +0x08 the placed geometry / scenery element

    // A zeroed one for `geometry`. Mac 0x3a9240, Win 0x522d80
    static void Create(Vx::VxCollisionGeometry* geometry);
    // Deletes the data of `geometry`. Mac 0x3a9470
    static void Destroy(Vx::VxCollisionGeometry* geometry);
    // The same for a composite's children too. Mac 0x3a8a70, Win 0x52df30
    static void DestroyAll(Vx::VxCollisionGeometry* geometry);
    // DestroyAll for every collision geometry of `sensor`. Mac 0x3a94a0, Win 0x52e730
    static void DestroyAll(Vx::VxCollisionSensor* sensor);
    static VortexShapeCollisionFilter* GetFilter(Vx::VxCollisionGeometry* geometry);   // Mac 0x3a94f0
    static VortexShapeUserData* GetShape(Vx::VxCollisionGeometry* geometry);           // Mac 0x3a95a0
    // m_shape, and the LEGO intersect filter on the model while there is one. Mac 0x3a9280
    static void SetShape(Vx::VxCollisionGeometry* geometry, VortexShapeUserData* shape);
    // m_filter (a composite's children too), and the intersect filter likewise. Mac 0x3a9510
    static void SetFilter(Vx::VxCollisionGeometry* geometry, VortexShapeCollisionFilter* filter);
};

class VortexPhysicsPlacedGeometry : public VortexShapeUserData {
public:
    // `geometry` is referenced. Mac 0x3a8620
    VortexPhysicsPlacedGeometry(UInt32 kind, VortexShapeCollisionFilter* filter, VortexPhysicsGeometry* geometry,
                                const Matrix3D& transform, UInt32 configuration);
    // A copy without the collision geometry. Mac 0x3a8680
    VortexPhysicsPlacedGeometry(const VortexPhysicsPlacedGeometry& o);
    // Releases the geometry; deletes the collision geometry (and its user data). Mac D1 0x3a8810
    // / D0 0x3a8860, Win sdd 0x52de70
    virtual ~VortexPhysicsPlacedGeometry();

    // Shows / hides the geometry (the collision geometry is made first when missing).
    // Mac 0x3a86f0 (Win inlined into 0x52b190)
    void SetEnabled(bool enabled);
    // The collision geometry (made from the geometry when missing, else its relative transform
    // updated). Win 0x52d740 (Mac inlined into 0x3a86f0)
    Vx::VxCollisionGeometry* UpdateCollisionGeometry();
    // A new configuration of a configurable geometry: the composite collision geometry gets the
    // parts of `configuration` (none for 0). Warns for other geometries. Mac 0x3a88d0, Win 0x52dfc0
    void SetConfiguration(UInt32 configuration);
    // A new collision geometry for `geometry` at `transform` (a composite of `placed`'s current
    // configuration's parts for a configurable geometry), its user data pointing to `placed`
    // (which may be NULL for a primitive geometry: then no intersect filter). Static: the
    // original tests the first argument for NULL. Mac 0x3a8af0, Win 0x52e150 (ECX = placed)
    static Vx::VxCollisionGeometry* CreateCollisionGeometry(VortexPhysicsPlacedGeometry* placed,
                                                            VortexPhysicsGeometry* geometry,
                                                            const Matrix3D& transform);
    // The collision geometry's relative transform from `transform`. Mac 0x3a9170
    void SetCollisionTransform(const Matrix3D& transform);

    Matrix3D                 m_transform;           // +0x10
    UInt32                   m_configuration;       // +0x50 (PhysicsPlacedConfiguredGeometry::m_type)
    Vx::VxCollisionGeometry* m_collisionGeometry;   // +0x54
    VortexPhysicsGeometry*   m_geometry;            // +0x58 referenced

private:
    VortexPhysicsPlacedGeometry& operator=(const VortexPhysicsPlacedGeometry&);
};

class VortexPhysicsBrickPlacedGeometry : public VortexPhysicsPlacedGeometry {
public:
    // Kind KindBrick (from the caller), the element's filter, feature `index` of `element`.
    // Mac 0x3a92b0
    VortexPhysicsBrickPlacedGeometry(UInt32 kind, VortexPhysicsGeometry* geometry, const Matrix3D& transform,
                                     UInt32 configuration, UInt32 index, VortexPhysicsPrimitiveElement& element);
    // Mac 0x3a9330
    VortexPhysicsBrickPlacedGeometry(const VortexPhysicsBrickPlacedGeometry& o);
    virtual ~VortexPhysicsBrickPlacedGeometry() {}        // Mac D1 0x3a93b0 / D0 0x3a9400, Win sdd 0x52e580

    PhysicsGeometryIdentifier m_identifier;   // +0x5c (element, feature index)

private:
    VortexPhysicsBrickPlacedGeometry& operator=(const VortexPhysicsBrickPlacedGeometry&);
};

} // namespace LEGO
