// LEGO Digital Designer — restored source
// LEGO::Scene (see Scene.h): construction, the update bracket and observer notifications, the
// graph bookkeeping, insertion / removal / connection / disconnection, CanConnect and picking,
// BatchInsert, groups, copying and merging scenes, drag snapping.
//
// Original object file: Scene.cpp (Mac TU 025): sub-TU 025n (Mac 0xa5a70-0xb49c0, Win
// ~0x47adf0-0x482a40) and 025q (Mac 0xbac90-0xc2860, Win 0x486e10-0x490a10), the Scene methods the
// linker placed in 025a/d/e/l (0x92ed0, 0x93010, 0x95970, 0x95ad0, 0x96240, 0x963a0, 0x96a40,
// 0x96b00, 0xa3230, 0xa4880), the callbacks / pickers emitted in the tail 025s, and the profiling
// timers of the TU initialiser (Mac 0xd66f0). Scene <-> StorageScene2 is in SceneStorage.cpp
// (025o). Every name is inferred unless noted.
#include "Scene.h"
#include "Brick.h"
#include "Part.h"
#include "BrickGroup.h"
#include "BrickConnect.h"
#include "Behavior.h"
#include "HingedElement.h"
#include "RigidElement.h"
#include "RigidConnection.h"
#include "PrimitiveRigidElement.h"
#include "PrimitiveRigidConnection.h"
#include "PrimitiveConnection.h"
#include "PrimitiveElement.h"
#include "ConnectionManipulator.h"
#include "ConnectivityWorldLookupTables.h"
#include "SceneGraphUtil.h"
#include "LEGO/String.h"
#include "Trace.h"
#include "ConnectionFieldPlanar.h"
#include "StringPullSolverCallback.h"
#include "ClickPullSolverCallback.h"
#include "ProfileTimer.h"

#include <algorithm>
#include <map>
#include <typeinfo>
#include <vector>
#if defined(LDD_SCENE_INDEX_CHECK_ENV)
#include <stdio.h>
#include <stdlib.h>
#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif
#endif
#include <math.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <libkern/OSAtomic.h>
#endif

namespace LEGO {

// =============================================================================================
// Profiling timers of the TU initialiser (Mac 0xd66f0; reported at exit). Started / stopped
// explicitly where the original does: cancelled paths return without Stop.
// =============================================================================================
static ProfileTimer s_canConnectTimer(String("Scene::CanConnect"), true);                        // Mac 0x77d1e0
static ProfileTimer s_insertTimer(String("Scene::Insert"), true);                                // Mac 0x77d210
static ProfileTimer s_removeTimer(String("Scene::Remove(Brick/Bricks)"), true);                  // Mac 0x77d240
static ProfileTimer s_batchCollisionTimer(String("Scene::BatchInsert test collision"), true);    // Mac 0x77d270
static ProfileTimer s_batchStructureTimer(String("Scene::BatchInsert update structure"), true);  // Mac 0x77d2a0
static ProfileTimer s_batchPhysicsTimer(String("Scene::BatchInsert build physics"), true);       // Mac 0x77d2d0
static ProfileTimer s_clearTimer(String("Scene::Clear"), true);                                  // Mac 0x77d300

namespace {

// ---------------------------------------------------------------------------------------------
// Small inline helpers of the original
// ---------------------------------------------------------------------------------------------

// The world transform of a primitive element's bone frame in a body (body transform * rigid
// transform * element transform) — the walk of Mac 0xaa850 & co.
inline Sphere BoneSphere(const PrimitiveElement* element, const Matrix3D& transform)
{
    Sphere sphere(element->m_boundingSphere);
    sphere.m_center *= transform;
    return sphere;
}

// Disconnects a primitive connection from both elements (inlined in Clear 0xa6370,
// RemoveElements 0xaed90, Disconnect 0xaf3a0).
void DetachPrimitiveConnection(PrimitiveConnection* connection)
{
    PrimitiveElement* element = connection->m_element;
    PrimitiveElement* other = connection->m_otherElement;
    element->DisconnectFields(connection);
    if (connection->m_rigidConnection != 0) {
        connection->m_rigidConnection->RemoveConnection(connection);
        connection->m_rigidConnection = 0;
    }
    connection->m_otherElement = 0;
    connection->m_element = 0;
    // NOTE: the second erase may release the last reference to the connection.
    element->m_connections.Erase(other);
    other->m_connections.Erase(element);
}

// The design IDs whose single stud connections are kept rigid (Mac 0xbe6c0: 3005 and the
// zero-terminated table at Mac 0x6300b4).
const UInt32 kSingleStudDesigns[] = {
    3005, 2453, 3062, 4589, 4070, 47905, 2921, 4733, 6541, 50746, 3070, 3024, 6141, 4740,
    3960, 2555, 4085, 6019, 4081, 49668, 4592, 33125, 71842, 30374, 3900, 3957, 33051, 4095,
    3741, 40379, 2488, 40232, 6255, 3626, 3624, 3833, 3901, 4530, 6093, 53982, 4588, 6211,
    70750, 6231, 4599, 30162, 4349, 42446, 61252, 0
};

template <class T>
void HoldAll(const BasicArray<T*>& from, BasicArray<RefPointer<T> >& to)
{
    for (typename BasicArray<T*>::ConstIterator it = from.Begin(); it != from.End(); ++it)
        SceneGraph::PushBackUnique(to, RefPointer<T>(*it));
}
} // namespace

// =============================================================================================
// Spatial index of Pick (performance; not in the originals). See "Spatial index" at the end.
// =============================================================================================
class SceneIndex {
public:
    SceneIndex();
    ~SceneIndex();
    // Scene::Pick: what the original walk visits (hinged elements, bodies, primitive rigid
    // elements, primitive elements; Test on each bounding sphere, Visit on the elements that pass).
    void Pick(const BasicArray<RefPointer<HingedElement> >& hinged, Scene::Picker& picker,
              const BasicArray<RigidElement*>* skip);
    // A body joins the scene (Scene::CreateRigidElement, Scene::AddHingedElement).
    void Add(RigidElement* body);
    // SceneIndexTouch / a reported transform change: the body's cache is out of date.
    static void Changed(RigidElement* body);
    // ~RigidElement: the body's cache goes.
    static void Forget(RigidElement* body);
    // LDD64 (Scene::SnapDragRay's cone picks): a Pick-like walk that lists the bodies `bound`
    // accepts instead of visiting them, then a picker over those bodies alone. False: a body cache
    // went since.
    void CollectBodies(const BasicArray<RefPointer<HingedElement> >& hinged, Scene::Picker& bound,
                       std::vector<const void*>& bodies, UInt32& releases);
    bool PickBodies(const std::vector<const void*>& bodies, UInt32 releases, Scene::Picker& picker);
    // The elements of `bodies` (CollectBodies, `releases` its count) by their direction from
    // `apex`, for cone picks with that apex (PickCones); false: not usable (walk instead).
    bool BuildCones(const std::vector<const void*>& bodies, UInt32 releases, const Vector3& apex);
    bool PickCones(Scene::ConePicker& picker);
    void DropCones();

private:
    struct ElementCache {
        ElementCache() : element(0), localSphere(0.0f), sphere(0.0f) {}
        PrimitiveElement* element;
        Matrix3D          local;         // element->m_transform it was computed from
        Sphere            localSphere;   // element->m_boundingSphere
        Matrix3D          world;         // the walk's `transform`
        Sphere            sphere;        // the walk's bone sphere
    };
    // An element's place in the element grid (apart from ElementCache: walks read those).
    struct ElementCells {
        ElementCells() : state(0) {}
        UInt32 state;      // 0 not in the grid, 1 in its cells, 2 oversize
        Int32  cells[6];   // cell range while in the cells
    };
    struct BodyCache;
    struct RigidCache {
        RigidCache() : rigid(0), owner(0), localSphere(0.0f), sphere(0.0f), index(0), stamp(0), gridded(false) {}
        PrimitiveRigidElement*    rigid;
        BodyCache*                owner;         // the body's cache
        Matrix3D                  local;         // rigid->m_transform
        Sphere                    localSphere;   // rigid->m_boundingSphere
        Matrix3D                  world;         // the walk's `rigidTransform`
        Sphere                    sphere;        // the walk's `rigidSphere`
        UInt32                    index;         // position in the body's m_elements
        UInt32                    stamp;         // marks of a query
        bool                      gridded;       // in the cells (else in the oversize list)
        Int32                     cells[6];      // cell range while gridded: min x y z, max x y z
        std::vector<ElementCache> elements;      // in the order of rigid->m_elements
        std::vector<ElementCells> gridCells;     // the same order: places in the element grid
    };
    typedef std::map<UInt64, std::vector<RigidCache*> > CellMap;
    struct BodyCache {
        BodyCache() : body(0), slot(0), version(0), count(0), data(0), localSphere(0.0f), sphere(0.0f),
                      fresh(true), reports(false), dirty(false), grid(false), cellSize(1.0f), seen(0), stamp(0),
                      hingedPos(0), bodyPos(0), unreported(~0u), topState(0), syncs(0) {}
        RigidElement*            body;
        UInt32                   slot;
        UInt32                   version;       // body->m_indexVersion it was built at
        UInt32                   count;         // body->m_elements size and storage then
        const void*              data;
        Sphere                   localSphere;   // body->m_boundingSphere
        Matrix3D                 transform;     // the body transform
        Sphere                   sphere;        // the walk's `bodySphere`
        bool                     fresh;         // not built yet
        bool                     reports;       // the physics body reports transform changes
        bool                     dirty;         // queued in m_dirty
        std::vector<RigidCache*> rigids;        // in the order of body->m_elements
        bool                     grid;          // cells in use (bodies with many rigid elements)
        Real32                   cellSize;
        CellMap                  cells;
        std::vector<RigidCache*> oversize;      // spheres too large (or not finite) for the cells
        UInt32                   seen;          // walk stamp
        UInt32                   stamp;         // marks of a query
        UInt32                   hingedPos;     // last known walk position
        UInt32                   bodyPos;
        UInt32                   unreported;    // position in m_unreported (~0: reports)
        UInt32                   topState;      // 0 not in the body grid, 1 in its cells, 2 oversize
        Int32                    topCells[6];
        UInt32                   syncs;         // Sync count (stale element grid entries)
        // while topState 1: per cell of topCells (in their order) the body's place in the cell's list
        std::vector<UInt32>      topSlots;
    };
    // A primitive element in the element grid: rigid->elements[element] of body `rigid->owner`.
    struct ElementRef {
        RigidCache* rigid;
        UInt32      element;
    };
    // The element grid: an open-addressing table of cells, each a list of pooled 16-byte nodes (a
    // map of vectors cost 88 MB for 70,000 bricks). Order within a cell does not matter:
    // PickElements sorts (LessRef).
    class ElementGrid {
    public:
        ElementGrid() : m_used(0), m_dead(0), m_freeNode(kNone), m_nodeCount(0) {}
        ~ElementGrid();
        void Add(UInt64 key, const ElementRef& ref);
        void Remove(UInt64 key, const ElementRef& ref);     // the first node of `ref` in the cell
        void Append(UInt64 key, std::vector<ElementRef>& out) const;

    private:
        enum { kBlockBits = 16 };
        static const UInt32 kNone = 0xffffffffu;
        struct Node {
            RigidCache* rigid;
            UInt32      element;
            UInt32      next;
        };
        struct Cell {
            UInt64 key;   // kEmpty, kDead or a CellKey
            UInt32 head;
        };
        static UInt64 Empty() { return ~(UInt64)0; }
        static UInt64 Dead() { return ~(UInt64)0 - 1; }
        Node& NodeAt(UInt32 i) const { return m_blocks[i >> kBlockBits][i & ((1u << kBlockBits) - 1)]; }
        size_t Find(UInt64 key) const;   // the cell's slot, or m_cells.size()
        void Rehash(size_t capacity);

        std::vector<Cell>  m_cells;      // power-of-two size
        size_t             m_used;       // live cells
        size_t             m_dead;       // emptied cells (probe chains go on through them)
        std::vector<Node*> m_blocks;
        UInt32             m_freeNode;
        UInt32             m_nodeCount;  // nodes ever taken from the blocks
    };
    typedef std::map<UInt64, std::vector<BodyCache*> > BodyCellMap;
    struct Slot {
        BodyCache* cache;
        UInt32     generation;
    };

    static SceneIndex* Owner(const RigidElement* body, UInt32* slot);
    BodyCache* Find(RigidElement* body);
    void Prepare();
    void Update(BodyCache& cache);
    void Walk(const BasicArray<RefPointer<HingedElement> >& hinged, Scene::Picker& picker,
              const BasicArray<RigidElement*>* skip, const Sphere* query,
              std::vector<const void*>* collect = 0);
    bool PickSphere(const BasicArray<RefPointer<HingedElement> >& hinged, Scene::Picker& picker,
                    const BasicArray<RigidElement*>* skip, const Sphere& query);
    void VisitBody(BodyCache& cache, Scene::Picker& picker, const Sphere* query);
    void Sync(BodyCache& cache, const Matrix3D& transform);
    void Compute(RigidCache& rigid, const Matrix3D& bodyTransform, bool reuse);
    bool Unchanged(const RigidCache& rigid) const;
    void BuildGrid(BodyCache& cache);
    void AddToGrid(BodyCache& cache, RigidCache* rigid);
    void RemoveFromGrid(BodyCache& cache, RigidCache* rigid);
    void AddToBodyGrid(BodyCache* cache);
    void RemoveFromBodyGrid(BodyCache* cache);
    void AddToElementGrid(RigidCache* rigid, UInt32 element);
    void RemoveFromElementGrid(RigidCache* rigid, UInt32 element);
    void RemoveFromElementGrid(RigidCache* rigid);
    bool PickElements(const BasicArray<RefPointer<HingedElement> >& hinged, Scene::Picker& picker,
                      const BasicArray<RigidElement*>* skip, const Sphere& query);
    void Validate(const BasicArray<RefPointer<HingedElement> >& hinged, std::vector<BodyCache*>& candidates);
    static bool LessRef(const ElementRef& a, const ElementRef& b)
    {
        if (a.rigid->owner != b.rigid->owner)
            return LessPosition(a.rigid->owner, b.rigid->owner);
        if (a.rigid != b.rigid)
            return a.rigid->index < b.rigid->index;
        return a.element < b.element;
    }
    void Sweep();
    void Release(UInt32 slot);
    static bool LessIndex(const RigidCache* a, const RigidCache* b) { return a->index < b->index; }
    static bool LessPosition(const BodyCache* a, const BodyCache* b)
    {
        return a->hingedPos != b->hingedPos ? a->hingedPos < b->hingedPos : a->bodyPos < b->bodyPos;
    }

