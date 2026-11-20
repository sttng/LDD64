// LEGO Digital Designer — restored source
// LEGO::VortexPhysicsScene — the Vortex world: a Vx::VxUniverse with one Vx::VxMechanism for the
// bodies and joints, the collision queries of the editor (insertion, drag, casts, element tests),
// snapshots, scenery elements and the stepping of the animation solver; and the helper classes
// of the same object: VortexPhysicsSnapshot, the temporary joints of the solvers
// (VortexHingeSolverHinge, VortexMoveTogetherHinge), the spring mirrors of the joints during a
// simulating hinge solve (VortexPhysicsConstraint{Hinge,Ball}SpringHelper), the manual hinge
// solver, PointGravitationAction and the collision collectors.
// VortexPhysicsScene.cpp (Mac TU 435, 0x3aac30-0x3b4342; Win 0x521080-0x528a8f, 0x52e9a0-
// 0x52eb3f, 0x52f4f0). Its VortexConnectionAttributeDatas functions (Mac 0x3b2100-0x3b24ef) are
// in VortexConnectionAttributeDatas.cpp.

// RTTI (Mac / Win vtables):
//   VortexPhysicsScene : BaseBrickPhysicsScene. sizeof 0x134 (VortexBrickPhysics slot 5 Mac
//     0x39ea10: operator new(0x134)). Mac 0x758a88 {base QI / AddRef / Release, 0x3b2cf0,
//     0x3aaf90, 0x3abd30, 0x3af3d0, 0x3af3e0, base 8 / 9, 0x3abd90, 0x3ac5d0, 0x3ac850, 0x3ac980,
//     0x3acb50, 0x3acbe0, 0x3ad030, 0x3ad500, base 18, 0x3adaf0, 0x3ade30, base 21-24, 0x3ae100,
//     0x3ae380, 0x3ae5d0, 0x3ae6a0, 0x3adee0, 0x3adfe0, 0x3adff0, 0x3ae0f0, 0x3ae6f0, 0x3ae740,
//     0x3ae760, 0x3ae7b0, 0x3ae780, 0x3ae810, 0x3ae820, 0x3ae870, 0x3aea60, 0x3af290, 0x3af5b0,
//     0x3af680, D1 0x3af560, D0 0x3af570, 0x3af420, 0x3af430}; Win 0xa2a044 {.., 0x52f4f0,
//     0x522260, 0x522e10, 0x525120, 0x525130, .., 0x522fd0, 0x523620, 0x523800, 0x523930,
//     0x523ab0, 0x523be0, 0x523f70, 0x5242d0, .., 0x524410, 0x524710, .., 0x5248a0, 0x5248f0,
//     0x524950, 0x524990, 0x5247d0, 0x524830, 0x524840, 0x5294e0, 0x5249d0, 0x524a00, 0x524a00,
//     0x524a50, 0x524a20, 0x40be80, 0x40be80, 0x524ad0, 0x524cb0, 0x525030, 0x5252b0, 0x525340,
//     sdd 0x522240, 0x459790, 0x525160}. NOTE: the work files pair several of these one slot off.
//   VortexPhysicsSnapshot : BaseBrickPhysicsSnapshot. sizeof 0x1c. Mac 0x758d40, Win 0xa2a1e8.
//   VortexHingeSolverHinge : BaseBrickPhysicsConnectionHelper (+0), BaseBrickPhysicsSimulationCallback
//     (+0x64), Object (+0x6c). sizeof 0x84. Mac 0x758c3c / 0x758c6c / 0x758c88; Win 0xa29fc4 /
//     0xa29fe8 / 0xa29ffc.
//   VortexMoveTogetherHinge : the same bases. sizeof 0xa8. Mac 0x758bcc / 0x758c04 / 0x758c20;
//     Win 0xa29f78 / 0xa29f9c / 0xa29fb0.
//   VortexPhysicsConstraintSpringHelper : BaseBrickPhysicsConnectionHelper (abstract, no vtable
//     of its own); VortexPhysicsConstraintHingeSpringHelper (sizeof 0x74, Mac 0x758b54, Win
//     0xa2a180), VortexPhysicsConstraintBallSpringHelper (sizeof 0x70, Mac 0x758b90, Win 0xa2a1b4).
//   VortexBrickPhysicsManualHingeSolver : BaseBrickPhysicsManualHingeSolver (+0),
//     VortexMoveTogetherHingeData (+0x40). sizeof 0x58. Mac 0x778f30 (+ 0x778f80 / 0x778f9c /
//     0x778fb4), Win 0xa2a118.
//   PointGravitationAction : Vx::VxAssemblyBase::UpdateSubscriber. Mac 0x758ca4, Win 0xa2a978.
//   LEGOUniverseCollector : Vx::VxUniverse::IntersectSubscriber (Mac 0x778e78, Win 0xa2a214);
//   LEGOIndividualCollector / CastIndividualCollector : Vx::VxIntersect::IntersectSubscriber
//     (Mac 0x778ea4 / 0x778efc, Win 0xa2a224 / 0xa2a234); TerrainRayCastCollector :
//     Vx::VxLineSegIntersectSub (Mac 0x778ed4, Win 0xa2a244).
// NOTE (Itanium vs MSVC): the temporary joints override BaseBrickPhysicsSimulationCallback slots;
// the Mac repeats those overrides in the primary vtable (slots 9-11), Win only in the callback's.
// Every name is inferred unless noted (log strings name several methods).
#pragma once
#include "LEGO/Types.h"
#include <float.h>
#include "Geometry.h"
#include "BasicArray.h"
#include "Object.h"
#include "BaseBrickPhysicsScene.h"
#include "BaseBrickPhysicsConnectionHelper.h"
#include "VortexMoveTogetherHingeData.h"
#include "VortexShapeUserData.h"
#include "LDDVortexCollisionFilter.h"

