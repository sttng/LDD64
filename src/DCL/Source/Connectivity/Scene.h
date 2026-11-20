// LEGO Digital Designer — restored source
// LEGO::Scene — the connectivity scene graph of a model: the bricks, parts, groups and primitive
// elements, and the rigid / hinged elements built from their connections; LEGO::SceneObserver.
//
// Original object file Scene.cpp, restored as Scene.cpp (Mac 025n
// core 0xa5a70-0xb49c0, 025q constructor / destructor 0xbac90-0xc2860, Scene methods placed in
// 025a/d/e/i/l; Win 0x47adf0-0x482a40, 0x486e10-0x490a10), SceneStorage.cpp (025o,
// 0xb49c0-0xb8ec0) and SimplePhysicsSceneDebugInfo.cpp (025p, 0xb8ec0-0xbac90).
//
// RTTI: class LEGO::Scene : BehaviorElement, Notifier<SceneObserver> (Win RTTI attribute 3).
//   Mac sizeof 0x1bc (BrickConnect::CreateScene 0xc2860): vptr +0 (vtable 0x7061c0
//       {GetBoundingBox 0xa62a0, GetScene 0xc4920, D1 0xbc060, D0 0xbc1a0, GetType 0xa5b20,
//       PhysicsCall5 0xb1c50, PhysicsCall6 0xb1ca0, OnFinalRelease 0xbc310}), m_behaviors +0x04,
//       Notifier<SceneObserver> +0x14 (vtable 0x7061ec {thunks 0xbc0c0, 0xbc210, 0xbc3f0};
//       observers +0x18, running iterations +0x28), own members +0x38-+0x1b0, Object at +0x1b4
//       (vtable 0x706210 {thunks 0xbc130, 0xbc290, 0xbc470, AddRef, Release}). VTT 0x706230.
//       Constructor 0xbac90, destructor body 0xbb570.
//   Win sizeof 0x1d4 (BrickConnect::CreateScene 0x490a10): vfptr +0 (0xa23bf4 {0x47adf0,
//       0x404f30, 0x51e780, 0x480ed0, 0x480f10}), vbptr +4, members +4 (Notifier at +0x18 with
//       its own vbptr; the sets / maps are 0x24 bytes), Object at +0x1cc (vftable 0xa23c0c
//       {sdd 0x4a1b50, OnFinalRelease 0x4a1b00, …}). Constructor 0x48a5a0, destructor 0x48a8e0.
//
// The member layout below is the Mac one (constructor 0xbac90). Every member and method name is
// inferred unless noted; the profiling timer names of the TU initialiser (Mac 0xd66f0:
// "Scene::CanConnect", "Scene::Insert", "Scene::Remove(Brick/Bricks)", "Scene::BatchInsert test
// collision", "Scene::BatchInsert update structure", "Scene::BatchInsert build physics",
// "Scene::Clear") name the corresponding methods.
#pragma once
#include "LEGO/Types.h"
#include "BehaviorElement.h"
#include "Notifier.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Geometry.h"
#include "BrickPhysics.h"
#include "PrimitiveElement.h"
#include "StorageScene2.h"

namespace LEGO {

class Scene;
class Brick;
class Part;
class BrickGroup;
class BrickConnect;
class BrickDatabase;
class HingedElement;
class RigidElement;
class RigidConnection;
class PrimitiveRigidElement;
class PrimitiveRigidConnection;
class PrimitiveConnection;
class ConnectionManipulator;
class PhysicsSceneDebugInfo;
class StringPullSolverCallback;   // StringPullSolverCallback.h
class ClickPullSolverCallback;    // ClickPullSolverCallback.h
struct Color;                     // SceneListener::AddPoint …

// ---------------------------------------------------------------------------------------------
// SceneObserver (RTTI: class LEGO::SceneObserver : Observer<Scene, SceneObserver>).
// Mac vtable 0x704bcc {D1 0x51a30, D0 0x51a60, OnFinalRelease 0x45120, OnAttached 0x51ab0,
// OnDetached 0x51ac0, 0x51ad0, 0x51ae0, 0x51af0, 0x51b00} (the four notifications are empty in
// the base class). Win vtable 0x9cf3a0 {OnAttached, OnDetached, 0x56b1c0, 0x56b1c0, 0x565ff0,
// 0x566000}.
// ---------------------------------------------------------------------------------------------
class SceneObserver : public Observer<Scene, SceneObserver> {
public:
    SceneObserver() {}
    virtual ~SceneObserver() {}
    // slot 5: after a physics step (Scene::NotifyObservers5, Mac 0x398f0). Mac 0x51ad0
    virtual void OnSceneNotification5() {}
    // slot 6: the structure (elements / connections) changed (Scene::EndUpdate). Mac 0x51ae0
    virtual void OnStructureChanged() {}
    // slot 7: bodies created / changed / removed since the last notification. Mac 0x51af0
    virtual void OnRigidElementsChanged(const BasicArray<RigidElement*>& /*added*/,
                                        const BasicArray<RigidElement*>& /*changed*/,
                                        const BasicArray<RigidElement*>& /*removed*/) {}
    // slot 8: groups added / with changed parts / moved in the hierarchy / removed. Mac 0x51b00
    virtual void OnGroupsChanged(const BasicArray<RefPointer<BrickGroup> >& /*added*/,
                                 const BasicArray<RefPointer<BrickGroup> >& /*changed*/,
                                 const BasicArray<RefPointer<BrickGroup> >& /*moved*/,
                                 const BasicArray<RefPointer<BrickGroup> >& /*removed*/) {}
};

// ---------------------------------------------------------------------------------------------
// The object at Scene +0x38: called with the code 'mdsb' (0x6d736462) when an update starts and
// 'conn' (0x636f6e6e) by the drag snapping. Reference counted through Mac slots 0 / 1
// (SetListener Mac 0xbab80; the Scene destructor calls slot 1). Never created inside TU 025; the
// class is VisualDebugger (see RemoveAllShapes); Mac slot 7 is the notification. (class and method
// names inferred)
// ---------------------------------------------------------------------------------------------
class SceneListener {
public:
    enum { CodeBeginUpdate = 0x6d736462, CodeConnect = 0x636f6e6e };
    virtual void AddRef() = 0;                                   // Mac slot 0
    virtual void Release() = 0;                                  // Mac slot 1
    // A point / line / triangle shape of `category`; returns the shape's id. `id`
    // 0: a new shape. Arguments from ClickStickCastUtility::DrawDebugInfo (Mac 0x365ee0) and
    // RenderVisualDebugger (Mac 0x3919a0). (names inferred)
    virtual UInt32 AddPoint(UInt32 category, UInt32 id, const Vector3& position, const Color& color,
                            Real32 size) = 0;                    // Mac slot 2
    virtual UInt32 AddLine(UInt32 category, UInt32 id, const Vector3& from, const Vector3& to,
                           const Color& color) = 0;              // Mac slot 3
    virtual UInt32 AddTriangle(UInt32 category, UInt32 id, const Vector3& a, const Vector3& b,
                               const Vector3& c, const Color& color) = 0; // Mac slot 4
    // Removes shape `id` of `category`; a new (empty) shape of `category`, its id
    // (RenderVisualDebugger Mac 0x3929e0 / 0x392c60). (names inferred)
    virtual void RemoveShape(UInt32 category, UInt32 id) = 0;   // Mac slot 5
    virtual UInt32 NewShape(UInt32 category) = 0;               // Mac slot 6
    // Removes every shape of category `code` (RenderVisualDebugger Mac 0x392cd0): the scene
    // drops its 'mdsb' / 'conn' debug shapes before drawing them again.
    virtual void OnSceneEvent(UInt32 code) = 0;                  // Mac slot 7 (+0x1c)
    // The class is the RTTI class LEGO::VisualDebugger (Mac vtable
    // 0x778950: 9 pure slots, D1 0x393ad0, D0 0x393ae0), implemented by RenderVisualDebugger
    // (RenderVisualDebugger.h), the BrickConnectSceneControl sub-object that
    // BrickConnectSceneControl::SetScene (Mac 0x339d0) installs as the listener.
    // Removes every shape (RenderVisualDebugger Mac 0x392db0; debug key 'J'). (name inferred)
    virtual void RemoveAllShapes() = 0;                          // Mac slot 8 (+0x20)
    virtual ~SceneListener() {}
};
typedef SceneListener VisualDebugger;   // (additive) the RTTI name

// Reported by Scene::BatchInsert for every colliding feature pair of an inserted brick (the
// BatchInsertPhysicsCollisionCallback calls slot 2 when the first element belongs to an
// inserted brick, slot 3 when the second one does). Class unknown. (names inferred)
class BatchInsertCollisionReporter {
public:
    virtual ~BatchInsertCollisionReporter() {}                   // Mac slots 0/1
    virtual void OnCollision(PrimitiveElement* other, UInt32 otherFeature,
                             PrimitiveElement* inserted, UInt32 insertedFeature) = 0;    // slot 2
    virtual void OnCollisionReversed(PrimitiveElement* inserted, UInt32 insertedFeature,
                                     PrimitiveElement* other, UInt32 otherFeature) = 0;  // slot 3
};

class SceneIndex;

class Scene : public BehaviorElement, public Notifier<SceneObserver> {
public:
    // ---- nested types --------------------------------------------------------------------

