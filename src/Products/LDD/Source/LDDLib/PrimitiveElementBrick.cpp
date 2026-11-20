// LEGO Digital Designer — restored source
// PrimitiveElementBrick (Mac TU 184, 0x237da0-0x23cc2f; Win 0x655a36-0x657079). See
// PrimitiveElementBrick.h.
// The rest of the TU is template code of the member arrays (BasicArray<BasicArray<Matrix3D>>,
// BasicArray<BasicArray<unsigned long>>, BasicArray<BasicArray<(BasicArray<)Behavior>>>
// destructors, Reserve 0x238ab0 / 0x238c00, insert/resize 0x23a830 / 0x23b9f0 / 0x23bfe0,
// assignment 0x23ae40 / 0x23b390 / 0x23b640, push_back 0x238ed0; Win 0x655bb1-0x655f60,
// 0x6563b4-0x656d78) and the TU initialiser 0x23cbf0.
#include "PrimitiveElementBrick.h"
#include "LDDPart.h"
#include "PrimitiveElement.h"

namespace LEGO {

// Mac 0x238110, Win 0x65661b
PrimitiveElementBrick::PrimitiveElementBrick(const BrickPalette::Brick* brick,
                                             const Matrix3D& transform)
    : m_brick(brick)
{
    SetTransform(transform);
}

// Mac 0x2383c0, Win 0x656dbc
PrimitiveElementBrick::PrimitiveElementBrick(const PrimitiveElementBrick& other)
    : Object(), m_brick(other.m_brick)
{
    m_boneTransforms.Resize(m_brick->m_parts.GetSize());
    m_materials.Resize(other.m_materials.GetSize());
    m_decorations.Resize(other.m_decorations.GetSize());
    for (UInt32 i = 0; i < m_boneTransforms.GetSize(); ++i)
        m_boneTransforms[i] = other.m_boneTransforms[i];
    for (UInt32 i = 0; i < m_materials.GetSize(); ++i)
        m_materials[i] = other.m_materials[i];
    for (UInt32 i = 0; i < m_decorations.GetSize(); ++i)
        m_decorations[i] = other.m_decorations[i];
    if (&other != this) {
        m_brickBehaviors = other.m_brickBehaviors;
        m_partBehaviors = other.m_partBehaviors;
        m_boneBehaviors = other.m_boneBehaviors;
    }
}

// Mac 0x2390e0, Win 0x656ff2
PrimitiveElementBrick::PrimitiveElementBrick(const LDDBrick* brick)
{
    m_brick = brick->GetPaletteBrick();
    m_boneTransforms.Reserve(brick->GetPartCount());
    m_materials.Reserve(brick->GetPartCount());
    m_decorations.Reserve(brick->GetPartCount());
    brick->GetBehaviors(m_brickBehaviors);
    m_partBehaviors.Resize(brick->GetPartCount());
    m_boneBehaviors.Resize(brick->GetPartCount());

    BasicArray<Behavior>*              partBehaviors = m_partBehaviors.Begin();
    BasicArray<BasicArray<Behavior> >* boneBehaviors = m_boneBehaviors.Begin();
    for (BasicArray<RefPointer<Part> >::ConstIterator it = brick->m_parts.Begin();
         it != brick->m_parts.End(); ++it, ++partBehaviors, ++boneBehaviors) {
        const Part* part = *it;
        m_materials.PushBack(part->m_materials);
        m_decorations.PushBack(part->m_decorations);
        part->GetBehaviors(*partBehaviors);
        boneBehaviors->Resize(part->GetElementCount());
        m_boneTransforms.PushBack(BasicArray<Matrix3D>());
        BasicArray<Matrix3D>& transforms = m_boneTransforms.Back();
        transforms.Resize(part->GetElementCount(), Matrix3D());
        BasicArray<Behavior>* elementBehaviors = boneBehaviors->Begin();
        for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = part->m_elements.Begin();
             e != part->m_elements.End(); ++e, ++elementBehaviors) {
            const PrimitiveElement* element = *e;
            element->GetTransform(transforms[element->GetBoneIndex()]);
            element->GetBehaviors(*elementBehaviors);
        }
    }
}

// Mac 0x23a480 / 0x23a4e0, Win 0x656608 / 0x65695f
PrimitiveElementBrick::~PrimitiveElementBrick()
{
}

// Mac 0x237e60, Win 0x656114
void PrimitiveElementBrick::SetTransform(const Matrix3D& transform)
{
    m_boneTransforms.Clear();
    m_boneTransforms.Resize(m_brick->m_parts.GetSize());
    for (UInt32 i = 0; i < m_brick->m_parts.GetSize(); ++i) {
        const BasicArray<Matrix3D>& bones = m_brick->m_parts[i]->m_boneTransforms;
        BasicArray<Matrix3D>& transforms = m_boneTransforms[i];
        transforms.Reserve(bones.GetSize());
        for (BasicArray<Matrix3D>::ConstIterator b = bones.Begin(); b != bones.End(); ++b)
            transforms.PushBack(Matrix3D(transform, *b));
    }
}

// Mac 0x239970, Win 0x655a3a
const BrickPalette::Part* PrimitiveElementBrick::GetPalettePart(UInt32 part) const
{
    return m_brick->m_parts[part];
}