#include "Vx/VxArray.h"
#include "Vx/VxTransform.h"
#include "Vx/VxIntersect.h"
#include "Vx/VxUniverse.h"
#include "Vx/VxAssemblyBase.h"

namespace Vx {
class VxUniverse;
class VxMechanism;
class VxPart;
class VxCollisionGeometry;
class VxConstraint;
}

namespace LEGO {

class BaseBrickPhysics;
class VortexBrickPhysics;
class VortexHingeSolverHinge;
class VortexMoveTogetherHinge;
class VortexPhysicsRigidElement;
class VortexPhysicsSceneryElement;
class VortexPhysicsPrimitiveElement;

// ---- the scene --------------------------------------------------------------------------------

class VortexPhysicsScene : public BaseBrickPhysicsScene {
public:
    // The scene of `universe` (VortexBrickPhysics slot 5 makes it): the cast sphere (radius 0.03)
    // and the mechanism of the bodies (added to the universe). Mac 0x3aac30 (C1 0x3aaf80), Win
    // inlined into VortexBrickPhysics::CreatePhysicsScene
    VortexPhysicsScene(BaseBrickPhysics* physics, Vx::VxUniverse* universe);
    // Removes the external assemblies, deletes the cast geometry, takes the universe out of the
    // frame and deletes it. NOTE: the mechanism is not deleted here. Mac D1 0x3af560 (body
    // 0x3af460) / D0 0x3af570, Win 0x525190 / sdd 0x522240
    virtual ~VortexPhysicsScene();