    std::vector<Slot>        m_slots;
    std::vector<UInt32>      m_free;
    UInt32                   m_live;
    UInt32                   m_walk;
    UInt32                   m_stamp;
    UInt32                   m_releases;    // Release calls (PickBodies)
    // BuildCones: an element of a body with its unit direction from the apex
    struct ConeEntry {
        BodyCache*    body;
        RigidCache*   rigid;
        ElementCache* element;
        UInt32        bodyIndex;    // position in m_coneBodies
        UInt32        rigidIndex;   // number of the rigid element (m_coneRigidStamp)
    };
    std::vector<ConeEntry>   m_coneEntries;     // in the order the walk visits them
    std::vector<UInt32>      m_coneNear;        // entries near the apex (candidates of every cone)
    std::vector<UInt64>      m_coneKeys;        // sorted cell keys ...
    std::vector<UInt32>      m_coneStarts;      // ... their first places in m_coneCellEntries
    std::vector<UInt32>      m_coneCellEntries; // entries by cell, in entry order within a cell
    std::vector<UInt32>      m_coneEntryStamp;
    std::vector<UInt32>      m_coneBodyStamp;   // per body: stamp * 2 + passed
    std::vector<UInt32>      m_coneRigidStamp;  // per rigid element: stamp * 2 + passed
    std::vector<UInt32>      m_coneCandidates;
    Vector3                  m_coneApex;
    Real32                   m_coneCell;        // cell size of the unit vectors
    UInt32                   m_coneReleases;
    UInt32                   m_coneStampValue;
    bool                     m_coneValid;
    bool                     m_complete;    // every body of the scene has a cache (after a walk)
    std::vector<UInt32>      m_dirty;       // slots of out-of-date caches
    std::vector<UInt32>      m_unreported;  // slots of bodies compared on every Pick
    BodyCellMap              m_bodyCells;   // bodies by their spheres
    std::vector<BodyCache*>  m_bodyOversize;
    std::vector<RigidCache*> m_candidates;
    std::vector<BodyCache*>  m_bodyCandidates;
    ElementGrid              m_elementCells;    // primitive elements by their spheres
    std::vector<ElementRef>  m_elementOversize;
    std::vector<ElementRef>  m_elementCandidates;
    // PickElements' sort: a candidate with its LessRef order as two numbers (body position;
    // rigid element, element), compared without the loads of LessRef
    struct KeyedRef {
        UInt64     position;
        UInt64     element;
        ElementRef ref;
    };
    static bool LessKey(const KeyedRef& a, const KeyedRef& b)
    {
        return a.position != b.position ? a.position < b.position : a.element < b.element;
    }
    std::vector<KeyedRef>    m_keyedCandidates;
};

// =============================================================================================
// Construction
// =============================================================================================

// Mac 0xbac90 (025q), Win 0x48a5a0
Scene::Scene(BrickConnect* owner, BrickPhysics* physics, PhysicsScene* physicsScene)
    : m_listener(0), m_brickConnect(owner), m_physics(physics), m_physicsScene(physicsScene),
      m_dragStart(0), m_dragSingle(false), m_manipulator(0),
      m_flag120(false), m_flag121(false), m_flag122(false), m_flag123(false), m_flag124(false),
      m_flag125(false), m_updateDepth(0), m_structureChanged(false), m_testCollisions(true)
{
    m_index = 0;
    m_physics->AddRef();
    m_physicsScene->AddRef();
}

// Mac 0xbc060 / 0xbc1a0 (body 0xbb570, thunks 0xbc0c0 0xbc130 0xbc210 0xbc290), Win 0x48a8e0
Scene::~Scene()
{
    m_physicsScene->Release();
    m_physics->Release();
    m_allBehaviors.Clear();
    m_movedGroups.Clear();
    m_removedGroups.Clear();
    m_changedGroups.Clear();
    m_addedGroups.Clear();
    m_dragScene = 0;
    m_groups.Clear();
    m_primitiveElements.Clear();
    m_bricks.Clear();
    m_hingedElements.Clear();
    m_brickConnect = 0;
    if (m_listener != 0)
        m_listener->Release();
    delete m_index;
}

// Mac 0xbc310 (thunks 0xbc3f0, 0xbc470), Win 0x4a1b00
void Scene::OnFinalRelease()
{
    NotifierT<Scene, SceneObserver>::OnFinalRelease();
    Clear();
    BehaviorElement::OnFinalRelease();
}

// Inline accessor (Mac: the +0x3c load)
BrickConnect* Scene::GetBrickConnect() const
{
    return m_brickConnect;
}

// Mac 0xa62a0, Win 0x47adf0
void Scene::GetBoundingBox(BoundingBox& box, bool recompute) const
{
    if (m_hingedElements.IsEmpty()) {
        box = BoundingBox(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }
    box.Reset();
    BoundingBox element(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    for (BasicArray<RefPointer<HingedElement> >::ConstIterator it = m_hingedElements.Begin();
         it != m_hingedElements.End(); ++it) {
        (*it)->GetBoundingBox(element, recompute);
        box.Union(element);
    }
}

// Mac 0xb1c50, Win 0x480ed0
UInt32 Scene::PhysicsCall5(UInt32 argument)
{
    if (m_physicsScene->IsSimulating() == 0)
        return 3;
    UInt32 result = m_physicsScene->Simulate5(argument);
    NotifyObservers5();
    return result;
}

// Mac 0xb1ca0, Win 0x480f10
UInt32 Scene::PhysicsCall6(bool argument)
{
    if (m_physicsScene->IsSimulating() == 0)
        return 0;
    UInt32 result = m_physicsScene->Simulate6(argument);
    NotifyObservers5();
    return result;
}

// =============================================================================================
// Update bracket and notifications
// =============================================================================================

// Mac 0x96a40 (025e; inlined everywhere)
void Scene::BeginUpdate()
{
    if (m_updateDepth == 0 && m_listener != 0)
        m_listener->OnSceneEvent(SceneListener::CodeBeginUpdate);
    ++m_updateDepth;
}

// Mac 0x96b00 (025e; inlined everywhere); the notification part is Mac 0xbde30
void Scene::EndUpdate()
{
    if (--m_updateDepth != 0)
        return;
    if (m_structureChanged) {
        m_structureChanged = false;
        Iterator iterator(*this);
        while (SceneObserver* observer = iterator.Next(m_observers))
            observer->OnStructureChanged();
    }
    if (!m_addedRigidElements.IsEmpty() || !m_changedRigidElements.IsEmpty() ||
        !m_removedRigidElements.IsEmpty()) {
        {
            Iterator iterator(*this);
            while (SceneObserver* observer = iterator.Next(m_observers))
                observer->OnRigidElementsChanged(m_addedRigidElements, m_changedRigidElements,
                                                 m_removedRigidElements);
        }
        m_addedRigidElements.Clear();
        m_changedRigidElements.Clear();
        m_removedRigidElements.Clear();
    }
    NotifyGroupChanges();
}

// Mac 0x963a0 (025e)
void Scene::NotifyGroupChanges()
{
    if (m_updateDepth != 0)
        return;
    if (m_addedGroups.IsEmpty() && m_changedGroups.IsEmpty() && m_movedGroups.IsEmpty() &&
        m_removedGroups.IsEmpty())
        return;
    {
        Iterator iterator(*this);
        while (SceneObserver* observer = iterator.Next(m_observers))
            observer->OnGroupsChanged(m_addedGroups, m_changedGroups, m_movedGroups, m_removedGroups);
    }
    m_addedGroups.Clear();
    m_changedGroups.Clear();
    m_movedGroups.Clear();
    m_removedGroups.Clear();
}

// Mac 0xc19b0
void Scene::NotifyRigidElementChanges()
{
    {
        Iterator iterator(*this);
        while (SceneObserver* observer = iterator.Next(m_observers))
            observer->OnRigidElementsChanged(m_addedRigidElements, m_changedRigidElements,
                                             m_removedRigidElements);
    }
    m_addedRigidElements.Clear();
    m_changedRigidElements.Clear();
    m_removedRigidElements.Clear();
}

// Mac 0x398f0 (inline, coalesced into TU 021), Win 0x4701e0
void Scene::NotifyObservers5()
{
    Iterator iterator(*this);
    while (SceneObserver* observer = iterator.Next(m_observers))
        observer->OnSceneNotification5();
}

// Inlined before every structural change: the active manipulation ends.
void Scene::DetachManipulator()
{
    if (m_manipulator != 0)
        m_manipulator->Detach();
    m_manipulator = 0;
}

// =============================================================================================
// Bookkeeping used by the element classes
// =============================================================================================

// inline (Mac 0xa58d0 push_back)
void Scene::AddHingedElement(HingedElement* element)
{
    m_hingedElements.PushBack(RefPointer<HingedElement>(element));
    if (m_index != 0)
        for (BasicArray<RefPointer<RigidElement> >::ConstIterator it = element->m_elements.Begin();
             it != element->m_elements.End(); ++it)
            m_index->Add(*it);
}

// inline (Mac 0xd1190 erase; Win inlined in HingedElement::Merge 0x47a2e1)
void Scene::RemoveHingedElement(HingedElement* element)
{
    SceneGraph::EraseValueScene(m_hingedElements, element);   // (Win: unordered)
}

// Mac 0xa4880 (025l)
RigidElement* Scene::CreateRigidElement(const Matrix3D& transform)
{
    RigidElement* element = new RigidElement;
    m_addedRigidElements.PushBack(element);
    element->m_body = m_physics->CreateRigidElement();
    element->m_body->SetUserData(element);
    m_physicsScene->AddRigidElement(element->m_body, transform);
    if (m_index != 0)
        m_index->Add(element);
    return element;
}

// Mac 0x95ad0 (025d)
void Scene::AddChangedRigidElement(RigidElement* element)
{
    SceneIndexTouch(element);
    if (!SceneGraph::Contains(m_addedRigidElements, element))
        SceneGraph::PushBackUnique(m_changedRigidElements, element);
}

// Mac 0xa3230 (025l)
void Scene::RemoveRigidElement(RigidElement* element)
{
    SceneIndexTouch(element);
    BasicArray<RigidElement*>::Iterator it = SceneGraph::Find(m_addedRigidElements, element);
    if (it != m_addedRigidElements.End()) {
        m_addedRigidElements.Erase((UInt32)(it - m_addedRigidElements.Begin()));
        return;
    }
    // every occurrence (std::remove, Mac 0xc1c70)
    UInt32 i = 0;
    while (i < m_changedRigidElements.GetSize()) {
        if (m_changedRigidElements[i] == element)
            m_changedRigidElements.Erase(i);
        else
            ++i;
    }
    SceneGraph::PushBackUnique(m_removedRigidElements, element);
}

// Mac 0x92ed0 (025a)
void Scene::AddBehavior(Behavior* behavior)
{
    RefPointer<Behavior> hold(behavior);
    if (!SceneGraph::Contains(m_allBehaviors, behavior))
        m_allBehaviors.PushBack(hold);
}

// Mac 0x93010 (025a)
void Scene::RemoveBehavior(Behavior* behavior)
{
    RefPointer<Behavior> hold(behavior);
    // NOTE: the original erases at the find result without checking for end().
    SceneGraph::EraseValue(m_allBehaviors, behavior);
}

// Mac 0x95970 (025d)
void Scene::AddChangedGroup(BrickGroup* group)
{
    RefPointer<BrickGroup> hold(group);
    if (!SceneGraph::Contains(m_addedGroups, group))
        SceneGraph::PushBackUnique(m_changedGroups, hold);
}

// Mac 0x96240 (025e)
void Scene::AddMovedGroup(BrickGroup* group)
{
    RefPointer<BrickGroup> hold(group);
    if (!SceneGraph::Contains(m_addedGroups, group))
        SceneGraph::PushBackUnique(m_movedGroups, hold);
}

// Mac 0xb1cf0
void Scene::OnGroupAdded(BrickGroup* group)
{
    m_addedGroups.PushBack(RefPointer<BrickGroup>(group));
    SceneGraph::EraseValue(m_changedGroups, group);
    SceneGraph::EraseValue(m_movedGroups, group);
}

// Mac 0xb1f40
void Scene::OnGroupRemoved(BrickGroup* group)
{
    RefPointer<BrickGroup> hold(group);
    SceneGraph::EraseValue(m_addedGroups, group);
    SceneGraph::EraseValue(m_changedGroups, group);
    SceneGraph::EraseValue(m_movedGroups, group);
    SceneGraph::PushBackUnique(m_removedGroups, hold);
}

// =============================================================================================
// Clearing
// =============================================================================================

// Mac 0xa6370, Win 0x47af00
void Scene::Clear()
{
    s_clearTimer.Start();
    BeginUpdate();
    DetachManipulator();
    m_physicsScene->Unknown6();
    while (!m_groups.IsEmpty())
        RemoveGroup(m_groups[0]);

    for (BasicArray<RefPointer<HingedElement> >::Iterator h = m_hingedElements.Begin();
         h != m_hingedElements.End(); ++h) {
        HingedElement* hinged = *h;
        for (BasicArray<RefPointer<RigidElement> >::Iterator b = hinged->m_elements.Begin();
             b != hinged->m_elements.End(); ++b) {
            RigidElement* body = *b;
            Matrix3D bodyTransform;
            body->m_body->GetTransform(bodyTransform);
            SceneIndexTouch(body);
            RemoveRigidElement(body);
            while (!body->m_connections.IsEmpty())
                body->m_connections.Begin()->second->Clear();
            for (BasicArray<RefPointer<PrimitiveRigidElement> >::Iterator r = body->m_elements.Begin();
                 r != body->m_elements.End(); ++r) {
                PrimitiveRigidElement* rigid = *r;
                Matrix3D rigidTransform(bodyTransform, rigid->m_transform);
                while (!rigid->m_connections.IsEmpty())
                    rigid->m_connections.Begin()->second->Clear();
                for (BasicArray<RefPointer<PrimitiveElement> >::Iterator e = rigid->m_elements.Begin();
                     e != rigid->m_elements.End(); ++e) {
                    PrimitiveElement* element = *e;
                    element->m_transform = Matrix3D(rigidTransform, element->m_transform);
                    body->m_body->RemovePrimitiveElement(element->m_physics);
                    while (!element->m_connections.IsEmpty())
                        DetachPrimitiveConnection(element->m_connections.Begin()->second);
                    element->m_rigid = 0;
                }
                rigid->m_elements.Clear();
            }
            m_physicsScene->RemoveRigidElement(body->m_body);
            body->m_elements.Clear();
        }
        hinged->m_elements.Clear();
    }
    m_hingedElements.Clear();
    m_primitiveElements.Clear();
    m_bricks.Clear();
    m_behaviors.Clear();
    m_allBehaviors.Clear();
    m_physicsScene->Unknown7();
    m_structureChanged = true;
    EndUpdate();
    s_clearTimer.Stop();
}

// =============================================================================================
// Groups
// =============================================================================================

// Mac 0xa6b10
void Scene::RemoveGroup(BrickGroup* group)
{
    BeginUpdate();
    BrickGroup* parent = group->m_parent;
    while (!group->m_parts.IsEmpty())
        group->m_parts[0]->SetGroup(parent);
    while (!group->m_groups.IsEmpty())
        group->m_groups[0]->SetParent(parent);
    group->SetParent(0);
    OnGroupRemoved(group);
    {
        RefPointer<BrickGroup> hold(group);
        // NOTE: the original erases at the find result without checking for end().
        SceneGraph::EraseValue(m_groups, group);
        group->m_scene = 0;
    }
    for (UInt32 i = 0; i < group->m_behaviors.GetSize(); ++i)
        RemoveBehavior(group->m_behaviors[i]);
    EndUpdate();
}

// Mac 0xa8ea0
BrickGroup* Scene::CreateGroup(BrickGroup* parent, const BasicArray<RefPointer<Part> >& parts)
{
    BeginUpdate();
    BrickGroup* group = m_brickConnect->CreateBrickGroup(this);
    if (parent == 0)
        m_groups.PushBack(RefPointer<BrickGroup>(group));
    else
        group->SetParent(parent);
    for (BasicArray<RefPointer<Part> >::ConstIterator it = parts.Begin(); it != parts.End(); ++it)
        (*it)->SetGroup(group);
    OnGroupAdded(group);
    EndUpdate();
    return group;
}

// Mac 0xb2260
void Scene::AddGroup(BrickGroup* group, BrickGroup* parent, const BasicArray<Part*>* parts,
                     const BasicArray<BrickGroup*>* groups)
{
    BeginUpdate();
    group->m_scene = this;
    if (parts != 0)
        for (BasicArray<Part*>::ConstIterator it = parts->Begin(); it != parts->End(); ++it)
            (*it)->SetGroup(group);
    if (groups != 0)
        for (BasicArray<BrickGroup*>::ConstIterator it = groups->Begin(); it != groups->End(); ++it)
            (*it)->SetParent(group);
    if (parent == 0)
        m_groups.PushBack(RefPointer<BrickGroup>(group));
    else
        group->SetParent(parent);
    OnGroupAdded(group);
    for (UInt32 i = 0; i < group->m_behaviors.GetSize(); ++i)
        AddBehavior(group->m_behaviors[i]);
    EndUpdate();
}

// Mac 0xb2410
void Scene::SetGroups(const BasicArray<Part*>& parts, const BasicArray<BrickGroup*>& groups)
{
    BeginUpdate();
    for (UInt32 i = 0; i < parts.GetSize(); ++i)
        parts[i]->SetGroup(groups[i]);
    EndUpdate();
}

// Mac 0xad110 (recursive)
void Scene::TakeGroups(const BasicArray<RefPointer<BrickGroup> >& groups)
{
    for (BasicArray<RefPointer<BrickGroup> >::ConstIterator it = groups.Begin(); it != groups.End(); ++it) {
        BrickGroup* group = *it;
        if (group->m_parent == 0)
            m_groups.PushBack(*it);
        if (group->m_scene != 0)
            group->m_scene->OnGroupRemoved(group);
        group->m_scene = this;
        OnGroupAdded(group);
        TakeGroups(group->m_groups);
    }
}

// =============================================================================================
// Picking
// =============================================================================================

// Mac 0xaa850
// LDD64: the walk goes through the spatial index ("Spatial index" at the end): the same
// elements, transforms and spheres in the same order, without recomputing transforms.
void Scene::Pick(Picker& picker) const
{
    if (m_index == 0)
        m_index = new SceneIndex;
    m_index->Pick(m_hingedElements, picker, 0);
}

// Mac 0xae240
void Scene::Pick(Picker& picker, const BasicArray<RigidElement*>& skip) const
{
    if (m_index == 0)
        m_index = new SceneIndex;
    m_index->Pick(m_hingedElements, picker, &skip);
}

// Mac 0xb2bf0
void Scene::PickRay(const Vector3& origin, const Vector3& direction, PickResult* result) const
{
    HideAwareRayPicker picker(origin, direction, result);
    Pick(picker);
}

// Mac 0xb2c60, Win 0x482270
void Scene::PickFrustum(const Frustum& frustum, PickResult* result) const
{
    HideAwareFrustumPicker picker(frustum, result);
    Pick(picker);
}

// ---- pickers (the Visit / Test bodies of 025n / 025s) --------------------------------------

namespace {
inline void AddPicked(Scene::PickResult* result, PrimitiveElement* element, const Vector3& reference,
                      const Sphere& sphere)
{
    result->Insert(Scene::PrimitiveElementWithDistance(element, reference.GetDistanceSquared(sphere.m_center),
                                                       sphere));
}
} // namespace

// Mac 0xc4040
bool Scene::FrustumPicker::Test(const Sphere& sphere)
{
    m_classification = m_frustum.Classify(sphere);
    return m_classification != Frustum::OUTSIDE;
}

// Mac 0xa5b30: a partly contained bone counts when all 8 corners of its box are inside.
void Scene::FrustumPicker::Visit(PrimitiveElement* element, const Matrix3D& transform, const Sphere& sphere)
{
    if (m_classification == Frustum::INTERSECTING) {
        const BoundingBox& box = element->m_bone->m_bounding;
        // NOTE: the corner (min x, max y, max z) is never tested; the max corner twice.
        Vector3 corners[8] = {
            box.m_min * transform,
            Vector3(box.m_min.x, box.m_min.y, box.m_max.z) * transform,
            Vector3(box.m_max.x, box.m_min.y, box.m_max.z) * transform,
            Vector3(box.m_max.x, box.m_min.y, box.m_min.z) * transform,
            Vector3(box.m_min.x, box.m_max.y, box.m_min.z) * transform,
            Vector3(box.m_max.x, box.m_max.y, box.m_max.z) * transform,
            Vector3(box.m_max.x, box.m_max.y, box.m_min.z) * transform,
            box.m_max * transform
        };
        UInt32 inside = 0;
        for (UInt32 p = 0; p < 4; ++p) {
            UInt32 positive = 0;
            for (UInt32 c = 0; c < 8; ++c)
                if (0.0f < m_frustum.m_planes[p].GetDistance(corners[c]))
                    ++positive;
            if (positive == 8)
                ++inside;
            else if (positive == 0)
                break;
        }
        if (inside == 4)
            AddPicked(m_result, element, m_frustum.m_apex, sphere);
    } else if (m_classification == Frustum::INSIDE) {
        AddPicked(m_result, element, m_frustum.m_apex, sphere);
    }
}

// Mac 0xc8930
void Scene::HideAwareFrustumPicker::Visit(PrimitiveElement* element, const Matrix3D& transform, const Sphere& sphere)
{
    if (element->m_flagAC)
        return;
    FrustumPicker::Visit(element, transform, sphere);
}

// Mac 0xc8830, Win 0x46ff00
bool Scene::RayPicker::Test(const Sphere& sphere)
{
    return PointRayDistanceSquared(m_origin, m_direction, sphere.m_center) < sphere.m_radius * sphere.m_radius;
}

// Mac 0xc88d0
void Scene::RayPicker::Visit(PrimitiveElement* element, const Matrix3D&, const Sphere& sphere)
{
    AddPicked(m_result, element, m_origin, sphere);
}

// Mac 0xc8870, Win 0x46fe50
void Scene::HideAwareRayPicker::Visit(PrimitiveElement* element, const Matrix3D& transform, const Sphere& sphere)
{
    if (!element->m_flagAC)
        RayPicker::Visit(element, transform, sphere);
}

// Mac 0xc7c90
bool Scene::ConePicker::Test(const Sphere& sphere)
{
    return m_cone.Intersects(sphere);
}

// Mac 0xc7d10
void Scene::ConePicker::Visit(PrimitiveElement* element, const Matrix3D&, const Sphere& sphere)
{
    AddPicked(m_result, element, m_cone.m_apex, sphere);
}

// Mac 0xc7cb0
void Scene::HideAwareConePicker::Visit(PrimitiveElement* element, const Matrix3D& transform, const Sphere& sphere)
{
    if (!element->m_flagAC)
        ConePicker::Visit(element, transform, sphere);
}

// Mac 0xc7ab0
bool Scene::SpherePicker::Test(const Sphere& sphere)
{
    return m_sphere.Intersects(sphere);
}

// Mac 0xc7ad0
void Scene::PrimitiveSpherePicker::Visit(PrimitiveElement* element, const Matrix3D& transform, const Sphere&)
{
    m_result->PushBack(Pair<PrimitiveElement*, Matrix3D>(element, transform));
}

// Mac 0xc7c30
void Scene::HideAwareOrderedElementsSpherePicker::Visit(PrimitiveElement* element, const Matrix3D&,
                                                        const Sphere& sphere)
{
    if (!element->m_flagAC)
        AddPicked(m_result, element, m_sphere.m_center, sphere);
}

// Mac 0xc4070, Win 0x47aca0
bool Scene::HideAwareSamePlaceFeaturesPicker::Test(const Sphere& sphere)
{
    return PointRayDistanceSquared(m_origin, m_direction, sphere.m_center) < sphere.m_radius * sphere.m_radius;
}

// Mac 0xa5fc0, Win 0x614413
void Scene::HideAwareSamePlaceFeaturesPicker::Visit(PrimitiveElement* element, const Matrix3D& transform,
                                                    const Sphere&)
{
    if (element->m_flagAC)
        return;
    for (BasicArray<RefPointer<ConnectionField> >::ConstIterator it = element->m_fields.Begin();
         it != element->m_fields.End(); ++it) {
        Matrix3D fieldTransform(transform, (*it)->GetTransform());
        Matrix3D offset;
        Real32 distance;
        UInt32 index;
        if (!(*it)->Pick(fieldTransform, m_origin, m_direction, m_distance, distance, index, offset))
            continue;
        Real32 difference = m_distance - distance;
        if (0.0f < difference && 0.0001f < (difference < 0.0f ? -difference : difference))
            m_result->Clear();
        m_distance = distance;
        Feature feature;
        feature.m_element = element;
        feature.m_field = *it;
        feature.m_index = index;
        feature.m_offset = offset;
        m_result->PushBack(feature);
    }
}

// =============================================================================================
// Physics collision callbacks (RTTI names; classes of this object file)
// =============================================================================================

// RTTI: LEGO::CanConnectPhysicsCollisionCallbackHelper : PhysicsCollisionCallback.
// Mac vtable 0x76249c {0xc82b0}, Win 0xa23f48 {0x47bef0}.
class CanConnectPhysicsCollisionCallbackHelper : public PhysicsCollisionCallback {
public:
    // Mac 0xc82b0, Win 0x47bef0: contacts inside one part are ignored, contacts of a knob
    // that connects are allowed.
    virtual UInt32 OnCollision(const PhysicsGeometryIdentifier& feature,
                               const PhysicsGeometryIdentifier& otherFeature)
    {
        PrimitiveElement* element = feature.m_element->GetUserData();
        PrimitiveElement* other = otherFeature.m_element->GetUserData();
        if (element->m_part == other->m_part)
            return Ignore;
        return IsConnectingFeature(element, feature.m_feature, other, otherFeature.m_feature) ? Connection : Collision;
    }

    // A knob feature of one element that some free planar field of the other element can
    // connect at its point. Mac 0xc8310, Win 0x47bf40
    static bool IsConnectingFeature(PrimitiveElement* element, UInt32 feature, PrimitiveElement* other,
                                    UInt32 otherFeature)
    {
        const Primitive::CollisionVolumeToAndFromFeatureLookup* lookup = &element->m_bone->m_featureLookup;
        Int32 index = (Int32)(feature - lookup->m_count);
        if (index < 0) {
            lookup = &other->m_bone->m_featureLookup;
            index = (Int32)(otherFeature - lookup->m_count);
            if (index < 0)
                return false;
            PrimitiveElement* swap = element;
            element = other;
            other = swap;
        }
        ConnectionField* field = element->m_fields[lookup->m_features[index].m_volume];
        if (field == 0)
            return false;
        UInt32 point = lookup->m_features[index].m_feature;
        Matrix3D transform;
        Matrix3D otherTransform;
        element->GetTransform(transform);
        other->GetTransform(otherTransform);
        Matrix3D fieldTransform(transform, field->GetTransform());
        for (BasicArray<RefPointer<ConnectionField> >::ConstIterator it = other->m_fields.Begin();
             it != other->m_fields.End(); ++it) {
            ConnectionField* candidate = *it;
            if ((candidate->GetFlags() & 1) != 0 || candidate->GetType() != ConnectionField::TYPE_PLANAR)
                continue;
            Matrix3D candidateTransform(otherTransform, candidate->GetTransform());
            ConnectionField::ConnectionIndices indices;
            UInt32 result = field->CanConnect(fieldTransform, *candidate, candidateTransform, indices);
            if ((result & ConnectionField::CONNECTION_SNAP) == 0) {
                if (result == ConnectionField::CONNECTION_BLOCKED)
                    return false;
                continue;
            }
            for (ConnectionField::ConnectionIndices::ConstIterator i = indices.Begin(); i != indices.End(); ++i)
                if (i->m_index == point)
                    return true;
        }
        return false;
    }
};

// RTTI: LEGO::CanConnectPhysicsCollisionCallback : PhysicsCollisionCallback.
// Mac vtable 0x762404 {0xc7b10}, Win 0xa23f40 {0x47bdf0}. Contacts of knobs that take part
// in one of the found connections are allowed.
class CanConnectPhysicsCollisionCallback : public PhysicsCollisionCallback {
public:
    explicit CanConnectPhysicsCollisionCallback(const BasicArray<PrimitiveConnectionInfo>* connections)
        : m_connections(connections) {}

    // Mac 0xc7b10, Win 0x47bdf0
    virtual UInt32 OnCollision(const PhysicsGeometryIdentifier& feature,
                               const PhysicsGeometryIdentifier& otherFeature)
    {
        PrimitiveElement* element = feature.m_element->GetUserData();
        PrimitiveElement* other = otherFeature.m_element->GetUserData();
        if (element->m_part == other->m_part)
            return Ignore;
        const Primitive::CollisionVolumeToAndFromFeatureLookup* lookup = &element->m_bone->m_featureLookup;
        Int32 index = (Int32)(feature.m_feature - lookup->m_count);
        const ConnectionField* field;
        UInt32 point;
        if (index < 0) {
            lookup = &other->m_bone->m_featureLookup;
            index = (Int32)(otherFeature.m_feature - lookup->m_count);
            if (index < 0)
                return Collision;
            field = other->m_fields[lookup->m_features[index].m_volume];
        } else {
            field = element->m_fields[lookup->m_features[index].m_volume];
        }
        point = lookup->m_features[index].m_feature;
        for (BasicArray<PrimitiveConnectionInfo>::ConstIterator it = m_connections->Begin();
             it != m_connections->End(); ++it) {
            if (it->m_field == field) {
                if (it->m_otherElement == other)
                    for (ConnectionField::ConnectionIndices::ConstIterator i = it->m_indices.Begin();
                         i != it->m_indices.End(); ++i)
                        if (i->m_index == point)
                            return Connection;
            } else if (it->m_otherField == field && it->m_element == element) {
                for (ConnectionField::ConnectionIndices::ConstIterator i = it->m_indices.Begin();
                     i != it->m_indices.End(); ++i)
                    if (i->m_otherIndex == point)
                        return Connection;
            }
        }
        return Collision;
    }

    const BasicArray<PrimitiveConnectionInfo>* m_connections;   // +0x04
};

// RTTI: LEGO::BatchInsertPhysicsCollisionCallback : CanConnectPhysicsCollisionCallbackHelper.
// Mac vtable 0x762474 {0xc7d70}, Win 0xa23d0c {0x47ed50}. Records the bricks that collide (the
// one not yet in the scene, or the second one) and reports every contact.
class BatchInsertPhysicsCollisionCallback : public CanConnectPhysicsCollisionCallbackHelper {
public:
    explicit BatchInsertPhysicsCollisionCallback(BatchInsertCollisionReporter* reporter) : m_reporter(reporter) {}

    // Mac 0xc7d70, Win 0x47ed50
    virtual UInt32 OnCollision(const PhysicsGeometryIdentifier& feature,
                               const PhysicsGeometryIdentifier& otherFeature)
    {
        PrimitiveElement* element = feature.m_element->GetUserData();
        PrimitiveElement* other = otherFeature.m_element->GetUserData();
        Brick* brick = element->m_part->m_brick;
        Brick* otherBrick = other->m_part->m_brick;
        if (SceneGraph::Contains(m_collided, brick)) {
            if (m_reporter == 0)
                return Ignore;
            if (!SceneGraph::Contains(m_collided, otherBrick))
                m_reporter->OnCollision(other, otherFeature.m_feature, element, feature.m_feature);
            else
                m_reporter->OnCollisionReversed(element, feature.m_feature, other, otherFeature.m_feature);
            return Ignore;
        }
        if (SceneGraph::Contains(m_collided, otherBrick)) {
            if (m_reporter != 0)
                m_reporter->OnCollision(element, feature.m_feature, other, otherFeature.m_feature);
            return Ignore;
        }
        if (element->m_part == other->m_part)
            return Ignore;
        if (IsConnectingFeature(element, feature.m_feature, other, otherFeature.m_feature))
            return Connection;
        if (element->m_rigid == 0) {
            SceneGraph::PushBackUnique(m_collided, RefPointer<Brick>(brick));
            if (m_reporter != 0)
                m_reporter->OnCollision(other, otherFeature.m_feature, element, feature.m_feature);
        } else {
            SceneGraph::PushBackUnique(m_collided, RefPointer<Brick>(otherBrick));
            if (m_reporter != 0)
                m_reporter->OnCollision(element, feature.m_feature, other, otherFeature.m_feature);
        }
        return Ignore;
    }

    BasicArray<RefPointer<Brick> > m_collided;   // +0x04
    BatchInsertCollisionReporter*  m_reporter;   // +0x14
};

// RTTI: LEGO::BatchInsertPhysicsProgressCallback : PhysicsProgressCallback.
// Mac vtable 0x7624a8 {0xc8560}, Win 0xa23f60 {0x47f090}: 0-33 % of the batch.
class BatchInsertPhysicsProgressCallback : public PhysicsProgressCallback {
public:
    explicit BatchInsertPhysicsProgressCallback(Scene::BatchInsertProgressCallback* progress)
        : m_continue(true), m_progress(progress) {}
    // Mac 0xc8560, Win 0x47f090
    // (returns m_continue, the original's al; was void)
    virtual bool OnProgress(UInt32 done, UInt32 total)
    {
        m_continue = m_progress->OnProgress((UInt32)(Int64)(((Real32)done / (Real32)total) * 33.0f));
        return m_continue;
    }
    bool                                m_continue;   // +0x04
    Scene::BatchInsertProgressCallback* m_progress;   // +0x08
};

// =============================================================================================
// CanConnect
// =============================================================================================

// Mac 0xaaa60
bool Scene::FindConnections(PrimitiveElement* element, const Matrix3D& transform,
                            const BasicArray<Pair<PrimitiveElement*, Matrix3D> >& candidates,
                            BasicArray<PrimitiveConnectionInfo>& connections) const
{
    connections.Reserve(0x400);
    for (BasicArray<RefPointer<ConnectionField> >::ConstIterator f = element->m_fields.Begin();
         f != element->m_fields.End(); ++f) {
        ConnectionField* field = *f;
        Matrix3D fieldTransform(transform, field->GetTransform());
        for (BasicArray<Pair<PrimitiveElement*, Matrix3D> >::ConstIterator c = candidates.Begin();
             c != candidates.End(); ++c) {
            PrimitiveElement* other = c->first;
            if (element->m_connections.Contains(other))
                continue;
            for (BasicArray<RefPointer<ConnectionField> >::ConstIterator g = other->m_fields.Begin();
                 g != other->m_fields.End(); ++g) {
                ConnectionField* otherField = *g;
                Matrix3D otherTransform(c->second, otherField->GetTransform());
                PrimitiveConnectionInfo info;
                UInt32 result = field->CanConnect(fieldTransform, *otherField, otherTransform, info.m_indices);
                if ((result & ConnectionField::CONNECTION_SNAP) == 0) {
                    if (result == ConnectionField::CONNECTION_BLOCKED)
                        return false;
                    continue;
                }
                info.m_element = element;
                info.m_otherElement = other;
                info.m_field = field;
                info.m_otherField = otherField;
                connections.PushBack(info);
            }
        }
    }
    return true;
}

// Mac 0xae9e0
bool Scene::CanConnect(PrimitiveElement* element, const Matrix3D& transform, ConnectResult& result)
{
    s_canConnectTimer.Start();
    result.m_transform = transform;
    result.m_connections.Clear();
    Sphere sphere(element->m_boundingSphere);
    sphere.m_center *= transform;
    BasicArray<Pair<PrimitiveElement*, Matrix3D> > candidates;
    PrimitiveSpherePicker picker(sphere, &candidates);
    Pick(picker);
    bool found = FindConnections(element, transform, candidates, result.m_connections);
    s_canConnectTimer.Stop();
    if (!found) {
        result.m_transform.SetIdentity();
        result.m_connections.Clear();
        return false;
    }
    return true;
}

// Mac 0xa9f20
bool Scene::CanConnect(const Brick* brick, const Matrix3D& transform, ConnectResult& result)
{
    result.m_transform = transform;
    result.m_connections.Clear();
    BasicArray<Matrix3D> placed;
    BasicArray<PhysicsPrimitiveElement*> physics;
    Matrix3D elementTransform;
    Matrix3D local;
    for (BasicArray<RefPointer<Part> >::ConstIterator p = brick->m_parts.Begin(); p != brick->m_parts.End(); ++p) {
        for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = (*p)->m_elements.Begin();
             e != (*p)->m_elements.End(); ++e) {
            PrimitiveElement* element = *e;
            element->GetTransform(local);
            elementTransform.Multiply(transform, local);
            Sphere sphere(element->m_boundingSphere);
            sphere.m_center *= elementTransform;
            BasicArray<Pair<PrimitiveElement*, Matrix3D> > candidates;
            PrimitiveSpherePicker picker(sphere, &candidates);
            Pick(picker);
            if (!FindConnections(element, elementTransform, candidates, result.m_connections)) {
                result.m_transform.SetIdentity();
                result.m_connections.Clear();
                return false;
            }
            if (m_testCollisions) {
                physics.PushBack(element->m_physics);
                placed.PushBack(elementTransform);
            }
        }
    }
    if (m_testCollisions) {
        BasicArray<const Matrix3D*> transforms;
        for (UInt32 i = 0; i < placed.GetSize(); ++i)
            transforms.PushBack(&placed[i]);
        CanConnectPhysicsCollisionCallback callback(&result.m_connections);
        if (!m_physicsScene->TestCollisions(physics.GetSize(), physics.Begin(), transforms.Begin(), &callback, 0)) {
            result.m_transform.SetIdentity();
            result.m_connections.Clear();
            return false;
        }
    }
    brick->GetInternalConnections(result.m_connections);
    return true;
}

// Mac 0xaacc0
bool Scene::CanConnect(Scene* source, const Matrix3D& transform, ConnectResult& result)
{
    result.m_transform = transform;
    result.m_connections.Clear();
    result.m_connections.Reserve(0x400);
    for (BasicArray<RefPointer<HingedElement> >::ConstIterator h = source->m_hingedElements.Begin();
         h != source->m_hingedElements.End(); ++h) {
        for (BasicArray<RefPointer<RigidElement> >::ConstIterator b = (*h)->m_elements.Begin();
             b != (*h)->m_elements.End(); ++b) {
            Matrix3D bodyTransform;
            (*b)->m_body->GetTransform(bodyTransform);
            Matrix3D moved(transform, bodyTransform);
            for (BasicArray<RefPointer<PrimitiveRigidElement> >::ConstIterator r = (*b)->m_elements.Begin();
                 r != (*b)->m_elements.End(); ++r) {
                Matrix3D rigidTransform(moved, (*r)->m_transform);
                for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = (*r)->m_elements.Begin();
                     e != (*r)->m_elements.End(); ++e) {
                    PrimitiveElement* element = *e;
                    Matrix3D elementTransform(rigidTransform, element->m_transform);
                    Sphere sphere(element->m_boundingSphere);
                    sphere.m_center *= elementTransform;
                    BasicArray<Pair<PrimitiveElement*, Matrix3D> > candidates;
                    PrimitiveSpherePicker picker(sphere, &candidates);
                    Pick(picker);
                    if (!FindConnections(element, elementTransform, candidates, result.m_connections)) {
                        result.m_transform.SetIdentity();
                        result.m_connections.Clear();
                        return false;
                    }
                }
            }
        }
    }
    if (m_testCollisions) {
        CanConnectPhysicsCollisionCallback callback(&result.m_connections);
        bool free = m_dragScene.Get() == source
                        ? m_physicsScene->TestDragCollision(transform, &callback)
                        : m_physicsScene->TestCollision(source->m_physicsScene, transform, &callback);
        if (!free) {
            result.m_transform.SetIdentity();
            result.m_connections.Clear();
            return false;
        }
    }
    return true;
}

// Mac 0xadda0
bool Scene::CanConnect(const BasicArray<RigidElement*>& bodies, ConnectResult& result)
{
    BasicArray<RigidElement*> skip;
    skip.PushBack(0);
    BasicArray<PrimitiveConnectionInfo> connections;
    for (BasicArray<RigidElement*>::ConstIterator b = bodies.Begin(); b != bodies.End(); ++b) {
        RigidElement* body = *b;
        Matrix3D bodyTransform;
        body->m_body->GetTransform(bodyTransform);
        skip[0] = body;
        for (BasicArray<RefPointer<PrimitiveRigidElement> >::ConstIterator r = body->m_elements.Begin();
             r != body->m_elements.End(); ++r) {
            Matrix3D rigidTransform(bodyTransform, (*r)->m_transform);
            for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = (*r)->m_elements.Begin();
                 e != (*r)->m_elements.End(); ++e) {
                PrimitiveElement* element = *e;
                Matrix3D elementTransform(rigidTransform, element->m_transform);
                Sphere sphere(element->m_boundingSphere);
                sphere.m_center *= elementTransform;
                BasicArray<Pair<PrimitiveElement*, Matrix3D> > candidates;
                PrimitiveSpherePicker picker(sphere, &candidates);
                Pick(picker, skip);
                if (!FindConnections(element, elementTransform, candidates, connections))
                    return false;
            }
        }
    }
    result.m_transform = Matrix3D();
    result.m_connections.Swap(connections);
    return true;
}

