// LEGO Digital Designer — restored source
// LEGO::Part (see Part.h) and the two PrimitiveElement functions placed in the same range
// (PrimitiveElement::SetHidden, PrimitiveElement::ApplyPointGroups).
//
// Original object file: Scene.cpp (Mac TU 025); this is sub-TU 025d, Mac 0x94100-0x95c80 (without
// the Scene functions 0x95970 / 0x95ad0 and the BrickGroup part list functions 0x94da0 / 0x94eb0,
// which are in Scene.cpp / BrickGroup.cpp), Win 0x46ef20-0x46f610 / 0x4718d0-0x4722a0.
#include "Part.h"
#include "Brick.h"
#include "BrickGroup.h"
#include "BrickDatabase.h"
#include "MaterialManual.h"
#include "Primitive.h"
#include "PrimitiveElement.h"
#include "PrimitiveConnection.h"
#include "PrimitiveRigidElement.h"
#include "ConnectionFieldPlanar.h"
#include "Scene.h"

namespace LEGO {

namespace {

// Transparent material: alpha of the diffuse colour below 0.99999 (the colour stays the
// default white when the material is unknown). (inline: Mac 0x94300, Win 0x471a50)
bool IsTransparentMaterial(const Primitive* primitive, UInt32 material)
{
    MaterialColor color;
    color.shininess = 0.0f;
    primitive->GetDatabase()->GetMaterialColor(material, color);
    return color.diffuse.a < 0.99999f;
}

// Sets material slot `index`; `groups` (optional) records a change of transparency as the
// point group override (ConnectionPoint::FLAG_GROUP for transparent materials).
// Win 0x471a90 (Mac inlined into 0x94300)
void SetMaterialSlot(UInt32 index, UInt32 material, const Primitive* primitive,
                     BasicArray<UInt32>& materials, bool& changed,
                     BasicArray<Part::PointGroup>* groups)
{
    UInt32 old = materials[index];
    if (old == material)
        return;
    if (groups) {
        bool wasTransparent = IsTransparentMaterial(primitive, old);
        bool isTransparent = IsTransparentMaterial(primitive, material);
        if (wasTransparent != isTransparent) {
            (*groups)[index].m_enabled = true;
            (*groups)[index].m_flags = isTransparent ? (UInt32)ConnectionPoint::FLAG_GROUP : 0;
        }
    }
    materials[index] = material;
    changed = true;
}

// Material slot `index` with the primitive's sub-material redirect table: slots of one
// redirect group (non-zero entry) change together; a slot without group sets slot 0 (and a
// material 0 is ignored there). Mac 0x94300, Win 0x471b00
void ApplyMaterial(UInt32 index, UInt32 material, const Primitive* primitive,
                   BasicArray<UInt32>& materials, bool& changed,
                   BasicArray<Part::PointGroup>* groups)
{
    const BasicArray<UInt32>& redirect = primitive->GetSubMaterialRedirect();
    if (redirect.IsEmpty()) {
        SetMaterialSlot(index, material, primitive, materials, changed, groups);
        return;
    }
    UInt32 group = redirect[index];
    if (group != 0) {
        for (UInt32 i = 0; i < redirect.GetSize(); ++i)
            if (redirect[i] == group)
                SetMaterialSlot(i, material, primitive, materials, changed, groups);
        return;
    }
    if (material == 0)
        return;
    // NOTE: index is ignored here: the slots without redirect group share slot 0.
    SetMaterialSlot(0, material, primitive, materials, changed, groups);
}

} // namespace

// ---------------------------------------------------------------------------------------------
// Part
// ---------------------------------------------------------------------------------------------

// Mac 0x95460 (base-object variant with VTT 0x95190), Win 0x472000
Part::Part(const Primitive* primitive)
    : m_brick(0), m_group(0), m_primitive(primitive), m_hidden(false)
{
    m_elements.Resize(primitive->GetBoneCount(), RefPointer<PrimitiveElement>());
    UInt32 materials = primitive->GetDecorationFaceCount();
    if (materials == 0)
        materials = 1;
    m_materials.Resize(materials, 0);
}

// Mac 0xc3790 (complete) / 0xc37f0 (deleting), thunks 0xc3870 / 0xc38d0 / 0xc3940 / 0xc39a0,
// base-object variant 0xcbb30; Win 0x46f4e0 (sdd 0x46f5e0)
Part::~Part()
{
}

// Mac 0xc3740 (025s), Win 0x46f440
UInt32 Part::GetDesignID() const
{
    return m_primitive->GetDesignID();
}

// Mac 0x942e0, Win 0x471a30
UInt32 Part::GetMaterial(UInt32 index) const
{
    UInt32 material = m_materials[index];
    return material != 0 ? material : m_materials[0];
}

// Mac 0x94920, Win 0x471e00
UInt32 Part::GetDecoration(UInt32 index) const
{
    return index < m_decorations.GetSize() ? m_decorations[index] : 0;
}

// Mac 0xc3770 (025s), Win 0x46f4b0
bool Part::IsFlexible() const
{
    return m_primitive->GetBoneCount() > 1;
}

// Mac 0x94120 (thunk 0x941b0), Win 0x4718d0
void Part::GetBoundingBox(BoundingBox& box, bool) const
{
    box.Reset();
    Matrix3D transform;
    for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator it = m_elements.Begin();
         it != m_elements.End(); ++it) {
        const PrimitiveElement* element = *it;
        element->GetTransform(transform);
        const Primitive::Bone& bone = element->m_primitive->GetBone(element->m_boneIndex);
        box.Union(bone.m_bounding * transform);
    }
}

