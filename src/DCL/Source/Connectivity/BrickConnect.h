// LEGO Digital Designer — restored source
// LEGO::BrickConnect — factory of Scene, Brick, Part, PrimitiveElement and BrickGroup objects
// (LDDBrickConnect, TU 351, derives from it) and cache of the shared physics geometries
// (collision-volume boxes, spheres, capsules; the knob geometry).
// BrickConnect.cpp = sub-TU 025r of the original Scene.cpp (Mac 0xc2860-0xc3220; constructor
// 0xc1fc0, base-object variant with VTT, in the 025q range, complete-object one inlined into
// LDDBrickConnect Mac 0x5c900; caches 0x9e0b0 / 0x9e280 / 0x9e440 in 025j; Win
// 0x490460-0x490e50, 0x4911d0, 0x4a1ab0).
// RTTI: class LEGO::BrickConnect : virtual Object.
//   Mac sizeof 0x9c: vptr +0 (vtable 0x706268: 0 CreateScene 0xc2860, 1 CreateBrick 0xc28c0,
//       2 CreatePart 0xc2a90, 3 CreatePrimitiveElement 0xc2ae0, 4 CreateBrickGroup 0xc2b50,
//       5/6 D1 0xc3060 / D0 0xc3130, 7 OnFinalRelease 0xc4930, 8 CloneBrick 0xc2da0,
//       9 ClonePart 0xc2dd0, 10 ClonePrimitiveElement 0xc2df0), Object at +0x94
//       (vtable 0x7062ac {thunks 0xc30c0, 0xc31a0, 0xc4950, …}); destructor body 0xc2e30.
//   Win: vfptr +0 (0xa23c30 {0x490a10, 0x490a80, 0x490af0, 0x490b60, 0x490be0, 0x490e10,
//       0x490e30, 0x490e50}), vbptr +4 (vbtable 0x9da448), members from +0x08 (Mac +4:
//       m_physics +0x08 … m_knobGeometry +0x14), CRITICAL_SECTION +0x18, the maps (0x24 bytes
//       each) +0x30 / +0x54 / +0x78, vtordisp +0x9c, Object at +0xa0 (vftable 0xa23c54 {sdd
//       0x4a1ab0, …}).
// Every name except the class / map RTTI names is inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicMap.h"
#include "Geometry.h"
#include "BrickPhysics.h"
#include "BrickDatabase.h"
#include "ConnectivityWorldLookupTables.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace LEGO {

class Scene;
class Brick;
class Part;
class BrickGroup;
class Primitive;
class PrimitiveElement;
class PhysicsScene;

class BrickConnect : public virtual Object {
public:
    // Map orders (RTTI BrickConnect::Vector3Less / Real32Less). Vector3: Vector3::operator<
    // (1e-5 per component); Real32: a < b by more than 0.001 (Mac 0x9e280).
    struct Vector3Less {
        bool operator()(const Vector3& a, const Vector3& b) const { return a < b; }
    };
    struct Real32Less {
        bool operator()(Real32 a, Real32 b) const
        {
            Real32 d = a - b;
            return d < 0.0f && (d < 0.0f ? -d : d) > 0.001f;
        }
    };
    typedef BasicMap<Vector3, PhysicsGeometryBox*, Vector3Less>     BoxMap;      // Mac 0x762b98
    typedef BasicMap<Real32, PhysicsGeometrySphere*, Real32Less>     SphereMap;   // Mac 0x762bb0
    typedef BasicMap<Vector3, PhysicsGeometryCapsule*, Vector3Less> CapsuleMap;  // Mac 0x762bc8

    // References `physics` and `database`; builds the database's ConnectivityWorldLookupTables,
    // the recursive cache mutex, the ConnectionFieldPlanar / Axle tables and the knob geometry: a
    // configurable geometry (BrickPhysics slot 13) with seven configurations of four uncached
    // boxes (knob, wall, tube, tube wall). Mac 0xc1fc0 (base-object variant), Win 0x490460 (this
    // in ECX, stack (physics, database, mostDerived))
    BrickConnect(BrickPhysics* physics, BrickDatabase* database);