    // ---- PhysicsScene slots
    virtual void* GetNativeScene();                                          // Mac 0x3b2cf0, Win 0x52f4f0: the universe
    // The constraints' attachment frames, the parts' collision boxes and the constraint forces as
    // lines (a new SimplePhysicsSceneDebugInfo). Mac 0x3aaf90, Win 0x522260
    virtual PhysicsSceneDebugInfo* GetDebugInfo();
    // `on`: the factory's geometries shrink by the safety margin (Inflate(-0.007)); else back
    // (ResetSize). Mac 0x3abd30, Win 0x522e10
    virtual void Unknown5(bool on);
    virtual void Unknown6();                                                 // Mac 0x3af3d0, Win 0x525120: the base's
    // The base's, then Unknown47 and the factory's UpdateUniverse. Mac 0x3af3e0, Win 0x525130
    virtual void Unknown7();
    // "VortexPhysicsScene::TestInsertPrimitiveElements": the elements at their transforms against
    // the world (collection via LEGOUniverseCollector; more than 32: grouped in composites).
    // Mac 0x3abd90, Win 0x522fd0
    virtual bool TestCollisions(UInt32 count, PhysicsPrimitiveElement* const* elements,
                                const Matrix3D* const* transforms, PhysicsCollisionCallback* callback,
                                PhysicsProgressCallback* progress);
    // The bodies of `other` (BaseBrickPhysicsScene) at `transform` against this world.
    // Mac 0x3ac5d0, Win 0x523620
    virtual bool TestCollision(PhysicsScene* other, const Matrix3D& transform, PhysicsCollisionCallback* callback);
    // "VortexPhysicsScene::BeginHitTest": the drag part made of `dragScene`'s bodies (false when it
    // has no geometry). Mac 0x3ac850, Win 0x523800
    virtual bool SetDragScene(PhysicsScene* dragScene);
    // "VortexPhysicsScene::HitTest": the drag part at `transform`. Mac 0x3ac980, Win 0x523930
    virtual bool TestDragCollision(const Matrix3D& transform, PhysicsCollisionCallback* callback);
    // "VortexPhysicsScene::EndHitTest". Mac 0x3acb50, Win 0x523ab0
    virtual bool ClearDragScene();
    // TestDragCollision's move of a drag part of many boxes (LDD64, VortexPhysicsScene.cpp)
    bool MoveLargeDragPart(const Vx::VxTransform& tm);
    // "VortexPhysicsScene::TerrainCastScene": sweeps the drag part from `from` to `to` (or, with
    // `terrainOnly`, a segment against the terrain). Mac 0x3acbe0, Win 0x523be0
    virtual bool CastDragScene(const Matrix3D& from, const Vector3& to, Real32& distance, bool terrainOnly);
    // One element of another scene swept to `position` against this world (the global collision
    // filter asks `callback`). Mac 0x3ad030, Win 0x523f70
    virtual bool TestElementCollision(PhysicsPrimitiveElement* element, const Vector3& position,
                                      PhysicsCollisionCallback* callback);
    // A VortexPhysicsSnapshot of `count` bodies. Mac 0x3ad500, Win 0x5242d0
    virtual PhysicsSnapshot* CreateSnapshot(PhysicsRigidElement* const* bodies, UInt32 count);
    // "VortexPhysicsScene::MoveTogether": a VortexMoveTogetherSolver with a VortexMoveTogetherHinge
    // between the two frames. Mac 0x3adaf0, Win 0x524410
    virtual PhysicsSolver* SolveMoveTogether(PhysicsRigidElement* body, const Vector3& position, UInt32 zero0,
                                             const Vector3& axis, PhysicsRigidElement* otherBody,
                                             const Vector3& otherPosition, UInt32 zero1, const Vector3& otherAxis,
                                             PhysicsCollisionCallback* callback, const PhysicsGeometryIdentifier* features,
                                             UInt32 featureCount, IUnknown* context);
    // A VortexAnchorSolver for `callback` (its AnchorPhysicsSolver). Mac 0x3ade30, Win 0x524710
    virtual IUnknown* AddSolverCallback(PhysicsSolverCallback* callback);
    virtual void AddSceneryElement(PhysicsSceneryElement* element, const Matrix3D& transform,
                                   UInt32 flags);                            // Mac 0x3ae100, Win 0x5248a0
    virtual void RemoveSceneryElement(PhysicsSceneryElement* element);       // Mac 0x3ae380, Win 0x5248f0
    virtual UInt32 GetSceneryElementCount(UInt32 flags);                     // Mac 0x3ae5d0, Win 0x524950
    virtual PhysicsSceneryElement* GetSceneryElement(UInt32 index, UInt32 flags); // Mac 0x3ae6a0, Win 0x524990
    // External assemblies (Vx::VxMechanism) in the universe: their parts / shapes get the scene's
    // user data. Slot 29 / 30: +0xe0, slot 31 / 32: +0xa4. Mac 0x3adee0 / 0x3adfe0 / 0x3adff0 /
    // 0x3ae0f0, Win 0x5247d0 / 0x524830 / 0x524840 / 0x5294e0
    virtual void  Unknown29(void* object);
    virtual void* Unknown30();
    virtual void  Unknown31(void* object);
    virtual void* Unknown32();

    // ---- BaseBrickPhysicsScene slots
    virtual void SetGravity(const Vector3& gravity);                         // Mac 0x3ae6f0, Win 0x5249d0
    virtual void EnableSimulation(BaseBrickPhysicsRigidElement* body);       // Mac 0x3ae740, Win 0x524a00: unfrozen
    virtual void EnableManualMotion(BaseBrickPhysicsRigidElement* body);     // Mac 0x3ae760, Win 0x524a00
    // Dynamic, velocities zero, frozen. Mac 0x3ae7b0, Win 0x524a50
    virtual void DisableSimulation(BaseBrickPhysicsRigidElement* body);
    // Static (control 0), unfrozen. Mac 0x3ae780, Win 0x524a20
    virtual void Unknown37(BaseBrickPhysicsRigidElement* body);
    virtual void Unknown38(void* argument);                                  // Mac 0x3ae810, Win 0x40be80 (empty)
    virtual void Unknown39(void* argument);                                  // Mac 0x3ae820, Win 0x40be80 (empty)
    virtual bool TestElementCollisions(const BasicArray<ActiveRigidElement>& elements); // Mac 0x3ae870, Win 0x524ad0
    virtual void RestoreSnapshotImpl(PhysicsSnapshot* snapshot);             // Mac 0x3aea60, Win 0x524cb0
    // The frame step of the universe (collision only with m_doCollide); counts the step.
    // Mac 0x3af290, Win 0x525030
    virtual void StepPhysics(Real32 time);
    // A VortexBrickPhysicsManualHingeSolver. Mac 0x3af5b0, Win 0x5252b0
    virtual BaseBrickPhysicsBaseHingeSolver* CreateHingeManualSolver(
        BaseBrickPhysicsConnectionHelper* hinge, bool first, const BasicArray<ActiveRigidElement>& elements,
        const BasicArray<BaseBrickPhysicsConnectionHelper*>& connections);
    // A VortexBrickPhysicsSimulationHingeSolver. Mac 0x3af680, Win 0x525340
    virtual BaseBrickPhysicsBaseHingeSolver* CreateHingeSimulateSolver(
        BaseBrickPhysicsConnectionHelper* hinge, bool first, const BasicArray<ActiveRigidElement>& elements,
        const BasicArray<BaseBrickPhysicsConnectionHelper*>& connections);