    // Element of the manipulators' BasicArray<Scene::ActiveRigidElement> (RTTI name; 0x14
    // bytes Mac: the manipulator destructor Mac 0xc40b0 destroys the array at +4).
    struct ActiveRigidElement {
        RigidElement*             m_element;    // +0x00
        BasicArray<RigidElement*> m_elements;   // +0x04
    };

    // Order of the drag caches (RTTI Scene::Matrix3DLess): Matrix3D::IsLess with 0.001
    // (Mac 0xaba40: 0x3a83126f).
    struct Matrix3DLess {
        bool operator()(const Matrix3D& a, const Matrix3D& b) const { return a.IsLess(b, 0.001f); }
    };

    // A placement found by CanConnect: the transform and the field connections it makes.
    // 0x50 bytes Mac (Scene +0xcc is one, BatchInsert / Insert use stack copies). (name inferred)
    struct ConnectResult {
        Matrix3D                             m_transform;     // +0x00
        BasicArray<PrimitiveConnectionInfo>  m_connections;   // +0x40
    };

    // A picked primitive element (RTTI Scene::PrimitiveElementWithDistance, BasicMultiset
    // element, 0x18 bytes Mac): squared distance of the bone's bounding sphere centre to the
    // picker's reference point. (member names inferred)
    struct PrimitiveElementWithDistance {
        PrimitiveElement* m_element;    // +0x00
        Real32            m_distance;   // +0x04
        Sphere            m_sphere;     // +0x08 world bounding sphere
        PrimitiveElementWithDistance(PrimitiveElement* element, Real32 distance, const Sphere& sphere)
            : m_element(element), m_distance(distance), m_sphere(sphere) {}
        bool operator<(const PrimitiveElementWithDistance& o) const { return m_distance < o.m_distance; }
    };
    typedef BasicMultiset<PrimitiveElementWithDistance> PickResult;

    // A picked connection field point (RTTI Scene::Feature, 0x4c bytes Mac). (names inferred)
    struct Feature {
        PrimitiveElement*      m_element;   // +0x00
        const ConnectionField* m_field;     // +0x04
        UInt32                 m_index;     // +0x08 point index
        Matrix3D               m_offset;    // +0x0c snap offset (ConnectionField::Pick)
    };