// Mac 0x239990, Win 0x655a4a
const BasicArray<Matrix3D>& PrimitiveElementBrick::GetBoneTransforms(UInt32 part) const
{
    if (!m_boneTransforms.IsEmpty())
        return m_boneTransforms[part];
    return GetPalettePart(part)->m_boneTransforms;
}

// Mac 0x2399c0, Win 0x655a6d
const BasicArray<UInt32>& PrimitiveElementBrick::GetMaterials(UInt32 part) const
{
    if (!m_materials.IsEmpty())
        return m_materials[part];
    return GetPalettePart(part)->m_materials;
}

// Mac 0x239cb0, Win 0x655a90
const BasicArray<UInt32>& PrimitiveElementBrick::GetDecorations(UInt32 part) const
{
    if (!m_decorations.IsEmpty())
        return m_decorations[part];
    return GetPalettePart(part)->m_decorations;
}

// Mac 0x2399f0, Win 0x6561fc
void PrimitiveElementBrick::SetMaterial(UInt32 material)
{
    m_materials.Resize(m_brick->m_parts.GetSize());
    for (UInt32 i = 0; i < m_brick->m_parts.GetSize(); ++i) {
        BasicArray<UInt32>& materials = m_materials[i];
        materials.Resize(m_brick->m_parts[i]->m_materials.GetSize(), 0);
        for (UInt32 j = 0; j < materials.GetSize(); ++j)
            materials[j] = material;
    }
}

// Mac 0x239ce0, Win 0x6562c1
void PrimitiveElementBrick::SetDecoration(UInt32 part, UInt32 index, UInt32 decoration)
{
    m_decorations.Resize(m_brick->m_parts.GetSize());
    BasicArray<UInt32>& decorations = m_decorations[part];
    if (decorations.GetSize() <= index)
        decorations.Resize(index + 1, 0);
    decorations[index] = decoration;
}

// Mac 0x239e40, Win 0x656347
void PrimitiveElementBrick::RemoveDecoration(UInt32 part, UInt32 index)
{
    m_decorations.Resize(m_brick->m_parts.GetSize());
    BasicArray<UInt32>& decorations = m_decorations[part];
    if (index < decorations.GetSize())
        decorations[index] = 0;
}

// Mac 0x239f40, Win 0x655c9f
bool PrimitiveElementBrick::BrickLess::operator()(const PrimitiveElementBrick* a,
                                                  const PrimitiveElementBrick* b) const
{
    if (a == 0 || b == 0)
        return false;
    if (a->m_brick->GetSortKey() < b->m_brick->GetSortKey())
        return true;
    if (!(a->m_brick->GetSortKey() == b->m_brick->GetSortKey()))
        return false;
    if (a->m_materials.IsEmpty() && b->m_materials.IsEmpty())
        return false;
    return a->IsMaterialLess(*b);
}

// Mac 0x239fe0, Win 0x655ab3
bool PrimitiveElementBrick::IsMaterialLess(const PrimitiveElementBrick& other) const
{
    for (UInt32 i = 0; i < m_materials.GetSize(); ++i) {
        const BasicArray<UInt32>& mine = GetMaterials(i);
        const BasicArray<UInt32>& theirs = other.GetMaterials(i);
        for (UInt32 j = 0; j < mine.GetSize(); ++j) {
            if (mine[j] < theirs[j])
                return true;
            if (theirs[j] < mine[j])
                return false;
        }
    }
    return false;
}

// Mac 0x23a190, Win 0x655d1b
bool PrimitiveElementBrick::operator==(const PrimitiveElementBrick& other) const
{
    return m_brick == other.m_brick && IsMaterialEqual(other);
}

// Mac 0x23a1b0, Win 0x655b3a
bool PrimitiveElementBrick::IsMaterialEqual(const PrimitiveElementBrick& other) const
{
    for (UInt32 i = 0; i < m_materials.GetSize(); ++i) {
        const BasicArray<UInt32>& mine = GetMaterials(i);
        const BasicArray<UInt32>& theirs = other.GetMaterials(i);
        for (UInt32 j = 0; j < mine.GetSize(); ++j)
            if (mine[j] != theirs[j])
                return false;
    }
    return true;
}

// Mac 0x23a310, Win 0x65697e
const BasicArray<PrimitiveElementBrick::Behavior>&
PrimitiveElementBrick::GetPartBehaviors(UInt32 part) const
{
    if (part < m_partBehaviors.GetSize())
        return m_partBehaviors[part];
    static const BasicArray<Behavior> empty;   // Mac guard 0x77e124
    return empty;
}

// Mac 0x23a3b0, Win 0x6569ec
const BasicArray<PrimitiveElementBrick::Behavior>&
PrimitiveElementBrick::GetBoneBehaviors(UInt32 part, UInt32 bone) const
{
    // NOTE: `bone` is compared with the number of parts, not of the part's bones (both builds).
    if (part < m_partBehaviors.GetSize() && bone < m_boneBehaviors.GetSize())
        return m_boneBehaviors[part][bone];
    static const BasicArray<Behavior> empty;   // Mac guard 0x77e138
    return empty;
}

// Mac 0x23a460
bool PrimitiveElementBrick::IsFlexible() const
{
    return m_brick->IsFlexible();
}

} // namespace LEGO
