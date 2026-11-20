// LEGO Digital Designer — restored source
// LEGO::PrimitiveRigidElement (see PrimitiveRigidElement.h).
//
// Original object file: Scene.cpp (Mac TU 025); this is sub-TU 025k, Mac 0xa0210-0xa2760 plus the
// inline virtuals of 025s (0xc3be0-0xc3e30); Win 0x477d10-0x478bf0. The Mac range also holds
// RigidElement::Merge (0xa0cb0, in RigidElement.cpp), the push_back-unique instance 0xa0bd0,
// BasicArray instances (0xa2200 / 0xa2570 PushBack, 0xa26b0 / 0xa26e0 destructors) and
// RigidElement::GetTransform (0xa2740, in RigidElement.cpp).
#include "PrimitiveRigidElement.h"
#include "PrimitiveElement.h"
#include "PrimitiveConnection.h"
#include "PrimitiveRigidConnection.h"
#include "RigidElement.h"
#include "Scene.h"
#include "SceneGraphUtil.h"

namespace LEGO {

namespace {
// World transform of a primitive rigid element: its body's transform times its own (its own
// alone while it has no body). (inline: Mac 0xa02a0, 0xa0530 ×2, 0xc4260)
void GetWorldTransform(const PrimitiveRigidElement* element, Matrix3D& transform)
{
    if (element->m_rigidElement == 0) {
        transform = element->m_transform;
    } else {
        element->m_rigidElement->m_body->GetTransform(transform);
        transform.Multiply(element->m_transform);
    }
}

// Bounding box of `element`'s bone in the space `transform` maps into. (inline)
BoundingBox GetBoneBox(const PrimitiveElement* element, const Matrix3D& transform)
{
    return element->m_primitive->GetBone(element->m_boneIndex).m_bounding * transform;
}
} // namespace

// Mac 0xa2450, Win 0x46f9c0
PrimitiveRigidElement::PrimitiveRigidElement(const Matrix3D& transform)
    : m_transform(transform), m_rigidElement(0),
      m_bounding(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), m_boundingSphere(1.0f)
{
}

// Mac 0xc3c10 (complete) / 0xc3c70 (deleting), base-object 0xcbf10; Win sdd 0x4a1ac0
PrimitiveRigidElement::~PrimitiveRigidElement()
{
}

// Mac 0xa0210, Win 0x477d10
void PrimitiveRigidElement::GetBoundingBox(BoundingBox& box, bool recompute) const
{
    if (!recompute) {
        box = m_bounding;
        return;
    }
    box.Reset();
    for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator it = m_elements.Begin();
         it != m_elements.End(); ++it)
        box.Union(GetBoneBox(*it, (*it)->m_transform));
}

// Mac 0xc3be0 (025s), Win 0x4705e0
Scene* PrimitiveRigidElement::GetScene() const
{
    return m_rigidElement != 0 ? m_rigidElement->GetScene() : 0;
}

// Mac 0xc3ce0 (025s, thunk 0xc3e30), Win 0x4a1b20
void PrimitiveRigidElement::OnFinalRelease()
{
    m_elements.Clear();
    Object::OnFinalRelease();
}

// Inlined (Mac 0xa1590 ×3, 0xa0210)
void PrimitiveRigidElement::UpdateBounds()
{
    m_bounding.Reset();
    for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator it = m_elements.Begin();
         it != m_elements.End(); ++it)
        m_bounding.Union(GetBoneBox(*it, (*it)->m_transform));
    m_boundingSphere.Set(m_bounding);
    SceneIndexTouch(m_rigidElement);
}