// =============================================================================================
// Inserting and connecting
// =============================================================================================

// Mac 0xbf380
PrimitiveRigidElement* Scene::CreatePrimitiveRigidElement(PrimitiveElement* element, const Matrix3D& transform)
{
    PrimitiveRigidElement* rigid = new PrimitiveRigidElement(transform);
    element->m_transform = Matrix3D();
    element->m_rigid = rigid;
    rigid->m_elements.PushBack(RefPointer<PrimitiveElement>(element));
    rigid->m_bounding.Reset();
    for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator it = rigid->m_elements.Begin();
         it != rigid->m_elements.End(); ++it)
        rigid->m_bounding.Union((*it)->m_primitive->GetBone((*it)->m_boneIndex).m_bounding * (*it)->m_transform);
    rigid->m_boundingSphere.Set(rigid->m_bounding);
    return rigid;
}

// Mac 0xbf710
RigidElement* Scene::CreateRigidElement(PrimitiveRigidElement* element, const Matrix3D& transform,
                                        HingedElement* hinged)
{
    RigidElement* body = CreateRigidElement(transform);
    element->m_transform = Matrix3D();
    element->m_rigidElement = body;
    body->m_elements.PushBack(RefPointer<PrimitiveRigidElement>(element));
    body->m_bounding.Reset();
    for (BasicArray<RefPointer<PrimitiveRigidElement> >::ConstIterator it = body->m_elements.Begin();
         it != body->m_elements.End(); ++it)
        body->m_bounding.Union((*it)->m_bounding * (*it)->m_transform);
    body->m_boundingSphere.Set(body->m_bounding);
    body->m_hinged = hinged;
    SceneIndexTouch(body);
    hinged->m_elements.PushBack(RefPointer<RigidElement>(body));
    for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator it = element->m_elements.Begin();
         it != element->m_elements.End(); ++it)
        body->m_body->AddPrimitiveElement((*it)->m_physics, Matrix3D(element->m_transform, (*it)->m_transform));
    return body;
}

// Mac 0xacfc0
void Scene::InsertElement(PrimitiveElement* element, const Matrix3D& transform)
{
    PrimitiveRigidElement* rigid = CreatePrimitiveRigidElement(element, transform);
    RefPointer<HingedElement> hinged = new HingedElement(this);
    AddHingedElement(hinged);
    CreateRigidElement(rigid, rigid->m_transform, hinged);
}

// Mac 0xaebe0
void Scene::InsertElement(PrimitiveElement* element, const ConnectResult& result)
{
    if (result.m_connections.IsEmpty()) {
        InsertElement(element, result.m_transform);
    } else {
        element->m_transform = result.m_transform;
        Connect(result.m_connections);
    }
    m_primitiveElements.PushBack(RefPointer<PrimitiveElement>(element));
    for (UInt32 i = 0; i < element->m_behaviors.GetSize(); ++i)
        AddBehavior(element->m_behaviors[i]);
    Part* part = element->m_part;
    if (part != 0) {
        for (UInt32 i = 0; i < part->m_behaviors.GetSize(); ++i)
            AddBehavior(part->m_behaviors[i]);
        Brick* brick = part->m_brick;
        if (brick != 0)
            for (UInt32 i = 0; i < brick->m_behaviors.GetSize(); ++i)
                AddBehavior(brick->m_behaviors[i]);
    }
}

// Mac 0xacd70
void Scene::Insert(Brick* brick, const ConnectResult& result)
{
    s_insertTimer.Start();
    if (m_manipulator != 0)
        m_manipulator->Detach();
    BeginUpdate();
    DetachManipulator();
    m_physicsScene->Unknown6();
    for (BasicArray<RefPointer<Part> >::ConstIterator p = brick->m_parts.Begin(); p != brick->m_parts.End(); ++p) {
        for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = (*p)->m_elements.Begin();
             e != (*p)->m_elements.End(); ++e) {
            PrimitiveElement* element = *e;
            m_primitiveElements.PushBack(RefPointer<PrimitiveElement>(element));
            InsertElement(element, Matrix3D(result.m_transform, element->m_transform));
        }
    }
    m_bricks.PushBack(RefPointer<Brick>(brick));
    Connect(result.m_connections);
    m_physicsScene->Unknown7();
    m_structureChanged = true;
    EndUpdate();
    s_insertTimer.Stop();
}

// Mac 0xbe6c0
void Scene::Connect(const BasicArray<PrimitiveConnectionInfo>& connections)
{
    BasicArray<PrimitiveConnection*> changed;
    BasicArray<RefPointer<PrimitiveRigidConnection> > changedRigid;
    BasicArray<RefPointer<RigidConnection> > changedBodies;
    for (BasicArray<PrimitiveConnectionInfo>::ConstIterator it = connections.Begin(); it != connections.End(); ++it) {
        PrimitiveElement* element = it->m_element;
        PrimitiveElement* other = it->m_otherElement;
        PrimitiveRigidElement* rigid = element->m_rigid;
        PrimitiveRigidElement* otherRigid = other->m_rigid;
        PrimitiveConnection* connection = 0;
        bool jointChanged = element->Connect(*it, other, connection);
        // single stud parts: their planar connections stay rigid
        if (it->m_field->GetType() == ConnectionField::TYPE_PLANAR) {
            UInt32 designID = element->m_primitive->GetDesignID();
            UInt32 otherDesignID = other->m_primitive->GetDesignID();
            for (const UInt32* id = kSingleStudDesigns; *id != 0; ++id)
                if ((designID == *id || otherDesignID == *id) && connection->m_suspended != 2)
                    connection->SetSuspended(2);
        }
        if (rigid == otherRigid)
            continue;
        if (connection->m_type == PrimitiveConnection::JointRigid) {
            if (rigid == 0) {
                otherRigid->AddElement(element);
            } else if (otherRigid == 0) {
                rigid->AddElement(other);
            } else {
                BasicArray<PrimitiveRigidConnection*> mergedRigid;
                BasicArray<RigidConnection*> mergedBodies;
                rigid->Merge(otherRigid, mergedRigid, mergedBodies);
                HoldAll(mergedRigid, changedRigid);
                HoldAll(mergedBodies, changedBodies);
            }
        } else if (jointChanged) {
            SceneGraph::PushBackUnique(changed, connection);
        }
    }
    UpdateConnections(changed, changedRigid, changedBodies);
}

// Mac 0xbee00, Win 0x48c010
void Scene::UpdateConnections(const BasicArray<PrimitiveConnection*>& connections,
                              BasicArray<RefPointer<PrimitiveRigidConnection> >& changed,
                              BasicArray<RefPointer<RigidConnection> >& changedRigid)
{
    BasicArray<RefPointer<PrimitiveRigidElement> > created;
    for (BasicArray<PrimitiveConnection*>::ConstIterator it = connections.Begin(); it != connections.End(); ++it) {
        PrimitiveConnection* connection = *it;
        PrimitiveElement* element = connection->m_element;
        PrimitiveElement* other = connection->m_otherElement;
        PrimitiveRigidElement* rigid = element->m_rigid;
        PrimitiveRigidElement* otherRigid = other->m_rigid;
        if (rigid == otherRigid)
            continue;
        if (rigid == 0) {
            rigid = CreatePrimitiveRigidElement(element, element->m_transform);
            created.PushBack(RefPointer<PrimitiveRigidElement>(rigid));
        } else if (otherRigid == 0) {
            otherRigid = CreatePrimitiveRigidElement(other, other->m_transform);
            created.PushBack(RefPointer<PrimitiveRigidElement>(otherRigid));
        }
        PrimitiveRigidConnection* rigidConnection;
        bool jointChanged;
        PrimitiveRigidElement::ConnectionMap::Iterator found = rigid->m_connections.Find(otherRigid);
        if (found == rigid->m_connections.End()) {
            rigidConnection = new PrimitiveRigidConnection(connection);
            jointChanged = true;
        } else {
            rigidConnection = found->second;
            jointChanged = rigidConnection->AddConnection(connection);
        }
        RigidElement* body = rigid->m_rigidElement;
        RigidElement* otherBody = otherRigid->m_rigidElement;
        if (body == otherBody)
            continue;
        if (rigidConnection->m_type == PrimitiveConnection::JointRigid) {
            if (body == 0) {
                otherBody->AddElement(rigid);
            } else if (otherBody == 0) {
                body->AddElement(otherRigid);
            } else {
                BasicArray<RigidConnection*> merged;
                body->Merge(otherBody, merged);
                HoldAll(merged, changedRigid);
            }
            RefPointer<PrimitiveRigidConnection> hold(rigidConnection);
            SceneGraph::EraseValue(changed, rigidConnection);
        } else if (jointChanged) {
            SceneGraph::PushBackUnique(changed, RefPointer<PrimitiveRigidConnection>(rigidConnection));
        }
    }
    UpdateRigidConnections(changed, changedRigid);
}

// Mac 0xbf500
void Scene::UpdateRigidConnections(BasicArray<RefPointer<PrimitiveRigidConnection> >& changed,
                                   BasicArray<RefPointer<RigidConnection> >& changedRigid)
{
    for (BasicArray<RefPointer<PrimitiveRigidConnection> >::Iterator it = changed.Begin(); it != changed.End(); ++it) {
        PrimitiveRigidConnection* connection = *it;
        if (connection->m_element == 0)
            continue;
        RigidElement* body = connection->m_element->m_rigidElement;
        RigidElement* otherBody = connection->m_otherElement->m_rigidElement;
        if (body == otherBody)
            continue;
        if (body == 0)
            body = CreateRigidElement(connection->m_element, connection->m_element->m_transform, otherBody->m_hinged);
        else if (otherBody == 0)
            otherBody = CreateRigidElement(connection->m_otherElement, connection->m_otherElement->m_transform,
                                           body->m_hinged);
        else
            body->m_hinged->Merge(otherBody->m_hinged);
        if (connection->m_type != PrimitiveConnection::JointRigid) {
            RigidConnection* rigidConnection;
            bool jointChanged;
            RigidElement::ConnectionMap::Iterator found = body->m_connections.Find(otherBody);
            if (found == body->m_connections.End()) {
                rigidConnection = new RigidConnection(connection);
                jointChanged = true;
            } else {
                rigidConnection = found->second;
                jointChanged = rigidConnection->AddConnection(connection);
            }
            if (rigidConnection->m_joint != 0) {
                if (jointChanged)
                    SceneGraph::PushBackUnique(changedRigid, RefPointer<RigidConnection>(rigidConnection));
                continue;
            }
        }
        BasicArray<RigidConnection*> merged;
        body->Merge(otherBody, merged);
        HoldAll(merged, changedRigid);
    }
    ResolveRigidConnections(changedRigid);
}

// Mac 0xbf940, Win 0x48e1e0
void Scene::ResolveRigidConnections(BasicArray<RefPointer<RigidConnection> >& changed)
{
    while (!changed.IsEmpty()) {
        BasicArray<RefPointer<RigidConnection> > next;
        for (BasicArray<RefPointer<RigidConnection> >::Iterator it = changed.Begin(); it != changed.End(); ++it) {
            RigidConnection* connection = *it;
            if (connection->m_element == 0)
                continue;
            if (connection->m_joint == 0) {
                BasicArray<RigidConnection*> merged;
                connection->m_element->Merge(connection->m_otherElement, merged);
                HoldAll(merged, next);
            } else {
                CheckJointLoop(connection, next);
            }
        }
        changed.Swap(next);
    }
}

// Mac 0xc1300: a loop of three bodies (this connection A-B, a connection B-C and one C-A) whose
// joints cannot move (hinges not on one line, ...) is made rigid by merging two of the bodies.
void Scene::CheckJointLoop(RigidConnection* connection, BasicArray<RefPointer<RigidConnection> >& changed)
{
    const Real32 epsilon = 0.01f;
    RigidElement* a = connection->m_element;
    RigidElement* b = connection->m_otherElement;
    UInt32 type = connection->m_joint != 0 ? connection->m_joint->GetType() : 0;
    for (RigidElement::ConnectionMap::Iterator it = b->m_connections.Begin(); it != b->m_connections.End(); ++it) {
        RigidConnection* second = it->second;
        UInt32 secondType = second->m_joint != 0 ? second->m_joint->GetType() : 0;
        if (second == connection || second->m_joint == 0)
            continue;
        RigidElement* c = it->first;
        RigidElement::ConnectionMap::Iterator found = c->m_connections.Find(a);
        if (found == c->m_connections.End())
            continue;
        RigidConnection* third = found->second;
        if (third->m_joint == 0)
            continue;
        UInt32 thirdType = third->m_joint->GetType();
        if (third->m_joint == 0)
            continue;
        bool balls = type == PrimitiveConnection::JointBall && secondType == PrimitiveConnection::JointBall;
        if (balls && thirdType == PrimitiveConnection::JointBall)
            continue;
        if (type == PrimitiveConnection::JointSlide || secondType == PrimitiveConnection::JointSlide ||
            thirdType == PrimitiveConnection::JointSlide)
            continue;
        if (type == PrimitiveConnection::JointFixed || secondType == PrimitiveConnection::JointFixed ||
            thirdType == PrimitiveConnection::JointFixed)
            continue;

        // the three frames in world space (A side of this connection and of the third one, B
        // side of the second one)
        Vector3 axis, normal, position, secondAxis, secondNormal, secondPosition, thirdAxis, thirdNormal,
            thirdPosition;
        if (connection->m_element == a) {
            axis = connection->m_axis; normal = connection->m_normal; position = connection->m_position;
        } else {
            axis = connection->m_otherAxis; normal = connection->m_otherNormal; position = connection->m_otherPosition;
        }
        if (second->m_element == b) {
            secondAxis = second->m_axis; secondNormal = second->m_normal; secondPosition = second->m_position;
        } else {
            secondAxis = second->m_otherAxis; secondNormal = second->m_otherNormal;
            secondPosition = second->m_otherPosition;
        }
        if (third->m_element == a) {
            thirdAxis = third->m_axis; thirdNormal = third->m_normal; thirdPosition = third->m_position;
        } else {
            thirdAxis = third->m_otherAxis; thirdNormal = third->m_otherNormal; thirdPosition = third->m_otherPosition;
        }
        Matrix3D transform;
        a->m_body->GetTransform(transform);
        axis = Rotate(axis, transform);
        normal = Rotate(normal, transform);
        position = position * transform;
        thirdAxis = Rotate(thirdAxis, transform);
        thirdNormal = Rotate(thirdNormal, transform);
        thirdPosition = thirdPosition * transform;
        b->m_body->GetTransform(transform);
        secondAxis = Rotate(secondAxis, transform);
        secondNormal = Rotate(secondNormal, transform);
        secondPosition = secondPosition * transform;

        // first: this and the second joint share a line / point
        bool first = true;
        if (!balls) {
            if (type == PrimitiveConnection::JointBall)
                first = secondPosition.IsOnLine(secondAxis, position, epsilon);
            else if (secondType == PrimitiveConnection::JointBall)
                first = position.IsOnLine(axis, secondPosition, epsilon);
            else
                first = position.IsSameLine(axis, secondPosition, secondAxis, epsilon);
        }
        // second: the second and the third joint
        bool secondPair;
        if (secondType == PrimitiveConnection::JointBall && thirdType == PrimitiveConnection::JointBall) {
            secondPair = true;
        } else if (secondType == PrimitiveConnection::JointBall) {
            secondPair = thirdPosition.IsOnLine(thirdAxis, secondPosition, epsilon);
        } else if (thirdType == PrimitiveConnection::JointBall) {
            secondPair = secondPosition.IsOnLine(secondAxis, thirdPosition, epsilon);
        } else {
            secondPair = secondPosition.IsSameLine(secondAxis, thirdPosition, thirdAxis, epsilon);
        }
        if (type == PrimitiveConnection::JointBall && thirdType == PrimitiveConnection::JointBall) {
            if (first || secondPair)
                continue;
            BasicArray<RigidConnection*> merged;
            b->Merge(c, merged);
            HoldAll(merged, changed);
            return;
        }
        // third: the third joint and this one
        bool thirdPair;
        if (thirdType == PrimitiveConnection::JointBall)
            thirdPair = position.IsOnLine(axis, thirdPosition, epsilon);
        else if (type == PrimitiveConnection::JointBall)
            thirdPair = thirdPosition.IsOnLine(thirdAxis, position, epsilon);
        else
            thirdPair = thirdPosition.IsSameLine(thirdAxis, position, axis, epsilon);

        BasicArray<RigidConnection*> merged;
        if (first) {
            if (thirdPair || secondPair)
                continue;
            a->Merge(c, merged);
        } else if (!thirdPair) {
            a->Merge(b, merged);
        } else if (!secondPair) {
            b->Merge(c, merged);
        } else {
            continue;
        }
        HoldAll(merged, changed);
        return;
    }
}

// =============================================================================================
// Manipulated elements (driven joints)
// =============================================================================================