    // Visitor of Scene::Pick (RTTI Scene::Picker; no virtual destructor: 2 Mac slots).
    class Picker {
    public:
        // slot 0: does the (world) bounding sphere need a closer look?
        virtual bool Test(const Sphere& sphere) = 0;
        // slot 1: a primitive element at `transform` whose bone sphere passed Test.
        virtual void Visit(PrimitiveElement* element, const Matrix3D& transform, const Sphere& sphere) = 0;
    };
    // Mac vtable 0x7067dc {Test 0xc4040, Visit 0xa5b30}, Win 0xa23e64 {0x47a950, 0x46ffb0}.
    class FrustumPicker : public Picker {
    public:
        FrustumPicker(const Frustum& frustum, PickResult* result)
            : m_frustum(frustum), m_result(result), m_classification(0) {}
        virtual bool Test(const Sphere& sphere);                                        // Mac 0xc4040
        virtual void Visit(PrimitiveElement* element, const Matrix3D& transform, const Sphere& sphere); // Mac 0xa5b30
        Frustum     m_frustum;          // +0x04 (4 planes; the apex +0x44 is the distance reference)
        PickResult* m_result;           // +0x50
        UInt32      m_classification;   // +0x54 Frustum::Classify of the last tested sphere
    };
    // Skips hidden elements (PrimitiveElement::m_flagAC). Mac vtable 0x76258c {0xc4040, 0xc8930}
    class HideAwareFrustumPicker : public FrustumPicker {
    public:
        HideAwareFrustumPicker(const Frustum& frustum, PickResult* result)
            : FrustumPicker(frustum, result) {}
        virtual void Visit(PrimitiveElement* element, const Matrix3D& transform, const Sphere& sphere); // Mac 0xc8930
    };
    // Mac vtable 0x76257c {Test 0xc8830, Visit 0xc88d0}
    class RayPicker : public Picker {
    public:
        RayPicker(const Vector3& origin, const Vector3& direction, PickResult* result)
            : m_origin(origin), m_direction(direction), m_result(result) {}
        virtual bool Test(const Sphere& sphere);                                        // Mac 0xc8830, Win 0x46ff00
        virtual void Visit(PrimitiveElement* element, const Matrix3D& transform, const Sphere& sphere); // Mac 0xc88d0
        Vector3     m_origin;      // +0x04
        Vector3     m_direction;   // +0x10
        PickResult* m_result;      // +0x1c
    };
    // Mac vtable 0x76254c {0xc8830, 0xc8870}
    class HideAwareRayPicker : public RayPicker {
    public:
        HideAwareRayPicker(const Vector3& origin, const Vector3& direction, PickResult* result)
            : RayPicker(origin, direction, result) {}
        virtual void Visit(PrimitiveElement* element, const Matrix3D& transform, const Sphere& sphere); // Mac 0xc8870, Win 0x46fe50
    };
    // Mac vtable 0x762464 {Test 0xc7c90, Visit 0xc7d10}
    class ConePicker : public Picker {
    public:
        ConePicker(const Cone& cone, PickResult* result) : m_cone(cone), m_result(result) {}
        virtual bool Test(const Sphere& sphere);                                        // Mac 0xc7c90
        virtual void Visit(PrimitiveElement* element, const Matrix3D& transform, const Sphere& sphere); // Mac 0xc7d10
        Cone        m_cone;     // +0x04 (the apex is the distance reference)
        PickResult* m_result;   // +0x34
    };
    // Mac vtable 0x7776b8 {0xc7c90, 0xc7cb0}
    class HideAwareConePicker : public ConePicker {
    public:
        HideAwareConePicker(const Cone& cone, PickResult* result) : ConePicker(cone, result) {}
        virtual void Visit(PrimitiveElement* element, const Matrix3D& transform, const Sphere& sphere); // Mac 0xc7cb0
    };
    // Mac vtable 0x7623f4 {Test 0xc7ab0, pure}
    class SpherePicker : public Picker {
    public:
        explicit SpherePicker(const Sphere& sphere) : m_sphere(sphere) {}
        virtual bool Test(const Sphere& sphere);                                        // Mac 0xc7ab0
        Sphere m_sphere;   // +0x04
    };
    // Collects (element, transform) pairs. Mac vtable 0x7623c8 {0xc7ab0, Visit 0xc7ad0}
    class PrimitiveSpherePicker : public SpherePicker {
    public:
        PrimitiveSpherePicker(const Sphere& sphere, BasicArray<Pair<PrimitiveElement*, Matrix3D> >* result)
            : SpherePicker(sphere), m_result(result) {}
        virtual void Visit(PrimitiveElement* element, const Matrix3D& transform, const Sphere& sphere); // Mac 0xc7ad0
        BasicArray<Pair<PrimitiveElement*, Matrix3D> >* m_result;   // +0x14
    };
    // Mac vtable 0x762424 {0xc7ab0, Visit 0xc7c30}
    class HideAwareOrderedElementsSpherePicker : public SpherePicker {
    public:
        HideAwareOrderedElementsSpherePicker(const Sphere& sphere, PickResult* result)
            : SpherePicker(sphere), m_result(result) {}
        virtual void Visit(PrimitiveElement* element, const Matrix3D& transform, const Sphere& sphere); // Mac 0xc7c30
        PickResult* m_result;   // +0x14 (distance from the sphere centre)
    };
    // Ray pick of field points: keeps the nearest features (within 0.0001 of each other).
    // Mac vtable 0x7067fc {Test 0xc4070, Visit 0xa5fc0}, Win 0x9cf598 {0x47aca0, 0x614413}
    class HideAwareSamePlaceFeaturesPicker : public Picker {
    public:
        HideAwareSamePlaceFeaturesPicker(BasicArray<Feature>* result, const Vector3& origin,
                                         const Vector3& direction, Real32 maxDistance)
            : m_result(result), m_origin(origin), m_direction(direction), m_distance(maxDistance) {}
        virtual bool Test(const Sphere& sphere);                                        // Mac 0xc4070, Win 0x47aca0
        virtual void Visit(PrimitiveElement* element, const Matrix3D& transform, const Sphere& sphere); // Mac 0xa5fc0
        BasicArray<Feature>* m_result;      // +0x04
        Vector3              m_origin;      // +0x08
        Vector3              m_direction;   // +0x14
        Real32               m_distance;    // +0x20 nearest distance so far (start: the maximum)
    };

    // Progress of BatchInsert (RTTI Scene::BatchInsertProgressCallback; Win vtable 0xa23e34
    // {sdd 0x470310, pure}). Returns false to cancel.
    class BatchInsertProgressCallback {
    public:
        virtual ~BatchInsertProgressCallback() {}                // Mac slots 0/1
        virtual bool OnProgress(UInt32 percent) = 0;             // slot 2
    };
    // Callbacks of Load (RTTI Scene::LoadCallback; Win vtable 0x9cf13c {sdd, pure, pure}).
    class LoadCallback {
    public:
        virtual ~LoadCallback() {}                               // Mac slots 0/1
        // A brick was created from `storage`; false drops the brick (not loaded, not reported
        // missing) (slot 2).
        virtual bool OnBrick(const StorageScene2::Brick* storage, Brick* brick) = 0;
        // 0-100; false cancels the load (slot 3).
        virtual bool OnProgress(UInt32 percent) = 0;
    };

