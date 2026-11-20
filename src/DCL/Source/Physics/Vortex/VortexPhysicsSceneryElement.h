// LEGO Digital Designer — restored source
// LEGO::VortexPhysicsSceneryElement — a static (or, with SetDynamic, free) object of the world
// outside the model in the Vortex physics: the terrain, View mode's balls. A Vx::VxPart with one
// placed geometry per creation entry.
// VortexPhysicsSceneryElement.cpp (Mac TU 437, 0x3b4af0-0x3b58ff; Win 0x5294e0-0x529dcf). Created
// by BrickPhysics::CreateSceneryElement (VortexBrickPhysics Mac 0x39ebb0), added with
// PhysicsScene::AddSceneryElement.

// RTTI: class LEGO::VortexPhysicsSceneryElement : PhysicsSceneryElement (+0),
//   VortexPhysicsSceneElement (+4), VortexShapeCollisionFilter (+0x1c). sizeof 0xb4.
//   Mac vtable 0x758da8 {0x3b4f70, 0x3b5040, 0x3b5060, 0x3b50a0, 0x3b5120, 0x3b55c0, 0x3b55d0,
//       0x3b5170, 0x3b5190, 0x3b5280, 0x3b55f0, 0x3b52e0, 0x3b5320, D1 0x3b5600, D0 0x3b56a0};
//       secondary 0x758dec {0x3b5720, 0x3b57c0}.
//   Win vtable 0xa2a290 {0x529720, 0x529780, 0x5297a0, 0x5297d0, 0x5297f0, 0x5294e0, 0x5294f0,
//       0x529830, 0x529850, 0x5298d0, 0x529500, 0x529910, 0x529950}; secondary 0xa2a2c8
//       {sdd 0x529dc0}. NOTE: the Win primary vtable has no destructor (the first virtual
//       destructor comes from the second base, MSVC puts it there only).
// Every name is inferred unless noted.
#pragma once
#include "LEGO/Types.h"
#include "Geometry.h"
#include "BasicArray.h"
#include "BrickPhysics.h"
#include "LDDVortexCollisionFilter.h"
#include "VortexPhysicsRigidElement.h"
#include "VortexPhysicsPlacedGeometry.h"

namespace LEGO {

class VortexPhysicsScene;

class VortexPhysicsSceneryElement : public PhysicsSceneryElement,
                                    public VortexPhysicsSceneElement,
                                    public VortexShapeCollisionFilter {
public:
    // One placed geometry per entry (kind KindBrick, this element's filter, the entry's transform
    // and type as configuration); the geometries must be Vortex ones (NOTE: the
    // QueryInterface result is not checked). Refcount 1, not dynamic, flags 1.
    // Mac 0x3b4af0 (C1 0x3b4f60)
    VortexPhysicsSceneryElement(const PhysicsPlacedConfiguredGeometry* geometries, UInt32 count,
                                const PhysicsMassProperties& properties);
    // NOTE: does not destroy the part (RemoveFromScene does). Mac D1 0x3b5600 / D0 0x3b56a0
    // (thunks 0x3b5720 / 0x3b57c0), Win sdd 0x529dc0
    virtual ~VortexPhysicsSceneryElement() {}

    // Answers PhysicsSceneryElement::GetIID() and PhysicsSceneElement::GetIID().
    // Mac 0x3b4f70, Win 0x529720
    virtual Int32  LEGO_COM_CALL QueryInterface(const InterfaceID& id, void** object);
    virtual UInt32 LEGO_COM_CALL AddRef();                  // Mac 0x3b5040, Win 0x529780
    virtual UInt32 LEGO_COM_CALL Release();                 // Mac 0x3b5060, Win 0x5297a0 (deletes at 0)
    // The part's transform (NOTE: the element must be in a scene). Mac 0x3b50a0, Win 0x5297d0
    virtual void GetTransform(Matrix3D& transform) const;
    // NOTE: a negative mass is made positive and resets the inertia to identity (the original's).
    // Mac 0x3b5120, Win 0x5297f0
    virtual const PhysicsMassProperties& GetMassProperties() const;
    virtual void* GetUserData() const;                      // Mac 0x3b55c0, Win 0x5294e0 (+0xa4)
    virtual void  SetUserData(void* data);                  // Mac 0x3b55d0, Win 0x5294f0
    virtual UInt32 GetGeometryCount() const;                // Mac 0x3b5170, Win 0x529830
    // Entry `index` (its geometry referenced for the caller). Mac 0x3b5190, Win 0x529850
    virtual bool GetGeometry(UInt32 index, PhysicsPlacedConfiguredGeometry& geometry) const;
    // Only with a mass: frees / freezes the part. Mac 0x3b5280, Win 0x5298d0
    virtual void SetDynamic(bool dynamic);
    virtual bool IsDynamic() const;                         // Mac 0x3b55f0, Win 0x529500
    // VxPart::addImpulse (the interface's name kept). Mac 0x3b52e0, Win 0x529910
    virtual void SetLinearVelocity(const Vector3& impulse);
    // VxPart::addImpulseAtPosition. Mac 0x3b5320, Win 0x529950
    virtual void AddImpulseAtPosition(const Vector3& impulse, const Vector3& position);

    // PhysicsScene::AddSceneryElement: a new part in the scene's mechanism with the geometries,
    // mass from the geometries scaled by 0.2, at `transform`; body kind 4 with flag 2, else 5;
    // every placed geometry gets `flags` as its shape kind. Mac 0x3b5380 (Win inlined into
    // 0x5248a0)
    void AddToScene(VortexPhysicsScene* scene, const Matrix3D& transform, UInt32 flags);
    // DestroyPart, no scene. Mac 0x3b5590 (Win inlined into 0x5248f0)
    void RemoveFromScene();

    BasicArray<VortexPhysicsPlacedGeometry> m_geometries;   // +0x3c
    mutable PhysicsMassProperties           m_properties;   // +0x4c (a copy)
    VortexPhysicsScene*                     m_scene;        // +0xa0
    void*                                   m_userData;     // +0xa4
    UInt32                                  m_flags;        // +0xa8 1 (AddSceneryElement's)
    bool                                    m_dynamic;      // +0xac
    volatile Int32                          m_refCount;     // +0xb0 1

private:
    VortexPhysicsSceneryElement(const VortexPhysicsSceneryElement&);
    VortexPhysicsSceneryElement& operator=(const VortexPhysicsSceneryElement&);
};

} // namespace LEGO