    // ---- this class's own slots (Mac 47 / 48, Win 46 / 47)
    virtual void Unknown47();                                                // Mac 0x3af420, Win 0x459790 (empty)
    // A VortexPhysicsFreeConnection's engine joint rebuilt (destroyed when present, updated).
    // Mac 0x3af430, Win 0x525160
    virtual void RebuildFreeConnection(BaseBrickPhysicsConnectionHelper* connection);

    // ---- non-virtual (used by the joints and the Vortex solvers)
    // A joint between the two bodies with the given frames (body space) for a hinge solver
    // (`hinges`: the solver's VortexMoveTogetherHingeData array); not referenced (Object refcount
    // 0). Locks the scene's dirty lists around the construction. Name from its log string
    // "VortexPhysicsScene::BuildHingeSolverHinge.". Mac 0x3aeb30, Win 0x524d30
    VortexHingeSolverHinge* BuildHingeSolverHinge(bool first, BaseBrickPhysicsRigidElement* body,
                                                  const Vector3& axis, const Vector3& normal,
                                                  const Vector3& position, BaseBrickPhysicsRigidElement* otherBody,
                                                  const Vector3& otherAxis, const Vector3& otherNormal,
                                                  const Vector3& otherPosition,
                                                  BasicArray<BaseBrickPhysicsConnectionHelper*>* hinges);
    // A spring mirror (`strength`: the stiffness table row) for every hinge / ball joint of
    // `connections` without motors into `springs`; a mirrored ball joint loses its engine joint.
    // `exclude` gets a weak spring. Name from its log strings. Mac 0x3aec50, Win 0x524df0
    void BuildConnectionSpringMirrors(UInt32 strength, const BasicArray<BaseBrickPhysicsConnectionHelper*>& connections,
                                      BasicArray<BaseBrickPhysicsConnectionHelper*>& springs,
                                      BaseBrickPhysicsConnectionHelper* exclude);
    // Deletes the mirrors and rebuilds the engine joints of `connections` that lost theirs.
    // Mac 0x3af1c0
    void DestroyConnectionSpringMirrors(const BasicArray<BaseBrickPhysicsConnectionHelper*>& connections,
                                        BasicArray<BaseBrickPhysicsConnectionHelper*>& springs);
    // VortexPhysicsRigidElement::ResetForces for every body of `elements`. Mac 0x3af250
    void ResetForces(const BasicArray<ActiveRigidElement>& elements);

    // The collision geometries of every body of `scene` (BaseBrickPhysicsScene) placed on `part`
    // (at the bodies' cached transforms, in composites of 16). Mac 0x3ac3a0, Win 0x523490
    static void CollectGeometries(BaseBrickPhysicsScene* scene, Vx::VxPart* part);
    // `shape` on `geometry` and, for a composite, on its children. Mac 0x3af700 (ECX / EDX),
    // Win 0x525440
    static void SetShape(Vx::VxCollisionGeometry* geometry, VortexShapeUserData* shape);

    // ---- data -------------------------------------------------------------------------------
    bool                        m_doCollide;          // +0x90 true (tools/diff/OrigVortex.h kSceneDoCollide)
    Vx::VxUniverse*             m_universe;           // +0x94
    Vx::VxMechanism*            m_mechanism;          // +0x98 the bodies and joints (both builds)
    VortexRigidBodyUserData     m_testBodyData;       // +0x9c kind 2: the temporary test / drag parts
    Vx::VxMechanism*            m_assemblyA;          // +0xa4 (slots 31 / 32)
    VortexShapeCollisionFilter  m_assemblyAFilter;    // +0xa8 (empty)
    VortexShapeUserData         m_assemblyAShape;     // +0xc8 kind 2
    VortexRigidBodyUserData     m_assemblyABodyData;  // +0xd8 kind 4
    Vx::VxMechanism*            m_assemblyB;          // +0xe0 (slots 29 / 30)
    VortexShapeCollisionFilter  m_assemblyBFilter;    // +0xe4 (empty)
    VortexShapeUserData         m_assemblyBShape;     // +0x104 kind 1
    VortexRigidBodyUserData     m_assemblyBBodyData;  // +0x114 kind 5
    BasicArray<VortexPhysicsSceneryElement*> m_sceneryElements; // +0x11c
    Vx::VxPart*                 m_dragPart;           // +0x12c SetDragScene .. ClearDragScene
    Vx::VxCollisionGeometry*    m_castGeometry;       // +0x130 sphere 0.03 (SolveMoveTogether's hinge)

private:
    VortexPhysicsScene(const VortexPhysicsScene&);
    VortexPhysicsScene& operator=(const VortexPhysicsScene&);
};

// ---- snapshot ---------------------------------------------------------------------------------

class VortexPhysicsSnapshot : public BaseBrickPhysicsSnapshot {
public:
    // One body: its part's transform, the LEGO transform and the body (0xc4 bytes).
    struct Entry {
        Vx::VxTransform            m_partTransform;  // +0x00
        Matrix3D                   m_transform;      // +0x80
        VortexPhysicsRigidElement* m_element;        // +0xc0
    };