    // Load: storage object -> scene object (Mac 0xb3690). 4 maps Mac +0x00/+0x1c/+0x38/+0x54.
    struct StorageMap {
        BasicMap<const StorageScene2::Brick*, Brick*>           m_bricks;     // +0x00
        BasicMap<const StorageScene2::Part*, Part*>             m_parts;      // +0x1c
        BasicMap<const StorageScene2::Bone*, PrimitiveElement*> m_elements;   // +0x38
    };
    // Save: scene object -> storage object (Mac 0xb5ae0).
    struct SaveMap {
        BasicMap<const Brick*, StorageScene2::Brick*>           m_bricks;     // +0x00
        BasicMap<const Part*, StorageScene2::Part*>             m_parts;      // +0x1c
        BasicMap<const PrimitiveElement*, StorageScene2::Bone*> m_elements;   // +0x38
    };
    // CopyBricks: source object -> copy (Mac 0xa6d30).
    struct CopyMap {
        BasicMap<const Brick*, Brick*>                       m_bricks;     // +0x00
        BasicMap<const Part*, Part*>                         m_parts;      // +0x1c
        BasicMap<const PrimitiveElement*, PrimitiveElement*> m_elements;   // +0x38
        BasicMap<const BrickGroup*, BrickGroup*>             m_groups;     // +0x54
    };

    // Behavior copy flags of CopyBricks (bits of `flags`; names inferred).
    enum CopyFlags {
        CopyFlagScene    = 0x01,   // the source scene's behaviors
        CopyFlagGroups   = 0x02,   // groups (and their behaviors)
        CopyFlagBricks   = 0x04,
        CopyFlagParts    = 0x08,
        CopyFlagElements = 0x10
    };

    // ---- construction ---------------------------------------------------------------------

    // Mac 0xbac90 (025q), Win 0x48a5a0. References the three objects.
    Scene(BrickConnect* owner, BrickPhysics* physics, PhysicsScene* physicsScene);

    // --- slot 0: union of the hinged elements' boxes (zero box when empty). Mac 0xa62a0, Win 0x47adf0
    virtual void GetBoundingBox(BoundingBox& box, bool recompute) const;
    // --- slot 1: Mac 0xc4920 (025s), Win 0x404f30
    virtual Scene* GetScene() const { return const_cast<Scene*>(this); }
    // --- slots 2/3: Mac 0xbc060 / 0xbc1a0 (body 0xbb570), Win 0x48a8e0 / sdd 0x4a1b50
    virtual ~Scene();
    // --- slot 4: Mac 0xa5b20, Win 0x51e780
    virtual UInt32 GetType() const { return TypeScene; }
    // --- slots 5/6: while the PhysicsScene simulates (slot 24) forward to its slots 22 / 23
    // and notify the observers (slot 5). Mac 0xb1c50 / 0xb1ca0, Win 0x480ed0 / 0x480f10
    virtual UInt32 PhysicsCall5(UInt32 argument);
    virtual UInt32 PhysicsCall6(bool argument);

    // ---- accessors (inline in the original) -------------------------------------------------
    BrickConnect*  GetBrickConnect() const;                 // m_brickConnect +0x3c
    BrickPhysics*  GetBrickPhysics() const { return m_physics; }
    PhysicsScene*  GetPhysicsScene() const { return m_physicsScene; }
    SceneListener* GetListener() const { return m_listener; }
    // Replaces the listener (references the new one, releases the old one). Mac 0xbab80 (025p)
    void SetListener(SceneListener* listener);
    // (additive) The collision test flag (+0x1b0; written inline by EditMode's
    // ToggleDoCanConnectPhysicsTestCommand, Mac 0x195da0). (names inferred)
    bool GetTestCollisions() const { return m_testCollisions; }
    void SetTestCollisions(bool test) { m_testCollisions = test; }

    // ---- debug drawing (SimplePhysicsSceneDebugInfo.cpp, sub-TU 025p; names inferred) -------
    // Lines / triangles of every body, each at `transform` * its physics transform
    // (`flags`: SimplePhysicsSceneDebugInfo::Flag...). Seeds rand() with 1 first, so the random
    // colours repeat. The caller takes the reference. Mac 0xbaa40, Win 0x48a460 (the Win
    // build has the flags of its only caller, 0x804, folded in)
    PhysicsSceneDebugInfo* GetDebugInfo(UInt32 flags, const Matrix3D& transform) const;
    // The same for one body (`transform`: its world transform); `parts`: only the elements of
    // these parts (NULL: all). Mac 0xb8ec0, Win 0x48a3e0 (`this` unused)
    PhysicsSceneDebugInfo* GetDebugInfo(const RigidElement* body, UInt32 flags,
                                        const BasicArray<Part*>* parts, const Matrix3D& transform) const;
    // No hinged element. Mac 0xa6350
    bool IsEmpty() const { return m_hingedElements.IsEmpty(); }
    const BasicArray<RefPointer<Brick> >&            GetBricks() const { return m_bricks; }
    const BasicArray<RefPointer<PrimitiveElement> >& GetPrimitiveElements() const { return m_primitiveElements; }
    const BasicArray<RefPointer<HingedElement> >&    GetHingedElements() const { return m_hingedElements; }
    const BasicArray<RefPointer<BrickGroup> >&       GetGroups() const { return m_groups; }
    // (BrickConnectSceneControl camera fit) The collision volumes of every
    // element as points in world space with a radius (spheres: centre, radius + margin; boxes:
    // the 8 corners of the box grown by margin, radius 0; capsules: the two end centres,
    // radius + margin), via GetSceneCVPointsCollisionVolumeVisitor. Mac 0xb7c30 (sub-TU 025o,
    // SceneStorage.cpp), Win 0x484ea0
    void GetSceneCVPoints(BasicArray<Pair<Vector3, Real32> >& points, Real32 margin) const;
    // The same points as spheres (centre, radius), in the same order, in an array of exactly
    // their count (a counting walk first): the camera fits read them as spheres, and the
    // original's point array (grown by half its size: 81 MB for 70,000 bricks) and its sphere
    // copy were both alive. (LDD64)
    void GetSceneCVSpheres(BasicArray<Sphere>& spheres, Real32 margin) const;
    // (SceneStorage.cpp) Element / brick counts of the
    // scene for the debug statistics text (BrickConnectSceneControl Mac 0x38480; the labels are
    // its strings). Mac 0xb7e10, Win 0x484ff0. (names inferred)
    enum StatisticsIndex {
        StatHingedElements = 0,               // "HingedElements"
        StatUnused1 = 1,                      // always 0, not shown
        StatRigidElements = 2,                // "RigidElements"
        StatRigidConnections = 3,             //   ", connections" (each counted from both sides / 2)
        StatPrimitiveRigidElements = 4,       // "PrimitiveRigidElements"
        StatPrimitiveRigidConnections = 5,    //   ", connections" (/ 2)
        StatPrimitiveElements = 6,            // "PrimitiveElements"
        StatPrimitiveConnections = 7,         //   ", connections" (/ 2)
        StatConnectionFields = 8,             // "ConnectionFields"
        StatFieldConnections = 9,             //   ", connections": PrimitiveConnection::Connection count / 2
        StatUniqueBricks = 10,                // "UniqueBricks": distinct brick design ids
        StatUniqueParts = 11,                 // "UniqueParts": distinct part design ids
        StatUniqueBones = 12,                 // "UniqueBones": bones of the distinct part designs (*)
        StatUniqueMaterials = 13,             // "UniqueMaterials"
        StatUniqueDecorations = 14,           // "UniqueDecorations" (non-zero ids)
        StatInstanceBricks = 15,              // "InstanceBricks": distinct Brick objects
        StatInstanceParts = 16,               // "InstanceParts": distinct Part objects
        StatInstanceBones = 17,               // "InstanceBones": primitive elements
        StatInstanceMaterials = 18,           // "InstanceMaterials": own (not redirected) material slots
        StatInstanceDecorations = 19,         // "InstanceDecorations": non-zero decoration slots
        StatCollisionVolumes = 20,            // "Default collision volumes"
        StatActiveCollisionVolumes = 21,      //   ", active"
        StatCount = 22
    };
    // (*) NOTE: counted when a part design is first seen while walking the elements.
    struct Statistics {
        UInt32 m_counts[22];
    };
    void GetStatistics(Statistics& statistics) const;