// Mac 0x942a0 (thunk 0x942c0), Win 0x471a20
Scene* Part::GetScene() const
{
    // NOTE: m_elements[0] without a check (every bone slot is filled by the loaders).
    return m_elements[0]->GetScene();
}

// Mac 0x95720, Win 0x4721c0
void Part::OnMaterialsChanged(const BasicArray<PointGroup>& groups)
{
    for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator it = m_elements.Begin();
         it != m_elements.End(); ++it)
        (*it)->ApplyPointGroups(groups);
    Scene* scene = GetScene();
    if (scene && scene->m_flag123 && m_group)
        scene->AddChangedGroup(m_group);
}

// Mac 0x94240, Win 0x471980
void Part::GetOrientedBoundingBox(BoundingBox& box, Matrix3D& transform) const
{
    if (m_elements.GetSize() < 2) {
        box = m_primitive->GetBoundingBox();
        m_elements[0]->GetTransform(transform);
    } else {
        GetBoundingBox(box, true);
        transform.SetIdentity();
    }
}

// Mac 0x946c0, Win 0x471c00
void Part::SetMaterial(UInt32 index, UInt32 material)
{
    bool changed = false;
    BasicArray<PointGroup> groups;
    PointGroup none;
    none.m_enabled = false;
    none.m_flags = 0;
    if (m_materials.GetSize() != 0)
        groups.Resize(m_materials.GetSize(), none);
    ApplyMaterial(index, material, m_primitive, m_materials, changed, &groups);
    if (changed)
        OnMaterialsChanged(groups);
}

// Mac 0x947f0, Win 0x471d00 (+ 0x471bb0)
void Part::SetMaterials(const BasicArray<UInt32>& materials)
{
    bool changed = false;
    BasicArray<PointGroup> groups;
    PointGroup none;
    none.m_enabled = false;
    none.m_flags = 0;
    if (m_materials.GetSize() != 0)
        groups.Resize(m_materials.GetSize(), none);
    if (m_materials.IsEmpty())
        return;
    // NOTE: `materials` is read for every material slot of the part without checking its size
    // (both builds; an LXFML part with fewer materials than its primitive has surfaces reads
    // past the array); fixed: a missing entry counts as 0, no material of its own.
    for (UInt32 i = 0; i < m_materials.GetSize(); ++i)
        ApplyMaterial(i, i < materials.GetSize() ? materials[i] : 0, m_primitive, m_materials, changed, &groups);
    if (changed)
        OnMaterialsChanged(groups);
}

// Mac 0x94950, Win 0x471e20
void Part::SetDecorations(const BasicArray<UInt32>& decorations)
{
    BasicArray<UInt32> faces;                   // the changed decoration faces
    UInt32 count = m_primitive->GetDecorationFaceCount();
    for (UInt32 i = 0; i < count; ++i) {
        UInt32 old = i < m_decorations.GetSize() ? m_decorations[i] : 0;
        UInt32 decoration = i < decorations.GetSize() ? decorations[i] : 0;
        if (old != decoration)
            faces.PushBack(i);
    }
    if (&decorations != &m_decorations)
        m_decorations = decorations;
    m_decorations.Resize(m_primitive->GetDecorationFaceCount(), 0);
    if (!faces.IsEmpty()) {
        Scene* scene = GetScene();
        if (scene && scene->m_flag122)
            AddChangedBodies(scene);
        OnDecorationsChanged(faces);
    }
}

