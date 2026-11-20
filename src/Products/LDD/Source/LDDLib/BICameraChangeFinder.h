// LEGO Digital Designer — restored source
// LEGO::BICameraChangeFinder — chooses the cameras of the automatic building guide. The
//   destructive generator feeds it bricks in removal order (AddBrick); occlusion queries on the
//   main thread measure how much of each brick every candidate camera (one per BICameraPosition)
//   sees. Bricks are grouped into steps (at most m_maxStepBricks) sharing a camera that sees each
//   brick >= m_threshold; GetEvents picks one camera per step with the fewest changes
//   (BICameraGraph) and returns the bricks in building order with a camera BIEventStep per step.
//   Without occlusion queries every brick counts as fully visible from the first camera.
// LEGO::BICameraGraph, BICameraNode, BICameraNodeLess — that path search.
// LEGO::BICameraPosition — a camera direction (from the model's centre) and whether it is
//   above the model.
// LEGO::BICameraChangeFinderInitializeRIM / ClearMeshMapRIM / OcclusionTesterRIM — the
//   RunInMainThreadObserver jobs (RIM = run in main thread) that touch the renderer.
//
// Home TU 356 "BICameraChangeFinder.cpp" (Mac 0x336870-0x33ed6f, modmap guess
// BICameraChangeFinderInitializeRIM.cpp; Win 0x57ccc0-0x5826ff). The Win functions of this TU
// are compiled without optimisation (every intermediate stored): their structure is the
// source's; the Mac build inlines most helpers (fields of view, TestDistances, Fit).
// RTTI: BICameraChangeFinder : Object; BICameraGraph : Object; BICameraNode : Object;
// BICameraChangeFinder::TestDistancesCollisionVolumeVisitor : PrimitiveElement::
// CollisionVolumeVisitor; the three RIMs : RunInMainThreadObserver.
// Vtables: BICameraChangeFinder Mac 0x74cfcc {D1 0x339d10, D0 0x339d20}, Win 0xa2c3d4
// {sdd 0x57fc00}; BICameraGraph Mac 0x74cfb0 {D1 0x3378e0, D0 0x3378f0}, Win sdd 0x57d670;
// BICameraNode Mac 0x777978 {D1 0x33ae20, D0 0x33ae80}, Win sdd 0x57cdc0; the visitor Win
// 0xa2c3f0 {capsule 0x5815c0, box 0x581380, sphere 0x581330} (Mac sphere 0x33aa90, box
// 0x33aaf0, capsule 0x33ad60).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Geometry.h"
#include "StorageScene2.h"
#include "PrimitiveElement.h"
#include "SystemUpdater.h"
#include "Texture.h"
#include "Material.h"
#include "Mesh.h"

namespace LEGO {

class BIBrick;
class BIBrickPrimitive;
class BIOcclusionCull;   // BICameraChangeFinder.cpp
class BIEvent;
class Render;
class Texture;
class Material;
class SingleMesh;
class VertexBuffer;

typedef BasicArray<RefPointer<StorageScene2::Camera> > BICameraList;   // (restoration aid)

// A camera direction and its priority (BICameraGraph::Priority: 1 above the model, 0 below).
// sizeof 0x10 (both). BasicArray<BICameraPosition> Mac vtable 0x777e38, Win sdd 0x56da60.
struct BICameraPosition {
    BICameraPosition() : m_priority(0) {}
    // Mac 0x337930 (name inferred)
    BICameraPosition(const Vector3& direction, bool above) : m_direction(direction), m_priority(above ? 1 : 0) {}

    Vector3 m_direction;   // +0x00
    UInt32  m_priority;    // +0x0c BICameraGraph::Priority
};

// One candidate camera of one step. sizeof 0x18 (Mac). Constructor inlined (Mac 0x336870).
class BICameraNode : public Object {
public:
    BICameraNode(UInt32 layer, StorageScene2::Camera* camera)
        : m_layer(layer), m_cost(0xffffffff), m_previous(0), m_camera(camera) {}
    virtual ~BICameraNode();                                               // Mac 0x33ae20 / 0x33ae80, Win sdd 0x57cdc0

