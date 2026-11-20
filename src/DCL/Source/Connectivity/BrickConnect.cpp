// LEGO Digital Designer — restored source
// LEGO::BrickConnect (see BrickConnect.h).
//
// Original object file: Scene.cpp (Mac TU 025): the caches sit in sub-TU 025j (Mac
// 0x9e0b0-0x9e680), the constructor in the 025q range (Mac 0xc1fc0), the rest in sub-TU 025r (Mac
// 0xc2860-0xc3220; Win 0x490460-0x490e50, 0x4911d0, 0x4a1ab0).
#include "BrickConnect.h"
#include "PrimitiveElement.h"
#include "Brick.h"
#include "Part.h"
#include "BrickGroup.h"
#include "Scene.h"
#include "ConnectionFieldPlanar.h"
#include "ConnectionFieldAxle.h"
#include "MathUtil.h"

namespace LEGO {

namespace {
// Locks the cache mutex for its lifetime. (inline: pthread_mutex_lock / unlock pairs)
class CacheLock {
public:
#if defined(_WIN32)
    explicit CacheLock(CRITICAL_SECTION& lock) : m_lock(lock) { EnterCriticalSection(&m_lock); }
    ~CacheLock() { LeaveCriticalSection(&m_lock); }
private:
    CRITICAL_SECTION& m_lock;
#else
    explicit CacheLock(pthread_mutex_t& lock) : m_lock(lock) { pthread_mutex_lock(&m_lock); }
    ~CacheLock() { pthread_mutex_unlock(&m_lock); }
private:
    pthread_mutex_t& m_lock;
#endif
    CacheLock(const CacheLock&);
    CacheLock& operator=(const CacheLock&);
};
} // namespace

// Knob geometry boxes (half sizes) and positions. (Mac 0xc1fc0 literals; Win 0xa2dc48-0xa2dc80)
// NOTE: three of the values are one float step away from the nearest float of their short
// decimal (both builds): 0.19 is 0x3e428f5d (Win: the double 0x3fc851eba0000000), the wall
// half width 0.015 is 0x3c75c290 and the tube wall half depth 0.23 is 0x3e6b851e.
static const Real32 KNOB_OFFSET      = 0.190000013f;   // wall boxes of configurations 2-4
static const Real32 KNOB_HEIGHT      = 0.088f;
static const Real32 KNOB_TUBE_HEIGHT = 0.2255f;
static const Real32 KNOB_TUBE_OFFSET = 0.3775f;   // wall boxes of configurations 6 / 7

// Mac 0xc1fc0 (base-object variant), Win 0x490460
BrickConnect::BrickConnect(BrickPhysics* physics, BrickDatabase* database)
    : m_physics(physics), m_database(database), m_knobGeometry(0)
{
    m_lookupTables = new ConnectivityWorldLookupTables(database);
#if defined(_WIN32)
    InitializeCriticalSection(&m_lock);
#else
    pthread_mutexattr_t attributes;
    pthread_mutexattr_init(&attributes);
    pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&m_lock, &attributes);
    pthread_mutexattr_destroy(&attributes);
#endif
    ConnectionFieldPlanar::InitializeTables();
    ConnectionFieldAxle::InitializeTables();
    m_physics->AddRef();

    PhysicsGeometryBox* knob = m_physics->CreateBox(Vector3(0.16f, 0.078f, 0.16f));
    PhysicsGeometryBox* wall = m_physics->CreateBox(Vector3(0.0150000006f, 0.078f, 0.114f));
    PhysicsGeometryBox* tube = m_physics->CreateBox(Vector3(0.317f, 0.2155f, 0.317f));
    PhysicsGeometryBox* tubeWall = m_physics->CreateBox(Vector3(0.0285f, 0.2155f, 0.229999989f));
    PhysicsGeometryConfigable* geometry = m_physics->CreateConfigurableGeometry();
    m_knobGeometry = geometry;

    // four placed boxes, re-used for every configuration (the last two turned by 90 degrees)
    PhysicsPlacedGeometry boxes[4];             // identity transforms (Matrix3D::Identity copies)
    boxes[2].m_transform.SetRotationY(PI * 0.5f);
    boxes[3].m_transform = boxes[2].m_transform;

    // 1: the knob
    boxes[0].m_geometry = knob;
    boxes[0].m_transform.SetTranslation(Vector3(0.0f, KNOB_HEIGHT, 0.0f));
    geometry->SetGeometries(1, boxes, 1);
    // 4: one wall at -x
    boxes[0].m_geometry = wall;
    boxes[0].m_transform.SetTranslation(Vector3(-KNOB_OFFSET, KNOB_HEIGHT, 0.0f));
    geometry->SetGeometries(4, boxes, 1);
    // 3: walls at -x, +x, -z
    boxes[1].m_geometry = wall;
    boxes[2].m_geometry = wall;
    boxes[1].m_transform.SetTranslation(Vector3(KNOB_OFFSET, KNOB_HEIGHT, 0.0f));
    boxes[2].m_transform.SetTranslation(Vector3(0.0f, KNOB_HEIGHT, -KNOB_OFFSET));
    geometry->SetGeometries(3, boxes, 3);
    // 2: walls on all four sides
    boxes[3].m_geometry = wall;
    boxes[3].m_transform.SetTranslation(Vector3(0.0f, KNOB_HEIGHT, KNOB_OFFSET));
    geometry->SetGeometries(2, boxes, 4);
    // 5: the tube
    boxes[0].m_geometry = tube;
    boxes[0].m_transform.SetTranslation(Vector3(0.0f, KNOB_TUBE_HEIGHT, 0.0f));
    geometry->SetGeometries(5, boxes, 1);
    // 6: tube walls on all four sides
    boxes[0].m_geometry = tubeWall;
    boxes[1].m_geometry = tubeWall;
    boxes[2].m_geometry = tubeWall;
    boxes[3].m_geometry = tubeWall;
    boxes[0].m_transform.SetTranslation(Vector3(-KNOB_TUBE_OFFSET, KNOB_TUBE_HEIGHT, 0.0f));
    boxes[1].m_transform.SetTranslation(Vector3(KNOB_TUBE_OFFSET, KNOB_TUBE_HEIGHT, 0.0f));
    boxes[2].m_transform.SetTranslation(Vector3(0.0f, KNOB_TUBE_HEIGHT, -KNOB_TUBE_OFFSET));
    boxes[3].m_transform.SetTranslation(Vector3(0.0f, KNOB_TUBE_HEIGHT, KNOB_TUBE_OFFSET));
    geometry->SetGeometries(6, boxes, 4);
    // 7: tube walls at -x and +x
    geometry->SetGeometries(7, boxes, 2);