    // ---- update bracket -------------------------------------------------------------------
    // Nested; the first one tells the listener ('mdsb'). Mac 0x96a40 (inlined everywhere)
    void BeginUpdate();
    // The last one notifies the observers (Mac 0xbde30). Mac 0x96b00
    void EndUpdate();

    // ---- graph bookkeeping (used by the element classes) -----------------------------------
    // m_hingedElements push_back / erase. (inline: Mac 0xa58d0, 0xd1190)
    void AddHingedElement(HingedElement* element);
    void RemoveHingedElement(HingedElement* element);
    // A new body (not referenced) at `transform` with a PhysicsRigidElement, added to the
    // PhysicsScene and to the bodies added in this update. Mac 0xa4880 (025l)
    RigidElement* CreateRigidElement(const Matrix3D& transform);
    // Records a changed body unless it was added in this update. Mac 0x95ad0 (025d)
    void AddChangedRigidElement(RigidElement* element);
    // A body leaves the scene: dropped from the added ones, or moved from the changed to the
    // removed ones. Mac 0xa3230 (025l)
    void RemoveRigidElement(RigidElement* element);
    // Every behavior of the scene's elements. Mac 0x92ed0 / 0x93010 (025a)
    void AddBehavior(Behavior* behavior);
    void RemoveBehavior(Behavior* behavior);
    // Group change lists (+0x158 added, +0x168 parts changed, +0x188 moved, +0x178 removed).
    void AddChangedGroup(BrickGroup* group);                // Mac 0x95970 (025d)
    void AddMovedGroup(BrickGroup* group);                  // Mac 0x96240 (025e)
    void OnGroupAdded(BrickGroup* group);                   // Mac 0xb1cf0
    void OnGroupRemoved(BrickGroup* group);                 // Mac 0xb1f40
    // Observer notifications.
    void NotifyGroupChanges();                              // Mac 0x963a0 (025e)
    void NotifyRigidElementChanges();                       // Mac 0xc19b0
    void NotifyObservers5();                                // Mac 0x398f0 (inline, coalesced in TU 021)

