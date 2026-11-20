// LEGO Digital Designer — restored source
// TU 368 "BGAutomaticScene.cpp" (Mac 0x359740-0x35a9df): LEGO::BGAutomaticScene,
// LEGO::BICollisionCallback.
#include "BGAutomaticScene.h"
#include "BIBrick.h"
#include "BIBrickPrimitive.h"
#include "BIBrickRepository.h"
#include "BIGraph.h"
#include "Scene.h"
#include "PrimitiveElement.h"
#include "Brick.h"

namespace LEGO {

// Mac 0x359740 (C2) / 0x3598e0 (C1), Win 0x59b580
BGAutomaticScene::BGAutomaticScene(Scene* scene)
    : m_scene(scene)
{
    if (&scene->m_bricks != &m_bricks)
        m_bricks = scene->m_bricks;
}

// Mac 0x35a680 / 0x35a690 (body 0x35a800, thunks 0x35a6d0 / 0x35a6f0), Win sdd 0x59c3b0
BGAutomaticScene::~BGAutomaticScene()
{
}

// Mac 0x35a610, Win 0x59b420
bool BGAutomaticScene::IsEmpty() const
{
    return m_primitives.GetSize() == 0;
}

// Mac 0x35a620, Win 0x59b440
bool BGAutomaticScene::Intersects(const BIBrick* brick) const
{
    return brick->Intersects(m_primitives);
}

// Mac 0x35a640, Win 0x59b470
bool BGAutomaticScene::Contains(const BIBrick* brick) const
{
    return brick->IsContainedIn(m_primitives);
}

// Mac 0x35a660, Win 0x59b4a0
const BasicSet<BIBrickPrimitive*>& BGAutomaticScene::GetPrimitives() const
{
    return m_primitives;
}

// Mac 0x359940, Win 0x59b690
void BGAutomaticScene::GetPrimitives(BasicSet<BIBrickPrimitive*>& primitives) const
{
    if (&primitives != &m_primitives)
        primitives = m_primitives;
}

// Mac 0x35a670, Win 0x5d5710
Scene* BGAutomaticScene::GetModelScene()
{
    return m_scene;
}

// Mac 0x35a5c0, Win 0x59c350
BasicArray<RefPointer<BIGraph> >& BGAutomaticScene::GetGraphs()
{
    return m_graphs;
}

// Mac 0x35a5d0, Win 0x59c390
BIGraph* BGAutomaticScene::FindGraph(BIBrickPrimitive* primitive)
{
    return BIGraph::FindGraph(m_graphs, primitive);
}

// Mac 0x35a5f0, Win 0x59c370
BIGraph* BGAutomaticScene::FindBrickGraph(BIBrick* brick)
{
    return BIGraph::FindBrickGraph(m_graphs, brick);
}

// Mac 0x35a580, Win 0x59c300
bool BGAutomaticScene::CanAddBricks(const BasicSet<BIBrick*>& bricks)
{
    // NOTE: the original walks the bricks without looking at them.
    for (BasicSet<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it) {
    }
    return true;
}

// Mac 0x35a0d0, Win 0x5e4100
bool BGAutomaticScene::IsValid(void* /*argument*/)
{
    return true;
}

// Mac 0x359d00, Win 0x59b9c0
bool BGAutomaticScene::GetInsertDirection(const BIBrick* brick, Vector3& direction, UInt32 mode, bool flag,
                                          BasicSet<BIBrickPrimitive*>& primitives)
{
    BasicSet<BIBrickPrimitive*> placed(GetPrimitives());
    for (BasicSet<BIBrickPrimitive*>::Iterator it = primitives.Begin(); it != primitives.End(); ++it)
        placed.Insert(*it);
    if (!GetRemoveDirection(brick, direction, flag, placed))
        return false;
    if (mode == 0)
        return true;
    // The primitives the brick is connected to that are not placed: they must be insertable
    // (upwards, one level less) on top of the brick.
    BasicSet<BIBrickPrimitive*> resting;
    const BIBrick::ConnectionMap& connections = brick->GetConnections();
    for (BIBrick::ConnectionMap::ConstIterator it = connections.Begin(); it != connections.End(); ++it) {
        if (!it->first->Intersects(placed))
            resting.Insert(it->first);
    }
    for (BasicSet<BIBrickPrimitive*>::Iterator it = resting.Begin(); it != resting.End(); ++it) {
        Vector3 up(0.0f, 1.0f, 0.0f);
        BasicSet<BIBrickPrimitive*> with(primitives);
        const BasicSet<BIBrickPrimitive*>& own = brick->GetPrimitiveSet();
        for (BasicSet<BIBrickPrimitive*>::ConstIterator p = own.Begin(); p != own.End(); ++p)
            with.Insert(*p);
        if (!GetInsertDirection(*it, up, mode - 1, flag, with))
            return false;
    }
    return true;
}

// Mac 0x3599d0, Win 0x59b6b0
bool BGAutomaticScene::GetRemoveDirection(const BIBrick* brick, Vector3& direction, bool flag,
                                          BasicSet<BIBrickPrimitive*>& primitives)
{
    BasicSet<BIBrickPrimitive*> placed(GetPrimitives());
    for (BasicSet<BIBrickPrimitive*>::Iterator it = primitives.Begin(); it != primitives.End(); ++it)
        placed.Insert(*it);
    // The directions allowed by the connections to the first placed primitive that has any.
    BasicArray<Vector3> directions;
    const BIBrick::ConnectionMap& connections = brick->GetConnections();
    for (BIBrick::ConnectionMap::ConstIterator it = connections.Begin(); it != connections.End(); ++it) {
        if (!it->first->Intersects(placed))
            continue;
        const BasicArray<Vector3>& candidates = it->second->m_directions;
        for (BasicArray<Vector3>::ConstIterator d = candidates.Begin(); d != candidates.End(); ++d)
            directions.PushBack(*d);
        if (!directions.IsEmpty())
            break;
    }
    if (directions.IsEmpty()) {
        direction = Vector3(0.0f, 1.0f, 0.0f);
        return true;
    }
    for (BasicArray<Vector3>::Iterator d = directions.Begin(); d != directions.End(); ++d) {
        if (!IsDirectionFree(brick, *d, primitives))
            continue;
        if (flag && !IsMoveFree(brick, *d))
            continue;
        direction = *d;
        return true;
    }
    return false;
}

// Mac 0x35a0e0, Win 0x59bce0
bool BGAutomaticScene::IsDirectionFree(const BIBrick* brick, Vector3 direction,
                                       const BasicSet<BIBrickPrimitive*>& primitives)
{
    Real32 weight = 0.0f;
    BasicSet<BIBrickPrimitive*> placed(GetPrimitives());
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = primitives.Begin(); it != primitives.End(); ++it)
        placed.Insert(*it);
    const BIBrick::ConnectionMap& connections = brick->GetConnections();
    for (BIBrick::ConnectionMap::ConstIterator it = connections.Begin(); it != connections.End(); ++it) {
        if (it->first->Intersects(placed) && !it->second->FindDirection(direction, weight))
            return false;
    }
    return true;
}

