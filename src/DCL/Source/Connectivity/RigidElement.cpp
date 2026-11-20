// LEGO Digital Designer — restored source
// LEGO::RigidElement (see RigidElement.h).
//
// Original object file: Scene.cpp (Mac TU 025); this is sub-TU 025l, Mac 0xa2760-0xa4ce0 plus Merge
// 0xa0cb0 / GetTransform 0xa2740 (025k) and GetScene 0xc4460 (025s); Win 0x478bf0-0x47a070. The Mac
// range also holds RigidElement::CreateManipulator (0xa2840, with ConnectionManipulator, 025i),
// Scene functions (0xa3230 RemoveRigidElement, 0xa4880 CreateRigidElement — with Scene, 025n),
// HingedElement::Merge (0xa3430, in HingedElement.cpp) and BasicArray / BasicMap instances
// (0xa2a30, 0xa2ce0, 0xa3150 push_back-unique, 0xa4630, 0xa4b10, 0xa4c50, 0xa4c80).
#include "RigidElement.h"
#include "PrimitiveElement.h"
#include "PrimitiveRigidElement.h"
#include "PrimitiveRigidConnection.h"
#include "RigidConnection.h"
#include "HingedElement.h"
#include "Scene.h"
#include "SceneGraphUtil.h"

namespace LEGO {

// Mac 0xa2aa0, Win 0x478e10
RigidElement::RigidElement()
    : m_hinged(0), m_bounding(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), m_boundingSphere(1.0f),
      m_body(0), m_flag60(false)
{
    m_indexVersion = 0;
    m_indexOwner = 0;
    m_indexSlot = 0;
    m_indexGeneration = 0;
}

// Mac 0xa2d10 (complete) / 0xa2dd0 (deleting), base-object 0xa2ba0, thunks 0xa2d70 / 0xa2e40;
// Win 0x478f40 (sdd 0x4a1ad0)
RigidElement::~RigidElement()
{
    if (m_indexOwner != 0)
        GetSceneIndexHooks().forget(this);
    if (m_body != 0)
        m_body->Release();
}

// (added, performance) Set by the spatial index of Scene::Pick (Scene.cpp).
SceneIndexHooks& GetSceneIndexHooks()
{
    static SceneIndexHooks s_hooks = { 0, 0 };
    return s_hooks;
}

// Mac 0xa2760, Win 0x478bf0
void RigidElement::GetBoundingBox(BoundingBox& box, bool recompute) const
{
    if (!recompute) {
        box = m_bounding;
        return;
    }
    box.Reset();
    for (BasicArray<RefPointer<PrimitiveRigidElement> >::ConstIterator it = m_elements.Begin();
         it != m_elements.End(); ++it) {
        const PrimitiveRigidElement* rigid = *it;
        for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = rigid->m_elements.Begin();
             e != rigid->m_elements.End(); ++e) {
            Matrix3D transform(rigid->m_transform, (*e)->m_transform);
            box.Union((*e)->m_primitive->GetBone((*e)->m_boneIndex).m_bounding * transform);
        }
    }
}

// Mac 0xc4460 (025s), Win 0x470600
Scene* RigidElement::GetScene() const
{
    return m_hinged != 0 ? m_hinged->GetScene() : 0;
}

// Mac 0xa2ec0 (thunk 0xa2f20), Win 0x4a1aa0
void RigidElement::OnFinalRelease()
{
    m_elements.Clear();
    SceneIndexTouch(this);
    Object::OnFinalRelease();
}

// Mac 0xa2740 (025k)
void RigidElement::GetTransform(Matrix3D& transform) const
{
    m_body->GetTransform(transform);
}

// Win 0x479f60 (Mac inlined into Split)
void RigidElement::UpdateBounds()
{
    m_bounding.Reset();
    for (BasicArray<RefPointer<PrimitiveRigidElement> >::ConstIterator it = m_elements.Begin();
         it != m_elements.End(); ++it)
        m_bounding.Union((*it)->m_bounding * (*it)->m_transform);
    m_boundingSphere.Set(m_bounding);
    SceneIndexTouch(this);
}