    UInt32                            m_layer;      // +0x08 the step (layer index)
    UInt32                            m_cost;       // +0x0c path cost (0xffffffff: not reached)
    BICameraNode*                     m_previous;   // +0x10 the node before on the cheapest path
    RefPointer<StorageScene2::Camera> m_camera;     // +0x14
};

// Orders the open nodes of the search by cost. (RTTI BasicMultiset<RefPointer<BICameraNode>,
// BICameraNodeLess>, Mac vtable 0x777e20, Win sdd 0x5826a0)
struct BICameraNodeLess {
    bool operator()(const RefPointer<BICameraNode>& a, const RefPointer<BICameraNode>& b) const
    {
        return a->m_cost < b->m_cost;
    }
};

// Mac sizeof 0x34: +0x08 m_layers, +0x18 m_priorities. Created by GetEvents (constructor inlined).
class BICameraGraph : public Object {
public:
    // Priority of a camera (RTTI BasicMap<Camera*, BICameraGraph::Priority>). (names inferred)
    enum Priority {
        PriorityBelow = 0,
        PriorityAbove = 1
    };
    typedef BasicMap<StorageScene2::Camera*, Priority> PriorityMap;

    BICameraGraph() {}
    virtual ~BICameraGraph();                                              // Mac 0x3378e0 (body 0x337760) / 0x3378f0, Win 0x57d5f0 / sdd 0x57d670

    // Appends a layer (a step) with a node per camera; unknown cameras get PriorityAbove.
    // Mac 0x336870 (name inferred)
    void AddLayer(const BICameraList& cameras);
    // The cheapest path through the layers, one camera per layer, first layer first (Dijkstra:
    // start cost 0 above / 6 below, edge costs GetDistance); empties the graph. Mac 0x336f60
    // (name inferred)
    void FindPath(BICameraList& path);
    // Cost of going from camera `a` to camera `b`: above → above 1 (same camera) or 2, below
    // → above 3, below → below 4 or 5, above → below 6. Mac 0x337610 (name inferred)
    UInt8 GetDistance(StorageScene2::Camera* a, StorageScene2::Camera* b);

    BasicArray<BasicArray<RefPointer<BICameraNode> > > m_layers;       // +0x08
    PriorityMap                                        m_priorities;   // +0x18
};

class BICameraChangeFinder : public Object {
public:
    // Collects the planes a camera must stay behind (PushPlanes for every collision volume).
    // Mac vtable (slots: sphere 0x33aa90, box 0x33aaf0, capsule 0x33ad60), Win 0xa2c3f0;
    // constructor Win 0x57ccc0 (inlined on Mac).
    class TestDistancesCollisionVolumeVisitor : public PrimitiveElement::CollisionVolumeVisitor {
    public:
        TestDistancesCollisionVolumeVisitor(BICameraChangeFinder* finder, Plane* planes)
            : m_finder(finder), m_planes(planes) {}
        // PushPlanes for a sphere (radius + 0.025) / the 8 corners of a box (half size + 0.025) /
        // both end spheres of a capsule (radius + 0.025). Win 0x581330 / 0x581380 / 0x5815c0
        virtual void Visit(PhysicsGeometrySphere* sphere, const Matrix3D& transform);
        virtual void Visit(PhysicsGeometryBox* box, const Matrix3D& transform);
        virtual void Visit(PhysicsGeometryCapsule* capsule, const Matrix3D& transform);

        BICameraChangeFinder* m_finder;   // +0x04
        Plane*                m_planes;   // +0x08 four frustum side planes
    };

    // `render`: the renderer the occlusion queries use (BGAutomaticApplication +0x54);
    // `width` / `height`: size of the occlusion render target; `maxStepBricks`: bricks per
    // step at most; `threshold`: visible fraction a camera needs; `unknown`: stored at +0x1c,
    // not read; `positions`: the camera directions. Creates the render target and the
    // material on the main thread (InitializeRIM); traces "Building Guide: Occlusion testing
    // not supported" when that fails. Mac 0x338110 (C1) / 0x337960, Win 0x57fa60
    BICameraChangeFinder(Render* render, UInt32 width, UInt32 height, UInt32 maxStepBricks, Real32 threshold,
                         UInt32 unknown, const BasicArray<BICameraPosition>& positions);
    virtual ~BICameraChangeFinder();                                       // Mac 0x339d10 (body 0x339950) / 0x339d20, Win 0x5803d0 / sdd 0x57fc00