    // Inline (VortexPhysicsScene::CreateSnapshot). Refcount 1, registered with `scene`.
    explicit VortexPhysicsSnapshot(BaseBrickPhysicsScene* scene) : BaseBrickPhysicsSnapshot(scene) {}
    // Mac D1 0x3b2bb0 / D0 0x3b2c40, Win 0x521a00 / sdd 0x521a90
    virtual ~VortexPhysicsSnapshot() {}

    // Takes the bodies' current transforms again. Mac 0x3ad3d0, Win 0x524240
    virtual void                 Update();
    virtual UInt32               GetCount() const;                           // Mac 0x3b2b30, Win 0x5219a0
    // NOTE: VxArray's range check (VxFatalError) on a bad index. Mac 0x3b2b50, Win 0x5219c0
    virtual PhysicsRigidElement* GetRigidElement(UInt32 index) const;
    virtual void                 Clear();                                    // Mac 0x3b2ba0, Win 0x5219e0

    Vx::VxArray<Entry> m_entries;                                            // +0x0c
};

// ---- the temporary joints of the solvers --------------------------------------------------------

// The joint a hinge solver turns the bodies with: a "toolJoint" VxHinge driven to the target
// angle (BuildHingeSolverHinge).
class VortexHingeSolverHinge : public BaseBrickPhysicsConnectionHelper,
                               public BaseBrickPhysicsSimulationCallback,
                               public Object {
public:
    // Inline (BuildHingeSolverHinge Mac 0x3aeb30)
    VortexHingeSolverHinge(VortexPhysicsScene* scene, BaseBrickPhysicsRigidElement* body, const Vector3& axis,
                           const Vector3& normal, const Vector3& position, BaseBrickPhysicsRigidElement* otherBody,
                           const Vector3& otherAxis, const Vector3& otherNormal, const Vector3& otherPosition,
                           BasicArray<BaseBrickPhysicsConnectionHelper*>* hinges)
        : BaseBrickPhysicsConnectionHelper(body, axis, normal, position, otherBody, otherAxis, otherNormal,
                                           otherPosition),
          BaseBrickPhysicsSimulationCallback(scene), m_vortexScene(scene), m_constraint(0), m_lockSpeed(2.0f),
          m_hinges(hinges)
    {
    }
    // Mac D1 0x3b2920 / D0 0x3b2970 (thunks 0x3b29d0 / 0x3b2a20 / 0x3b2a80 / 0x3b2ad0), Win
    // 0x521870 / sdd 0x5218f0 (thunks 0x529180 / 0x5291c0)
    virtual ~VortexHingeSolverHinge() {}

    virtual UInt32 GetType() const;                         // Mac 0x3b28d0, Win 0x521770: 7
    virtual void*  GetNativeConstraint();                   // Mac 0x3b28e0, Win 0x521850
    virtual bool   HasNativeConstraint();                   // Mac 0x3b2900, Win 0x521860
    virtual bool   Unknown3();                              // Mac 0x3b2910, Win 0x4aede0: false
    // 1 while the joint is more than 0.001 from its target (motorised once within 1.6 % of the
    // lock position); `limits` unused. Mac 0x3b1c00, Win 0x527d90
    virtual UInt32 CheckSeparation(const PhysicsSeparationLimits& limits);
    // The locked VxHinge between the frames, limited between 0 and the current angle.
    // Mac 0x3b1ca0, Win 0x527e00
    virtual bool   UpdateConstraint();
    virtual void   DestroyConstraint();                     // Mac 0x3b2010, Win 0x528090
    // Every spring mirror of the solver adapts (slot 12); 1. Mac 0x3b2060 (thunk 0x3b20b0),
    // Win 0x5280d0
    virtual UInt32 Step(Real32 time);
    using Object::AddRef;
    using Object::Release;

    VortexPhysicsScene*                            m_vortexScene;   // +0x74
    Vx::VxConstraint*                              m_constraint;    // +0x78 the VxHinge
    Real32                                         m_lockSpeed;     // +0x7c 2
    BasicArray<BaseBrickPhysicsConnectionHelper*>* m_hinges;        // +0x80 the solver's spring mirrors
};

// The joint of PhysicsScene::SolveMoveTogether: a spring (+ a linear constraint) that pulls the
// two frames together, then a VxHinge once they are within 0.1.
class VortexMoveTogetherHinge : public BaseBrickPhysicsConnectionHelper,
                                public BaseBrickPhysicsSimulationCallback,
                                public Object {
public:
    // Inline (SolveMoveTogether Mac 0x3adaf0)
    VortexMoveTogetherHinge(VortexPhysicsScene* scene, BaseBrickPhysicsRigidElement* body, const Vector3& axis,
                            const Vector3& normal, const Vector3& position, BaseBrickPhysicsRigidElement* otherBody,
                            const Vector3& otherAxis, const Vector3& otherNormal, const Vector3& otherPosition,
                            PhysicsCollisionCallback* callback, const PhysicsGeometryIdentifier* features,
                            UInt32 featureCount, Vx::VxCollisionGeometry* castGeometry,
                            BasicArray<BaseBrickPhysicsConnectionHelper*>* hinges)
        : BaseBrickPhysicsConnectionHelper(body, axis, normal, position, otherBody, otherAxis, otherNormal,
                                           otherPosition),
          BaseBrickPhysicsSimulationCallback(scene), m_vortexScene(scene), m_unknown78(0), m_constraint(0),
          m_linear(0), m_attached(false), m_callback(callback), m_features(features), m_featureCount(featureCount),
          m_castGeometry(castGeometry), m_maxForce(2.0f), m_hinges(hinges),
          m_bodyData(VortexRigidBodyUserData::KindMoveTogether)
    {
    }
    // Mac D1 0x3b26c0 / D0 0x3b2710 (thunks 0x3b2770 / 0x3b27c0 / 0x3b2820 / 0x3b2870), Win
    // 0x5217a0 / sdd 0x521830 (thunks 0x5291b0 / 0x5291f0)
    virtual ~VortexMoveTogetherHinge() {}

    virtual UInt32 GetType() const;                         // Mac 0x3b2670, Win 0x521770: 7
    virtual void*  GetNativeConstraint();                   // Mac 0x3b2680, Win 0x521780
    virtual bool   HasNativeConstraint();                   // Mac 0x3b26a0, Win 0x521790
    virtual bool   Unknown3();                              // Mac 0x3b26b0, Win 0x4aede0: false
    // 1 (stretched) unless the axes are within limits.m_alignment and the positions farther than
    // limits.m_distance. NOTE: returns 0 / 1 only. Mac 0x3b0cb0, Win 0x527140
    virtual UInt32 CheckSeparation(const PhysicsSeparationLimits& limits);
    // Not attached: a VxSpring (natural length 0, limited) and a VxLinear2 between the
    // positions; attached: a VxHinge. Mac 0x3b0dc0, Win 0x527300
    virtual bool   UpdateConstraint();
    virtual void   DestroyConstraint();                     // Mac 0x3b14e0, Win 0x527860
    // The features stop colliding. Mac 0x3b1560 (thunk 0x3b15b0), Win 0x5278d0
    virtual void   Start();
    // The features collide again; `result` cleared when a moved body collides.
    // Mac 0x3b1600 (thunk 0x3b1a60), Win 0x527910
    virtual void   Stop(bool& result);
    // Not attached: the mirrors reset, attach within 0.1; attached: the mirrors adapt and the
    // hinge stiffens by 10 %. 0. Mac 0x3b1a80 (thunk 0x3b1be0), Win 0x527a80
    virtual UInt32 Step(Real32 time);
    using Object::AddRef;
    using Object::Release;

    VortexPhysicsScene*                            m_vortexScene;   // +0x74
    UInt32                                         m_unknown78;     // +0x78 0
    Vx::VxConstraint*                              m_constraint;    // +0x7c VxSpring, then VxHinge
    Vx::VxConstraint*                              m_linear;        // +0x80 VxLinear2 (not attached)
    bool                                           m_attached;      // +0x84
    PhysicsCollisionCallback*                      m_callback;      // +0x88
    const PhysicsGeometryIdentifier*               m_features;      // +0x8c
    UInt32                                         m_featureCount;  // +0x90
    Vx::VxCollisionGeometry*                       m_castGeometry;  // +0x94 the scene's (unused)
    Real32                                         m_maxForce;      // +0x98 2
    BasicArray<BaseBrickPhysicsConnectionHelper*>* m_hinges;        // +0x9c the solver's spring mirrors
    VortexRigidBodyUserData                        m_bodyData;      // +0xa0 kind 3
};

// ---- the spring mirrors of a simulating hinge solve -------------------------------------------

// A copy of a joint (same bodies / frames) as a soft engine constraint; the solver tunes its
// stiffness (slots 9-12) from the strength table row m_strength. (no vtable of its own)
class VortexPhysicsConstraintSpringHelper : public BaseBrickPhysicsConnectionHelper {
public:
    // Inline (the two subclasses' constructors): a mirror of `connection`.
    VortexPhysicsConstraintSpringHelper(UInt32 strength, const BaseBrickPhysicsConnectionHelper& connection)
        : BaseBrickPhysicsConnectionHelper(connection), m_strength(strength), m_constraint(0)
    {
    }
    virtual ~VortexPhysicsConstraintSpringHelper() {}