// Mac 0xa02a0, Win 0x477f00
void PrimitiveRigidElement::AddElement(PrimitiveElement* element)
{
    element->m_rigid = this;
    m_elements.PushBack(RefPointer<PrimitiveElement>(element));

    // the element's world transform relative to this element
    Matrix3D world;
    GetWorldTransform(this, world);
    Matrix3D local(element->m_transform);
    Matrix3D::Solve(world, local);
    element->m_transform = local;
    m_bounding.Union(GetBoneBox(element, local));
    m_boundingSphere.Set(m_bounding);

    // NOTE: unlike the world transform above, the rest requires a body.
    RigidElement* body = m_rigidElement;
    Matrix3D inBody(element->m_rigid->m_transform, local);
    body->m_bounding.Union(GetBoneBox(element, inBody));
    body->m_boundingSphere.Set(body->m_bounding);
    body->m_body->AddPrimitiveElement(element->m_physics, inBody);
    SceneIndexTouch(body);
    body->GetScene()->AddChangedRigidElement(body);
}

// Win 0x477de0 (Mac inlined into Scene, 0xbe6c0)
bool PrimitiveRigidElement::AddConnection(PrimitiveRigidElement* other, PrimitiveConnection* connection,
                                          PrimitiveRigidConnection*& rigidConnection)
{
    ConnectionMap::Iterator it = m_connections.Find(other);
    if (it == m_connections.End()) {
        rigidConnection = new PrimitiveRigidConnection(connection);
        return true;
    }
    rigidConnection = it->second;
    return rigidConnection->AddConnection(connection);
}

// Mac 0xa0530, Win 0x478040
PrimitiveRigidElement* PrimitiveRigidElement::Merge(PrimitiveRigidElement* other,
                                                    BasicArray<PrimitiveRigidConnection*>& changed,
                                                    BasicArray<RigidConnection*>& changedRigid)
{
    if (other == this)
        return other;
    PrimitiveRigidElement* large = other;
    PrimitiveRigidElement* small = this;
    if (other->m_elements.GetSize() < m_elements.GetSize()) {
        large = this;
        small = other;
    }

    // the small element's transform relative to the large one
    Matrix3D largeWorld;
    GetWorldTransform(large, largeWorld);
    Matrix3D relative;
    GetWorldTransform(small, relative);
    Matrix3D::Solve(largeWorld, relative);
    large->m_bounding.Union(small->m_bounding * relative);
    large->m_boundingSphere.Set(large->m_bounding);

    for (BasicArray<RefPointer<PrimitiveElement> >::Iterator it = small->m_elements.Begin();
         it != small->m_elements.End(); ++it) {
        PrimitiveElement* element = *it;
        element->m_rigid = large;
        large->m_elements.PushBack(RefPointer<PrimitiveElement>(element));
        element->m_transform = Matrix3D(relative, element->m_transform);
        small->m_rigidElement->m_body->RemovePrimitiveElement(element->m_physics);
        large->m_rigidElement->m_body->AddPrimitiveElement(element->m_physics,
                                                           Matrix3D(large->m_transform, element->m_transform));
    }
    small->m_elements.Clear();
    SceneIndexTouch(large->m_rigidElement);
    SceneIndexTouch(small->m_rigidElement);

    // the small element's connections: to the large one they become internal, the others move
    while (!small->m_connections.IsEmpty()) {
        ConnectionMap::Iterator first = small->m_connections.Begin();
        PrimitiveRigidElement* target = first->first;
        PrimitiveRigidConnection* connection = first->second;
        if (target == large) {
            connection->Clear();
            continue;
        }
        ConnectionMap::Iterator existing = large->m_connections.Find(target);
        if (existing != large->m_connections.End()) {
            PrimitiveRigidConnection* kept = existing->second;
            if (kept->Merge(connection))
                SceneGraph::PushBackUnique(changed, kept);
        } else {
            connection->SetRigidElements(large, target);
        }
    }

    // the small element leaves its body; then the bodies are merged
    RigidElement* smallBody = small->m_rigidElement;
    RefPointer<PrimitiveRigidElement> hold(small);
    SceneGraph::EraseValueScene(smallBody->m_elements, small);   // (Win: unordered)
    SceneIndexTouch(smallBody);
    hold = 0;
    large->m_rigidElement->Merge(smallBody, changedRigid);
    return large;
}