// Win 0x472220 (Mac inlined into 0x94950)
void Part::AddChangedBodies(Scene* scene) const
{
    for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator it = m_elements.Begin();
         it != m_elements.End(); ++it)
        scene->AddChangedRigidElement((*it)->m_rigid->m_rigidElement);
}

// Mac 0x94d60, Win 0x471fa0
void Part::SetGroup(BrickGroup* group)
{
    if (m_group == group)
        return;
    if (m_group)
        m_group->RemovePart(this);
    m_group = group;
    if (group)
        group->AddPart(this);
}

// Mac 0x94f60, Win 0x471fd0
void Part::SetBrick(Brick* brick)
{
    if (m_brick == brick)
        return;
    if (brick)
        brick->AddPart(this);
    if (m_brick)
        m_brick->RemovePart(this);
    m_brick = brick;
}

// Win 0x472250 (Mac inlined into Brick::SetHidden 0x93510)
void Part::SetHidden(bool hidden, bool updateBodies, bool updateGroup, Scene* scene)
{
    m_hidden = hidden;
    for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator it = m_elements.Begin();
         it != m_elements.End(); ++it)
        (*it)->SetHidden(hidden, updateBodies, scene);
    if (updateGroup && m_group)
        scene->AddChangedGroup(m_group);
}

// ---------------------------------------------------------------------------------------------
// PrimitiveElement functions of the 025d range
// ---------------------------------------------------------------------------------------------

// Mac 0x95bb0, Win 0x477c10
void PrimitiveElement::SetHidden(bool hidden, bool updateBodies, Scene* scene)
{
    m_flagAC = hidden;
    for (BasicArray<RefPointer<ConnectionField> >::ConstIterator it = m_fields.Begin();
         it != m_fields.End(); ++it)
        (*it)->SetPointsBlocked(hidden);
    if (!updateBodies)
        return;
    scene->AddChangedRigidElement(m_rigid->m_rigidElement);
    if (!scene->m_flag120)
        return;
    for (ConnectionMap::ConstIterator it = m_connections.Begin(); it != m_connections.End(); ++it) {
        PrimitiveElement* other = it->second->GetOther(this);
        if (!other->m_flagAC)
            scene->AddChangedRigidElement(other->m_rigid->m_rigidElement);
    }
}

// Mac 0x95790, Win 0x477950
void PrimitiveElement::ApplyPointGroups(const BasicArray<ConnectionField::PointGroup>& groups)
{
    for (BasicArray<RefPointer<ConnectionField> >::ConstIterator it = m_fields.Begin();
         it != m_fields.End(); ++it)
        (*it)->ApplyPointGroups(groups);
    Scene* scene = GetScene();
    if (!scene)
        return;
    if (scene->m_flag121)
        scene->AddChangedRigidElement(m_rigid->m_rigidElement);
    if (!scene->m_flag120)
        return;
    for (ConnectionMap::ConstIterator it = m_connections.Begin(); it != m_connections.End(); ++it) {
        const PrimitiveConnection* connection = it->second;
        const BasicArray<PrimitiveConnection::Connection>& list = connection->m_connections;
        for (UInt32 c = 0; c < list.GetSize(); ++c) {
            const PrimitiveConnection::Connection& entry = list[c];
            if (entry.m_field->GetType() != ConnectionField::TYPE_PLANAR)
                continue;
            // the side whose field has the odd kind (the knob side) and its point indices
            bool first = (entry.m_field->GetKind() & 1) != 0;
            PrimitiveElement* element = first ? connection->m_element : connection->m_otherElement;
            const ConnectionFieldPlanar* field = static_cast<const ConnectionFieldPlanar*>(
                first ? entry.m_field : entry.m_otherField);
            PrimitiveElement* changed = 0;
            for (UInt32 i = 0; i < entry.m_indices.GetSize(); ++i) {
                const ConnectionField::ConnectionIndex& pair = entry.m_indices[i];
                const ConnectionPoint& point = field->GetPoint(first ? pair.m_index : pair.m_otherIndex);
                if ((point.m_flags & ConnectionPoint::FLAG_FIXED) == 0)
                    continue;
                // the group of this element's point: the knob point itself, or the point it
                // is connected to (NOTE: dereferenced without a check)
                UInt32 group = element == this ? point.m_group : point.m_connection->m_group;
                if (groups[group].m_enabled) {
                    changed = connection->GetOther(this);
                    break;
                }
            }
            if (changed)
                scene->AddChangedRigidElement(changed->m_rigid->m_rigidElement);
        }
    }
}

} // namespace LEGO