    virtual void Stiffen() = 0;                             // slot 9: x1.5 up to the row's maximum
    virtual void Soften() = 0;                              // slot 10: x0.5 down to the row's minimum
    virtual void ResetStiffness() = 0;                      // slot 11: the row's default
    virtual void Adapt() = 0;                               // slot 12: from the joint's state

    UInt32            m_strength;     // +0x64 row of the stiffness tables (Mac 0x63fd30 / 0x63fd48)
    Vx::VxConstraint* m_constraint;   // +0x68
};

class VortexPhysicsConstraintHingeSpringHelper : public VortexPhysicsConstraintSpringHelper {
public:
    // A mirror of `hinge` (positions moved by 1 along the axes); `motor`: driven to the
    // current angle. Mac 0x3b0100, Win 0x525f80
    VortexPhysicsConstraintHingeSpringHelper(UInt32 strength, const BaseBrickPhysicsConnectionHelper& hinge, bool motor);
    // Mac D1 0x3b2620 / D0 0x3b2630, Win 0x5216f0 / sdd 0x521750
    virtual ~VortexPhysicsConstraintHingeSpringHelper() {}

    virtual UInt32 GetType() const;                         // Mac 0x3b25f0, Win 0x51e780: 1
    virtual void*  GetNativeConstraint();                   // Mac 0x3afa00, Win 0x52a700
    virtual bool   HasNativeConstraint();                   // Mac 0x3b2600, Win 0x51cf20
    virtual bool   Unknown3();                              // Mac 0x3b2610, Win 0x4aede0: false
    virtual UInt32 CheckSeparation(const PhysicsSeparationLimits& limits); // Mac 0x3af8b0, Win 0x5255e0
    virtual bool   UpdateConstraint();                      // Mac 0x3afa20, Win 0x5257f0
    virtual void   DestroyConstraint();                     // Mac 0x3afd70, Win 0x525b00
    virtual void   Stiffen();                               // Mac 0x3afdc0, Win 0x525b40
    virtual void   Soften();                                // Mac 0x3afe30, Win 0x525bb0
    virtual void   ResetStiffness();                        // Mac 0x3afea0, Win 0x525c20
    virtual void   Adapt();                                 // Mac 0x3afee0, Win 0x525c50