namespace {

typedef BasicArray<RigidConnection*> ConnectionLoop;

RigidElement* OtherSide(RigidConnection* connection, RigidElement* element)
{
    return connection->m_element == element ? connection->m_otherElement : connection->m_element;
}

// Depth-first walk over the joints from `body` (reached through `connection`), recording the
// loops of joints: when a joint leads to a body touching a joint on the current path, the
// path from there plus that joint is a loop. Joints are visited once (m_flag68).
// (name inferred) Mac 0xbc500, Win 0x48af90
void FindJointLoops(RigidConnection* connection, RigidElement* body, ConnectionLoop& path,
                    BasicArray<ConnectionLoop>& loops)
{
    connection->m_flag68 = true;
    path.PushBack(connection);
    for (RigidElement::ConnectionMap::Iterator it = body->m_connections.Begin(); it != body->m_connections.End();
         ++it) {
        RigidConnection* next = it->second;
        if (next->m_flag68)
            continue;
        RigidElement* other = OtherSide(next, body);
        for (UInt32 i = path.GetSize(); i-- > 0;) {
            RigidConnection* onPath = path[i];
            bool touches = false;
            for (RigidElement::ConnectionMap::Iterator c = other->m_connections.Begin();
                 c != other->m_connections.End(); ++c)
                if (c->second == onPath) {
                    touches = true;
                    break;
                }
            if (!touches)
                continue;
            loops.PushBack(ConnectionLoop());
            ConnectionLoop& loop = loops.Back();
            for (UInt32 j = i; j < path.GetSize(); ++j)
                loop.PushBack(path[j]);
            loop.PushBack(next);
            break;
        }
        FindJointLoops(next, other, path, loops);
    }
    path.PopBack();
}

// Removes from every loop the joints of each other loop it contains as a sub-sequence (the
// first occurrence). (name inferred) Mac inline in 0x9d050 (std::search 0xbdc00), Win 0x48b1b0
void RemoveContainedLoops(BasicArray<ConnectionLoop>& loops)
{
    for (UInt32 i = 0; i < loops.GetSize(); ++i) {
        for (UInt32 j = 0; j < loops.GetSize(); ++j) {
            if (i == j)
                continue;
            ConnectionLoop& inner = loops[i];
            ConnectionLoop& outer = loops[j];
            UInt32 n = inner.GetSize();
            if (n == 0 || n > outer.GetSize())
                continue;
            for (UInt32 start = 0; start + n <= outer.GetSize(); ++start) {
                UInt32 k = 0;
                while (k < n && outer[start + k] == inner[k])
                    ++k;
                if (k != n)
                    continue;
                ConnectionLoop rest;
                for (UInt32 m = 0; m < outer.GetSize(); ++m)
                    if (m < start || m >= start + n)
                        rest.PushBack(outer[m]);
                outer.Swap(rest);
                break;
            }
        }
    }
}

// Every loop holding one of the connections joins the connections (closure); the loop is
// emptied. (name inferred) Mac 0xbcab0, Win 0x48b240
void AddLoopConnections(BasicArray<RigidConnection*>& connections, BasicArray<ConnectionLoop>& loops)
{
    if (loops.IsEmpty())
        return;
    for (UInt32 u = 0; u < connections.GetSize(); ++u) {
        for (UInt32 l = 0; l < loops.GetSize(); ++l) {
            ConnectionLoop& loop = loops[l];
            if (!SceneGraph::Contains(loop, connections[u]))
                continue;
            for (UInt32 i = 0; i < loop.GetSize(); ++i)
                SceneGraph::PushBackUnique(connections, loop[i]);
            loop.Clear();
        }
    }
}

// Adds the bodies reachable from `body` without crossing a visited (active) body to the
// active element's followers. (name inferred) Mac 0xbcfa0, Win 0x48b310
void CollectFollowers(Scene::ActiveRigidElement& active, RigidElement* body)
{
    body->m_flag60 = true;
    for (RigidElement::ConnectionMap::Iterator it = body->m_connections.Begin(); it != body->m_connections.End();
         ++it) {
        RigidElement* other = OtherSide(it->second, body);
        if (other->m_flag60)
            continue;
        active.m_elements.PushBack(other);
        CollectFollowers(active, other);
    }
}

// (inline: Mac 0xbceb0, Win 0x4a0e70 behind a std::find)
void PushBackActive(BasicArray<Scene::ActiveRigidElement>& elements, RigidElement* body)
{
    for (UInt32 i = 0; i < elements.GetSize(); ++i)
        if (elements[i].m_element == body)
            return;
    Scene::ActiveRigidElement active;
    active.m_element = body;
    elements.PushBack(active);
}

} // namespace

// Mac 0x9d050, Win 0x48ac40
void Scene::CollectManipulatedElements(RigidConnection* connection, RigidElement* fixed,
                                       BasicArray<RigidConnection*>& connections,
                                       BasicArray<ActiveRigidElement>& elements)
{
    for (BasicArray<RefPointer<HingedElement> >::ConstIterator h = m_hingedElements.Begin();
         h != m_hingedElements.End(); ++h)
        for (BasicArray<RefPointer<RigidElement> >::ConstIterator e = (*h)->m_elements.Begin();
             e != (*h)->m_elements.End(); ++e) {
            (*e)->m_flag60 = false;
            for (RigidElement::ConnectionMap::Iterator c = (*e)->m_connections.Begin();
                 c != (*e)->m_connections.End(); ++c)
                c->second->m_flag68 = false;
        }
    BasicArray<ConnectionLoop> loops;
    ConnectionLoop path;
    FindJointLoops(connection, fixed, path, loops);
    RemoveContainedLoops(loops);
    connections.PushBack(connection);
    AddLoopConnections(connections, loops);
    for (UInt32 i = 0; i < connections.GetSize(); ++i) {
        PushBackActive(elements, connections[i]->m_element);
        PushBackActive(elements, connections[i]->m_otherElement);
    }
    for (UInt32 i = 0; i < elements.GetSize(); ++i)
        elements[i].m_element->m_flag60 = true;
    for (UInt32 i = 0; i < elements.GetSize(); ++i)
        CollectFollowers(elements[i], elements[i].m_element);
}

// =============================================================================================
// Disconnecting and removing
// =============================================================================================


// Mac 0xaed90
void Scene::RemoveElements(const BasicArray<RefPointer<PrimitiveElement> >& elements, bool notifyPhysics)
{
    if (m_manipulator != 0)
        m_manipulator->Detach();
    if (notifyPhysics) {
        DetachManipulator();
        m_physicsScene->Unknown6();
    }
    BasicArray<PrimitiveRigidElement*> rigids;
    BasicArray<RigidElement*> bodies;
    BasicArray<HingedElement*> hinged;
    for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator it = elements.Begin(); it != elements.End(); ++it) {
        RigidElement* body = (*it)->m_rigid->m_rigidElement;
        body->m_body->RemovePrimitiveElement((*it)->m_physics);
        AddChangedRigidElement(body);
    }
    for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator it = elements.Begin(); it != elements.End(); ++it) {
        PrimitiveElement* element = *it;
        PrimitiveRigidElement* rigid = element->m_rigid;
        RigidElement* body = 0;
        {
            RefPointer<PrimitiveElement> hold(element);
            SceneGraph::EraseValueScene(m_primitiveElements, element);   // (Win: unordered)
        }
        while (!element->m_connections.IsEmpty()) {
            PrimitiveConnection* connection = element->m_connections.Begin()->second;
            if (connection->m_rigidConnection != 0) {
                if (body == 0) {
                    body = rigid->m_rigidElement;
                    SceneGraph::PushBackUnique(bodies, body);
                    SceneGraph::PushBackUnique(hinged, body->m_hinged);
                }
                RigidConnection* rigidConnection = connection->m_rigidConnection->m_rigidConnection;
                if (m_flag120 && rigidConnection != 0) {
                    // NOTE: once per planar field connection
                    for (BasicArray<PrimitiveConnection::Connection>::ConstIterator c =
                             connection->m_connections.Begin();
                         c != connection->m_connections.End(); ++c)
                        if (c->m_field->GetType() == ConnectionField::TYPE_PLANAR)
                            AddChangedRigidElement(rigidConnection->m_element == rigid->m_rigidElement
                                                       ? rigidConnection->m_otherElement
                                                       : rigidConnection->m_element);
                }
            }
            DetachPrimitiveConnection(connection);
        }
        Matrix3D world;
        element->GetTransform(world);
        element->m_transform = world;
        element->m_rigid = 0;
        {
            RefPointer<PrimitiveElement> hold(element);
            SceneGraph::EraseValue(rigid->m_elements, element);
        }
        SceneIndexTouch(rigid->m_rigidElement);
        SceneGraph::PushBackUnique(rigids, rigid);
    }
    SplitElements(rigids, bodies, hinged);
    if (notifyPhysics) {
        m_physicsScene->Unknown7();
        m_structureChanged = true;
    }
}

// Mac 0xaf3a0
void Scene::Disconnect(const BasicArray<PrimitiveConnection*>& connections)
{
    if (connections.IsEmpty())
        return;
    if (m_manipulator != 0)
        m_manipulator->Detach();
    BeginUpdate();
    DetachManipulator();
    m_physicsScene->Unknown6();
    BasicArray<PrimitiveRigidElement*> rigids;
    BasicArray<RigidElement*> bodies;
    BasicArray<HingedElement*> hinged;
    for (BasicArray<PrimitiveConnection*>::ConstIterator it = connections.Begin(); it != connections.End(); ++it) {
        PrimitiveConnection* connection = *it;
        PrimitiveRigidElement* rigid = connection->m_element->m_rigid;
        RigidElement* body = rigid->m_rigidElement;
        if (connection->m_rigidConnection == 0) {
            SceneGraph::PushBackUnique(rigids, rigid);
            SceneGraph::PushBackUnique(bodies, body);
            SceneGraph::PushBackUnique(hinged, body->m_hinged);
            AddChangedRigidElement(body);
        } else {
            PrimitiveRigidElement* otherRigid = connection->m_otherElement->m_rigid;
            RigidElement* otherBody = otherRigid->m_rigidElement;
            SceneGraph::PushBackUnique(rigids, rigid);
            SceneGraph::PushBackUnique(rigids, otherRigid);
            SceneGraph::PushBackUnique(bodies, body);
            if (body != otherBody)
                SceneGraph::PushBackUnique(bodies, otherBody);
            SceneGraph::PushBackUnique(hinged, body->m_hinged);
            if (m_flag120) {
                if (connection->m_rigidConnection->m_rigidConnection == 0) {
                    AddChangedRigidElement(body);
                } else {
                    for (BasicArray<PrimitiveConnection::Connection>::ConstIterator c =
                             connection->m_connections.Begin();
                         c != connection->m_connections.End(); ++c)
                        if (c->m_field->GetType() == ConnectionField::TYPE_PLANAR) {
                            AddChangedRigidElement(body);
                            AddChangedRigidElement(otherBody);
                        }
                }
            }
        }
        DetachPrimitiveConnection(connection);
    }
    SplitElements(rigids, bodies, hinged);
    m_physicsScene->Unknown7();
    m_structureChanged = true;
    EndUpdate();
}

// Mac 0xb0080, Win 0x48df90
void Scene::SplitElements(const BasicArray<PrimitiveRigidElement*>& elements, BasicArray<RigidElement*>& bodies,
                          BasicArray<HingedElement*>& hinged)
{
    BasicArray<RigidElement*> updated;
    for (BasicArray<PrimitiveRigidElement*>::ConstIterator it = elements.Begin(); it != elements.End(); ++it) {
        RigidElement* split = 0;
        RigidElement* update = 0;
        (*it)->Split(split, update);
        if (split != 0)
            SceneGraph::PushBackUnique(bodies, split);
        if (update != 0)
            SceneGraph::PushBackUnique(updated, update);
    }
    // bodies that were not split only get new bounds
    for (BasicArray<RigidElement*>::ConstIterator it = updated.Begin(); it != updated.End(); ++it) {
        RigidElement* body = *it;
        if (SceneGraph::Contains(bodies, body))
            continue;
        body->m_bounding.Reset();
        for (BasicArray<RefPointer<PrimitiveRigidElement> >::ConstIterator e = body->m_elements.Begin();
             e != body->m_elements.End(); ++e)
            body->m_bounding.Union((*e)->m_bounding * (*e)->m_transform);
        body->m_boundingSphere.Set(body->m_bounding);
        SceneIndexTouch(body);
    }
    for (BasicArray<RigidElement*>::ConstIterator it = bodies.Begin(); it != bodies.End(); ++it) {
        HingedElement* split = 0;
        BasicArray<RigidConnection*> changed;
        (*it)->Split(split, changed);
        BasicArray<RefPointer<RigidConnection> > held;
        HoldAll(changed, held);
        ResolveRigidConnections(held);
        if (split != 0)
            SceneGraph::PushBackUnique(hinged, split);
    }
    for (BasicArray<HingedElement*>::ConstIterator it = hinged.Begin(); it != hinged.End(); ++it)
        (*it)->Split();
}

// Mac 0xa9b10
void Scene::RemoveBricks(const BasicArray<RefPointer<Brick> >& bricks, bool notifyPhysics)
{
    s_removeTimer.Start();
    BasicArray<RefPointer<PrimitiveElement> > elements;
    elements.Reserve(bricks.GetSize() * 2);
    for (BasicArray<RefPointer<Brick> >::ConstIterator b = bricks.Begin(); b != bricks.End(); ++b) {
        Brick* brick = *b;
        for (BasicArray<RefPointer<Part> >::ConstIterator p = brick->m_parts.Begin(); p != brick->m_parts.End();
             ++p) {
            Part* part = *p;
            for (UInt32 i = 0; i < part->m_behaviors.GetSize(); ++i)
                RemoveBehavior(part->m_behaviors[i]);
            for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = part->m_elements.Begin();
                 e != part->m_elements.End(); ++e) {
                for (UInt32 i = 0; i < (*e)->m_behaviors.GetSize(); ++i)
                    RemoveBehavior((*e)->m_behaviors[i]);
                elements.PushBack(*e);
            }
        }
        {
            RefPointer<Brick> hold(brick);
            SceneGraph::EraseValueScene(m_bricks, brick);   // (Win: unordered)
        }
        for (UInt32 i = 0; i < brick->m_behaviors.GetSize(); ++i)
            RemoveBehavior(brick->m_behaviors[i]);
    }
    RemoveElements(elements, notifyPhysics);
    s_removeTimer.Stop();
}

// Mac 0xae970
void Scene::Remove(const BasicArray<RefPointer<Brick> >& bricks)
{
    if (bricks.IsEmpty())
        return;
    BeginUpdate();
    RemoveBricks(bricks, true);
    EndUpdate();
}

// Mac 0xa7450, Win 0x47f110
void Scene::BatchInsert(const BasicArray<RefPointer<Brick> >& bricks, BasicArray<RefPointer<Brick> >& failed,
                        BatchInsertProgressCallback* progress, bool testCollision,
                        BatchInsertCollisionReporter* reporter)
{
    if (bricks.IsEmpty()) {
        if (progress != 0)
            progress->OnProgress(100);
        return;
    }
    if (progress != 0)
        progress->OnProgress(0);
    // NOTE: the manipulator is detached here but only cleared below
    if (m_manipulator != 0)
        m_manipulator->Detach();
    BeginUpdate();
    BatchInsertPhysicsCollisionCallback collisions(reporter);
    BasicArray<RefPointer<Brick> > notConnected;

    // 1. collision test of every element at its transform (0-33 %)
    if (testCollision && m_testCollisions) {
        bool cancelled = false;
        {
            s_batchCollisionTimer.Start();
            BasicArray<PhysicsPrimitiveElement*> physics;
            BasicArray<const Matrix3D*> transforms;
            physics.Reserve(bricks.GetSize() * 2);
            transforms.Reserve(bricks.GetSize() * 2);
            for (BasicArray<RefPointer<Brick> >::ConstIterator b = bricks.Begin(); b != bricks.End(); ++b)
                for (BasicArray<RefPointer<Part> >::ConstIterator p = (*b)->m_parts.Begin(); p != (*b)->m_parts.End();
                     ++p)
                    for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = (*p)->m_elements.Begin();
                         e != (*p)->m_elements.End(); ++e) {
                        physics.PushBack((*e)->m_physics);
                        transforms.PushBack(&(*e)->m_transform);
                    }
            BatchInsertPhysicsProgressCallback physicsProgress(progress);
            m_physicsScene->TestCollisions(physics.GetSize(), physics.Begin(), transforms.Begin(), &collisions,
                                           progress != 0 ? &physicsProgress : 0);
            s_batchCollisionTimer.Stop();
            if (progress != 0 && (!physicsProgress.m_continue || !progress->OnProgress(33)))
                cancelled = true;
        }
        if (cancelled) {
            // NOTE: cancelled during the collision test: nothing inserted, no Unknown7
            EndUpdate();
            return;
        }
    }

    // 2. insert the bricks that did not collide (33-67 %, or 0-50 % without collision test)
    DetachManipulator();
    m_physicsScene->Unknown6();
    {
        s_batchStructureTimer.Start();
        UInt32 index = 0;
        for (BasicArray<RefPointer<Brick> >::ConstIterator b = bricks.Begin(); b != bricks.End(); ++b, ++index) {
            Brick* brick = *b;
            if (SceneGraph::Contains(collisions.m_collided, brick))
                continue;
            bool notConnectable = false;
            for (BasicArray<RefPointer<Part> >::ConstIterator p = brick->m_parts.Begin();
                 p != brick->m_parts.End() && !notConnectable; ++p) {
                for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = (*p)->m_elements.Begin();
                     e != (*p)->m_elements.End(); ++e) {
                    PrimitiveElement* element = *e;
                    ConnectResult result;
                    if (!CanConnect(element, element->m_transform, result)) {
                        // the first element that cannot connect ends the brick (Mac 0xa7450:
                        // result 0x11 leaves the element loop, the part loop stops on the
                        // flag), so the brick is listed once; the elements inserted before it
                        // leave again below
                        notConnected.PushBack(RefPointer<Brick>(brick));
                        notConnectable = true;
                        break;
                    }
                    InsertElement(element, result);
                    if (progress == 0)
                        continue;
                    Real32 done = (Real32)index / (Real32)bricks.GetSize();
                    // NOTE: without collision test the percentage is int(done) * 50, 0 until the end
                    UInt32 percent = testCollision ? (UInt32)(Int32)(done * 33.0f) + 33 : (UInt32)(Int32)done * 50;
                    if (!progress->OnProgress(percent)) {
                        Clear();
                        m_physicsScene->Unknown7();
                        m_structureChanged = true;
                        EndUpdate();
                        return;
                    }
                }
            }
            if (!notConnectable)
                m_bricks.PushBack(RefPointer<Brick>(brick));
        }

        // the inserted elements of the bricks that could not connect leave again
        if (!notConnected.IsEmpty()) {
            BasicArray<RefPointer<PrimitiveElement> > remove;
            for (BasicArray<RefPointer<Brick> >::ConstIterator b = notConnected.Begin(); b != notConnected.End(); ++b)
                for (BasicArray<RefPointer<Part> >::ConstIterator p = (*b)->m_parts.Begin(); p != (*b)->m_parts.End();
                     ++p)
                    for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = (*p)->m_elements.Begin();
                         e != (*p)->m_elements.End(); ++e)
                        if ((*e)->m_rigid != 0)
                            remove.PushBack(*e);
            if (!remove.IsEmpty())
                RemoveElements(remove, false);
        }
        s_batchStructureTimer.Stop();
    }
    if (&collisions.m_collided != &failed)
        failed = collisions.m_collided;
    SceneGraph::Append(failed, notConnected);

    // 3. physics (67-100 %)
    if (progress != 0 && !progress->OnProgress(testCollision ? 67 : 50)) {
        Clear();
        m_physicsScene->Unknown7();
        m_structureChanged = true;
        EndUpdate();
        return;
    }
    s_batchPhysicsTimer.Start();
    m_physicsScene->Unknown7();
    m_structureChanged = true;
    EndUpdate();
    s_batchPhysicsTimer.Stop();
    if (progress != 0)
        progress->OnProgress(100);
}

// Mac 0xae540
void Scene::Reconnect(const ConnectResult& result)
{
    if (result.m_connections.IsEmpty())
        return;
    BasicArray<RefPointer<Brick> > bricks;
    for (BasicArray<PrimitiveConnectionInfo>::ConstIterator it = result.m_connections.Begin();
         it != result.m_connections.End(); ++it) {
        SceneGraph::PushBackUnique(bricks, RefPointer<Brick>(it->m_element->m_part->m_brick));
        SceneGraph::PushBackUnique(bricks, RefPointer<Brick>(it->m_otherElement->m_part->m_brick));
    }
    if (!bricks.IsEmpty()) {
        BeginUpdate();
        RemoveBricks(bricks, true);
        EndUpdate();
    }
    BasicArray<RefPointer<Brick> > failed;
    BatchInsert(bricks, failed, 0, false, 0);
}

// =============================================================================================
// Moving elements
// =============================================================================================

// Mac 0xb0940
bool Scene::MoveElements(const BasicArray<Pair<PrimitiveElement*, Matrix3D> >& elements, bool testCollision)
{
    typedef BasicArray<Pair<PrimitiveElement*, Matrix3D> > Transforms;
    if (m_manipulator != 0)
        m_manipulator->Detach();
    BeginUpdate();
    BasicArray<RefPointer<PrimitiveElement> > moved;
    moved.Reserve(elements.GetSize());
    for (Transforms::ConstIterator it = elements.Begin(); it != elements.End(); ++it)
        if (it->first->GetScene() != 0)
            moved.PushBack(RefPointer<PrimitiveElement>(it->first));
    RemoveElements(moved, true);
    moved.Clear();
    if (testCollision) {
        CanConnectPhysicsCollisionCallbackHelper callback;
        BasicArray<PhysicsPrimitiveElement*> physics;
        BasicArray<const Matrix3D*> transforms;
        physics.Reserve(elements.GetSize());
        transforms.Reserve(elements.GetSize());
        for (Transforms::ConstIterator it = elements.Begin(); it != elements.End(); ++it) {
            physics.PushBack(it->first->m_physics);
            transforms.PushBack(&it->second);
        }
        if (!m_physicsScene->TestCollisions(physics.GetSize(), physics.Begin(), transforms.Begin(), &callback, 0)) {
            // NOTE: the elements stay removed (SetTransforms moves them back)
            EndUpdate();
            return false;
        }
    }
    DetachManipulator();
    m_physicsScene->Unknown6();
    bool failed = false;
    for (Transforms::ConstIterator it = elements.Begin(); it != elements.End(); ++it) {
        ConnectResult result;
        if (!CanConnect(it->first, it->second, result)) {
            // the elements inserted so far leave again
            RemoveElements(moved, true);
            failed = true;
            break;
        }
        InsertElement(it->first, result);
        moved.PushBack(RefPointer<PrimitiveElement>(it->first));
    }
    m_physicsScene->Unknown7();
    m_structureChanged = true;
    EndUpdate();
    return !failed;
}

// Mac 0xb2aa0
bool Scene::SetTransforms(const BasicArray<Pair<PrimitiveElement*, Matrix3D> >& elements, bool testCollision)
{
    BasicArray<Pair<PrimitiveElement*, Matrix3D> > changed;
    BasicArray<Pair<PrimitiveElement*, Matrix3D> > previous;
    GetChangedTransforms(elements, changed, &previous);
    if (changed.IsEmpty())
        return true;
    BeginUpdate();
    bool moved = MoveElements(changed, testCollision);
    if (!moved)
        MoveElements(previous, false);
    EndUpdate();
    return moved;
}

// Mac 0xb2a80
bool Scene::SetTransforms(const BasicArray<Pair<PrimitiveElement*, Matrix3D> >& elements)
{
    return SetTransforms(elements, false);
}

// Mac 0xb24e0
void Scene::GetTransforms(BasicArray<Pair<PrimitiveElement*, Matrix3D> >& transforms) const
{
    transforms.Clear();
    transforms.Reserve(m_bricks.GetSize() * 2);
    for (BasicArray<RefPointer<HingedElement> >::ConstIterator h = m_hingedElements.Begin();
         h != m_hingedElements.End(); ++h)
        for (BasicArray<RefPointer<RigidElement> >::ConstIterator b = (*h)->m_elements.Begin();
             b != (*h)->m_elements.End(); ++b) {
            Matrix3D bodyTransform;
            (*b)->m_body->GetTransform(bodyTransform);
            for (BasicArray<RefPointer<PrimitiveRigidElement> >::ConstIterator r = (*b)->m_elements.Begin();
                 r != (*b)->m_elements.End(); ++r) {
                Matrix3D rigidTransform(bodyTransform, (*r)->m_transform);
                for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = (*r)->m_elements.Begin();
                     e != (*r)->m_elements.End(); ++e)
                    transforms.PushBack(MakePair((PrimitiveElement*)*e, Matrix3D(rigidTransform, (*e)->m_transform)));
            }
        }
}

// Mac 0xb26a0
void Scene::GetTransforms(const BasicArray<PrimitiveElement*>& elements,
                          BasicArray<Pair<PrimitiveElement*, Matrix3D> >& transforms)
{
    transforms.Reserve(elements.GetSize());
    Matrix3D transform;
    for (BasicArray<PrimitiveElement*>::ConstIterator it = elements.Begin(); it != elements.End(); ++it) {
        (*it)->GetTransform(transform);
        transforms.PushBack(MakePair(*it, transform));
    }
}

// Mac 0xb2950
void Scene::GetTransforms(const BasicArray<Pair<PrimitiveElement*, Matrix3D> >& entries,
                          BasicArray<Pair<PrimitiveElement*, Matrix3D> >& transforms)
{
    transforms.Reserve(entries.GetSize());
    Matrix3D transform;
    for (BasicArray<Pair<PrimitiveElement*, Matrix3D> >::ConstIterator it = entries.Begin(); it != entries.End();
         ++it) {
        it->first->GetTransform(transform);
        transforms.PushBack(MakePair(it->first, transform));
    }
}

