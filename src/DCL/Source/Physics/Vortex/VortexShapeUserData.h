// LEGO Digital Designer — restored source
// LEGO::VortexShapeUserData / LEGO::VortexRigidBodyUserData — the polymorphic user-data roots of
// the Vortex glue: what the collision geometries (VxCollisionGeometry user data, see
// VortexCollisionGeometryData in VortexPhysicsPlacedGeometry.h) and the bodies (VxPart user data)
// point back to. The original emits both vtables in VortexPhysicsScene.cpp (Mac TU 435).
// Every name is inferred.

// RTTI: class LEGO::VortexShapeUserData (no base). Mac vtable 0x778de8 {D1 0x3aaf70 (empty),
//   D0 0x3b2d90}; Win sdd 0x521080. Mac sizeof 0x10.
// RTTI: class LEGO::VortexRigidBodyUserData (no base). Mac vtable 0x778dd0 {D1 0x3aaf60,
//   D0 0x3b2d80}; Win 0xa29f70 {sdd 0x5212f0}. sizeof 8 (the kind at +4, stored before the
//   vtable by every user: VortexPhysicsScene Mac 0x3aac30 +0xa0 / +0xdc / +0x118,
//   VortexPhysicsRigidElement Mac 0x3aa150 +0xd0, VortexPhysicsSceneryElement Mac 0x3b4af0 +8).
#pragma once
#include "LEGO/Types.h"

namespace LEGO {

class VortexShapeCollisionFilter;

class VortexShapeUserData {
public:
    // Shape kinds seen in the collision callbacks (LDDVortexCollisionFilter Mac 0x3b49a0,
    // TerrainRayCastCollector Mac 0x3b3b80): 0 a brick geometry (VortexPhysicsBrickPlacedGeometry),
    // 4 the terrain; the scene's own shapes of its two external assemblies (PhysicsScene
    // slots 31 / 29, Mac 0x3adff0 / 0x3adee0) are 2 / 1.
    enum Kind { KindBrick = 0, KindAssemblyB = 1, KindAssemblyA = 2, KindTerrain = 4 };

    // Inline: Mac stores +4 / +8 / +0xc before the derived vtable (VortexPhysicsPlacedGeometry
    // Mac 0x3a8620).
    VortexShapeUserData(UInt32 kind, VortexShapeCollisionFilter* filter)
        : m_kind(kind), m_enabled(true), m_filter(filter)
    {
    }
    virtual ~VortexShapeUserData() {}                     // Mac D1 0x3aaf70 / D0 0x3b2d90, Win sdd 0x521080

    UInt32                      m_kind;     // +0x04
    bool                        m_enabled;  // +0x08 true (VortexPhysicsPlacedGeometry::SetEnabled)
    VortexShapeCollisionFilter* m_filter;   // +0x0c the element's filter (flexible connections)
};

// The user data of every Vx::VxPart of the glue (VxBaseUserData::mUserData).
class VortexRigidBodyUserData {
public:
    // Kinds (m_kind): 1 a VortexPhysicsRigidElement, 2 the scene's temporary test bodies, 3 a
    // VortexMoveTogetherHinge's (not on a part), 4 / 5 the parts of the scene's external
    // assemblies (PhysicsScene slots 31 / 29), 5 a scenery element (4 when added with flag 2,
    // the terrain). (names inferred)
    enum Kind { KindRigid = 1, KindTest = 2, KindMoveTogether = 3, KindAssemblyA = 4, KindAssemblyB = 5,
                KindScenery = 5, KindTerrain = 4 };

    explicit VortexRigidBodyUserData(UInt32 kind) : m_kind(kind) {}
    virtual ~VortexRigidBodyUserData() {}                 // Mac D1 0x3aaf60 / D0 0x3b2d80, Win sdd 0x5212f0

    UInt32 m_kind;                                        // +0x04
};

} // namespace LEGO