    // --- slots 0-4: the factories (new objects, not referenced). (025r)
    virtual Scene*            CreateScene(PhysicsScene* physicsScene);                 // Mac 0xc2860, Win 0x490a10
    // CreateScene on a new physics scene of m_physics (BrickPhysics slot 5
    // with `flags`); null when that fails. Used by BrickConnectSceneControl::SetScene (Mac
    // 0x339d0) without a scene. Mac 0xc2810 (name inferred)
    Scene* NewScene(UInt32 flags);
    virtual Brick*            CreateBrick(UInt32 designID);                            // Mac 0xc28c0, Win 0x490a80
    virtual Part*             CreatePart(const Primitive* primitive);                  // Mac 0xc2a90, Win 0x490af0
    // Mac 0xc2ae0, Win 0x490b60
    virtual PrimitiveElement* CreatePrimitiveElement(const Primitive* primitive, UInt32 boneIndex,
                                                     const Matrix3D& transform);
    virtual BrickGroup*       CreateBrickGroup(Scene* scene);                          // Mac 0xc2b50, Win 0x490be0
    // --- slots 5/6: releases the cached geometries and the physics. Mac 0xc3060 / 0xc3130
    // (body 0xc2e30), Win 0x490990 / 0x4911d0 (025r)
    virtual ~BrickConnect();

    // Shared geometries (created on first use through m_physics; owned by the cache).
    // Mac 0x9e0b0 / 0x9e280 / 0x9e440 (025j)
    PhysicsGeometryBox*     GetBox(const Vector3& halfSize);
    PhysicsGeometrySphere*  GetSphere(Real32 radius);
    // `radius` = CollisionVolume::m_size.y, `length` = m_size.x
    PhysicsGeometryCapsule* GetCapsule(Real32 radius, Real32 length);

    BrickPhysics*    GetPhysics() const { return m_physics; }
    BrickDatabase*   GetDatabase() const { return m_database; }
    PhysicsGeometry* GetKnobGeometry() const { return m_knobGeometry; }

    // ---- data (Mac offsets) -------------------------------------------------------------------
    BrickPhysics*      m_physics;        // +0x04 one reference
    RefPointer<BrickDatabase> m_database; // +0x08
    RefPointer<ConnectivityWorldLookupTables> m_lookupTables;  // +0x0c built from m_database
    PhysicsGeometry*   m_knobGeometry;   // +0x10 one reference (the knob feature geometry)
#if defined(_WIN32)
    CRITICAL_SECTION   m_lock;           // guards the caches
#else
    pthread_mutex_t    m_lock;           // +0x14 (0x2c bytes)
#endif
    BoxMap             m_boxes;          // +0x40
    SphereMap          m_spheres;        // +0x5c
    CapsuleMap         m_capsules;       // +0x78

protected:
    // --- slot 7: Mac 0xc4930 (thunk 0xc4950)
    virtual void OnFinalRelease() { Object::OnFinalRelease(); }

public:
    // --- slots 8-10: a new object of the same kind (Mac: CreateBrick(brick->GetDesignID()),
    // CreatePart(part's primitive), CreatePrimitiveElement(primitive, bone, world transform)).
    // Mac 0xc2da0 / 0xc2dd0 / 0xc2df0, Win 0x490e10 / 0x490e30 / 0x490e50 (025r)
    virtual Brick*            CloneBrick(const class AbstractBrick* brick);
    virtual Part*             ClonePart(const Part* part);
    virtual PrimitiveElement* ClonePrimitiveElement(const PrimitiveElement* element);

private:
    BrickConnect(const BrickConnect&);
    BrickConnect& operator=(const BrickConnect&);
};

} // namespace LEGO