    // ---- structure -------------------------------------------------------------------------
    // Removes everything. Mac 0xa6370, Win 0x47af00
    void Clear();
    // Merges `other` (all its elements, moved by result.m_transform) into this scene and makes
    // result's connections. Mac 0xb13f0 (0xa9030 when `other` is not empty)
    void Merge(Scene* other, const ConnectResult& result);
    void MergeScene(Scene* other, const ConnectResult& result);       // Mac 0xa9030
    // Inserts `brick` at result.m_transform with result's connections. Mac 0xacd70
    void Insert(Brick* brick, const ConnectResult& result);
    // Removes bricks (and their elements). Mac 0xae970
    void Remove(const BasicArray<RefPointer<Brick> >& bricks);
    void RemoveBricks(const BasicArray<RefPointer<Brick> >& bricks, bool notifyPhysics);  // Mac 0xa9b10
    // Inserts many bricks at their elements' transforms (collision test through the
    // PhysicsScene first when `testCollision` and m_testCollisions). Bricks that collide or
    // cannot connect go to `failed`. Mac 0xa7450, Win 0x47f110
    void BatchInsert(const BasicArray<RefPointer<Brick> >& bricks, BasicArray<RefPointer<Brick> >& failed,
                     BatchInsertProgressCallback* progress, bool testCollision,
                     BatchInsertCollisionReporter* reporter);
    // Removes and re-inserts the bricks of the connections of `result`. Mac 0xae540
    void Reconnect(const ConnectResult& result);
    // Connects the field pairs (elements already in the scene) and rebuilds the rigid
    // structure. Mac 0xbe6c0
    void Connect(const BasicArray<PrimitiveConnectionInfo>& connections);
    // Disconnects field connections. Mac 0xaf3a0
    void Disconnect(const BasicArray<PrimitiveConnection*>& connections);
    // After connections were added: primitive rigid / rigid connections and merges.
    // Mac 0xbee00, Win 0x48c010
    void UpdateConnections(const BasicArray<PrimitiveConnection*>& connections,
                           BasicArray<RefPointer<PrimitiveRigidConnection> >& changed,
                           BasicArray<RefPointer<RigidConnection> >& changedRigid);
    // After disconnections: splits the primitive rigid elements, bodies and hinged elements.
    // Mac 0xb0080, Win 0x48df90
    void SplitElements(const BasicArray<PrimitiveRigidElement*>& elements, BasicArray<RigidElement*>& bodies,
                       BasicArray<HingedElement*>& hinged);
    // The bodies a manipulator moves when it drives `connection` with `fixed` held: the joint
    // chain reachable from the free side and the moving bodies with their followers.
    // Mac 0x9d050
    void CollectManipulatedElements(RigidConnection* connection, RigidElement* fixed,
                                    BasicArray<RigidConnection*>& connections,
                                    BasicArray<ActiveRigidElement>& elements);
    // Removes primitive elements (disconnects them). Mac 0xaed90
    void RemoveElements(const BasicArray<RefPointer<PrimitiveElement> >& elements, bool notifyPhysics);
    // Moves elements to new world transforms (remove + reinsert with new connections); false
    // (nothing moved back) on collision or a failed insert. Mac 0xb0940
    bool MoveElements(const BasicArray<Pair<PrimitiveElement*, Matrix3D> >& elements, bool testCollision);
    // MoveElements of the elements whose transform changed; undone on failure. Mac 0xb2aa0 / 0xb2a80
    bool SetTransforms(const BasicArray<Pair<PrimitiveElement*, Matrix3D> >& elements, bool testCollision);
    bool SetTransforms(const BasicArray<Pair<PrimitiveElement*, Matrix3D> >& elements);
    // Moves the parts' elements into `target` at their world transforms (m_bricks of both
    // scenes is not touched). (name inferred) Mac 0xb04b0, Win 0x4802d0
    void MoveParts(const BasicArray<RefPointer<Part> >& parts, Scene* target);
    // Copies bricks (and optionally groups / behaviors) into `target`. Mac 0xa6d30, Win 0x47b480
    void CopyBricks(const BasicArray<RefPointer<Brick> >& bricks, Scene* target, CopyMap& map,
                    bool copyGroups, UInt32 flags);
    // CopyBricks of the parts' bricks. Mac 0xa8cc0
    void CopyParts(const BasicArray<RefPointer<Part> >& parts, Scene* target, CopyMap& map,
                   bool copyGroups, UInt32 flags);

    // ---- queries --------------------------------------------------------------------------
    // The connections `element` would make at `transform` (result.m_transform = transform);
    // false when a field reports a blocking collision. Mac 0xae9e0
    bool CanConnect(PrimitiveElement* element, const Matrix3D& transform, ConnectResult& result);
    // Every element of `brick` at `transform` (relative); physics collision test when
    // m_testCollisions. Mac 0xa9f20
    bool CanConnect(const Brick* brick, const Matrix3D& transform, ConnectResult& result);
    // Every element of `source` moved by `transform`. Mac 0xaacc0
    bool CanConnect(Scene* source, const Matrix3D& transform, ConnectResult& result);
    // Every element of the bodies (each one ignoring itself). Mac 0xadda0
    bool CanConnect(const BasicArray<RigidElement*>& bodies, ConnectResult& result);
    // Walks the scene with a picker. Mac 0xaa850
    void Pick(Picker& picker) const;
    // The same, skipping the elements of the bodies in `skip`. Mac 0xae240
    void Pick(Picker& picker, const BasicArray<RigidElement*>& skip) const;
    void PickRay(const Vector3& origin, const Vector3& direction, PickResult* result) const;     // Mac 0xb2bf0
    void PickFrustum(const Frustum& frustum, PickResult* result) const;     // Mac 0xb2c60, Win 0x482270
    // Field connections of `element` at `transform` with the `candidates`; false on a
    // blocking (CONNECTION_BLOCKED) pair. Mac 0xaaa60
    bool FindConnections(PrimitiveElement* element, const Matrix3D& transform,
                         const BasicArray<Pair<PrimitiveElement*, Matrix3D> >& candidates,
                         BasicArray<PrimitiveConnectionInfo>& connections) const;
    // All parts. Mac 0xb2d20
    void GetParts(BasicArray<RefPointer<Part> >& parts) const;
    // World transforms: every element (the array is cleared first), Mac 0xb24e0; appended for
    // `elements`, Mac 0xb26a0; appended for the elements of `entries` (their current
    // transforms), Mac 0xb2950.
    void GetTransforms(BasicArray<Pair<PrimitiveElement*, Matrix3D> >& transforms) const;
    static void GetTransforms(const BasicArray<PrimitiveElement*>& elements,
                              BasicArray<Pair<PrimitiveElement*, Matrix3D> >& transforms);
    static void GetTransforms(const BasicArray<Pair<PrimitiveElement*, Matrix3D> >& entries,
                              BasicArray<Pair<PrimitiveElement*, Matrix3D> >& transforms);
    // Entries whose transform differs from the element's (and the current transforms).
    // Mac 0xb27b0
    static void GetChangedTransforms(const BasicArray<Pair<PrimitiveElement*, Matrix3D> >& transforms,
                                     BasicArray<Pair<PrimitiveElement*, Matrix3D> >& changed,
                                     BasicArray<Pair<PrimitiveElement*, Matrix3D> >* current);
    // Parts plus (optionally) the other parts of their bricks / groups / wires. Mac 0xb30e0, Win 0x4824b0
    static void ExpandParts(const BasicArray<RefPointer<Part> >& parts, BasicArray<RefPointer<Part> >& result,
                            bool bricks, bool groups, bool wires);
    // Mac 0xb30c0
    static bool IsWire(UInt32 designID);
    // Appends the primitive elements of parts. Mac 0xb35c0
    static void GetElements(const BasicArray<RefPointer<Part> >& parts,
                            BasicArray<RefPointer<PrimitiveElement> >& elements);
    // Bricks of parts (unique). Mac 0xa8df0
    static void GetBricks(const BasicArray<RefPointer<Part> >& parts, BasicArray<RefPointer<Brick> >& bricks);