// Mac 0xb27b0
void Scene::GetChangedTransforms(const BasicArray<Pair<PrimitiveElement*, Matrix3D> >& transforms,
                                 BasicArray<Pair<PrimitiveElement*, Matrix3D> >& changed,
                                 BasicArray<Pair<PrimitiveElement*, Matrix3D> >* current)
{
    changed.Reserve(transforms.GetSize());
    Matrix3D transform;
    for (BasicArray<Pair<PrimitiveElement*, Matrix3D> >::ConstIterator it = transforms.Begin();
         it != transforms.End(); ++it) {
        it->first->GetTransform(transform);
        if (transform != it->second) {
            changed.PushBack(*it);
            if (current != 0)
                current->PushBack(MakePair(it->first, transform));
        }
    }
}

// Mac 0xb04b0, Win 0x4802d0
void Scene::MoveParts(const BasicArray<RefPointer<Part> >& parts, Scene* target)
{
    if (parts.IsEmpty())
        return;
    if (m_manipulator != 0)
        m_manipulator->Detach();
    BeginUpdate();
    target->BeginUpdate();
    BasicArray<RefPointer<PrimitiveElement> > elements;
    BasicArray<Pair<PrimitiveElement*, Matrix3D> > transforms;
    elements.Reserve(parts.GetSize() * 2);
    transforms.Reserve(parts.GetSize() * 2);
    Matrix3D transform;
    for (BasicArray<RefPointer<Part> >::ConstIterator p = parts.Begin(); p != parts.End(); ++p)
        for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = (*p)->m_elements.Begin();
             e != (*p)->m_elements.End(); ++e) {
            elements.PushBack(*e);
            (*e)->GetTransform(transform);
            transforms.PushBack(MakePair((PrimitiveElement*)*e, transform));
        }
    RemoveElements(elements, true);
    target->MoveElements(transforms, false);
    target->EndUpdate();
    EndUpdate();
}

// =============================================================================================
// Merging scenes
// =============================================================================================

// Mac 0xa9030
void Scene::MergeScene(Scene* other, const ConnectResult& result)
{
    if (other == this)
        return;
    if (m_manipulator != 0)
        m_manipulator->Detach();
    BeginUpdate();
    DetachManipulator();
    m_physicsScene->Unknown6();
    other->BeginUpdate();
    other->DetachManipulator();
    other->m_physicsScene->Unknown6();

    // the hinged elements and their bodies move over (bodies by result.m_transform); the
    // joints are rebuilt in this scene's physics world
    for (BasicArray<RefPointer<HingedElement> >::ConstIterator h = other->m_hingedElements.Begin();
         h != other->m_hingedElements.End(); ++h) {
        HingedElement* hinged = *h;
        AddHingedElement(hinged);
        hinged->m_scene = this;
        BasicArray<RigidConnection*> joints;
        for (BasicArray<RefPointer<RigidElement> >::ConstIterator b = hinged->m_elements.Begin();
             b != hinged->m_elements.End(); ++b)
            for (RigidElement::ConnectionMap::Iterator c = (*b)->m_connections.Begin();
                 c != (*b)->m_connections.End(); ++c) {
                RigidConnection* connection = c->second;
                if (connection->m_joint == 0)
                    continue;
                joints.PushBack(connection);
                connection->m_joint->Release();
                connection->m_joint = 0;
                connection->m_constraints.Clear();
            }
        for (BasicArray<RefPointer<RigidElement> >::ConstIterator b = hinged->m_elements.Begin();
             b != hinged->m_elements.End(); ++b) {
            RigidElement* body = *b;
            Matrix3D transform;
            body->m_body->GetTransform(transform);
            Matrix3D moved(result.m_transform, transform);
            other->m_physicsScene->RemoveRigidElement(body->m_body);
            m_physicsScene->AddRigidElement(body->m_body, moved);
            m_addedRigidElements.PushBack(body);
            other->RemoveRigidElement(body);
        }
        for (BasicArray<RigidConnection*>::ConstIterator it = joints.Begin(); it != joints.End(); ++it) {
            RigidConnection* connection = *it;
            if (connection->m_joint != 0) {
                connection->m_joint->Release();
                connection->m_joint = 0;
                connection->m_constraints.Clear();
            }
            connection->Update();
        }
    }
    other->m_hingedElements.Clear();
    TakeGroups(other->m_groups);
    other->m_groups.Clear();
    SceneGraph::Append(m_behaviors, other->m_behaviors);
    other->m_behaviors.Clear();
    SceneGraph::Append(m_allBehaviors, other->m_allBehaviors);
    other->m_allBehaviors.Clear();
    SceneGraph::Append(m_bricks, other->m_bricks);
    other->m_bricks.Clear();
    SceneGraph::Append(m_primitiveElements, other->m_primitiveElements);
    other->m_primitiveElements.Clear();
    Connect(result.m_connections);
    other->m_physicsScene->Unknown7();
    other->m_structureChanged = true;
    other->EndUpdate();
    m_physicsScene->Unknown7();
    m_structureChanged = true;
    EndUpdate();
}

// Mac 0xb13f0
void Scene::Merge(Scene* other, const ConnectResult& result)
{
    if (!other->m_hingedElements.IsEmpty())
        MergeScene(other, result);
}

// =============================================================================================
// Copying
// =============================================================================================

// Mac 0xa7050
Brick* Scene::CloneBrick(BrickConnect* brickConnect, const Brick* brick, CopyMap& map)
{
    Brick* copy = brickConnect->CloneBrick(brick);
    copy->m_name = brick->m_name;
    for (BasicArray<RefPointer<Part> >::ConstIterator p = brick->m_parts.Begin(); p != brick->m_parts.End(); ++p) {
        const Part* part = *p;
        Part* partCopy = brickConnect->ClonePart(part);
        partCopy->SetBrick(copy);
        for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = part->m_elements.Begin();
             e != part->m_elements.End(); ++e) {
            PrimitiveElement* elementCopy = brickConnect->ClonePrimitiveElement(*e);
            elementCopy->SetPart(partCopy);
            map.m_elements[*e] = elementCopy;
        }
        partCopy->SetMaterials(part->m_materials);
        partCopy->SetDecorations(part->m_decorations);
        map.m_parts[part] = partCopy;
    }
    map.m_bricks[brick] = copy;
    return copy;
}

// Mac 0xa6d30, Win 0x47b480
void Scene::CopyBricks(const BasicArray<RefPointer<Brick> >& bricks, Scene* target, CopyMap& map, bool copyGroups,
                       UInt32 flags)
{
    if (target == this || bricks.IsEmpty())
        return;
    target->BeginUpdate();
    BasicArray<RefPointer<Brick> > copies;
    BasicArray<RefPointer<Brick> > failed;
    for (BasicArray<RefPointer<Brick> >::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it)
        copies.PushBack(RefPointer<Brick>(CloneBrick(m_brickConnect, *it, map)));
    target->BatchInsert(copies, failed, 0, false, 0);
    if ((flags & CopyFlagGroups) != 0 || copyGroups)
        target->CopyGroups(map);
    if (flags != 0)
        target->CopyBehaviors(map, flags, this);
    target->EndUpdate();
}

namespace {
// A new group like `source` (transform, pivot, name) without parts. (inline in Mac 0xa86c0)
BrickGroup* CopyGroup(Scene* scene, const BrickGroup* source, Scene::CopyMap& map)
{
    BasicArray<RefPointer<Part> > none;
    BrickGroup* group = scene->CreateGroup(0, none);
    map.m_groups[source] = group;
    Vector3 pivot(source->m_vector8c);
    group->m_transform = source->m_transform;
    group->m_vector8c = pivot;
    group->m_name = source->m_name;
    return group;
}
} // namespace

// Mac 0xa86c0, Win 0x47b720: the copied parts join copies of their groups (created on first
// use, with copies of the parent chain).
// NOTE (both builds): the parts are visited in the order of their addresses (the map is keyed by
// pointer), so the order the copied groups are created in depends on where the heap placed the
// parts, on 32-bit as on 64-bit builds. Kept: a stable order of its own would change the 32-bit
// behaviour.
void Scene::CopyGroups(CopyMap& map)
{
    BeginUpdate();
    for (BasicMap<const Part*, Part*>::Iterator it = map.m_parts.Begin(); it != map.m_parts.End(); ++it) {
        const BrickGroup* source = it->first->m_group;
        if (source == 0)
            continue;
        BrickGroup* group;
        BasicMap<const BrickGroup*, BrickGroup*>::Iterator found = map.m_groups.Find(source);
        if (found != map.m_groups.End()) {
            group = found->second;
        } else {
            group = CopyGroup(this, source, map);
            BrickGroup* child = group;
            for (const BrickGroup* parent = source->m_parent; parent != 0 && child->m_parent == 0;
                 parent = parent->m_parent) {
                BasicMap<const BrickGroup*, BrickGroup*>::Iterator copied = map.m_groups.Find(parent);
                BrickGroup* parentCopy = copied != map.m_groups.End() ? copied->second : CopyGroup(this, parent, map);
                child->SetParent(parentCopy);
                child = parentCopy;
            }
        }
        it->second->SetGroup(group);
    }
    EndUpdate();
}

// Mac 0xa8b50
void Scene::CopyBehaviors(CopyMap& map, UInt32 flags, Scene* source)
{
    BeginUpdate();
    if ((flags & CopyFlagElements) != 0)
        for (BasicMap<const PrimitiveElement*, PrimitiveElement*>::Iterator it = map.m_elements.Begin();
             it != map.m_elements.End(); ++it)
            it->first->CopyBehaviorsTo(*it->second);
    if ((flags & CopyFlagParts) != 0)
        for (BasicMap<const Part*, Part*>::Iterator it = map.m_parts.Begin(); it != map.m_parts.End(); ++it)
            it->first->CopyBehaviorsTo(*it->second);
    if ((flags & CopyFlagBricks) != 0)
        for (BasicMap<const Brick*, Brick*>::Iterator it = map.m_bricks.Begin(); it != map.m_bricks.End(); ++it)
            it->first->CopyBehaviorsTo(*it->second);
    if ((flags & CopyFlagGroups) != 0)
        for (BasicMap<const BrickGroup*, BrickGroup*>::Iterator it = map.m_groups.Begin(); it != map.m_groups.End();
             ++it)
            it->first->CopyBehaviorsTo(*it->second);
    if (source != 0 && (flags & CopyFlagScene) != 0)
        source->CopyBehaviorsTo(*this);
    EndUpdate();
}

// Mac 0xa8cc0
void Scene::CopyParts(const BasicArray<RefPointer<Part> >& parts, Scene* target, CopyMap& map, bool copyGroups,
                      UInt32 flags)
{
    BasicArray<RefPointer<Brick> > bricks;
    GetBricks(parts, bricks);
    CopyBricks(bricks, target, map, copyGroups, flags);
}

// Mac 0xa8df0
void Scene::GetBricks(const BasicArray<RefPointer<Part> >& parts, BasicArray<RefPointer<Brick> >& bricks)
{
    for (BasicArray<RefPointer<Part> >::ConstIterator it = parts.Begin(); it != parts.End(); ++it)
        SceneGraph::PushBackUnique(bricks, RefPointer<Brick>((*it)->m_brick));
}

// =============================================================================================
// Part and element queries
// =============================================================================================

// Mac 0xb2d20
void Scene::GetParts(BasicArray<RefPointer<Part> >& parts) const
{
    parts.Clear();
    if (m_bricks.IsEmpty()) {
        parts.Reserve(m_primitiveElements.GetSize());
        for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator it = m_primitiveElements.Begin();
             it != m_primitiveElements.End(); ++it)
            SceneGraph::PushBackUnique(parts, RefPointer<Part>((*it)->m_part));
    } else {
        parts.Reserve(m_bricks.GetSize() * 2);
        for (BasicArray<RefPointer<Brick> >::ConstIterator b = m_bricks.Begin(); b != m_bricks.End(); ++b)
            SceneGraph::Append(parts, (*b)->m_parts);
    }
}

// Mac 0xb30c0
bool Scene::IsWire(UInt32 designID)
{
    return ConnectivityWorldLookupTables::GetWires(designID) != 0;
}

// Mac 0xb30e0, Win 0x4824b0
void Scene::ExpandParts(const BasicArray<RefPointer<Part> >& parts, BasicArray<RefPointer<Part> >& result,
                        bool bricks, bool groups, bool wires)
{
    if (!bricks && !groups) {
        if (&parts != &result)
            result = parts;
        return;
    }
    BasicArray<RefPointer<Part> > expanded;
    if (!groups) {
        SceneGraph::Append(expanded, parts);
    } else {
        for (BasicArray<RefPointer<Part> >::ConstIterator it = parts.Begin(); it != parts.End(); ++it) {
            SceneGraph::PushBackUnique(expanded, *it);
            BrickGroup* group = (*it)->m_group;
            if (group != 0) {
                while (group->m_parent != 0)
                    group = group->m_parent;
                // NOTE: appended without the unique check
                group->GetAllParts(expanded);
            }
        }
    }
    if (!bricks) {
        for (BasicArray<RefPointer<Part> >::ConstIterator it = expanded.Begin(); it != expanded.End(); ++it)
            SceneGraph::PushBackUnique(result, *it);
        return;
    }
    for (BasicArray<RefPointer<Part> >::ConstIterator it = expanded.Begin(); it != expanded.End(); ++it) {
        Part* part = *it;
        Brick* brick = part->m_brick;
        const ConnectivityWorldLookupTables::WireList* list =
            wires ? ConnectivityWorldLookupTables::GetWires(brick->GetDesignID()) : 0;
        if (list == 0) {
            for (BasicArray<RefPointer<Part> >::ConstIterator p = brick->m_parts.Begin(); p != brick->m_parts.End();
                 ++p)
                SceneGraph::PushBackUnique(result, *p);
            continue;
        }
        // wire assemblies: a part that is not one of the wire parts stays alone, a wire part
        // brings the assembly's other wire parts
        if (std::find(list->begin(), list->end(), part->GetDesignID()) == list->end()) {
            SceneGraph::PushBackUnique(result, RefPointer<Part>(part));
            continue;
        }
        for (BasicArray<RefPointer<Part> >::ConstIterator p = brick->m_parts.Begin(); p != brick->m_parts.End(); ++p)
            if (std::find(list->begin(), list->end(), (*p)->GetDesignID()) != list->end())
                SceneGraph::PushBackUnique(result, *p);
    }
}

// Mac 0xb35c0
void Scene::GetElements(const BasicArray<RefPointer<Part> >& parts, BasicArray<RefPointer<PrimitiveElement> >& elements)
{
    for (BasicArray<RefPointer<Part> >::ConstIterator p = parts.Begin(); p != parts.End(); ++p)
        SceneGraph::Append(elements, (*p)->m_elements);
}

// =============================================================================================
// Drag snapping (ClickStick tools)
// =============================================================================================

// Mac 0xab0e0
void Scene::BeginDrag(Scene* source, PrimitiveElement* first, bool single)
{
    m_dragScene = source;
    m_dragMisses.Clear();
    m_dragHits.Clear();
    m_physicsScene->SetDragScene(source->m_physicsScene);
    m_dragResult.m_transform.SetIdentity();
    m_dragResult.m_connections.Clear();
    m_dragStart = 0;
    m_dragSingle = first != 0 && single;
    if (first != 0) {
        const BasicArray<RefPointer<PrimitiveElement> >& elements = m_dragScene->m_primitiveElements;
        while (m_dragStart < elements.GetSize() && elements[m_dragStart] != first)
            ++m_dragStart;
    }
}

// Mac 0xab280
void Scene::EndDrag()
{
    m_physicsScene->ClearDragScene();
    m_dragScene = 0;
    m_dragMisses.Clear();
    m_dragHits.Clear();
}

// Mac 0xab340
bool Scene::CanConnectDrag(const Matrix3D& transform, ConnectResult& result)
{
    if (m_dragScene->m_hingedElements.IsEmpty())
        return false;
    return CanConnect(m_dragScene, transform, result);
}

// Mac 0xaba40
bool Scene::CanConnectDragCached(const Matrix3D& transform, ConnectResult& result)
{
    if (m_dragScene->m_hingedElements.IsEmpty())
        return false;
    if (m_dragMisses.Find(transform) != m_dragMisses.End())
        return false;
    BasicMap<Matrix3D, BasicArray<PrimitiveConnectionInfo>, Matrix3DLess>::Iterator hit = m_dragHits.Find(transform);
    if (hit != m_dragHits.End()) {
        result.m_transform = hit->first;
        result.m_connections = hit->second;
        return true;
    }
    if (CanConnect(m_dragScene, transform, result)) {
        m_dragHits[result.m_transform] = result.m_connections;
        return true;
    }
    m_dragMisses.Insert(transform);
    return false;
}

// Mac 0xab370
namespace {
// SnapDrag / SnapDragRay: from this many carried elements on, those that cannot snap are passed
const UInt32 kManyDragElements = 32;
// An occupied planar field snaps to nothing there: IsCompatible accepts only fields of its own
// type, and the planar CanConnect of either field turns an occupied one down (0).
inline bool OccupiedPlanar(const ConnectionField* field)
{
    return field->GetType() == ConnectionField::TYPE_PLANAR && field->IsOccupied();
}
// No field of the element can snap: each is an occupied planar field, or (`flagged`) has flag 1.
// In a group cut from a model most elements are so.
bool NoFreeField(const PrimitiveElement* element, bool flagged)
{
    for (BasicArray<RefPointer<ConnectionField> >::ConstIterator f = element->m_fields.Begin();
         f != element->m_fields.End(); ++f) {
        const ConnectionField* field = *f;
        if (!(flagged && (field->GetFlags() & 1) != 0) && !OccupiedPlanar(field))
            return false;
    }
    return true;
}
} // namespace

bool Scene::SnapDrag(const Matrix3D& transform, Real32 maxDistance, ConnectResult& result, bool& snapped)
{
    snapped = false;
    if (m_dragScene->m_hingedElements.IsEmpty())
        return false;
    if (m_listener != 0)
        m_listener->OnSceneEvent(SceneListener::CodeConnect);
    m_dragResult.m_transform.SetIdentity();
    m_dragResult.m_connections.Clear();
    const BasicArray<RefPointer<PrimitiveElement> >& elements = m_dragScene->m_primitiveElements;
    UInt32 count = elements.GetSize();
    UInt32 tries = m_dragSingle ? 1 : count;
    bool found = false;
    for (UInt32 k = 0; k < tries; ++k) {
        PrimitiveElement* element = elements[(m_dragStart + k) % count];
        // its pick and the field pairs below would come to nothing (NoFreeField)
        if (count >= kManyDragElements && NoFreeField(element, false))
            continue;
        Matrix3D elementTransform;
        element->GetTransform(elementTransform);
        Sphere sphere(element->m_boundingSphere);
        sphere.m_center *= elementTransform;
        sphere.m_center *= transform;
        PickResult picked;
        HideAwareOrderedElementsSpherePicker picker(sphere, &picked);
        Pick(picker);
        // the scene elements nearest first; the first one that snaps ends the search
        for (PickResult::Iterator p = picked.Begin(); p != picked.End(); ++p) {
            PrimitiveElement* other = p->m_element;
            for (BasicArray<RefPointer<ConnectionField> >::ConstIterator f = element->m_fields.Begin();
                 f != element->m_fields.End(); ++f) {
                const ConnectionField* field = *f;
                if (OccupiedPlanar(field))
                    continue;
                Matrix3D fieldTransform(transform, elementTransform);
                fieldTransform.Multiply(field->GetTransform());
                for (BasicArray<RefPointer<ConnectionField> >::ConstIterator g = other->m_fields.Begin();
                     g != other->m_fields.End(); ++g) {
                    const ConnectionField* otherField = *g;
                    if (!otherField->IsCompatible(*field))
                        continue;
                    if (OccupiedPlanar(otherField))
                        continue;
                    Matrix3D otherTransform;
                    other->GetTransform(otherTransform);
                    otherTransform.Multiply(otherField->GetTransform());
                    Real32 distance;
                    Matrix3D snap;
                    if ((otherField->CanConnect(otherTransform, *field, fieldTransform, maxDistance, distance, snap) & 6) ==
                        0)
                        continue;
                    // the drag transform that puts the field at the snapped place
                    Matrix3D fieldInDrag(elementTransform, field->GetTransform());
                    Matrix3D inverse;
                    inverse.SetInverse(fieldInDrag);
                    snap.Multiply(inverse);
                    ConnectResult candidate;
                    bool connects = CanConnectDragCached(snap, candidate);
                    if (!candidate.m_connections.IsEmpty() && connects) {
                        maxDistance = distance;
                        result.m_transform = candidate.m_transform;
                        result.m_connections.Swap(candidate.m_connections);
                        found = true;
                    }
                }
            }
            if (found)
                return true;
        }
    }
    return CanConnectDragCached(transform, result);
}

// =============================================================================================
// Ray snapping of the drag scene (ClickStick tools)
// =============================================================================================

namespace {

// result <-> candidate (the transform by copy, the connection arrays by pointer swap; inline in
// Mac 0xabd10 at 0xac00c and 0xac951).
inline void SwapConnectResult(Scene::ConnectResult& result, Scene::ConnectResult& candidate)
{
    Matrix3D transform(result.m_transform);
    result.m_transform = candidate.m_transform;
    candidate.m_transform = transform;
    result.m_connections.Swap(candidate.m_connections);
}

} // namespace

namespace {
// SnapDragRay: from this many carried elements on, their cone picks go over the bodies of a bound
const UInt32 kManyDragCones = 32;
// ... when it lists at most this many bodies (each element then goes over all of them)
const size_t kConeBodiesMaximum = 512;
// ... else, after this many picks without a connection, the elements of those bodies indexed by
// direction from the apex (SceneIndex::BuildCones)
const UInt32 kConeIndexAfter = 1024;

// accepts the spheres its cone accepts with some room (radius 0.1 % + 0.001 larger)
class ConeBoundPicker : public Scene::Picker {
public:
    explicit ConeBoundPicker(const Cone& cone) : m_cone(cone) {}
    virtual bool Test(const Sphere& sphere)
    {
        Sphere s(sphere);
        s.m_radius = s.m_radius * 1.001f + 0.001f;
        return m_cone.Intersects(s);
    }
    virtual void Visit(PrimitiveElement*, const Matrix3D&, const Sphere&) {}
    Cone m_cone;
};
} // namespace

