// LEGO Digital Designer — restored source
// LEGO::HingedElement (see HingedElement.h; the inline virtuals are in the header).
//
// Original object file: Scene.cpp (Mac TU 025); this is sub-TU 025m, Mac 0xa4ce0-0xa5a70 plus Merge
// 0xa3430 (025l); Win 0x47a070-0x47a950. The Mac range also holds BasicArray instances (0xa5610 /
// 0xa58d0 PushBack, 0xa5860 / 0xa5a10 destructors).
#include "HingedElement.h"
#include "PrimitiveElement.h"
#include "PrimitiveRigidElement.h"
#include "RigidElement.h"
#include "RigidConnection.h"
#include "Scene.h"
#include "SceneGraphUtil.h"

namespace LEGO {

// Mac 0xa4ce0, Win 0x47a070
void HingedElement::GetBoundingBox(BoundingBox& box, bool recompute) const
{
    box.Reset();
    if (!recompute) {
        BoundingBox bodyBox(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        Matrix3D transform;
        for (BasicArray<RefPointer<RigidElement> >::ConstIterator it = m_elements.Begin();
             it != m_elements.End(); ++it) {
            (*it)->m_body->GetTransform(transform);
            (*it)->GetBoundingBox(bodyBox, false);
            box.Union(bodyBox * transform);
        }
        return;
    }
    for (BasicArray<RefPointer<RigidElement> >::ConstIterator it = m_elements.Begin();
         it != m_elements.End(); ++it) {
        Matrix3D transform;
        (*it)->m_body->GetTransform(transform);
        const BasicArray<RefPointer<PrimitiveRigidElement> >& rigids = (*it)->m_elements;
        for (BasicArray<RefPointer<PrimitiveRigidElement> >::ConstIterator r = rigids.Begin();
             r != rigids.End(); ++r) {
            Matrix3D rigidTransform(transform, (*r)->m_transform);
            for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = (*r)->m_elements.Begin();
                 e != (*r)->m_elements.End(); ++e) {
                Matrix3D elementTransform(rigidTransform, (*e)->m_transform);
                box.Union((*e)->m_primitive->GetBone((*e)->m_boneIndex).m_bounding * elementTransform);
            }
        }
    }
}

// Mac 0xa3430 (025l), Win 0x47a240
HingedElement* HingedElement::Merge(HingedElement* other)
{
    if (other == this)
        return other;
    HingedElement* large = other;
    HingedElement* small = this;
    if (other->m_elements.GetSize() < m_elements.GetSize()) {
        large = this;
        small = other;
    }
    for (BasicArray<RefPointer<RigidElement> >::Iterator it = small->m_elements.Begin();
         it != small->m_elements.End(); ++it) {
        large->m_elements.PushBack(*it);
        (*it)->m_hinged = large;
    }
    small->m_elements.Clear();

    // NOTE: the scene is taken from `this`, which may be the small element.
    RefPointer<HingedElement> hold(small);
    m_scene->RemoveHingedElement(small);
    return large;
}

// Mac 0xa4ec0, Win 0x47a340
void HingedElement::CollectGroup(BasicArray<RefPointer<RigidElement> >& group, RigidElement* element)
{
    group.PushBack(RefPointer<RigidElement>(element));
    SceneGraph::EraseValueScene(m_elements, element);   // (Win: unordered)
    for (RigidElement::ConnectionMap::Iterator it = element->m_connections.Begin();
         it != element->m_connections.End(); ++it) {
        RigidElement* next = it->second->m_element;
        if (next == element)
            next = it->second->m_otherElement;
        if (!SceneGraph::Contains(group, next))
            CollectGroup(group, next);
    }
}

// Mac 0xa5190, Win 0x47a530
void HingedElement::Split()
{
    if (m_elements.IsEmpty()) {
        // nothing left: leave the scene
        RefPointer<HingedElement> hold(this);
        m_scene->RemoveHingedElement(this);
        return;
    }

    typedef BasicArray<RefPointer<RigidElement> > Group;
    BasicArray<Group> groups;
    while (!m_elements.IsEmpty()) {
        groups.PushBack(Group());
        CollectGroup(groups.Back(), m_elements.Begin()->Get());
    }
    m_elements.Swap(groups[0]);
    for (UInt32 i = 1; i < groups.GetSize(); ++i) {
        HingedElement* element = new HingedElement(m_scene);
        m_scene->AddHingedElement(element);
        element->m_elements.Swap(groups[i]);
        for (BasicArray<RefPointer<RigidElement> >::Iterator it = element->m_elements.Begin();
             it != element->m_elements.End(); ++it)
            (*it)->m_hinged = element;
    }
}

} // namespace LEGO