    // ---- groups ---------------------------------------------------------------------------
    // A new group (BrickConnect::CreateBrickGroup) with `parts`, below `parent` or top level.
    // Mac 0xa8ea0, Win 0x47bae0 (?)
    BrickGroup* CreateGroup(BrickGroup* parent, const BasicArray<RefPointer<Part> >& parts);
    // Inserts an existing group. Mac 0xb2260
    void AddGroup(BrickGroup* group, BrickGroup* parent, const BasicArray<Part*>* parts,
                  const BasicArray<BrickGroup*>* groups);
    // Mac 0xa6b10
    void RemoveGroup(BrickGroup* group);
    // parts[i] -> groups[i]. Mac 0xb2410
    void SetGroups(const BasicArray<Part*>& parts, const BasicArray<BrickGroup*>& groups);

    // ---- drag snapping (ClickStick tools) --------------------------------------------------
    // `source` (its elements at their transforms relative to the dragged transform) becomes
    // the drag scene; the snap loop starts at `first` (only that element when `single`).
    // Mac 0xab0e0
    void BeginDrag(Scene* source, PrimitiveElement* first, bool single);
    void EndDrag();                                                                   // Mac 0xab280
    // CanConnect of the drag scene at `transform`. Mac 0xab340
    bool CanConnectDrag(const Matrix3D& transform, ConnectResult& result);
    // The same with the transform caches (misses / hits, Matrix3DLess). Mac 0xaba40
    bool CanConnectDragCached(const Matrix3D& transform, ConnectResult& result);
    // Snaps the drag scene near `transform`: the drag elements' fields against the fields of
    // the scene elements near them (ConnectionField::CanConnect within `maxDistance`), else
    // CanConnectDragCached(transform). NOTE: `snapped` is only cleared. Mac 0xab370
    bool SnapDrag(const Matrix3D& transform, Real32 maxDistance, ConnectResult& result, bool& snapped);
    // Ray snapping of the drag scene (ClickStickCastUtility, Mac TU 376: the
    // pick ray `origin` + t * `direction`, the drag scene at `transform`; `maxDistance` is the
    // squared snap distance that ConnectionField::CanConnect (ray overload, slot 17) accepts).
    //  1. The connections of the last result (m_dragResult, starting with those of the drag
    //     start element) whose drag field is an axle / slider field are tried again along the
    //     ray; a connecting snap returns true at once.
    //  2. For each drag element (from m_dragStart; only that one when m_dragSingle) the scene
    //     elements inside the cone from `origin` around the element's bounding sphere
    //     (HideAwareConePicker, nearest first; only the `candidates` of this scene when that
    //     array is not empty) are tried field by field; the first scene element that gives a
    //     connecting placement wins (the nearest snap of that element). A placement without
    //     connections that is closer (centre of the drag scene's box) to the plane through
    //     `origin` normal to `direction` than the ones before is remembered instead: when no
    //     connecting placement is found, result.m_transform becomes the best of them and
    //     `snapped` is set (the return value stays false).
    // On success result (and m_dragResult) hold the placement. `snapped` is cleared first.
    // Mac 0xabd10 (Win 0x47d060); without candidates Mac 0xabc90 (Win 0x47cfd0). (names inferred)
    bool SnapDragRay(const BasicArray<RefPointer<PrimitiveElement> >& candidates, const Vector3& origin,
                     const Vector3& direction, const Matrix3D& transform, Real32 maxDistance,
                     ConnectResult& result, bool& snapped);
    bool SnapDragRay(const Vector3& origin, const Vector3& direction, const Matrix3D& transform,
                     Real32 maxDistance, ConnectResult& result, bool& snapped);

    // ---- solvers (names inferred) ---------------------------------------------
    // HingeAlignTool (Mac 0x159fa0): moves the bodies of the two features together with
    // PhysicsScene::SolveMoveTogether (slot 19). For an axle feature (FieldType 2) the solver
    // gets the axis (+y of the field point frame) and position of both features in their body
    // space, the second position moved along its axis into the plane of the first one; for a
    // planar feature (0) the same without that correction; any other field type passes zero
    // vectors. The solver's collision callback is a CanConnectPhysicsCollisionCallbackHelper,
    // the features are the physics features of both points (planar: the knob feature of a fixed
    // point, axle: the whole element when the field's kind is odd). The context
    // (MoveTogetherSolverContext, a PhysicsSolverCallback) records the transforms when the
    // solver starts and moves the changed elements on its result. Returns the solver (one
    // reference for the caller, which the HingeAlignTool releases; NULL when it could not
    // start).
    // Mac 0xb1410 (helper 0xb1910), Win 0x480a06 (helper 0x480610)
    PhysicsSolver* MoveTogether(const Feature& feature, const Feature& otherFeature);
    // MoveTogetherSolverContext::OnResult: the elements whose transform changed since
    // `transforms` was recorded are moved (MoveElements without collision test) to where the
    // solver put them; if that fails they go back and `success` is cleared. Mac 0xc1e70
    void ApplySolvedTransforms(const BasicArray<Pair<PrimitiveElement*, Matrix3D> >& transforms, bool& success);
    // A new solver callback (one reference for the caller) registered with the PhysicsScene
    // (AddSolverCallback, slot 20; the returned handle is released at once). Used by the
    // StringPullTool / ClickPullTool. Mac 0xb1b90 / 0xb1bf0, Win 0x480dd0 / 0x480e50
    StringPullSolverCallback* CreateStringPullSolverCallback();
    ClickPullSolverCallback*  CreateClickPullSolverCallback();

