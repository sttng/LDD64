// LEGO Digital Designer — restored source
// LEGO::VortexPhysicsPrimitiveElement — the collision geometry of one primitive bone in the Vortex
// physics: its placed geometries (one VortexPhysicsBrickPlacedGeometry per feature, the
// configuration = the feature's collision type), the collision filter of its flexible
// connections and its placement in the body.
// VortexPhysicsPrimitiveElement.cpp (Mac TU 433, 0x3a9600-0x3aa04f; Win 0x52af80-0x52b68f). Mac
// 0x3a99c0 is the C1 forwarder of the constructor (Win: 0x6c9834 is a CRT function, not it); Mac
// 0x3a9860 the PushBack of BasicArray<VortexPhysicsBrickPlacedGeometry>. Created by
// VortexBrickPhysics slot 7.

// RTTI: class LEGO::VortexPhysicsPrimitiveElement : BaseBrickPhysicsPrimitiveElement (+0),
//   VortexShapeCollisionFilter (+0x64). sizeof 0xd4.
//   Mac vtable 0x758998 {base QI / AddRef / Release, 0x3a99d0, 0x3a9a10, 0x3a9a90, base 0x253870,
//       0x253880, 0x253890, 0x3a9b90, 0x3a9bc0, 0x3a9bf0, D1 0x3a9e70, D0 0x3a9f10};
//   Win vtable 0xa2a37c {base, 0x52b140, 0x52b170, 0x52b260, base, 0x52b2e0, 0x52b320, 0x52b3a0,
//       sdd 0x52af80}.
// Every name is inferred unless noted.
#pragma once
#include "LEGO/Types.h"
#include "Geometry.h"
#include "BasicArray.h"
#include "BaseBrickPhysicsPrimitiveElement.h"
#include "LDDVortexCollisionFilter.h"
#include "VortexPhysicsPlacedGeometry.h"

namespace Vx { class VxCollisionGeometry; }

namespace LEGO {

class VortexPhysicsPrimitiveElement : public BaseBrickPhysicsPrimitiveElement, public VortexShapeCollisionFilter {
public:
    // One VortexPhysicsBrickPlacedGeometry per entry of `geometries` (their geometries must be
    // Vortex ones; the placed geometries reference them), configuration = m_type.
    // Mac 0x3a9600 (C1 0x3a99c0)
    VortexPhysicsPrimitiveElement(const PhysicsPlacedConfiguredGeometry* geometries, UInt32 count,
                                  const PhysicsMassProperties& properties);
    virtual ~VortexPhysicsPrimitiveElement() {}           // Mac D1 0x3a9e70 / D0 0x3a9f10, Win sdd 0x52af80

    // The configuration of feature `feature`'s placed geometry. Mac 0x3a99d0, Win 0x52b140
    virtual void   SetFeatureType(UInt32 feature, UInt32 type);
    virtual UInt32 GetGeometryCount() const;              // Mac 0x3a9a10, Win 0x52b170
    // Feature `index`: its geometry queried for PhysicsGeometry (one reference for the caller),
    // placement and configuration. Mac 0x3a9a90, Win 0x52b260
    virtual bool   GetGeometry(UInt32 index, PhysicsPlacedConfiguredGeometry& geometry) const;
    // Both elements' filters skip each other. Mac 0x3a9b90 / 0x3a9bc0, Win 0x52b2e0 / 0x52b320
    virtual void   AddFlexibleConnection(PhysicsPrimitiveElement* other);
    virtual void   RemoveFlexibleConnection(PhysicsPrimitiveElement* other);
    virtual void   SetTransform(const Matrix3D& transform);   // Mac 0x3a9bf0, Win 0x52b3a0

    // Shows / hides feature `index` (-1: all). Mac 0x3a9a30, Win 0x52b190 (EAX = index, BL =
    // enabled, EDI = this). Callers: VortexPhysicsScene, VortexAnchorSolver.
    void SetGeometryEnabled(Int32 index, bool enabled);
    // A new composite of the enabled features' collision geometries (NULL when none), with this
    // element's filter; `inBody` false: placed at m_transform (unless identity, 1e-14).
    // Mac 0x3a9c70, Win 0x52b3c0. Caller: VortexPhysicsScene.
    Vx::VxCollisionGeometry* CreateCollisionGeometry(bool inBody);

    Matrix3D                                     m_transform;    // +0x84 identity (SetTransform)
    BasicArray<VortexPhysicsBrickPlacedGeometry> m_geometries;   // +0xc4

private:
    VortexPhysicsPrimitiveElement(const VortexPhysicsPrimitiveElement&);
    VortexPhysicsPrimitiveElement& operator=(const VortexPhysicsPrimitiveElement&);
};

} // namespace LEGO