// Mac 0xa2f80, Win 0x479190
void RigidElement::AddElement(PrimitiveRigidElement* element)
{
    element->m_rigidElement = this;
    m_elements.PushBack(RefPointer<PrimitiveRigidElement>(element));

    Matrix3D world;
    m_body->GetTransform(world);
    Matrix3D local(element->m_transform);
    Matrix3D::Solve(world, local);
    element->m_transform = local;
    m_bounding.Union(element->m_bounding * local);
    m_boundingSphere.Set(m_bounding);
    for (BasicArray<RefPointer<PrimitiveElement> >::Iterator it = element->m_elements.Begin();
         it != element->m_elements.End(); ++it)
        m_body->AddPrimitiveElement((*it)->m_physics, Matrix3D(local, (*it)->m_transform));
    SceneIndexTouch(this);
    GetScene()->AddChangedRigidElement(this);
}

// Win 0x479020 (Mac inlined into Scene, 0xbe6c0)
bool RigidElement::AddConnection(RigidElement* other, PrimitiveRigidConnection* connection,
                                 RigidConnection*& rigidConnection)
{
    ConnectionMap::Iterator it = m_connections.Find(other);
    if (it == m_connections.End()) {
        rigidConnection = new RigidConnection(connection);
        return true;
    }
    rigidConnection = it->second;
    return rigidConnection->AddConnection(connection);
}

// Mac 0xa0cb0 (025k), Win 0x479310
RigidElement* RigidElement::Merge(RigidElement* other, BasicArray<RigidConnection*>& changed)
{
    if (other == this)
        return other;
    RigidElement* large = other;
    RigidElement* small = this;
    if (other->m_elements.GetSize() < m_elements.GetSize()) {
        large = this;
        small = other;
    }

    // the small body's transform relative to the large one
    Matrix3D largeWorld;
    large->m_body->GetTransform(largeWorld);
    Matrix3D relative;
    small->m_body->GetTransform(relative);
    Matrix3D::Solve(largeWorld, relative);
    large->m_bounding.Union(small->m_bounding * relative);
    large->m_boundingSphere.Set(large->m_bounding);

    for (BasicArray<RefPointer<PrimitiveRigidElement> >::Iterator it = small->m_elements.Begin();
         it != small->m_elements.End(); ++it) {
        PrimitiveRigidElement* element = *it;
        element->m_rigidElement = large;
        large->m_elements.PushBack(RefPointer<PrimitiveRigidElement>(element));
        element->m_transform = Matrix3D(relative, element->m_transform);
        for (BasicArray<RefPointer<PrimitiveElement> >::Iterator e = element->m_elements.Begin();
             e != element->m_elements.End(); ++e) {
            small->m_body->RemovePrimitiveElement((*e)->m_physics);
            large->m_body->AddPrimitiveElement((*e)->m_physics,
                                               Matrix3D(element->m_transform, (*e)->m_transform));
        }
    }
    small->m_elements.Clear();
    SceneIndexTouch(large);
    SceneIndexTouch(small);

    // the small body's connections: to the large one they disappear, the others move
    while (!small->m_connections.IsEmpty()) {
        ConnectionMap::Iterator first = small->m_connections.Begin();
        RigidElement* target = first->first;
        RigidConnection* connection = first->second;
        if (target == large) {
            connection->Clear();
            continue;
        }
        ConnectionMap::Iterator existing = large->m_connections.Find(target);
        if (existing != large->m_connections.End())
            existing->second->Merge(connection);
        else
            connection->SetRigidElements(large, target);
    }
    // NOTE: every connection of the large body is reported as changed.
    for (ConnectionMap::Iterator it = large->m_connections.Begin(); it != large->m_connections.End(); ++it)
        SceneGraph::PushBackUnique(changed, it->second.Get());

    // NOTE: the scene is taken from `this`, which may be the small body.
    Scene* scene = GetScene();
    scene->AddChangedRigidElement(large);
    scene->RemoveRigidElement(small);
    scene->GetPhysicsScene()->RemoveRigidElement(small->m_body);

    HingedElement* hinged = small->m_hinged;
    RefPointer<RigidElement> hold(small);
    SceneGraph::EraseValueScene(hinged->m_elements, small);   // (Win: unordered)
    hold = 0;
    large->m_hinged->Merge(hinged);
    return large;
}