    // The size of the view the cameras are made for (field of view). Mac 0x338120 (name inferred)
    void SetViewportSize(Real32 width, Real32 height);
    // Starts a sequence over `primitives` (the model): makes one camera per position.
    // Mac 0x338150, Win 0x57fc60 (name inferred)
    void Begin(const BasicSet<BIBrickPrimitive*>& primitives);
    // The next brick removed from the model. Mac 0x338840, Win 0x57fe50 (name inferred)
    bool AddBrick(BIBrick* brick);
    // Appends the bricks in building order with the step events. Mac 0x339260, Win 0x57ff40
    // (name inferred)
    void GetEvents(BasicArray<RefPointer<BIEvent> >& events);

    // ---- helpers (names inferred) -------------------------------------------------------------
    // Runs the InitializeRIM; true when the render target and the material exist. Mac 0x337dd0
    bool Initialize(UInt32 width, UInt32 height);
    // Assigns `brick` to the current step or closes it. Mac 0x338930, Win 0x5805b0
    bool Process(BIBrick* brick);
    // A camera looking from `eye` at the model of `primitives`, fitted (Fit), moved back by a
    // third and at least `minDistance` from its target. Mac 0x338510, Win 0x580db0
    RefPointer<StorageScene2::Camera> MakeCamera(Vector3 eye, const BasicSet<BIBrickPrimitive*>& primitives,
                                                 Real32 width, Real32 height, Real32 minDistance);
    // Moves `eye` and `target` so that the frustum just encloses the collision volumes of
    // `primitives`. Mac 0x339fa0 (inlines the rest), Win 0x581670
    void Fit(const Matrix3D& view, const Matrix3D& projection, Vector3& eye, Vector3& target, Real32 width,
             Real32 height, const BasicSet<BIBrickPrimitive*>& primitives);
    // Pushes the planes out to every collision volume. Inlined on Mac, Win 0x581240
    void TestDistances(const Matrix3D& view, const Matrix3D& projection, Plane* planes,
                       const BasicSet<BIBrickPrimitive*>& primitives);
    // Moves every plane of `planes` (4) back to `point` * `transform` - normal * `radius` when
    // that is behind it. Mac 0x33a8d0, Win 0x5811a0
    void PushPlanes(const Vector3& point, Real32 radius, const Matrix3D& transform, Plane* planes);

    // ---- data (Mac offsets; names inferred) ---------------------------------------------------
    bool                                    m_initialized;     // +0x08
    RefPointer<Texture>                     m_target;          // +0x0c occlusion render target
    RefPointer<Material>                    m_material;        // +0x10 of the occlusion meshes
    Render*                                 m_render;          // +0x14
    Real32                                  m_threshold;       // +0x18
    UInt32                                  m_unknown1c;       // +0x1c
    BasicArray<BICameraPosition>            m_positions;       // +0x20
    BasicMap<const BIBrick*, RefPointer<SingleMesh> > m_meshes;  // +0x30 occlusion meshes
    UInt32                                  m_maxStepBricks;   // +0x4c
    BasicSet<BIBrick*>                      m_currentBricks;   // +0x50 bricks of the open step
    BasicMap<UInt32, BICameraList>          m_stepCameras;     // +0x6c last event of a step -> cameras
    BICameraList                            m_currentCameras;  // +0x88 cameras of the open step
    BICameraList                            m_cameras;         // +0x98 one per position
    BasicSet<BIBrickPrimitive*>             m_remaining;       // +0xa8 primitives still in the model
    BasicArray<RefPointer<BIEvent> >        m_events;          // +0xc4 the bricks added
    Real32                                  m_width;           // +0xd4 SetViewportSize
    Real32                                  m_height;          // +0xd8
    BIOcclusionCull*                        m_cull;            // screen grids of the cameras
};

// ---- the main-thread jobs -----------------------------------------------------------------------

// Asks the renderer for occlusion queries and creates the render target (RGBA, depth, clamp,
// `width` x `height`) and a white ColoredMaterial (shininess 5). Mac sizeof 0x74:
// +0x54 m_render, +0x58 m_width, +0x5c m_height, +0x60 m_supported, +0x64 m_target,
// +0x68 m_material. Mac vtable 0x777c84, Win: constructor inlined (0x57d6d0?), Update 0x57d7e0.
class BICameraChangeFinderInitializeRIM : public RunInMainThreadObserver {
public:
    BICameraChangeFinderInitializeRIM(Render* render, Real32 width, Real32 height)
        : m_render(render), m_width(width), m_height(height), m_supported(false) {}
    virtual ~BICameraChangeFinderInitializeRIM();                          // Mac 0x33dca0 / 0x33dd00, Win sdd 0x57dbe0
    virtual void Update();                                                 // Mac 0x33ddc0, Win 0x57d7e0