    knob->Release();
    wall->Release();
    tube->Release();
    tubeWall->Release();
}

// Mac 0xc2860, Win 0x490a10
Scene* BrickConnect::CreateScene(PhysicsScene* physicsScene)
{
    return new Scene(this, m_physics, physicsScene);
}

// Mac 0xc2810 (Win inlined at its callers)
Scene* BrickConnect::NewScene(UInt32 flags)
{
    PhysicsScene* physicsScene = m_physics->CreatePhysicsScene(flags);
    if (physicsScene == 0)
        return 0;
    Scene* scene = CreateScene(physicsScene);
    physicsScene->Release();
    return scene;
}

// Mac 0xc28c0, Win 0x490a80
Brick* BrickConnect::CreateBrick(UInt32 designID)
{
    return new Brick(designID);
}

// Mac 0xc2a90, Win 0x490af0
Part* BrickConnect::CreatePart(const Primitive* primitive)
{
    return new Part(primitive);
}

// Mac 0xc2b50, Win 0x490be0
BrickGroup* BrickConnect::CreateBrickGroup(Scene* scene)
{
    return new BrickGroup(scene);
}

// Mac 0xc2da0, Win 0x490e10
Brick* BrickConnect::CloneBrick(const AbstractBrick* brick)
{
    return CreateBrick(brick->GetDesignID());
}

// Mac 0xc2dd0, Win 0x490e30
Part* BrickConnect::ClonePart(const Part* part)
{
    return CreatePart(part->GetPrimitive());
}

// Mac 0xc3060 (complete) / 0xc3130 (deleting), body 0xc2e30, thunks 0xc30c0 / 0xc31a0;
// Win 0x490990 / sdd 0x4911d0
BrickConnect::~BrickConnect()
{
    for (BoxMap::Iterator it = m_boxes.Begin(); it != m_boxes.End(); ++it)
        it->second->Release();
    for (SphereMap::Iterator it = m_spheres.Begin(); it != m_spheres.End(); ++it)
        it->second->Release();
    for (CapsuleMap::Iterator it = m_capsules.Begin(); it != m_capsules.End(); ++it)
        it->second->Release();
    m_knobGeometry->Release();
    m_physics->Release();
#if defined(_WIN32)
    DeleteCriticalSection(&m_lock);
#else
    pthread_mutex_destroy(&m_lock);
#endif
}

// Mac 0x9e0b0 (025j)
PhysicsGeometryBox* BrickConnect::GetBox(const Vector3& halfSize)
{
    CacheLock lock(m_lock);
    BoxMap::Iterator it = m_boxes.Find(halfSize);
    if (it != m_boxes.End())
        return it->second;
    PhysicsGeometryBox* box = m_physics->CreateBox(halfSize);
    m_boxes[halfSize] = box;
    return box;
}

// Mac 0x9e280 (025j)
PhysicsGeometrySphere* BrickConnect::GetSphere(Real32 radius)
{
    CacheLock lock(m_lock);
    SphereMap::Iterator it = m_spheres.Find(radius);
    if (it != m_spheres.End())
        return it->second;
    PhysicsGeometrySphere* sphere = m_physics->CreateSphere(radius);
    m_spheres[radius] = sphere;
    return sphere;
}

// Mac 0x9e440 (025j)
PhysicsGeometryCapsule* BrickConnect::GetCapsule(Real32 radius, Real32 length)
{
    CacheLock lock(m_lock);
    Vector3 key(length, radius, 0.0f);
    CapsuleMap::Iterator it = m_capsules.Find(key);
    if (it != m_capsules.End())
        return it->second;
    PhysicsGeometryCapsule* capsule = m_physics->CreateCapsule(radius, length);
    m_capsules[Vector3(length, radius, 0.0f)] = capsule;
    return capsule;
}

// Mac 0xc2ae0, Win 0x490b60
PrimitiveElement* BrickConnect::CreatePrimitiveElement(const Primitive* primitive, UInt32 boneIndex,
                                                       const Matrix3D& transform)
{
    return new PrimitiveElement(primitive, boneIndex, transform, m_physics, this);
}

// Mac 0xc2df0, Win 0x490e50
PrimitiveElement* BrickConnect::ClonePrimitiveElement(const PrimitiveElement* element)
{
    Matrix3D transform;
    element->GetTransform(transform);
    return CreatePrimitiveElement(element->GetPrimitive(), element->GetBoneIndex(), transform);
}

} // namespace LEGO