// Mac 0x35a290, Win 0x59bf50
bool BGAutomaticScene::IsMoveFree(const BIBrick* brick, Vector3 direction)
{
    const BasicArray<PrimitiveElement*>& elements = brick->GetElements();
    for (BasicArray<PrimitiveElement*>::ConstIterator it = elements.Begin(); it != elements.End(); ++it) {
        PrimitiveElement* element = *it;
        BICollisionCallback callback(brick, element->GetPhysicsElement(), &m_primitives);
        Matrix3D transform;
        element->GetTransform(transform);
        transform.Translate(direction * 10.0f);
        Vector3 position(transform.GetTranslation());
        if (!GetModelScene()->m_physicsScene->TestElementCollision(element->GetPhysicsElement(), position, &callback))
            return false;
    }
    return true;
}

// Mac 0x3598f0, Win 0x59b660
void BGAutomaticScene::Clear()
{
    m_graphs.Clear();
    m_primitives.Clear();
}

// Mac 0x35a3a0, Win 0x59c080
bool BGAutomaticScene::AddBrick(BIBrick* brick, bool update)
{
    if (update) {
        const BasicSet<BIBrickPrimitive*>& primitives = brick->GetPrimitiveSet();
        for (BasicSet<BIBrickPrimitive*>::ConstIterator it = primitives.Begin(); it != primitives.End(); ++it) {
            m_primitives.Insert(*it);
            BIGraph::AddPrimitive(m_graphs, *it);
        }
    }
    return true;
}

// Mac 0x35a410, Win 0x59c110
bool BGAutomaticScene::AddBrick(const BasicSet<BIBrick*>& bricks, bool update)
{
    if (update) {
        for (BasicSet<BIBrick*>::ConstIterator b = bricks.Begin(); b != bricks.End(); ++b) {
            const BasicSet<BIBrickPrimitive*>& primitives = (*b)->GetPrimitiveSet();
            for (BasicSet<BIBrickPrimitive*>::ConstIterator it = primitives.Begin(); it != primitives.End(); ++it) {
                m_primitives.Insert(*it);
                BIGraph::AddPrimitive(m_graphs, *it);
            }
        }
    }
    return true;
}

// Mac 0x35a4b0, Win 0x59c1f0
bool BGAutomaticScene::RemoveBrick(BIBrick* brick, bool update)
{
    if (update) {
        const BasicSet<BIBrickPrimitive*>& primitives = brick->GetPrimitiveSet();
        for (BasicSet<BIBrickPrimitive*>::ConstIterator it = primitives.Begin(); it != primitives.End(); ++it) {
            m_primitives.Erase(*it);
            BIGraph::RemovePrimitive(m_graphs, *it);
        }
    }
    return true;
}

// Mac 0x35a520, Win 0x59c280
bool BGAutomaticScene::RemoveBrick(const BasicSet<BIBrick*>& bricks, bool update)
{
    for (BasicSet<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it) {
        if (!RemoveBrick(*it, update))
            return false;
    }
    return true;
}

// ---------------------------------------------------------------------------------------------

// Mac 0x35a730, Win 0x59be80
UInt32 BICollisionCallback::OnCollision(const PhysicsGeometryIdentifier& feature,
                                        const PhysicsGeometryIdentifier& otherFeature)
{
    PhysicsPrimitiveElement* other = feature.m_element;
    if (other == m_element)
        other = otherFeature.m_element;
    PrimitiveElement* element = other->GetUserData();
    BIBrickPrimitive* primitive = BIBrickRepository::GetInstance().GetPrimitive(element, false);
    if (!m_brick->ContainsElement(element)) {
        if (m_primitives->Contains(primitive) && primitive->IsEnabled()) {
            m_hit = true;
            return Collision;
        }
    }
    return Ignore;
}

} // namespace LEGO