// Mac 0xa35d0, Win 0x479790
void RigidElement::CollectGroup(BasicArray<RefPointer<PrimitiveRigidElement> >& group,
                                PrimitiveRigidElement* element)
{
    group.PushBack(RefPointer<PrimitiveRigidElement>(element));
    SceneGraph::EraseValueScene(m_elements, element);   // (Win: unordered)
    for (PrimitiveRigidElement::ConnectionMap::Iterator it = element->m_connections.Begin();
         it != element->m_connections.End(); ++it) {
        PrimitiveRigidConnection* connection = it->second;
        if (connection->m_type != 0)
            continue;
        PrimitiveRigidElement* next = connection->m_element;
        if (next == element)
            next = connection->m_otherElement;
        if (!SceneGraph::Contains(group, next))
            CollectGroup(group, next);
    }
}

// Mac 0xa38a0, Win 0x4799a0
void RigidElement::Split(HingedElement*& hinged, BasicArray<RigidConnection*>& changed)
{
    if (m_elements.IsEmpty()) {
        // nothing left: the body leaves the scene
        hinged = m_hinged;
        SceneIndexTouch(this);
        Scene* scene = GetScene();
        scene->RemoveRigidElement(this);
        scene->GetPhysicsScene()->RemoveRigidElement(m_body);
        RefPointer<RigidElement> hold(this);
        SceneGraph::EraseValueScene(m_hinged->m_elements, this);   // (Win: unordered)
        return;
    }

    typedef BasicArray<RefPointer<PrimitiveRigidElement> > Group;
    BasicArray<Group> groups;
    while (!m_elements.IsEmpty()) {
        groups.PushBack(Group());
        CollectGroup(groups.Back(), m_elements.Begin()->Get());
    }
    SceneIndexTouch(this);

    if (groups.GetSize() == 1) {
        m_elements.Swap(groups[0]);
        UpdateBounds();
        return;
    }

    hinged = m_hinged;
    while (!m_connections.IsEmpty())
        m_connections.Begin()->second->Clear();

    // the first group stays in this body, every other one gets a new body at the same place
    BasicArray<RigidElement*> bodies;
    bodies.PushBack(this);
    m_elements.Swap(groups[0]);
    UpdateBounds();
    GetScene()->AddChangedRigidElement(this);
    for (UInt32 i = 1; i < groups.GetSize(); ++i) {
        Matrix3D transform;
        m_body->GetTransform(transform);
        RigidElement* body = GetScene()->CreateRigidElement(transform);
        body->m_hinged = m_hinged;
        m_hinged->m_elements.PushBack(RefPointer<RigidElement>(body));
        bodies.PushBack(body);
        body->m_elements.Swap(groups[i]);
        for (BasicArray<RefPointer<PrimitiveRigidElement> >::Iterator it = body->m_elements.Begin();
             it != body->m_elements.End(); ++it) {
            PrimitiveRigidElement* element = *it;
            for (BasicArray<RefPointer<PrimitiveElement> >::Iterator e = element->m_elements.Begin();
                 e != element->m_elements.End(); ++e) {
                element->m_rigidElement->m_body->RemovePrimitiveElement((*e)->m_physics);
                body->m_body->AddPrimitiveElement((*e)->m_physics,
                                                  Matrix3D(element->m_transform, (*e)->m_transform));
            }
            element->m_rigidElement = body;
        }
        body->UpdateBounds();
    }

    // rigid connections between the bodies from their primitive rigid connections
    for (BasicArray<RigidElement*>::Iterator b = bodies.Begin(); b != bodies.End(); ++b) {
        for (BasicArray<RefPointer<PrimitiveRigidElement> >::Iterator it = (*b)->m_elements.Begin();
             it != (*b)->m_elements.End(); ++it) {
            for (PrimitiveRigidElement::ConnectionMap::Iterator c = (*it)->m_connections.Begin();
                 c != (*it)->m_connections.End(); ++c) {
                PrimitiveRigidConnection* connection = c->second;
                if (connection->m_rigidConnection != 0)
                    continue;
                RigidElement* a = connection->m_element->m_rigidElement;
                RigidElement* other = connection->m_otherElement->m_rigidElement;
                if (a == other)
                    continue;
                RefPointer<RigidConnection> rigidConnection;
                ConnectionMap::Iterator existing = a->m_connections.Find(other);
                if (existing != a->m_connections.End()) {
                    rigidConnection = existing->second;
                    rigidConnection->AddConnection(connection);
                } else {
                    rigidConnection = new RigidConnection(connection);
                }
                SceneGraph::PushBackUnique(changed, rigidConnection.Get());
            }
        }
    }
}

} // namespace LEGO