    // ---- storage (SceneStorage.cpp) --------------------------------------------------------
    // Builds the scene from `storage` (Mac 0xb3690, Win 0x482e30). Returns false when the
    // callback cancelled. Bricks whose design is unknown go to `missing`, colliding /
    // unconnectable ones to `collided`.
    bool Load(const StorageScene2& storage, StorageMap& map,
              BasicArray<RefPointer<StorageScene2::Brick> >& collided,
              BasicArray<RefPointer<StorageScene2::Brick> >& missing, BrickDatabase* database,
              LoadCallback* callback, BatchInsertCollisionReporter* reporter);
    // Applies the behaviors of the storage objects to the loaded ones. Mac 0xa5a70
    static void ApplyBehaviors(StorageMap& map);
    // One storage group (recursively). Mac 0xb5800, Win 0x482b60
    void LoadGroup(StorageMap& map, const StorageScene2::Group& group, BrickGroup* parent);
    // Appends the scene to `storage` (rigid systems, joints, bricks, groups, behaviors).
    // Mac 0xb5ae0, Win 0x483ed0
    void Save(StorageScene2& storage, SaveMap& map) const;
    // One group (recursively). Mac 0xb78a0
    static void SaveGroup(SaveMap& map, const BrickGroup* group, BasicArray<RefPointer<StorageScene2::Group> >& groups);
    // Writes the scene with highlighted elements to "<app data>/LEGO Company/<name>.lxfml".
    // Mac 0xb49c0
    void DumpSnapshot(const BasicMap<PrimitiveElement*, UInt32>& colors,
                      const BasicMap<RefPointer<PrimitiveElement>, Pair<UInt32, Matrix3D> >& extra,
                      const String& name) const;

    // ---- data (Mac offsets) -----------------------------------------------------------------
    SceneListener*                            m_listener;              // +0x38 one reference
    RefPointer<BrickConnect>                  m_brickConnect;          // +0x3c
    BrickPhysics*                             m_physics;               // +0x40 one reference
    PhysicsScene*                             m_physicsScene;          // +0x44 one reference
    BasicArray<RefPointer<HingedElement> >    m_hingedElements;        // +0x48
    BasicArray<RefPointer<Brick> >            m_bricks;                // +0x58
    BasicArray<RefPointer<PrimitiveElement> > m_primitiveElements;     // +0x68
    BasicArray<RefPointer<BrickGroup> >       m_groups;                // +0x78 top level groups
    RefPointer<Scene>                         m_dragScene;             // +0x88
    BasicSet<Matrix3D, Matrix3DLess>          m_dragMisses;            // +0x8c transforms without connection
    BasicMap<Matrix3D, BasicArray<PrimitiveConnectionInfo>, Matrix3DLess> m_dragHits;  // +0xa8
    UInt32                                    m_dragStart;             // +0xc4 element to try first
    bool                                      m_dragSingle;            // +0xc8 only that element
    ConnectResult                             m_dragResult;            // +0xcc
    ConnectionManipulator*                    m_manipulator;           // +0x11c (not referenced)
    bool                                      m_flag120;               // +0x120 report bodies of planar connections as changed
    bool                                      m_flag121;               // +0x121 (used by Part)
    bool                                      m_flag122;               // +0x122 (used by Part)
    bool                                      m_flag123;               // +0x123 (used by Part)
    bool                                      m_flag124;               // +0x124 (used by Brick)
    bool                                      m_flag125;               // +0x125 (used by Brick)
    BasicArray<RigidElement*>                 m_addedRigidElements;    // +0x128
    BasicArray<RigidElement*>                 m_changedRigidElements;  // +0x138
    BasicArray<RigidElement*>                 m_removedRigidElements;  // +0x148
    BasicArray<RefPointer<BrickGroup> >       m_addedGroups;           // +0x158
    BasicArray<RefPointer<BrickGroup> >       m_changedGroups;         // +0x168
    BasicArray<RefPointer<BrickGroup> >       m_removedGroups;         // +0x178
    BasicArray<RefPointer<BrickGroup> >       m_movedGroups;           // +0x188
    UInt32                                    m_updateDepth;           // +0x198
    bool                                      m_structureChanged;      // +0x19c
    BasicArray<RefPointer<Behavior> >         m_allBehaviors;          // +0x1a0
    bool                                      m_testCollisions;        // +0x1b0 true
    // (added, performance) Pick's spatial index (Scene.cpp, "Spatial index"); built on the first
    // Pick.
    mutable SceneIndex*                       m_index;

public:
    // Tests: with checking on, every Pick also runs the original walk and compares what the two
    // visit (elements, transforms and spheres bit for bit, in order); the number of differences.
    static void SetIndexChecking(bool check);
    static UInt32 GetIndexMismatches();

protected:
    // --- slot 7: detaches the observers and clears the scene. Mac 0xbc310 (thunks 0xbc3f0,
    // 0xbc470), Win 0x4a1b00
    virtual void OnFinalRelease();

private:
    // Inserts one element without connections: own primitive rigid element, body and hinged
    // element. Mac 0xacfc0
    void InsertElement(PrimitiveElement* element, const Matrix3D& transform);
    // Inserts one element with the connections of `result`. Mac 0xaebe0
    void InsertElement(PrimitiveElement* element, const ConnectResult& result);
    // New primitive rigid element (not referenced) holding `element`. Mac 0xbf380
    PrimitiveRigidElement* CreatePrimitiveRigidElement(PrimitiveElement* element, const Matrix3D& transform);
    // New body for `element` in `hinged`. Mac 0xbf710
    RigidElement* CreateRigidElement(PrimitiveRigidElement* element, const Matrix3D& transform,
                                     HingedElement* hinged);
    // Rigid connections for changed primitive rigid connections. Mac 0xbf500
    void UpdateRigidConnections(BasicArray<RefPointer<PrimitiveRigidConnection> >& changed,
                                BasicArray<RefPointer<RigidConnection> >& changedRigid);
    // Until stable: merges bodies of jointless / looped rigid connections. Mac 0xbf940, Win 0x48e1e0
    void ResolveRigidConnections(BasicArray<RefPointer<RigidConnection> >& changed);
    // Merges the bodies of a closed loop of three joints that cannot move. Mac 0xc1300
    void CheckJointLoop(RigidConnection* connection, BasicArray<RefPointer<RigidConnection> >& changed);
    // CopyBricks helpers. Mac 0xa7050, 0xa86c0, 0xa8b50
    static Brick* CloneBrick(BrickConnect* brickConnect, const Brick* brick, CopyMap& map);
    void CopyGroups(CopyMap& map);
    void CopyBehaviors(CopyMap& map, UInt32 flags, Scene* source);
    // Merge helper: the groups (recursively) move to this scene. Mac 0xad110
    void TakeGroups(const BasicArray<RefPointer<BrickGroup> >& groups);
    // Ends the active manipulation (manipulator slot 6) — inlined everywhere.
    void DetachManipulator();

    Scene(const Scene&);
    Scene& operator=(const Scene&);
};

} // namespace LEGO