    Render*              m_render;      // +0x54
    Real32               m_width;       // +0x58
    Real32               m_height;      // +0x5c
    bool                 m_supported;   // +0x60
    RefPointer<Texture>  m_target;      // +0x64
    RefPointer<Material> m_material;    // +0x68
};

// Clears the mesh cache (the meshes hold render resources). Mac sizeof 0x60 (+0x54 m_meshes).
class BICameraChangeFinderClearMeshMapRIM : public RunInMainThreadObserver {
public:
    explicit BICameraChangeFinderClearMeshMapRIM(BasicMap<const BIBrick*, RefPointer<SingleMesh> >* meshes)
        : m_meshes(meshes) {}
    virtual ~BICameraChangeFinderClearMeshMapRIM();                        // Mac 0x33aef0 / 0x33af90, Win sdd 0x57f9a0
    virtual void Update();                                                 // Mac 0x33b040, Win 0x57f980

    BasicMap<const BIBrick*, RefPointer<SingleMesh> >* m_meshes;   // +0x54
};

// Measures, for every camera, the fraction of m_brick's samples visible past m_occluders.
// Mac sizeof 0xb4: +0x54 m_render, +0x58 m_meshes, +0x5c m_width, +0x60 m_height,
// +0x64 m_results (BasicArray<Real32>), +0x74 m_target, +0x78 m_material, +0x7c m_brick,
// +0x80 m_occluders, +0x9c m_cameras. Constructor Mac 0x339d60.
class BICameraChangeFinderOcclusionTesterRIM : public RunInMainThreadObserver {
public:
    BICameraChangeFinderOcclusionTesterRIM(Render* render, Real32 width, Real32 height,
                                           BasicMap<const BIBrick*, RefPointer<SingleMesh> >* meshes,
                                           RefPointer<Texture> target, RefPointer<Material> material);
    virtual ~BICameraChangeFinderOcclusionTesterRIM();                     // Mac 0x33b350 / 0x33b3b0, Win sdd 0x57f840
    virtual void Update();                                                 // Mac 0x33b470, Win 0x57deb0

    // CountVisible(with the occluders) / CountVisible(alone). Mac 0x33ba90, Win 0x57f6d0
    Real32 ComputeVisibility(BIBrick* brick, const BasicSet<BIBrickPrimitive*>& occluders, StorageScene2::Camera* camera);
    // Samples of `brick` visible from `camera` after drawing the occluders that do not
    // intersect it. Mac 0x33bba0, Win 0x57f210
    UInt32 CountVisible(BIBrick* brick, const BasicSet<BIBrickPrimitive*>& occluders, StorageScene2::Camera* camera);
    // The cached occlusion mesh of `brick`: one box per collision volume. Mac 0x33c010,
    // Win 0x57ebc0
    SingleMesh* GetMesh(BIBrick* brick);
    // Writes the 24 vertices / 36 indices of `box` * `transform` as box `index` of `buffer`.
    // Mac 0x33c560, Win 0x57df40
    void AddBox(Render* render, const BoundingBox& box, const Matrix3D& transform, VertexBuffer* buffer, UInt32 index);

    Render*                                             m_render;      // +0x54
    BasicMap<const BIBrick*, RefPointer<SingleMesh> >*  m_meshes;      // +0x58
    Real32                                              m_width;       // +0x5c
    Real32                                              m_height;      // +0x60
    BasicArray<Real32>                                  m_results;     // +0x64 one per camera
    RefPointer<Texture>                                 m_target;      // +0x74
    RefPointer<Material>                                m_material;    // +0x78
    BIBrick*                                            m_brick;       // +0x7c
    BasicSet<BIBrickPrimitive*>                         m_occluders;   // +0x80
    BICameraList                                        m_cameras;     // +0x9c
    // Set by BICameraChangeFinder::Process instead of m_occluders: the occluders are
    // *m_remaining and m_currentPrimitives, drawn when their screen rectangle overlaps the
    // brick's (m_cull).
    UInt32 CountVisibleCulled(BIBrick* brick, UInt32 cameraIndex, StorageScene2::Camera* camera);

    const BasicSet<BIBrickPrimitive*>*                  m_remaining;
    BasicSet<BIBrickPrimitive*>                         m_currentPrimitives;
    BIOcclusionCull*                                    m_cull;
};

} // namespace LEGO
