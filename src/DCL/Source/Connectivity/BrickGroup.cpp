// LEGO Digital Designer — restored source
// LEGO::BrickGroup (see BrickGroup.h).
//
// Original object file: Scene.cpp (Mac TU 025); this is sub-TU 025e, Mac 0x95c80-0x97310 (without
// the Scene functions 0x96240, 0x963a0, 0x96a40, 0x96b00, 0x97230, which are in Scene.cpp), plus
// the constructor (Mac 0xc2ba0, 025r) and AddPart / RemovePart (Mac 0x94eb0 / 0x94da0, 025d range).
// Win 0x46f640-0x46f850, 0x4722a0-0x472ad0.
#include "BrickGroup.h"
#include "Part.h"
#include "Scene.h"
#include "SceneGraphUtil.h"

namespace LEGO {

// Mac 0xc2ba0 (025r), Win 0x46f640
BrickGroup::BrickGroup(Scene* scene)
    : m_scene(scene), m_parent(0)
{
}

// Mac 0xc3a20 (complete) / 0xc3a80 (deleting), thunks 0xc3af0 / 0xc3b60, base-object
// variant 0xcbcf0; Win sdd 0x46f820
BrickGroup::~BrickGroup()
{
}

// Mac 0x95c90, Win 0x4722a0
void BrickGroup::GetBoundingBox(BoundingBox& box, bool recompute) const
{
    box.Reset();
    for (BasicArray<RefPointer<Part> >::ConstIterator it = m_parts.Begin(); it != m_parts.End(); ++it) {
        BoundingBox partBox(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        (*it)->GetBoundingBox(partBox, recompute);
        box.Union(partBox);
    }
    for (BasicArray<RefPointer<BrickGroup> >::ConstIterator it = m_groups.Begin(); it != m_groups.End(); ++it) {
        BoundingBox groupBox(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        (*it)->GetBoundingBox(groupBox, recompute);
        box.Union(groupBox);
    }
}

// Mac 0x95d70, Win 0x472380
void BrickGroup::SetParent(BrickGroup* parent)
{
    if (m_parent == parent)
        return;
    RefPointer<BrickGroup> self(this);          // the group may lose its last list reference
    BrickGroup* oldParent = m_parent;
    if (oldParent) {
        m_parent = 0;
        SceneGraph::EraseValue(oldParent->m_groups, this);
        m_scene->AddChangedGroup(oldParent);
    }
    if (parent == 0) {
        m_scene->m_groups.PushBack(RefPointer<BrickGroup>(this));
        m_scene->AddMovedGroup(this);
    } else {
        parent->m_groups.PushBack(RefPointer<BrickGroup>(this));
        m_parent = parent;
        m_scene->AddChangedGroup(parent);
        if (oldParent == 0) {
            // was a top-level group of the scene
            BasicArray<RefPointer<BrickGroup> >& groups = m_scene->m_groups;
            if (SceneGraph::Find(groups, this) != groups.End()) {
                SceneGraph::EraseValue(groups, this);
                m_scene->AddMovedGroup(this);
            }
        }
    }
    m_scene->NotifyGroupChanges();
}

// Mac 0x96a90, Win 0x4728e0
bool BrickGroup::Contains(const Part* part, bool recursive) const
{
    for (BasicArray<RefPointer<Part> >::ConstIterator it = m_parts.Begin(); it != m_parts.End(); ++it)
        if (*it == part)
            return true;
    if (recursive) {
        for (BasicArray<RefPointer<BrickGroup> >::ConstIterator it = m_groups.Begin(); it != m_groups.End(); ++it)
            if ((*it)->Contains(part, true))
                return true;
    }
    return false;
}

// Mac 0x96dc0 (insert range 0x96e30), Win 0x4728a0
void BrickGroup::GetAllParts(BasicArray<RefPointer<Part> >& parts) const
{
    SceneGraph::Append(parts, m_parts);
    for (BasicArray<RefPointer<BrickGroup> >::ConstIterator it = m_groups.Begin(); it != m_groups.End(); ++it)
        (*it)->GetAllParts(parts);
}

// Mac 0x96790, Win 0x472580
void BrickGroup::RemoveParts(const BasicArray<RefPointer<Part> >& parts, BasicArray<RefPointer<Part> >* moved,
                             BasicArray<RefPointer<BrickGroup> >* oldGroups)
{
    // NOTE: m_scene is used without a check (groups always belong to a scene).
    m_scene->BeginUpdate();
    for (BasicArray<RefPointer<Part> >::ConstIterator it = parts.Begin(); it != parts.End(); ++it) {
        Part* part = *it;
        if (!Contains(part, true))
            continue;
        if (moved)
            moved->PushBack(RefPointer<Part>(part));
        if (oldGroups)
            oldGroups->PushBack(RefPointer<BrickGroup>(part->m_group));
        part->SetGroup(m_parent);
    }
    m_scene->EndUpdate();
}

// Mac 0x96b20, Win 0x472710
void BrickGroup::AddParts(const BasicArray<RefPointer<Part> >& parts, BasicArray<RefPointer<Part> >* moved,
                          BasicArray<RefPointer<BrickGroup> >* oldGroups)
{
    m_scene->BeginUpdate();
    for (BasicArray<RefPointer<Part> >::ConstIterator it = parts.Begin(); it != parts.End(); ++it) {
        Part* part = *it;
        if (Contains(part, true))
            continue;
        if (moved)
            moved->PushBack(RefPointer<Part>(part));
        if (oldGroups)
            oldGroups->PushBack(RefPointer<BrickGroup>(part->m_group));
        part->SetGroup(this);
    }
    m_scene->EndUpdate();
}

// Mac 0x94eb0 (025d), Win 0x472940
void BrickGroup::AddPart(Part* part)
{
    m_parts.PushBack(RefPointer<Part>(part));
    if (m_scene) {
        m_scene->AddChangedGroup(this);
        m_scene->NotifyGroupChanges();
    }
}

// Mac 0x94da0 (025d), Win 0x4729f0
void BrickGroup::RemovePart(Part* part)
{
    RefPointer<Part> reference(part);
    BasicArray<RefPointer<Part> >::Iterator it = SceneGraph::Find(m_parts, part);
    reference = 0;
    if (it == m_parts.End())
        return;
    SceneGraph::EraseValueUnordered(m_parts, part);
    if (m_scene) {
        m_scene->AddChangedGroup(this);
        m_scene->NotifyGroupChanges();
    }
}

} // namespace LEGO