    // Motor stiffness / damping / maximum force / friction from `stiffness` (scaled down near
    // the target angle unless row 1). Mac 0x3afc40, Win 0x5259f0
    void SetStiffness(double stiffness);
    // The signed angle between the frames' normals about the first axis. Mac 0x3aff60,
    // Win 0x525ce0
    Real32 GetAngle() const;

    Real32 m_targetAngle;   // +0x6c -GetAngle() at construction
    bool   m_motor;         // +0x70
};

class VortexPhysicsConstraintBallSpringHelper : public VortexPhysicsConstraintSpringHelper {
public:
    // A mirror of `ball` (the second frame re-expressed in the second body); `strong`: the
    // row's stiffness, else 10. Mac 0x3b0b70, Win 0x526d30
    VortexPhysicsConstraintBallSpringHelper(UInt32 strength, const BaseBrickPhysicsConnectionHelper& ball, bool strong);
    // Mac D1 0x3b2d30 / D0 0x3b2d40, Win sdd 0x526fc0
    virtual ~VortexPhysicsConstraintBallSpringHelper() {}

    virtual UInt32 GetType() const;                         // Mac 0x3b2d00, Win 0x475ed0: 5
    virtual void*  GetNativeConstraint();                   // Mac 0x3b02c0, Win 0x52a700
    virtual bool   HasNativeConstraint();                   // Mac 0x3b2d10, Win 0x51cf20
    virtual bool   Unknown3();                              // Mac 0x3b2d20, Win 0x4aede0: false
    virtual UInt32 CheckSeparation(const PhysicsSeparationLimits& limits); // Mac 0x3b01e0, Win 0x526060
    virtual bool   UpdateConstraint();                      // Mac 0x3b02e0, Win 0x526150 (a VxRPRO)
    virtual void   DestroyConstraint();                     // Mac 0x3b04f0, Win 0x525b00
    virtual void   Stiffen();                               // Mac 0x3b0540, Win 0x526380
    virtual void   Soften();                                // Mac 0x3b05e0, Win 0x526430
    virtual void   ResetStiffness();                        // Mac 0x3b0690, Win 0x5264e0
    virtual void   Adapt();                                 // Mac 0x3b08a0, Win 0x5268e0

    bool m_strong;          // +0x6c
};

// ---- solvers / actions / collectors -----------------------------------------------------------

// The manual hinge solver of the Vortex scene: the base's Solve, plus the hinge array of the
// Vortex solvers (BuildHingeSolverHinge).
class VortexBrickPhysicsManualHingeSolver : public BaseBrickPhysicsManualHingeSolver, public VortexMoveTogetherHingeData {
public:
    // Inline (CreateHingeManualSolver Mac 0x3af5b0)
    VortexBrickPhysicsManualHingeSolver(VortexPhysicsScene* scene, BaseBrickPhysicsConnectionHelper* hinge, bool first,
                                        const BasicArray<ActiveRigidElement>& elements,
                                        const BasicArray<BaseBrickPhysicsConnectionHelper*>& connections)
        : BaseBrickPhysicsManualHingeSolver(scene, hinge, first, elements, connections), m_vortexScene(scene)
    {
    }
    // Mac D1 0x3b3c30 / D0 0x3b3c70 (thunks 0x3b3cd0 / 0x3b3d10 / 0x3b3d80 / 0x3b3dc0), Win sdd
    // 0x521610 (thunks 0x5291a0 / 0x5291d0)
    virtual ~VortexBrickPhysicsManualHingeSolver() {}