// Mac 0xabd10, Win 0x47d060
bool Scene::SnapDragRay(const BasicArray<RefPointer<PrimitiveElement> >& candidates, const Vector3& origin,
                        const Vector3& direction, const Matrix3D& transform, Real32 maxDistance,
                        ConnectResult& result, bool& snapped)
{
    snapped = false;
    if (m_dragScene->m_hingedElements.IsEmpty())
        return false;
    if (m_listener != 0)
        m_listener->OnSceneEvent(SceneListener::CodeConnect);

    // 1. the axle / slider connections of the last result, starting with one of the start element
    const BasicArray<PrimitiveConnectionInfo>& last = m_dragResult.m_connections;
    if (!last.IsEmpty()) {
        PrimitiveElement* first = m_dragScene->m_primitiveElements[m_dragStart];
        UInt32 start = 0;
        for (UInt32 i = 0; i < last.GetSize(); ++i)
            if (last[i].m_element == first || last[i].m_otherElement == first) {
                start = i;
                break;
            }
        for (UInt32 k = 0; k < m_dragResult.m_connections.GetSize(); ++k) {
            const PrimitiveConnectionInfo& info = last[(k + start) % last.GetSize()];
            bool dragFirst = info.m_element->GetScene() == m_dragScene;
            const ConnectionField* dragField = dragFirst ? info.m_field : info.m_otherField;
            if ((dragField->GetType() | 4) != ConnectionField::TYPE_SLIDER)   // axle (2) or slider (6)
                continue;
            PrimitiveElement* dragElement = dragFirst ? info.m_element : info.m_otherElement;
            PrimitiveElement* sceneElement = dragFirst ? info.m_otherElement : info.m_element;
            const ConnectionField* sceneField = dragFirst ? info.m_otherField : info.m_field;
            Matrix3D elementTransform;
            dragElement->GetTransform(elementTransform);
            Matrix3D fieldTransform(transform, elementTransform);
            fieldTransform.Multiply(dragField->GetTransform());
            Matrix3D sceneFieldTransform;
            sceneElement->GetTransform(sceneFieldTransform);
            // NOTE: the original also builds Matrix3D(transform, scene element transform) here
            // (Mac 0xabee5) and never uses it.
            sceneFieldTransform.Multiply(sceneField->GetTransform());
            Real32 distance;
            Matrix3D snap;
            if ((sceneField->CanConnect(sceneFieldTransform, *dragField, fieldTransform, origin, direction, maxDistance,
                                        distance, snap) & ConnectionField::CONNECTION_SNAP) == 0)
                continue;
            Matrix3D fieldInDrag(elementTransform, dragField->GetTransform());
            Matrix3D inverse;
            inverse.SetInverse(fieldInDrag);
            snap.Multiply(inverse);
            ConnectResult candidate;
            if (CanConnectDragCached(snap, candidate) && !candidate.m_connections.IsEmpty()) {
                SwapConnectResult(result, candidate);
                m_dragResult = result;
                return true;
            }
        }
    }

    // 2. cone picks along the ray
    BoundingBox box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    m_dragScene->GetBoundingBox(box, true);
    Vector3 center = box.GetCenter();
    Plane plane(direction, origin);
    Matrix3D best;
    m_dragResult.m_transform.SetIdentity();
    m_dragResult.m_connections.Clear();
    Real32 bestDistance = 3.4028235e38f;   // FLT_MAX (Mac 0x630000)
    const BasicArray<RefPointer<PrimitiveElement> >& elements = m_dragScene->m_primitiveElements;
    UInt32 count = elements.GetSize();
    UInt32 tries = m_dragSingle ? 1 : count;
    bool found = false;
    // LDD64: each carried element's cone pick walks every body (10^8 sphere tests for 131,072
    // elements over 1,024 bodies). A cone around all of them lists the bodies once; each element's
    // pick then goes over those, in the walk's order, visiting the same elements.
    std::vector<const void*> coneBodies;
    UInt32 coneReleases = 0;
    bool coneFiltered = false;
    bool coneBound = false;     // coneBodies listed (the bound holds every element's cone)
    bool coneIndexed = false;   // BuildCones done
    struct ConeIndexDrop {
        SceneIndex* index;
        ~ConeIndexDrop() { if (index != 0) index->DropCones(); }
    } coneIndexDrop = { 0 };
    if (candidates.IsEmpty() && tries >= kManyDragCones) {
        std::vector<Real32> cones;   // per element: axis, angle (computed as in the loop)
        cones.reserve((size_t)count * 4);
        double sum[3] = {0.0, 0.0, 0.0};
        bool usable = true;
        for (UInt32 k = 0; k < count && usable; ++k) {
            PrimitiveElement* element = elements[k];
            Matrix3D elementTransform;
            element->GetTransform(elementTransform);
            Sphere sphere(element->m_boundingSphere);
            sphere.m_center *= elementTransform;
            sphere.m_center *= transform;
            Vector3 axis = sphere.m_center - origin;
            Real32 length = axis.GetLength();
            Real32 angle = atanf(sphere.m_radius / length);
            axis /= length;
            if (!(length > 0.0f) || !(angle >= 0.0f && angle < 1.5f) ||
                !(axis.x == axis.x && axis.y == axis.y && axis.z == axis.z))
                usable = false;
            cones.push_back(axis.x);
            cones.push_back(axis.y);
            cones.push_back(axis.z);
            cones.push_back(angle);
            sum[0] += axis.x;
            sum[1] += axis.y;
            sum[2] += axis.z;
        }
        const double norm = sqrt(sum[0] * sum[0] + sum[1] * sum[1] + sum[2] * sum[2]);
        if (usable && norm > 1e-6 * count) {
            const double a[3] = {sum[0] / norm, sum[1] / norm, sum[2] / norm};
            double half = 0.0;
            for (size_t i = 0; i < cones.size(); i += 4) {
                double c = a[0] * cones[i] + a[1] * cones[i + 1] + a[2] * cones[i + 2];
                c = c > 1.0 ? 1.0 : (c < -1.0 ? -1.0 : c);
                const double h = acos(c) + cones[i + 3];
                if (h > half)
                    half = h;
            }
            half += 0.01;   // room far above the rounding of the cone tests
            if (half < 1.3) {
                ConeBoundPicker bound(Cone(origin, Vector3((Real32)a[0], (Real32)a[1], (Real32)a[2]), (Real32)half));
                if (m_index == 0)
                    m_index = new SceneIndex;
                m_index->CollectBodies(m_hingedElements, bound, coneBodies, coneReleases);
                coneFiltered = coneBodies.size() <= kConeBodiesMaximum;
                coneBound = true;
            }
        }
    }
    UInt32 conePicks = 0;
    for (UInt32 k = 0; k < tries; ++k) {
        PrimitiveElement* element = m_dragScene->m_primitiveElements[(m_dragStart + k) % count];
        // its pick and the field pairs below would come to nothing (NoFreeField)
        if (count >= kManyDragElements && NoFreeField(element, true))
            continue;
        Matrix3D elementTransform;
        element->GetTransform(elementTransform);
        Sphere sphere(element->m_boundingSphere);
        sphere.m_center *= elementTransform;
        sphere.m_center *= transform;
        Vector3 axis = sphere.m_center - origin;
        Real32 length = axis.GetLength();
        Real32 angle = atanf(sphere.m_radius / length);
        axis /= length;
        PickResult picked;
        HideAwareConePicker picker(Cone(origin, axis, angle), &picked);
        if (candidates.IsEmpty()) {
            if (coneBound && !coneFiltered && !coneIndexed && ++conePicks > kConeIndexAfter) {
                coneIndexed = true;
                coneIndexDrop.index = m_index;
                m_index->BuildCones(coneBodies, coneReleases, origin);
            }
            if (coneFiltered ? !m_index->PickBodies(coneBodies, coneReleases, picker)
                             : (!coneIndexed || !m_index->PickCones(picker)))
            Pick(picker);
        } else {
            for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator c = candidates.Begin(); c != candidates.End();
                 ++c) {
                PrimitiveElement* candidate = *c;
                if (candidate->GetScene() != this)
                    continue;
                Matrix3D candidateTransform;
                candidate->GetTransform(candidateTransform);
                Sphere candidateSphere(candidate->m_boundingSphere);
                candidateSphere.m_center *= candidateTransform;
                if (picker.Test(candidateSphere))
                    picker.Visit(candidate, candidateTransform, candidateSphere);
            }
        }
        for (PickResult::Iterator p = picked.Begin(); p != picked.End(); ++p) {
            PrimitiveElement* other = p->m_element;
            Real32 nearest = maxDistance;   // per scene element
            for (BasicArray<RefPointer<ConnectionField> >::ConstIterator f = element->m_fields.Begin();
                 f != element->m_fields.End(); ++f) {
                const ConnectionField* field = *f;
                if ((field->GetFlags() & 1) != 0)
                    continue;
                if (OccupiedPlanar(field))   // every pair turned down below
                    continue;
                Matrix3D fieldTransform(transform, elementTransform);
                fieldTransform.Multiply(field->GetTransform());
                for (BasicArray<RefPointer<ConnectionField> >::ConstIterator g = other->m_fields.Begin();
                     g != other->m_fields.End(); ++g) {
                    const ConnectionField* otherField = *g;
                    if ((otherField->GetFlags() & 1) != 0)
                        continue;
                    if (!otherField->IsCompatible(*field))
                        continue;
                    // ConnectionFieldPlanar::CanConnect is 0 when either field is occupied: nothing
                    // to do
                    if (OccupiedPlanar(otherField))
                        continue;
                    Matrix3D otherTransform;
                    other->GetTransform(otherTransform);
                    otherTransform.Multiply(otherField->GetTransform());
                    Real32 distance;
                    Matrix3D snap;
                    if ((otherField->CanConnect(otherTransform, *field, fieldTransform, origin, direction, nearest,
                                                distance, snap) & ConnectionField::CONNECTION_SNAP) == 0)
                        continue;
                    Matrix3D fieldInDrag(elementTransform, field->GetTransform());
                    Matrix3D inverse;
                    inverse.SetInverse(fieldInDrag);
                    snap.Multiply(inverse);
                    ConnectResult candidate;
                    if (CanConnectDragCached(snap, candidate)) {
                        if (!candidate.m_connections.IsEmpty()) {
                            nearest = distance;
                            SwapConnectResult(result, candidate);
                            found = true;
                        }
                    } else if (!found) {
                        // no connection: keep the placement nearest to the plane of the ray origin
                        Real32 planeDistance = plane.GetDistance(center * snap);
                        if (planeDistance < bestDistance) {
                            best = snap;
                            snapped = true;
                            bestDistance = planeDistance;
                        }
                    }
                }
            }
            if (found) {
                m_dragResult = result;
                return true;
            }
        }
    }
    if (snapped)
        result.m_transform = best;
    return false;
}

// Mac 0xabc90, Win 0x47cfd0
bool Scene::SnapDragRay(const Vector3& origin, const Vector3& direction, const Matrix3D& transform,
                        Real32 maxDistance, ConnectResult& result, bool& snapped)
{
    BasicArray<RefPointer<PrimitiveElement> > candidates;
    return SnapDragRay(candidates, origin, direction, transform, maxDistance, result, snapped);
}

// =============================================================================================
// Solvers
// =============================================================================================

// RTTI: LEGO::MoveTogetherSolverContext : PhysicsSolverCallback. The context of
// Scene::MoveTogether: records the scene's transforms when the solver starts and applies the
// changed ones to the scene on its result. Its members are the solver's collision callback and
// feature list as well. (member names inferred)
//   Mac sizeof 0x30 (operator new in 0xb1410); vtable 0x7624cc {QueryInterface 0xc8600, AddRef
//       0xc8680, Release 0xc86a0, OnBegin 0xc86e0, OnEnd 0xc8700, OnResult 0xc8710, OnStep
//       0xc8740, D1 0xc8750, D0 0xc87a0}.
//   Win vtable 0xa23d14 {0x480730, 0x480770, 0x480790, 0x480700, 0x40be80, 0x480710, 0x40be80,
//       sdd 0x480840}; constructor 0x480680, destructor 0x4807c0.
class MoveTogetherSolverContext : public PhysicsSolverCallback {
public:
    explicit MoveTogetherSolverContext(Scene* scene) : m_scene(scene), m_refCount(1)
    {
        m_features.Reserve(16);   // operator new[](0x80)
    }

    // Mac 0xc8600, Win 0x480730: only the PhysicsSolverCallback id.
    virtual Int32 LEGO_COM_CALL QueryInterface(const InterfaceID& id, void** object)
    {
        if (memcmp(&id, &PhysicsSolverCallback::GetIID(), sizeof id) != 0) {
            *object = 0;
#if defined(_WIN32)
            return (Int32)0x80004005;
#else
            return (Int32)0x80000008;
#endif
        }
        *object = this;
        AddRef();
        return 0;
    }
    // Mac 0xc8680, Win 0x480770. NOTE: Mac returns nothing useful (OSAtomicAdd32's result is
    // dropped); the new count here.
    virtual UInt32 LEGO_COM_CALL AddRef()
    {
#if defined(_WIN32)
        return (UInt32)InterlockedIncrement((LONG*)&m_refCount);
#else
        return (UInt32)OSAtomicAdd32(1, (int32_t*)&m_refCount);
#endif
    }
    // Mac 0xc86a0, Win 0x480790
    virtual UInt32 LEGO_COM_CALL Release()
    {
#if defined(_WIN32)
        Int32 count = InterlockedDecrement((LONG*)&m_refCount);
#else
        Int32 count = OSAtomicAdd32(-1, (int32_t*)&m_refCount);
#endif
        if (count == 0)
            delete this;
        return (UInt32)count;
    }
    // Mac 0xc86e0, Win 0x480700
    virtual void OnBegin(PhysicsSolver* /*solver*/) { m_scene->GetTransforms(m_transforms); }
    // Mac 0xc8700, Win 0x40be80
    virtual void OnEnd(bool& /*result*/) {}
    // Mac 0xc8710, Win 0x480710
    virtual void OnResult(bool& success)
    {
        if (success)
            m_scene->ApplySolvedTransforms(m_transforms, success);
    }
    // Mac 0xc8740, Win 0x40be80
    virtual void OnStep(Real32 /*time*/) {}
    // Mac 0xc8750 / 0xc87a0, Win sdd 0x480840 (body 0x4807c0)
    virtual ~MoveTogetherSolverContext() {}

    BasicArray<Pair<PrimitiveElement*, Matrix3D> > m_transforms;          // +0x04 (OnBegin)
    CanConnectPhysicsCollisionCallbackHelper       m_collisionCallback;   // +0x14
    BasicArray<PhysicsGeometryIdentifier>          m_features;            // +0x18
    Scene*                                         m_scene;               // +0x28 (not referenced)
    UInt32                                         m_refCount;            // +0x2c

private:
    MoveTogetherSolverContext(const MoveTogetherSolverContext&);
    MoveTogetherSolverContext& operator=(const MoveTogetherSolverContext&);
};

namespace {

// The physics features of a picked point: the knob feature of a fixed planar point, the whole
// element (feature -1) for an axle field of odd kind. Mac 0xb1910 (ECX = element, EDX = field),
// Win 0x480610
void AddSolverFeatures(PrimitiveElement* element, const ConnectionField* field, UInt32 index,
                       BasicArray<PhysicsGeometryIdentifier>& features)
{
    if (field->GetType() == ConnectionField::TYPE_AXLE) {
        if ((field->GetKind() & 1) == 0)
            return;
        PhysicsGeometryIdentifier identifier;
        identifier.m_element = element->m_physics;
        identifier.m_feature = 0xffffffff;
        features.PushBack(identifier);
    } else if (field->GetType() == ConnectionField::TYPE_PLANAR) {
        const ConnectionFieldPlanar* planar = static_cast<const ConnectionFieldPlanar*>(field);
        if ((planar->GetPoint(index).m_flags & ConnectionPoint::FLAG_FIXED) == 0)
            return;
        element->GetFeature(field->GetIndex(), index, features);
    }
}

// The point frame of a feature in its body: rigid element transform * element transform *
// field transform * snap offset (inline in Mac 0xb1410).
inline void GetFeatureFrame(const Scene::Feature& feature, Matrix3D& frame)
{
    frame.Multiply(feature.m_element->m_rigid->GetLocalTransform(), feature.m_element->m_transform);
    frame.Multiply(feature.m_field->GetTransform());
    frame.Multiply(feature.m_offset);
}

} // namespace

// Mac 0xb1410, Win 0x480a06
PhysicsSolver* Scene::MoveTogether(const Feature& feature, const Feature& otherFeature)
{
    RigidElement* body = feature.m_element->m_rigid->GetRigidElement();
    RigidElement* otherBody = otherFeature.m_element->m_rigid->GetRigidElement();
    PhysicsRigidElement* physicsBody = body->m_body;
    PhysicsRigidElement* otherPhysicsBody = otherBody->m_body;
    Vector3 axis;
    Vector3 position;
    Vector3 otherAxis;
    Vector3 otherPosition;
    UInt32 type = feature.m_field->GetType();
    if (type == ConnectionField::TYPE_AXLE) {
        Matrix3D frame;
        Matrix3D otherFrame;
        GetFeatureFrame(feature, frame);
        axis = Rotate(Vector3(0.0f, 1.0f, 0.0f), frame);
        position = frame.GetTranslation();
        GetFeatureFrame(otherFeature, otherFrame);
        otherAxis = Rotate(Vector3(0.0f, 1.0f, 0.0f), otherFrame);
        otherPosition = otherFrame.GetTranslation();
        // the second axis pointing the same way (world space) and its position moved along it
        // into the plane through the first position
        physicsBody->GetTransform(frame);
        Vector3 worldAxis = Rotate(axis, frame);
        otherPhysicsBody->GetTransform(otherFrame);
        Vector3 otherWorldAxis = Rotate(otherAxis, otherFrame);
        if (worldAxis * otherWorldAxis < 0.0f)
            otherAxis *= -1.0f;
        Plane plane(Rotate(otherAxis, otherFrame), otherPosition * otherFrame);
        Real32 distance = plane.GetDistance(position * frame);
        otherPosition += otherAxis * distance;
    } else if (type == ConnectionField::TYPE_PLANAR) {
        Matrix3D frame;
        GetFeatureFrame(feature, frame);
        axis = Rotate(Vector3(0.0f, 1.0f, 0.0f), frame);
        position = frame.GetTranslation();
        GetFeatureFrame(otherFeature, frame);
        otherAxis = Rotate(Vector3(0.0f, 1.0f, 0.0f), frame);
        otherPosition = frame.GetTranslation();
    }
    MoveTogetherSolverContext* context = new MoveTogetherSolverContext(this);
    AddSolverFeatures(feature.m_element, feature.m_field, feature.m_index, context->m_features);
    AddSolverFeatures(otherFeature.m_element, otherFeature.m_field, otherFeature.m_index, context->m_features);
    const PhysicsGeometryIdentifier* features = context->m_features.IsEmpty() ? 0 : &context->m_features[0];
    PhysicsSolver* result = m_physicsScene->SolveMoveTogether(physicsBody, axis, 0, position, otherPhysicsBody, otherAxis, 0,
                                                      otherPosition, &context->m_collisionCallback, features,
                                                      context->m_features.GetSize(), context);
    context->Release();
    return result;
}

// Mac 0xc1e70 (Win inlined in 0x480710)
void Scene::ApplySolvedTransforms(const BasicArray<Pair<PrimitiveElement*, Matrix3D> >& transforms, bool& success)
{
    BasicArray<Pair<PrimitiveElement*, Matrix3D> > previous;
    BasicArray<Pair<PrimitiveElement*, Matrix3D> > current;
    GetChangedTransforms(transforms, previous, &current);
    if (previous.IsEmpty())
        return;
    BeginUpdate();
    if (!MoveElements(current, false)) {
        success = false;
        MoveElements(previous, false);
    }
    EndUpdate();
}

// Mac 0xb1b90, Win 0x480dd0
StringPullSolverCallback* Scene::CreateStringPullSolverCallback()
{
    StringPullSolverCallback* callback = new StringPullSolverCallback(this);
    IUnknown* handle = m_physicsScene->AddSolverCallback(callback);
    // NOTE: the original releases the handle unchecked; the headless physics returns none
    if (handle != 0)
        handle->Release();
    return callback;
}

// Mac 0xb1bf0, Win 0x480e50
ClickPullSolverCallback* Scene::CreateClickPullSolverCallback()
{
    ClickPullSolverCallback* callback = new ClickPullSolverCallback(this);
    IUnknown* handle = m_physicsScene->AddSolverCallback(callback);
    // NOTE: the original releases the handle unchecked; the headless physics returns none
    if (handle != 0)
        handle->Release();
    return callback;
}

// =============================================================================================
// Spatial index (performance; not in the originals)
//
// Scene::Pick walks every body and recomputes every world transform on each call; loading or
// dragging over a large model runs thousands of Picks. The index caches per body the walk's world
// transforms and bounding spheres (same expressions, same bits) and keeps a uniform grid of the
// primitive elements' spheres (cells of 1.6). Sphere pickers test only the bodies and rigid
// elements holding elements their sphere can meet; large queries use grids of the bodies' and rigid
// elements' spheres; other pickers walk the scene with the cached values.
//
// Exactness (one thread at a time, as its scene): A cache is used only while its inputs are
// unchanged: SceneIndexTouch marks changed bodies, physics transform reports mark moved ones, other
// transforms are compared on every Pick. Bodies join only in Scene::CreateRigidElement and
// AddHingedElement and leave with ~RigidElement; a cache whose body left the scene is dropped when
// met. The grids only narrow the candidates (their extents include Sphere::Intersects' rounding).
// Candidates are tested and visited in the original walk's order, so pickers see the original's
// Visit and Test calls, minus Tests that would fail. Scene::SetIndexChecking runs the original walk
// next to every Pick and counts differences.
// =============================================================================================

namespace {

bool s_indexCheck = false;
UInt32 s_indexMismatches = 0;

const UInt32 kGridMinimum = 24;        // bodies with fewer primitive rigid elements are scanned
const UInt32 kBodyGridMinimum = 32;    // scenes with fewer bodies are walked
const Real32 kBodyCellSize = 6.4f;     // cells of the body grid
const Real32 kElementCellSize = 1.6f;  // cells of the element grid (two 1x1 pitches)
const UInt64 kMaxElementQueryCells = 64;   // larger queries use the body grid
const Int32 kMaxCellSpan = 8;          // spheres over more cells per axis: the oversize lists
const Int32 kCellLimit = 1 << 20;      // cell coordinates are clamped to +-2^20
const UInt64 kMaxQueryCells = 512;     // larger queries walk (or scan the body)

inline bool SameBits(const void* a, const void* b, size_t size)
{
    return memcmp(a, b, size) == 0;
}

inline bool IsFinite(Real32 v)
{
    return v == v && v - v == 0.0f;
}

inline bool IsFinite(const Sphere& s)
{
    return IsFinite(s.m_center.x) && IsFinite(s.m_center.y) && IsFinite(s.m_center.z) && IsFinite(s.m_radius) &&
           s.m_radius >= 0.0f;
}

// Half-extent of a sphere in the grids, with room for the rounding of Sphere::Intersects: a sphere
// pair that passes that test always has overlapping extents.
inline Real32 Extent(const Sphere& s)
{
    const Vector3& c = s.m_center;
    Real32 magnitude = (c.x < 0 ? -c.x : c.x) + (c.y < 0 ? -c.y : c.y) + (c.z < 0 ? -c.z : c.z) + s.m_radius;
    return s.m_radius + magnitude * 1e-4f + 1e-5f;
}

inline Int32 CellOf(Real32 v, Real32 size)
{
    Real32 c = floorf(v / size);
    if (!(c > (Real32)-kCellLimit))
        return -kCellLimit;
    if (c > (Real32)kCellLimit)
        return kCellLimit;
    return (Int32)c;
}

inline UInt64 CellKey(Int32 x, Int32 y, Int32 z)
{
    return ((UInt64)(UInt32)(x + kCellLimit) << 42) | ((UInt64)(UInt32)(y + kCellLimit) << 21) |
           (UInt64)(UInt32)(z + kCellLimit);
}

// The cells of a sphere's extent.
inline void Range(const Sphere& s, Real32 size, Int32 range[6])
{
    const Real32 e = Extent(s);
    const Real32 c[3] = {s.m_center.x, s.m_center.y, s.m_center.z};
    for (int a = 0; a < 3; ++a) {
        range[a] = CellOf(c[a] - e, size);
        range[3 + a] = CellOf(c[a] + e, size);
    }
}

// The cells of a stored sphere: false when it does not fit (too large, or not finite).
inline bool StoredRange(const Sphere& s, Real32 size, Int32 range[6])
{
    if (!IsFinite(s))
        return false;
    Range(s, size, range);
    for (int a = 0; a < 3; ++a)
        if (range[3 + a] - range[a] >= kMaxCellSpan)
            return false;
    return true;
}

inline UInt64 CellCount(const Int32 range[6])
{
    return (UInt64)(range[3] - range[0] + 1) * (UInt64)(range[4] - range[1] + 1) * (UInt64)(range[5] - range[2] + 1);
}

// Records what a picker visits (index checking).
class RecordingPicker : public Scene::Picker {
public:
    explicit RecordingPicker(Scene::Picker& inner) : m_inner(inner) {}
    virtual bool Test(const Sphere& sphere) { return m_inner.Test(sphere); }
    virtual void Visit(PrimitiveElement* element, const Matrix3D& transform, const Sphere& sphere)
    {
        Entry entry;
        entry.element = element;
        memcpy(entry.transform, &transform, sizeof entry.transform);
        memcpy(entry.sphere, &sphere, sizeof entry.sphere);
        m_visits.push_back(entry);
    }
    struct Entry {
        PrimitiveElement* element;
        UInt8             transform[sizeof(Matrix3D)];
        UInt8             sphere[sizeof(Sphere)];
        bool operator==(const Entry& o) const
        {
            return element == o.element && SameBits(transform, o.transform, sizeof transform) &&
                   SameBits(sphere, o.sphere, sizeof sphere);
        }
    };
    std::vector<Entry> m_visits;

private:
    Scene::Picker& m_inner;
};

// The original walk (Mac 0xaa850 / 0xae240), for the index checking.
void LinearPick(const BasicArray<RefPointer<HingedElement> >& hingedElements, Scene::Picker& picker,
                const BasicArray<RigidElement*>* skip)
{
    for (BasicArray<RefPointer<HingedElement> >::ConstIterator h = hingedElements.Begin();
         h != hingedElements.End(); ++h) {
        for (BasicArray<RefPointer<RigidElement> >::ConstIterator b = (*h)->m_elements.Begin();
             b != (*h)->m_elements.End(); ++b) {
            RigidElement* body = *b;
            if (skip != 0 && SceneGraph::Contains(*skip, body))
                continue;
            Matrix3D bodyTransform;
            body->m_body->GetTransform(bodyTransform);
            Sphere bodySphere(body->m_boundingSphere);
            bodySphere.m_center *= bodyTransform;
            if (!picker.Test(bodySphere))
                continue;
            for (BasicArray<RefPointer<PrimitiveRigidElement> >::ConstIterator r = body->m_elements.Begin();
                 r != body->m_elements.End(); ++r) {
                PrimitiveRigidElement* rigid = *r;
                Matrix3D rigidTransform(bodyTransform, rigid->m_transform);
                Sphere rigidSphere(rigid->m_boundingSphere);
                rigidSphere.m_center *= rigidTransform;
                if (!picker.Test(rigidSphere))
                    continue;
                for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = rigid->m_elements.Begin();
                     e != rigid->m_elements.End(); ++e) {
                    PrimitiveElement* element = *e;
                    Matrix3D transform(rigidTransform, element->m_transform);
                    Sphere sphere = BoneSphere(element, transform);
                    if (picker.Test(sphere))
                        picker.Visit(element, transform, sphere);
                }
            }
        }
    }
}