// Mac 0xa12c0, Win 0x478440
void PrimitiveRigidElement::CollectGroup(BasicArray<RefPointer<PrimitiveElement> >& group,
                                         PrimitiveElement* element)
{
    group.PushBack(RefPointer<PrimitiveElement>(element));
    SceneGraph::EraseValueScene(m_elements, element);   // (Win: unordered)
    for (PrimitiveElement::ConnectionMap::Iterator it = element->m_connections.Begin();
         it != element->m_connections.End(); ++it) {
        PrimitiveConnection* connection = it->second;
        if (connection->m_type != PrimitiveConnection::JointRigid)
            continue;
        PrimitiveElement* next = connection->m_element;
        if (next == element)
            next = connection->m_otherElement;
        if (!SceneGraph::Contains(group, next))
            CollectGroup(group, next);
    }
}

// Mac 0xa1590, Win 0x478650
void PrimitiveRigidElement::Split(RigidElement*& split, RigidElement*& update)
{
    if (m_elements.IsEmpty()) {
        // nothing left: leave the body
        split = m_rigidElement;
        SceneIndexTouch(m_rigidElement);
        RefPointer<PrimitiveRigidElement> hold(this);
        SceneGraph::EraseValueScene(m_rigidElement->m_elements, this);   // (Win: unordered)
        return;
    }

    typedef BasicArray<RefPointer<PrimitiveElement> > Group;
    BasicArray<Group> groups;
    while (!m_elements.IsEmpty()) {
        groups.PushBack(Group());
        CollectGroup(groups.Back(), m_elements.Begin()->Get());
    }
    SceneIndexTouch(m_rigidElement);

    if (groups.GetSize() == 1) {
        m_elements.Swap(groups[0]);
        UpdateBounds();
        update = m_rigidElement;
        return;
    }

    split = m_rigidElement;
    while (!m_connections.IsEmpty())
        m_connections.Begin()->second->Clear();

    // the first group stays here, every other one becomes a new element in the same body
    BasicArray<PrimitiveRigidElement*> elements;
    elements.PushBack(this);
    m_elements.Swap(groups[0]);
    UpdateBounds();
    for (UInt32 i = 1; i < groups.GetSize(); ++i) {
        PrimitiveRigidElement* element = new PrimitiveRigidElement(m_transform);
        element->m_rigidElement = m_rigidElement;
        m_rigidElement->m_elements.PushBack(RefPointer<PrimitiveRigidElement>(element));
        SceneIndexTouch(m_rigidElement);
        elements.PushBack(element);
        element->m_elements.Swap(groups[i]);
        for (BasicArray<RefPointer<PrimitiveElement> >::Iterator it = element->m_elements.Begin();
             it != element->m_elements.End(); ++it)
            (*it)->m_rigid = element;
        element->UpdateBounds();
    }

    // primitive rigid connections between the elements from their primitive connections
    for (BasicArray<PrimitiveRigidElement*>::Iterator rigid = elements.Begin();
         rigid != elements.End(); ++rigid) {
        for (BasicArray<RefPointer<PrimitiveElement> >::Iterator it = (*rigid)->m_elements.Begin();
             it != (*rigid)->m_elements.End(); ++it) {
            for (PrimitiveElement::ConnectionMap::Iterator c = (*it)->m_connections.Begin();
                 c != (*it)->m_connections.End(); ++c) {
                PrimitiveConnection* connection = c->second;
                if (connection->m_rigidConnection != 0)
                    continue;
                PrimitiveRigidElement* a = connection->m_element->m_rigid;
                PrimitiveRigidElement* b = connection->m_otherElement->m_rigid;
                if (a == b)
                    continue;
                ConnectionMap::Iterator existing = a->m_connections.Find(b);
                if (existing != a->m_connections.End())
                    existing->second->AddConnection(connection);
                else
                    new PrimitiveRigidConnection(connection);
            }
        }
    }
}

} // namespace LEGO