    VortexPhysicsScene* m_vortexScene;                      // +0x54
};

// Pulls a body point towards a world point (impulse = 3 min(d, 3) * strength along d).
class PointGravitationAction : public Vx::VxAssemblyBase::UpdateSubscriber {
public:
    PointGravitationAction(VortexPhysicsRigidElement* element, const Vector3& point, const Vector3& target,
                           double strength)
        : m_element(element), m_point(point), m_target(target), m_strength(strength), m_part(0)
    {
    }
    // Mac D1 0x3b24f0 / D0 0x3b2550 (the inline VxRefCount destructor), Win sdd 0x52eb20
    virtual ~PointGravitationAction() {}
    // `dt` unused. Mac 0x3af770, Win 0x5254b0
    virtual void update(Vx::VxReal dt);

    VortexPhysicsRigidElement* m_element;    // +0x08
    Vector3                    m_point;      // +0x0c body space
    Vector3                    m_target;     // +0x18 world
    double                     m_strength;   // +0x24 (Win +0x28)
    // The element's part (set by VortexAnchorSolver::Anchor::SetPointAttraction; not read).
    // sizeof Mac 0x30, Win 0x38
    Vx::VxPart*                m_part;       // +0x2c (Win +0x30)
};

// The universe collector of TestCollisions: brick contacts deeper than m_tolerance count as a
// collision unless the callback says otherwise.
class LEGOUniverseCollector : public Vx::VxUniverse::IntersectSubscriber {
public:
    LEGOUniverseCollector(PhysicsCollisionCallback* callback, double tolerance)
        : m_callback(callback), m_tolerance(tolerance), m_collides(false)
    {
    }
    virtual ~LEGOUniverseCollector() {}                     // Mac D1 0x3ac390 / D0 0x3b2da0, Win sdd 0x521d80
    virtual void notifyDisjoint(int event, Vx::VxIntersectResult* result);   // Mac 0x3b2db0, Win 0x424ec0 (empty)
    // Mac 0x3b2dc0, Win 0x521b60
    virtual void notifyIntersect(int event, Vx::VxIntersectResult* result, Vx::VxDynamicsResponseInput* response);

    PhysicsCollisionCallback* m_callback;    // +0x04
    double                    m_tolerance;   // +0x08 (Win +0x08)
    bool                      m_collides;    // +0x10
};

// The collector of the single-sensor queries (insertion / drag / element tests); stops at the
// first collision.
class LEGOIndividualCollector : public Vx::VxIntersect::IntersectSubscriber {
public:
    LEGOIndividualCollector(PhysicsCollisionCallback* callback, double tolerance)
        : m_callback(callback), m_tolerance(tolerance), m_collides(false)
    {
    }
    virtual ~LEGOIndividualCollector() {}                   // Mac D1 0x3ac840 / D0 0x3b3b70, Win sdd 0x522030
    // 2 (stop) on a collision, else 0. Mac 0x3b3720, Win 0x521e00
    virtual int notifyIntersect(Vx::VxCollisionGeometry* other);

    PhysicsCollisionCallback* m_callback;    // +0x0c
    double                    m_tolerance;   // +0x10
    bool                      m_collides;    // +0x18
};

// The collector of the drag sweep: the earliest time of impact.
class CastIndividualCollector : public Vx::VxIntersect::IntersectSubscriber {
public:
    CastIndividualCollector() : m_hit(false), m_timeOfImpact(DBL_MAX) {}
    virtual ~CastIndividualCollector() {}                   // Mac D1 0x3ad020 / D0 0x3b3c20, Win sdd 0x523b60
    // Mac 0x3b3bc0, Win 0x523b20
    virtual int notifyIntersect(Vx::VxCollisionGeometry* other);

    bool   m_hit;            // +0x0c
    double m_timeOfImpact;   // +0x10 DBL_MAX
};

// The segment query of CastDragScene: the terrain only.
class TerrainRayCastCollector : public Vx::VxLineSegIntersectSub {
public:
    // 0 for a terrain geometry (shape kind 4), else 1 (skip). Mac 0x3b3b80, Win 0x523b90
    virtual int filter(Vx::VxCollisionGeometry* geometry);
    virtual ~TerrainRayCastCollector() {}                   // Mac D1 0x3ad010 / D0 0x3b3bb0, Win sdd 0x523bc0
};

} // namespace LEGO