// The query sphere of the pickers whose Test is SpherePicker::Test (null: any other picker).
const Sphere* QuerySphere(Scene::Picker& picker)
{
    if (typeid(picker) == typeid(Scene::PrimitiveSpherePicker) ||
        typeid(picker) == typeid(Scene::HideAwareOrderedElementsSpherePicker))
        return &static_cast<Scene::SpherePicker&>(picker).m_sphere;
    return 0;
}

// Reported transform changes of the physics bodies (PhysicsTransformReporter).
void OnPhysicsTransformChanged(void* userData)
{
    RigidElement* body = static_cast<RigidElement*>(userData);
    if (body->m_indexOwner != 0)
        SceneIndex::Changed(body);
}

struct InstallPhysicsTransformListener {
    InstallPhysicsTransformListener() { GetPhysicsTransformListener() = &OnPhysicsTransformChanged; }
} s_installPhysicsTransformListener;

#if defined(LDD_SCENE_INDEX_CHECK_ENV)
// Test builds (-DLDD_SCENE_INDEX_CHECK_ENV): LDD_SCENE_INDEX_CHECK=1 in the environment turns the
// checking on for the whole program; differences are reported at exit (exit status 99).
void ReportIndexMismatches()
{
    if (s_indexMismatches != 0) {
        fprintf(stderr, "SCENE INDEX: %u Pick calls differ from the original walk\n", (unsigned)s_indexMismatches);
        fflush(stderr);
        _exit(99);
    }
}
struct IndexCheckFromEnvironment {
    IndexCheckFromEnvironment()
    {
        const char* value = getenv("LDD_SCENE_INDEX_CHECK");
        if (value != 0 && value[0] == '1') {
            s_indexCheck = true;
            atexit(&ReportIndexMismatches);
        }
    }
} s_indexCheckFromEnvironment;
#endif

} // namespace

void Scene::SetIndexChecking(bool check)
{
    s_indexCheck = check;
}

UInt32 Scene::GetIndexMismatches()
{
    return s_indexMismatches;
}

SceneIndex::SceneIndex() : m_live(0), m_walk(0), m_stamp(0), m_complete(false)
{
    m_releases = 0;
    m_coneCell = 1.0f;
    m_coneReleases = 0;
    m_coneStampValue = 0;
    m_coneValid = false;
    // what ~RigidElement and SceneIndexTouch call for the bodies an index owns (RigidElement.h)
    SceneIndexHooks& hooks = GetSceneIndexHooks();
    hooks.changed = &SceneIndex::Changed;
    hooks.forget = &SceneIndex::Forget;
}

// The bodies with caches are alive (~RigidElement forgets them): those that still point here
// stop doing so.
SceneIndex::~SceneIndex()
{
    for (UInt32 i = 0; i < m_slots.size(); ++i) {
        BodyCache* cache = m_slots[i].cache;
        if (cache == 0)
            continue;
        RigidElement* body = cache->body;
        if (body->m_indexOwner == this && body->m_indexSlot == i)
            body->m_indexOwner = 0;
        Release(i);
    }
}

SceneIndex* SceneIndex::Owner(const RigidElement* body, UInt32* slot)
{
    SceneIndex* index = body->m_indexOwner;
    if (index == 0 || body->m_indexSlot >= index->m_slots.size())
        return 0;
    const Slot& entry = index->m_slots[body->m_indexSlot];
    if (entry.cache == 0 || entry.generation != body->m_indexGeneration || entry.cache->body != body)
        return 0;
    *slot = body->m_indexSlot;
    return index;
}

void SceneIndex::Changed(RigidElement* body)
{
    UInt32 slot;
    SceneIndex* index = Owner(body, &slot);
    if (index == 0)
        return;
    BodyCache* cache = index->m_slots[slot].cache;
    if (!cache->dirty) {
        cache->dirty = true;
        index->m_dirty.push_back(slot);
    }
}

void SceneIndex::Forget(RigidElement* body)
{
    UInt32 slot;
    SceneIndex* index = Owner(body, &slot);
    if (index != 0)
        index->Release(slot);
    body->m_indexOwner = 0;
}

void SceneIndex::Add(RigidElement* body)
{
    BodyCache* cache = Find(body);
    if (!cache->dirty) {
        cache->dirty = true;
        m_dirty.push_back(cache->slot);
    }
}

SceneIndex::BodyCache* SceneIndex::Find(RigidElement* body)
{
    if (body->m_indexOwner == this && body->m_indexSlot < m_slots.size()) {
        const Slot& slot = m_slots[body->m_indexSlot];
        if (slot.cache != 0 && slot.generation == body->m_indexGeneration && slot.cache->body == body)
            return slot.cache;
    }
    UInt32 index;
    if (!m_free.empty()) {
        index = m_free.back();
        m_free.pop_back();
    } else {
        index = (UInt32)m_slots.size();
        Slot slot = {0, 0};
        m_slots.push_back(slot);
    }
    BodyCache* cache = new BodyCache;
    cache->body = body;
    cache->slot = index;
    m_slots[index].cache = cache;
    ++m_live;
    // a cache in another scene's index stays there until that index meets it
    body->m_indexOwner = this;
    body->m_indexSlot = index;
    body->m_indexGeneration = m_slots[index].generation;
    void* reporter = 0;
    if (body->m_body != 0 && body->m_body->QueryInterface(PhysicsTransformReporter::GetIID(), &reporter) >= 0 &&
        reporter != 0) {
        cache->reports = true;
        body->m_body->Release();
    } else {
        cache->unreported = (UInt32)m_unreported.size();
        m_unreported.push_back(index);
    }
    return cache;
}

// The cached values of a body against the body (its transform from the physics).
void SceneIndex::Update(BodyCache& cache)
{
    RigidElement* body = cache.body;
    Matrix3D bodyTransform;
    body->m_body->GetTransform(bodyTransform);
    if (cache.fresh || cache.version != body->m_indexVersion || cache.count != body->m_elements.GetSize() ||
        cache.data != (const void*)body->m_elements.Begin() ||
        !SameBits(&cache.localSphere, &body->m_boundingSphere, sizeof(Sphere)) ||
        !SameBits(&cache.transform, &bodyTransform, sizeof(Matrix3D)))
        Sync(cache, bodyTransform);
}

// Before a grid query: the marked caches and the bodies that do not report their transform.
void SceneIndex::Prepare()
{
    for (UInt32 i = 0; i < m_dirty.size(); ++i) {
        BodyCache* cache = m_slots[m_dirty[i]].cache;
        if (cache == 0 || !cache->dirty)
            continue;
        cache->dirty = false;
        Update(*cache);
    }
    m_dirty.clear();
    for (UInt32 i = 0; i < m_unreported.size(); ++i)
        Update(*m_slots[m_unreported[i]].cache);
}

void SceneIndex::Pick(const BasicArray<RefPointer<HingedElement> >& hinged, Scene::Picker& picker,
                      const BasicArray<RigidElement*>* skip)
{
    const Sphere* query = QuerySphere(picker);
    if (s_indexCheck) {
        RecordingPicker reference(picker);
        LinearPick(hinged, reference, skip);
        RecordingPicker indexed(picker);
        if (query == 0 || !PickSphere(hinged, indexed, skip, *query))
            Walk(hinged, indexed, skip, query);
        if (!(reference.m_visits == indexed.m_visits))
            ++s_indexMismatches;
    }
    if (query == 0 || !PickSphere(hinged, picker, skip, *query))
        Walk(hinged, picker, skip, query);
}

// The walk of the original over the scene, with the cached values.
void SceneIndex::Walk(const BasicArray<RefPointer<HingedElement> >& hingedElements, Scene::Picker& picker,
                      const BasicArray<RigidElement*>* skip, const Sphere* query,
                      std::vector<const void*>* collect)
{
    ++m_walk;
    UInt32 seen = 0;
    UInt32 hingedPos = 0;
    for (BasicArray<RefPointer<HingedElement> >::ConstIterator h = hingedElements.Begin();
         h != hingedElements.End(); ++h, ++hingedPos) {
        UInt32 bodyPos = 0;
        for (BasicArray<RefPointer<RigidElement> >::ConstIterator b = (*h)->m_elements.Begin();
             b != (*h)->m_elements.End(); ++b, ++bodyPos) {
            RigidElement* body = *b;
            BodyCache* cache = Find(body);
            if (cache->seen != m_walk) {
                cache->seen = m_walk;
                ++seen;
            }
            cache->hingedPos = hingedPos;
            cache->bodyPos = bodyPos;
            if (skip != 0 && SceneGraph::Contains(*skip, body))
                continue;
            // (a reporting body that is not marked is up to date)
            if (cache->dirty || !cache->reports || cache->fresh || cache->version != body->m_indexVersion) {
                cache->dirty = false;
                Update(*cache);
            }
            if (!picker.Test(cache->sphere))
                continue;
            if (collect != 0) {
                collect->push_back(cache);
                continue;
            }
            VisitBody(*cache, picker, query);
        }
    }
    // the marks left: skipped bodies (still marked) and caches of bodies not in the scene (dropped
    // below)
    UInt32 kept = 0;
    for (UInt32 i = 0; i < m_dirty.size(); ++i) {
        BodyCache* cache = m_slots[m_dirty[i]].cache;
        if (cache == 0 || !cache->dirty)
            continue;
        if (cache->seen != m_walk) {
            cache->dirty = false;
            continue;
        }
        m_dirty[kept++] = m_dirty[i];
    }
    m_dirty.resize(kept);
    if (m_live > seen)
        Sweep();
    m_complete = true;
}

void SceneIndex::CollectBodies(const BasicArray<RefPointer<HingedElement> >& hinged, Scene::Picker& bound,
                               std::vector<const void*>& bodies, UInt32& releases)
{
    bodies.clear();
    Walk(hinged, bound, 0, 0, &bodies);
    releases = m_releases;
}

bool SceneIndex::PickBodies(const std::vector<const void*>& bodies, UInt32 releases, Scene::Picker& picker)
{
    if (releases != m_releases)
        return false;
    const Sphere* query = QuerySphere(picker);
    for (std::vector<const void*>::const_iterator it = bodies.begin(); it != bodies.end(); ++it) {
        BodyCache* cache = (BodyCache*)*it;
        RigidElement* body = cache->body;
        // as the walk: the cache brought up to date, the body's sphere tested, the body visited
        if (cache->dirty || !cache->reports || cache->fresh || cache->version != body->m_indexVersion) {
            cache->dirty = false;
            Update(*cache);
        }
        if (!picker.Test(cache->sphere))
            continue;
        VisitBody(*cache, picker, query);
    }
    return true;
}

namespace {
// PickCones: room for the rounding of Cone::Intersects (float) and of the directions (radians)
const double kConeSlack = 0.002;
// the cell of a unit vector component
inline Int32 ConeCell(double v, double cell) { return (Int32)floor(v / cell); }
inline UInt64 ConeKey(Int32 x, Int32 y, Int32 z)
{
    return ((UInt64)(UInt32)(x + (1 << 20)) << 42) | ((UInt64)(UInt32)(y + (1 << 20)) << 21) |
           (UInt64)(UInt32)(z + (1 << 20));
}
} // namespace

// Cone::Intersects accepts a sphere (centre C at distance L from the apex in direction u, radius r)
// only if L <= r / cos^2(angle) or phi <= angle + asin(r / L), phi the angle between u and the
// axis. So each element goes into the direction cells its ball (u, asin(r / L) + room) meets, or
// the near list (L <= 2 r); a cone tests the elements of the cells it meets and the near list, in
// the walk's order.
bool SceneIndex::BuildCones(const std::vector<const void*>& bodies, UInt32 releases, const Vector3& apex)
{
    DropCones();
    if (releases != m_releases || !m_dirty.empty())
        return false;
    struct Ball {
        double x, y, z, radius;
    };
    std::vector<Ball> balls;
    double sum = 0.0;
    UInt32 summed = 0;
    UInt32 rigidCount = 0;
    for (size_t b = 0; b < bodies.size(); ++b) {
        BodyCache* cache = (BodyCache*)bodies[b];
        // a cache the walk would bring up to date first: walk instead
        if (cache->dirty || !cache->reports || cache->fresh || cache->version != cache->body->m_indexVersion) {
            DropCones();
            return false;
        }
        for (std::vector<RigidCache*>::iterator r = cache->rigids.begin(); r != cache->rigids.end(); ++r, ++rigidCount)
            for (std::vector<ElementCache>::iterator e = (*r)->elements.begin(); e != (*r)->elements.end(); ++e) {
                ConeEntry entry = { cache, *r, &*e, (UInt32)b, rigidCount };
                const double dx = (double)e->sphere.m_center.x - apex.x;
                const double dy = (double)e->sphere.m_center.y - apex.y;
                const double dz = (double)e->sphere.m_center.z - apex.z;
                const double length = sqrt(dx * dx + dy * dy + dz * dz);
                const double radius = (double)e->sphere.m_radius * 1.001 + 0.001;
                if (!(length == length) || !(radius == radius) || length > 1e30 || radius > 1e30) {
                    DropCones();
                    return false;
                }
                const UInt32 index = (UInt32)m_coneEntries.size();
                m_coneEntries.push_back(entry);
                Ball ball = { 0.0, 0.0, 0.0, -1.0 };
                if (length <= 2.0 * radius) {
                    m_coneNear.push_back(index);
                } else {
                    ball.x = dx / length;
                    ball.y = dy / length;
                    ball.z = dz / length;
                    ball.radius = asin(radius / length) + kConeSlack;
                    sum += ball.radius;
                    ++summed;
                }
                balls.push_back(ball);
            }
    }
    // cells about twice the mean ball
    double cell = summed != 0 ? 2.0 * sum / summed : 0.1;
    cell = cell < 0.002 ? 0.002 : (cell > 0.25 ? 0.25 : cell);
    std::vector<std::pair<UInt64, UInt32> > pairs;
    pairs.reserve(balls.size() * 8);
    for (UInt32 i = 0; i < (UInt32)balls.size(); ++i) {
        const Ball& ball = balls[i];
        if (ball.radius < 0.0)
            continue;
        const Int32 x0 = ConeCell(ball.x - ball.radius, cell), x1 = ConeCell(ball.x + ball.radius, cell);
        const Int32 y0 = ConeCell(ball.y - ball.radius, cell), y1 = ConeCell(ball.y + ball.radius, cell);
        const Int32 z0 = ConeCell(ball.z - ball.radius, cell), z1 = ConeCell(ball.z + ball.radius, cell);
        if ((double)(x1 - x0 + 1) * (y1 - y0 + 1) * (z1 - z0 + 1) > 64.0) {
            m_coneNear.push_back(i);   // a large ball: a candidate of every cone
            continue;
        }
        for (Int32 x = x0; x <= x1; ++x)
            for (Int32 y = y0; y <= y1; ++y)
                for (Int32 z = z0; z <= z1; ++z)
                    pairs.push_back(std::make_pair(ConeKey(x, y, z), i));
    }
    std::sort(pairs.begin(), pairs.end());
    std::sort(m_coneNear.begin(), m_coneNear.end());
    m_coneCellEntries.resize(pairs.size());
    for (size_t i = 0; i < pairs.size(); ++i) {
        if (i == 0 || pairs[i].first != pairs[i - 1].first) {
            m_coneKeys.push_back(pairs[i].first);
            m_coneStarts.push_back((UInt32)i);
        }
        m_coneCellEntries[i] = pairs[i].second;
    }
    m_coneStarts.push_back((UInt32)pairs.size());
    m_coneEntryStamp.assign(m_coneEntries.size(), 0);
    m_coneBodyStamp.assign(bodies.size(), 0);
    m_coneRigidStamp.assign(rigidCount, 0);
    m_coneApex = apex;
    m_coneCell = (Real32)cell;
    m_coneReleases = releases;
    m_coneStampValue = 0;
    m_coneValid = true;
    return true;
}

bool SceneIndex::PickCones(Scene::ConePicker& picker)
{
    const Cone& cone = picker.m_cone;
    if (!m_coneValid || m_coneReleases != m_releases || !m_dirty.empty() || !(cone.m_angle >= 0.0f) ||
        !(cone.m_angle <= 0.7f) || cone.m_apex.x != m_coneApex.x || cone.m_apex.y != m_coneApex.y ||
        cone.m_apex.z != m_coneApex.z)
        return false;
    const double ax = cone.m_axis.x, ay = cone.m_axis.y, az = cone.m_axis.z;
    const double length = sqrt(ax * ax + ay * ay + az * az);
    if (!(length > 0.999 && length < 1.001))
        return false;
    const double cell = m_coneCell;
    // the chord to a direction within the angle: the axis is unit length up to 0.001
    const double radius = cone.m_angle + kConeSlack + 0.002;
    const Int32 x0 = ConeCell(ax - radius, cell), x1 = ConeCell(ax + radius, cell);
    const Int32 y0 = ConeCell(ay - radius, cell), y1 = ConeCell(ay + radius, cell);
    const Int32 z0 = ConeCell(az - radius, cell), z1 = ConeCell(az + radius, cell);
    if ((double)(x1 - x0 + 1) * (y1 - y0 + 1) * (z1 - z0 + 1) > 4096.0)
        return false;
    if (++m_coneStampValue >= 0x7fffffffu) {
        m_coneStampValue = 1;
        std::fill(m_coneEntryStamp.begin(), m_coneEntryStamp.end(), 0u);
        std::fill(m_coneBodyStamp.begin(), m_coneBodyStamp.end(), 0u);
        std::fill(m_coneRigidStamp.begin(), m_coneRigidStamp.end(), 0u);
    }
    const UInt32 stamp = m_coneStampValue;
    std::vector<UInt32>& candidates = m_coneCandidates;
    candidates.clear();
    for (std::vector<UInt32>::const_iterator n = m_coneNear.begin(); n != m_coneNear.end(); ++n) {
        m_coneEntryStamp[*n] = stamp;
        candidates.push_back(*n);
    }
    for (Int32 x = x0; x <= x1; ++x)
        for (Int32 y = y0; y <= y1; ++y)
            for (Int32 z = z0; z <= z1; ++z) {
                const UInt64 key = ConeKey(x, y, z);
                std::vector<UInt64>::const_iterator k = std::lower_bound(m_coneKeys.begin(), m_coneKeys.end(), key);
                if (k == m_coneKeys.end() || *k != key)
                    continue;
                const size_t c = (size_t)(k - m_coneKeys.begin());
                for (UInt32 i = m_coneStarts[c]; i < m_coneStarts[c + 1]; ++i) {
                    const UInt32 entry = m_coneCellEntries[i];
                    if (m_coneEntryStamp[entry] != stamp) {
                        m_coneEntryStamp[entry] = stamp;
                        candidates.push_back(entry);
                    }
                }
            }
    // the walk's order: by body, rigid element, element (the order of the entries)
    std::sort(candidates.begin(), candidates.end());
    for (std::vector<UInt32>::const_iterator c = candidates.begin(); c != candidates.end(); ++c) {
        const ConeEntry& entry = m_coneEntries[*c];
        UInt32& body = m_coneBodyStamp[entry.bodyIndex];
        if (body >> 1 != stamp)
            body = stamp << 1 | (picker.Test(entry.body->sphere) ? 1u : 0u);
        if ((body & 1) == 0)
            continue;
        UInt32& rigid = m_coneRigidStamp[entry.rigidIndex];
        if (rigid >> 1 != stamp)
            rigid = stamp << 1 | (picker.Test(entry.rigid->sphere) ? 1u : 0u);
        if ((rigid & 1) == 0)
            continue;
        if (picker.Test(entry.element->sphere))
            picker.Visit(entry.element->element, entry.element->world, entry.element->sphere);
    }
    return true;
}

void SceneIndex::DropCones()
{
    m_coneValid = false;
    std::vector<ConeEntry>().swap(m_coneEntries);
    std::vector<UInt32>().swap(m_coneNear);
    std::vector<UInt64>().swap(m_coneKeys);
    std::vector<UInt32>().swap(m_coneStarts);
    std::vector<UInt32>().swap(m_coneCellEntries);
    std::vector<UInt32>().swap(m_coneEntryStamp);
    std::vector<UInt32>().swap(m_coneBodyStamp);
    std::vector<UInt32>().swap(m_coneRigidStamp);
    std::vector<UInt32>().swap(m_coneCandidates);
}

// A sphere picker: the bodies from the body grid, in the walk's order. False: walk instead.
bool SceneIndex::PickSphere(const BasicArray<RefPointer<HingedElement> >& hingedElements, Scene::Picker& picker,
                            const BasicArray<RigidElement*>* skip, const Sphere& query)
{
    if (!m_complete || m_live < kBodyGridMinimum || !IsFinite(query))
        return false;
    if (PickElements(hingedElements, picker, skip, query))
        return true;
    Int32 range[6];
    Range(query, kBodyCellSize, range);
    if (CellCount(range) > kMaxQueryCells)
        return false;
    Prepare();
    ++m_stamp;
    std::vector<BodyCache*>& candidates = m_bodyCandidates;
    candidates.clear();
    for (std::vector<BodyCache*>::iterator it = m_bodyOversize.begin(); it != m_bodyOversize.end(); ++it) {
        (*it)->stamp = m_stamp;
        candidates.push_back(*it);
    }
    for (Int32 x = range[0]; x <= range[3]; ++x)
        for (Int32 y = range[1]; y <= range[4]; ++y)
            for (Int32 z = range[2]; z <= range[5]; ++z) {
                BodyCellMap::iterator cell = m_bodyCells.find(CellKey(x, y, z));
                if (cell == m_bodyCells.end())
                    continue;
                for (std::vector<BodyCache*>::iterator it = cell->second.begin(); it != cell->second.end(); ++it)
                    if ((*it)->stamp != m_stamp) {
                        (*it)->stamp = m_stamp;
                        candidates.push_back(*it);
                    }
            }
    Validate(hingedElements, candidates);
    std::sort(candidates.begin(), candidates.end(),
              [](const BodyCache* a, const BodyCache* b) { return LessPosition(a, b); });
    // (copied: the list is reused by the next query)
    std::vector<BodyCache*> bodies(candidates);
    for (std::vector<BodyCache*>::iterator it = bodies.begin(); it != bodies.end(); ++it) {
        BodyCache* cache = *it;
        if (skip != 0 && SceneGraph::Contains(*skip, cache->body))
            continue;
        cache->dirty = false;
        Update(*cache);
        if (!picker.Test(cache->sphere))
            continue;
        VisitBody(*cache, picker, &query);
    }
    return true;
}

// Drops the candidates whose cache is no longer in use (replaced in another index, or the body
// left the scene's hinged elements) and updates the scene positions of the others.
void SceneIndex::Validate(const BasicArray<RefPointer<HingedElement> >& hingedElements,
                          std::vector<BodyCache*>& candidates)
{
    const UInt32 hingedCount = hingedElements.GetSize();
    UInt32 kept = 0;
    for (UInt32 i = 0; i < candidates.size(); ++i) {
        BodyCache* cache = candidates[i];
        RigidElement* body = cache->body;
        // (a cache the body no longer points to was replaced in another index: dropped)
        if (body->m_indexOwner != this || body->m_indexSlot != cache->slot ||
            body->m_indexGeneration != m_slots[cache->slot].generation) {
            Release(cache->slot);
            continue;
        }
        HingedElement* hinged = body->m_hinged;
        bool found = false;
        if (hinged != 0) {
            if (!(cache->hingedPos < hingedCount && hingedElements[cache->hingedPos].Get() == hinged)) {
                for (UInt32 k = 0; k < hingedCount; ++k)
                    if (hingedElements[k].Get() == hinged) {
                        cache->hingedPos = k;
                        found = true;
                        break;
                    }
            } else {
                found = true;
            }
            if (found) {
                const BasicArray<RefPointer<RigidElement> >& bodies = hinged->m_elements;
                const UInt32 count = bodies.GetSize();
                if (!(cache->bodyPos < count && bodies[cache->bodyPos].Get() == body)) {
                    found = false;
                    for (UInt32 k = 0; k < count; ++k)
                        if (bodies[k].Get() == body) {
                            cache->bodyPos = k;
                            found = true;
                            break;
                        }
                }
            }
        }
        if (!found) {
            body->m_indexOwner = 0;
            Release(cache->slot);
            continue;
        }
        candidates[kept++] = cache;
    }
    candidates.resize(kept);
}

// A sphere query through the element grid: the Test and Visit calls of the original walk for the
// elements whose spheres can meet the query, in its order. False for a query over too many cells.
bool SceneIndex::PickElements(const BasicArray<RefPointer<HingedElement> >& hingedElements, Scene::Picker& picker,
                              const BasicArray<RigidElement*>* skip, const Sphere& query)
{
    Int32 range[6];
    Range(query, kElementCellSize, range);
    if (CellCount(range) > kMaxElementQueryCells)
        return false;
    Prepare();
    std::vector<ElementRef>& refs = m_elementCandidates;
    refs.clear();
    refs.insert(refs.end(), m_elementOversize.begin(), m_elementOversize.end());
    for (Int32 x = range[0]; x <= range[3]; ++x)
        for (Int32 y = range[1]; y <= range[4]; ++y)
            for (Int32 z = range[2]; z <= range[5]; ++z) {
                m_elementCells.Append(CellKey(x, y, z), refs);
            }
    // the bodies, checked and placed (dropped caches are not read again: refs are matched by
    // address against the kept ones)
    ++m_stamp;
    std::vector<BodyCache*>& bodies = m_bodyCandidates;
    bodies.clear();
    for (std::vector<ElementRef>::iterator it = refs.begin(); it != refs.end(); ++it)
        if (it->rigid->owner->stamp != m_stamp) {
            it->rigid->owner->stamp = m_stamp;
            bodies.push_back(it->rigid->owner);
        }
    const size_t before = bodies.size();
    Validate(hingedElements, bodies);
    if (bodies.size() != before) {
        std::vector<BodyCache*> kept(bodies);
        std::sort(kept.begin(), kept.end());
        UInt32 n = 0;
        for (UInt32 i = 0; i < refs.size(); ++i)
            if (std::binary_search(kept.begin(), kept.end(), refs[i].rigid->owner))
                refs[n++] = refs[i];
        refs.resize(n);
    }
    {
        // LessRef's order (positions are unique after Validate, a rigid element's index is its
        // place in the body, and equal keys are the same element read from several cells)
        std::vector<KeyedRef>& keyed = m_keyedCandidates;
        keyed.resize(refs.size());
        for (size_t k = 0; k < refs.size(); ++k) {
            const ElementRef& r = refs[k];
            keyed[k].position = (UInt64)r.rigid->owner->hingedPos << 32 | r.rigid->owner->bodyPos;
            keyed[k].element = (UInt64)r.rigid->index << 32 | r.element;
            keyed[k].ref = r;
        }
        // (a function object: the comparison inlined)
        std::sort(keyed.begin(), keyed.end(), [](const KeyedRef& a, const KeyedRef& b) { return LessKey(a, b); });
        for (size_t k = 0; k < refs.size(); ++k)
            refs[k] = keyed[k].ref;
    }
    // (copied: the lists are reused by nested queries)
    std::vector<ElementRef> work(refs);
    UInt32 i = 0;
    while (i < work.size()) {
        BodyCache* cache = work[i].rigid->owner;
        UInt32 end = i + 1;
        while (end < work.size() && work[end].rigid->owner == cache)
            ++end;
        if (skip == 0 || !SceneGraph::Contains(*skip, cache->body)) {
            const UInt32 syncs = cache->syncs;
            cache->dirty = false;
            Update(*cache);
            if (cache->syncs != syncs) {
                // changed since the grid was read: the body's own walk
                if (picker.Test(cache->sphere))
                    VisitBody(*cache, picker, &query);
            } else if (picker.Test(cache->sphere)) {
                RigidCache* rigid = 0;
                bool rigidPassed = false;
                for (UInt32 k = i; k < end; ++k) {
                    const ElementRef& ref = work[k];
                    if (k > i && ref.rigid == work[k - 1].rigid && ref.element == work[k - 1].element)
                        continue;   // in several cells
                    if (ref.rigid != rigid) {
                        rigid = ref.rigid;
                        rigidPassed = picker.Test(rigid->sphere);
                    }
                    if (!rigidPassed)
                        continue;
                    ElementCache& e = rigid->elements[ref.element];
                    if (picker.Test(e.sphere))
                        picker.Visit(e.element, e.world, e.sphere);
                }
            }
        }
        i = end;
    }
    return true;
}

SceneIndex::ElementGrid::~ElementGrid()
{
    for (size_t i = 0; i < m_blocks.size(); ++i)
        delete[] m_blocks[i];
}

size_t SceneIndex::ElementGrid::Find(UInt64 key) const
{
    if (m_cells.empty())
        return 0;
    const size_t mask = m_cells.size() - 1;
    UInt64 h = key * 0x9e3779b97f4a7c15ull;
    for (size_t i = (size_t)(h ^ (h >> 29)) & mask;; i = (i + 1) & mask) {
        if (m_cells[i].key == key)
            return i;
        if (m_cells[i].key == Empty())
            return m_cells.size();
    }
}

void SceneIndex::ElementGrid::Rehash(size_t capacity)
{
    std::vector<Cell> old;
    old.swap(m_cells);
    Cell empty = {Empty(), kNone};
    m_cells.assign(capacity, empty);
    const size_t mask = capacity - 1;
    for (size_t j = 0; j < old.size(); ++j) {
        if (old[j].key == Empty() || old[j].key == Dead())
            continue;
        UInt64 h = old[j].key * 0x9e3779b97f4a7c15ull;
        size_t i = (size_t)(h ^ (h >> 29)) & mask;
        while (m_cells[i].key != Empty())
            i = (i + 1) & mask;
        m_cells[i] = old[j];
    }
    m_dead = 0;
}

void SceneIndex::ElementGrid::Add(UInt64 key, const ElementRef& ref)
{
    size_t slot = Find(key);
    if (slot == m_cells.size()) {
        // a new cell: at most 3/4 of the table in use (dead cells count until the next rehash)
        if ((m_used + m_dead + 1) * 4 > m_cells.size() * 3) {
            size_t capacity = 16;
            while ((m_used + 1) * 4 > capacity * 3)
                capacity *= 2;
            Rehash(capacity);
        }
        const size_t mask = m_cells.size() - 1;
        UInt64 h = key * 0x9e3779b97f4a7c15ull;
        slot = (size_t)(h ^ (h >> 29)) & mask;
        while (m_cells[slot].key != Empty() && m_cells[slot].key != Dead())
            slot = (slot + 1) & mask;
        if (m_cells[slot].key == Dead())
            --m_dead;
        m_cells[slot].key = key;
        m_cells[slot].head = kNone;
        ++m_used;
    }
    UInt32 n = m_freeNode;
    if (n != kNone) {
        m_freeNode = NodeAt(n).next;
    } else {
        if ((m_nodeCount >> kBlockBits) == m_blocks.size())
            m_blocks.push_back(new Node[(size_t)1 << kBlockBits]);
        n = m_nodeCount++;
    }
    Node& node = NodeAt(n);
    node.rigid = ref.rigid;
    node.element = ref.element;
    node.next = m_cells[slot].head;
    m_cells[slot].head = n;
}

void SceneIndex::ElementGrid::Remove(UInt64 key, const ElementRef& ref)
{
    const size_t slot = Find(key);
    if (slot == m_cells.size())
        return;
    UInt32* link = &m_cells[slot].head;
    while (*link != kNone) {
        Node& node = NodeAt(*link);
        if (node.rigid == ref.rigid && node.element == ref.element) {
            const UInt32 n = *link;
            *link = node.next;
            node.next = m_freeNode;
            m_freeNode = n;
            break;
        }
        link = &node.next;
    }
    if (m_cells[slot].head == kNone) {
        m_cells[slot].key = Dead();
        --m_used;
        ++m_dead;
    }
}

void SceneIndex::ElementGrid::Append(UInt64 key, std::vector<ElementRef>& out) const
{
    const size_t slot = Find(key);
    if (slot == m_cells.size())
        return;
    for (UInt32 n = m_cells[slot].head; n != kNone;) {
        const Node& node = NodeAt(n);
        ElementRef ref = {node.rigid, node.element};
        out.push_back(ref);
        n = node.next;
    }
}

void SceneIndex::AddToElementGrid(RigidCache* rigid, UInt32 element)
{
    ElementCells& entry = rigid->gridCells[element];
    ElementRef ref = {rigid, element};
    if (!StoredRange(rigid->elements[element].sphere, kElementCellSize, entry.cells)) {
        entry.state = 2;
        m_elementOversize.push_back(ref);
        return;
    }
    entry.state = 1;
    const Int32* range = entry.cells;
    for (Int32 x = range[0]; x <= range[3]; ++x)
        for (Int32 y = range[1]; y <= range[4]; ++y)
            for (Int32 z = range[2]; z <= range[5]; ++z)
                m_elementCells.Add(CellKey(x, y, z), ref);
}

void SceneIndex::RemoveFromElementGrid(RigidCache* rigid, UInt32 element)
{
    ElementCells& entry = rigid->gridCells[element];
    if (entry.state == 2) {
        for (std::vector<ElementRef>::iterator it = m_elementOversize.begin(); it != m_elementOversize.end(); ++it)
            if (it->rigid == rigid && it->element == element) {
                *it = m_elementOversize.back();
                m_elementOversize.pop_back();
                break;
            }
    } else if (entry.state == 1) {
        const Int32* range = entry.cells;
        for (Int32 x = range[0]; x <= range[3]; ++x)
            for (Int32 y = range[1]; y <= range[4]; ++y)
                for (Int32 z = range[2]; z <= range[5]; ++z) {
                    ElementRef ref = {rigid, element};
                    m_elementCells.Remove(CellKey(x, y, z), ref);
                }
    }
    entry.state = 0;
}

void SceneIndex::RemoveFromElementGrid(RigidCache* rigid)
{
    for (UInt32 e = 0; e < rigid->gridCells.size(); ++e)
        RemoveFromElementGrid(rigid, e);
}

// The primitive rigid elements of a body whose sphere passed: through the body's grid for a
// sphere query, else all of them.
void SceneIndex::VisitBody(BodyCache& cache, Scene::Picker& picker, const Sphere* query)
{
    if (query != 0 && cache.grid && IsFinite(*query)) {
        Int32 range[6];
        Range(*query, cache.cellSize, range);
        if (CellCount(range) <= kMaxQueryCells) {
            ++m_stamp;
            std::vector<RigidCache*>& candidates = m_candidates;
            candidates.clear();
            for (std::vector<RigidCache*>::iterator it = cache.oversize.begin(); it != cache.oversize.end(); ++it) {
                (*it)->stamp = m_stamp;
                candidates.push_back(*it);
            }
            for (Int32 x = range[0]; x <= range[3]; ++x)
                for (Int32 y = range[1]; y <= range[4]; ++y)
                    for (Int32 z = range[2]; z <= range[5]; ++z) {
                        CellMap::iterator cell = cache.cells.find(CellKey(x, y, z));
                        if (cell == cache.cells.end())
                            continue;
                        for (std::vector<RigidCache*>::iterator it = cell->second.begin(); it != cell->second.end(); ++it)
                            if ((*it)->stamp != m_stamp) {
                                (*it)->stamp = m_stamp;
                                candidates.push_back(*it);
                            }
                    }
            // in the order of the body's array, as the original walk
            std::sort(candidates.begin(), candidates.end(), LessIndex);
            for (std::vector<RigidCache*>::iterator it = candidates.begin(); it != candidates.end(); ++it) {
                RigidCache* rigid = *it;
                if (!picker.Test(rigid->sphere))
                    continue;
                for (std::vector<ElementCache>::iterator e = rigid->elements.begin(); e != rigid->elements.end(); ++e)
                    if (picker.Test(e->sphere))
                        picker.Visit(e->element, e->world, e->sphere);
            }
            return;
        }
    }
    for (std::vector<RigidCache*>::iterator it = cache.rigids.begin(); it != cache.rigids.end(); ++it) {
        RigidCache* rigid = *it;
        if (!picker.Test(rigid->sphere))
            continue;
        for (std::vector<ElementCache>::iterator e = rigid->elements.begin(); e != rigid->elements.end(); ++e)
            if (picker.Test(e->sphere))
                picker.Visit(e->element, e->world, e->sphere);
    }
}

// The walk's values of a primitive rigid element (Scene::Pick's expressions). `reuse`: entries
// whose element transform and sphere did not change are kept.
void SceneIndex::Compute(RigidCache& cache, const Matrix3D& bodyTransform, bool reuse)
{
    PrimitiveRigidElement* rigid = cache.rigid;
    Matrix3D rigidTransform(bodyTransform, rigid->m_transform);
    if (reuse && !SameBits(&rigidTransform, &cache.world, sizeof(Matrix3D)))
        reuse = false;
    cache.local = rigid->m_transform;
    cache.localSphere = rigid->m_boundingSphere;
    Sphere rigidSphere(rigid->m_boundingSphere);
    rigidSphere.m_center *= rigidTransform;
    cache.world = rigidTransform;
    cache.sphere = rigidSphere;
    const UInt32 kept = reuse ? (UInt32)cache.elements.size() : 0;
    const UInt32 count = rigid->m_elements.GetSize();
    // the element grid entries of the elements that go or are recomputed leave it first
    for (UInt32 k = count; k < (UInt32)cache.elements.size(); ++k)
        RemoveFromElementGrid(&cache, k);
    cache.elements.resize(count);
    cache.gridCells.resize(count);
    UInt32 i = 0;
    for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = rigid->m_elements.Begin();
         e != rigid->m_elements.End(); ++e, ++i) {
        PrimitiveElement* element = *e;
        ElementCache& entry = cache.elements[i];
        if (i < kept && entry.element == element && SameBits(&entry.local, &element->m_transform, sizeof(Matrix3D)) &&
            SameBits(&entry.localSphere, &element->m_boundingSphere, sizeof(Sphere)))
            continue;
        RemoveFromElementGrid(&cache, i);
        entry.element = element;
        entry.local = element->m_transform;
        entry.localSphere = element->m_boundingSphere;
        Matrix3D transform(rigidTransform, element->m_transform);
        entry.world = transform;
        entry.sphere = BoneSphere(element, transform);
        AddToElementGrid(&cache, i);
    }
}

bool SceneIndex::Unchanged(const RigidCache& cache) const
{
    const PrimitiveRigidElement* rigid = cache.rigid;
    if (!SameBits(&cache.local, &rigid->m_transform, sizeof(Matrix3D)) ||
        !SameBits(&cache.localSphere, &rigid->m_boundingSphere, sizeof(Sphere)) ||
        cache.elements.size() != rigid->m_elements.GetSize())
        return false;
    UInt32 i = 0;
    for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = rigid->m_elements.Begin();
         e != rigid->m_elements.End(); ++e, ++i) {
        const ElementCache& entry = cache.elements[i];
        const PrimitiveElement* element = *e;
        if (entry.element != element || !SameBits(&entry.local, &element->m_transform, sizeof(Matrix3D)) ||
            !SameBits(&entry.localSphere, &element->m_boundingSphere, sizeof(Sphere)))
            return false;
    }
    return true;
}

// Brings a body's cache up to date: unchanged entries are kept, the others recomputed.
void SceneIndex::Sync(BodyCache& cache, const Matrix3D& transform)
{
    RigidElement* body = cache.body;
    const bool moved = cache.fresh || !SameBits(&cache.transform, &transform, sizeof(Matrix3D));
    cache.fresh = false;
    cache.transform = transform;
    cache.version = body->m_indexVersion;
    cache.count = body->m_elements.GetSize();
    cache.data = body->m_elements.Begin();
    cache.localSphere = body->m_boundingSphere;
    Sphere bodySphere(body->m_boundingSphere);
    bodySphere.m_center *= transform;
    if (cache.topState == 0 || !SameBits(&cache.sphere, &bodySphere, sizeof(Sphere))) {
        RemoveFromBodyGrid(&cache);
        cache.sphere = bodySphere;
        AddToBodyGrid(&cache);
    }

    std::vector<RigidCache*> old;
    old.swap(cache.rigids);
    std::map<PrimitiveRigidElement*, UInt32> lookup;   // built when the arrays stop matching
    bool lookupBuilt = false;
    const bool useGrid = cache.count >= kGridMinimum;
    const bool rebuild = useGrid && (!cache.grid || moved);
    cache.rigids.reserve(cache.count);
    UInt32 i = 0;
    for (BasicArray<RefPointer<PrimitiveRigidElement> >::ConstIterator r = body->m_elements.Begin();
         r != body->m_elements.End(); ++r, ++i) {
        PrimitiveRigidElement* rigid = *r;
        RigidCache* entry = 0;
        if (i < old.size() && old[i] != 0 && old[i]->rigid == rigid) {
            entry = old[i];
            old[i] = 0;
        } else {
            if (!lookupBuilt) {
                for (UInt32 k = 0; k < old.size(); ++k)
                    if (old[k] != 0)
                        lookup[old[k]->rigid] = k;
                lookupBuilt = true;
            }
            std::map<PrimitiveRigidElement*, UInt32>::iterator found = lookup.find(rigid);
            if (found != lookup.end() && old[found->second] != 0) {
                entry = old[found->second];
                old[found->second] = 0;
                lookup.erase(found);
            }
        }
        if (entry == 0) {
            entry = new RigidCache;
            entry->rigid = rigid;
            entry->owner = &cache;
            Compute(*entry, transform, false);
            if (useGrid && !rebuild)
                AddToGrid(cache, entry);
        } else if (moved || !Unchanged(*entry)) {
            Sphere before(entry->sphere);
            Compute(*entry, transform, !moved);
            if (useGrid && !rebuild && !SameBits(&before, &entry->sphere, sizeof(Sphere))) {
                RemoveFromGrid(cache, entry);
                AddToGrid(cache, entry);
            }
        }
        entry->index = i;
        cache.rigids.push_back(entry);
    }
    for (std::vector<RigidCache*>::iterator it = old.begin(); it != old.end(); ++it) {
        if (*it == 0)
            continue;
        if (cache.grid)
            RemoveFromGrid(cache, *it);
        RemoveFromElementGrid(*it);
        delete *it;
    }
    if (!useGrid) {
        cache.grid = false;
        cache.cells.clear();
        cache.oversize.clear();
    } else if (rebuild) {
        BuildGrid(cache);
    }
    ++cache.syncs;
}

void SceneIndex::BuildGrid(BodyCache& cache)
{
    cache.cells.clear();
    cache.oversize.clear();
    // cell size: twice the mean radius of the finite spheres
    double sum = 0.0;
    UInt32 count = 0;
    for (std::vector<RigidCache*>::iterator it = cache.rigids.begin(); it != cache.rigids.end(); ++it) {
        Real32 r = (*it)->sphere.m_radius;
        if (IsFinite(r) && r > 0.0f) {
            sum += r;
            ++count;
        }
    }
    Real32 size = count != 0 ? (Real32)(2.0 * sum / count) : 1.0f;
    if (!(size > 0.05f))
        size = 0.05f;
    cache.cellSize = size;
    cache.grid = true;
    for (std::vector<RigidCache*>::iterator it = cache.rigids.begin(); it != cache.rigids.end(); ++it) {
        (*it)->gridded = false;
        AddToGrid(cache, *it);
    }
}

void SceneIndex::AddToGrid(BodyCache& cache, RigidCache* rigid)
{
    if (!cache.grid)
        return;
    Int32* range = rigid->cells;
    if (!StoredRange(rigid->sphere, cache.cellSize, range)) {
        rigid->gridded = false;
        cache.oversize.push_back(rigid);
        return;
    }
    rigid->gridded = true;
    for (Int32 x = range[0]; x <= range[3]; ++x)
        for (Int32 y = range[1]; y <= range[4]; ++y)
            for (Int32 z = range[2]; z <= range[5]; ++z)
                cache.cells[CellKey(x, y, z)].push_back(rigid);
}

void SceneIndex::RemoveFromGrid(BodyCache& cache, RigidCache* rigid)
{
    if (!cache.grid)
        return;
    if (!rigid->gridded) {
        std::vector<RigidCache*>::iterator it = std::find(cache.oversize.begin(), cache.oversize.end(), rigid);
        if (it != cache.oversize.end())
            cache.oversize.erase(it);
        return;
    }
    const Int32* range = rigid->cells;
    for (Int32 x = range[0]; x <= range[3]; ++x)
        for (Int32 y = range[1]; y <= range[4]; ++y)
            for (Int32 z = range[2]; z <= range[5]; ++z) {
                CellMap::iterator cell = cache.cells.find(CellKey(x, y, z));
                if (cell == cache.cells.end())
                    continue;
                std::vector<RigidCache*>& list = cell->second;
                std::vector<RigidCache*>::iterator it = std::find(list.begin(), list.end(), rigid);
                if (it != list.end())
                    list.erase(it);
                if (list.empty())
                    cache.cells.erase(cell);
            }
    rigid->gridded = false;
}

void SceneIndex::AddToBodyGrid(BodyCache* cache)
{
    if (!StoredRange(cache->sphere, kBodyCellSize, cache->topCells)) {
        cache->topState = 2;
        m_bodyOversize.push_back(cache);
        return;
    }
    cache->topState = 1;
    const Int32* range = cache->topCells;
    cache->topSlots.clear();
    for (Int32 x = range[0]; x <= range[3]; ++x)
        for (Int32 y = range[1]; y <= range[4]; ++y)
            for (Int32 z = range[2]; z <= range[5]; ++z) {
                std::vector<BodyCache*>& list = m_bodyCells[CellKey(x, y, z)];
                cache->topSlots.push_back((UInt32)list.size());
                list.push_back(cache);
            }
}

// LDD64: the place of cell (x, y, z) in a body's cell range `range` (the order of topSlots)
static UInt32 TopCellIndex(const Int32* range, Int32 x, Int32 y, Int32 z)
{
    const UInt32 ny = (UInt32)(range[4] - range[1] + 1), nz = (UInt32)(range[5] - range[2] + 1);
    return ((UInt32)(x - range[0]) * ny + (UInt32)(y - range[1])) * nz + (UInt32)(z - range[2]);
}

void SceneIndex::RemoveFromBodyGrid(BodyCache* cache)
{
    if (cache->topState == 2) {
        std::vector<BodyCache*>::iterator it = std::find(m_bodyOversize.begin(), m_bodyOversize.end(), cache);
        if (it != m_bodyOversize.end())
            m_bodyOversize.erase(it);
    } else if (cache->topState == 1) {
        const Int32* range = cache->topCells;
        // the body's place in each list is known (topSlots): the last body of the list takes it
        // (PickSphere sorts what it collects, so the order in a cell does not matter)
        UInt32 n = 0;
        for (Int32 x = range[0]; x <= range[3]; ++x)
            for (Int32 y = range[1]; y <= range[4]; ++y)
                for (Int32 z = range[2]; z <= range[5]; ++z) {
                    BodyCellMap::iterator cell = m_bodyCells.find(CellKey(x, y, z));
                    const UInt32 slot = cache->topSlots[n++];
                    if (cell == m_bodyCells.end())
                        continue;
                    std::vector<BodyCache*>& list = cell->second;
                    BodyCache* last = list.back();
                    if (last != cache) {
                        list[slot] = last;
                        last->topSlots[TopCellIndex(last->topCells, x, y, z)] = slot;
                    }
                    list.pop_back();
                    if (list.empty())
                        m_bodyCells.erase(cell);
                }
    }
    cache->topState = 0;
}

// Drops the caches of bodies that the last walk did not meet (no longer in the scene).
void SceneIndex::Sweep()
{
    for (UInt32 i = 0; i < m_slots.size(); ++i) {
        BodyCache* cache = m_slots[i].cache;
        if (cache != 0 && cache->seen != m_walk) {
            RigidElement* body = cache->body;
            if (body->m_indexOwner == this && body->m_indexSlot == i && body->m_indexGeneration == m_slots[i].generation)
                body->m_indexOwner = 0;
            Release(i);
        }
    }
}

void SceneIndex::Release(UInt32 index)
{
    ++m_releases;
    BodyCache* cache = m_slots[index].cache;
    RemoveFromBodyGrid(cache);
    for (std::vector<RigidCache*>::iterator it = cache->rigids.begin(); it != cache->rigids.end(); ++it)
        RemoveFromElementGrid(*it);
    if (cache->unreported != ~0u) {
        UInt32 last = m_unreported.back();
        m_unreported[cache->unreported] = last;
        m_slots[last].cache->unreported = cache->unreported;
        m_unreported.pop_back();
    }
    for (std::vector<RigidCache*>::iterator it = cache->rigids.begin(); it != cache->rigids.end(); ++it)
        delete *it;
    delete cache;
    m_slots[index].cache = 0;
    ++m_slots[index].generation;
    m_free.push_back(index);
    --m_live;
}

} // namespace LEGO
