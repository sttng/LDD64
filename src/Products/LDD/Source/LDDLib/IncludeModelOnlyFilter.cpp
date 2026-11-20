// LEGO Digital Designer — restored source
// Selection filters (Mac TU 083), see IncludeModelOnlyFilter.h.
#include "IncludeModelOnlyFilter.h"
#include "PrimitiveElement.h"
#include "PrimitiveRigidElement.h"
#include "RigidElement.h"
#include "HingedElement.h"
#include "Scene.h"
#include "Brick.h"
#include "Part.h"
#include "Primitive.h"

namespace LEGO {

// The picked element's model (Mac: element +0x58 -> rigid +0x44 -> hinged +0x04).
static HingedElement* GetHingedElement(const PrimitiveElement* picked)
{
    return picked->GetRigidElement()->GetRigidElement()->GetHingedElement();
}

// Removes every part for which `keep` is false (each removal moves the rest down).
template <class Predicate>
static void KeepParts(BasicArray<RefPointer<Part> >& parts, Predicate keep)
{
    UInt32 i = 0;
    while (i < parts.GetSize()) {
        if (keep(parts[i]))
            ++i;
        else
            parts.Erase(i);
    }
}

// Mac 0x181270, Win 0x66026a
void IncludeSceneOnlyFilter::Apply(const PrimitiveElement* picked, BasicArray<RefPointer<Part> >& parts) const
{
    parts.Clear();
    Scene* scene = GetHingedElement(picked)->GetScene();
    const BasicArray<RefPointer<Brick> >& bricks = scene->GetBricks();
    for (BasicArray<RefPointer<Brick> >::ConstIterator b = bricks.Begin(); b != bricks.End(); ++b) {
        const BasicArray<RefPointer<Part> >& brickParts = (*b)->GetParts();
        for (BasicArray<RefPointer<Part> >::ConstIterator p = brickParts.Begin(); p != brickParts.End(); ++p)
            parts.PushBack(*p);
    }
}

// Mac 0x181330, Win 0x66033d
void IncludeModelOnlyFilter::Apply(const PrimitiveElement* picked, BasicArray<RefPointer<Part> >& parts) const
{
    parts.Clear();
    const BasicArray<RefPointer<RigidElement> >& rigids = GetHingedElement(picked)->GetElements();
    for (BasicArray<RefPointer<RigidElement> >::ConstIterator r = rigids.Begin(); r != rigids.End(); ++r) {
        const BasicArray<RefPointer<PrimitiveRigidElement> >& groups = (*r)->GetElements();
        for (BasicArray<RefPointer<PrimitiveRigidElement> >::ConstIterator g = groups.Begin(); g != groups.End(); ++g) {
            const BasicArray<RefPointer<PrimitiveElement> >& elements = (*g)->GetElements();
            for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = elements.Begin(); e != elements.End(); ++e)
                parts.PushBack(RefPointer<Part>((*e)->GetPart()));
        }
    }
}

namespace {
struct SameDesign {
    UInt32 m_designID;
    bool operator()(const RefPointer<Part>& part) const { return part->GetDesignID() == m_designID; }
};
struct SameMaterial {
    UInt32 m_material;
    bool operator()(const RefPointer<Part>& part) const { return part->GetMaterial(0) == m_material; }
};
}

// Mac 0x181490, Win 0x6602b4
void IncludeDesignOnlyFilter::Apply(const PrimitiveElement* picked, BasicArray<RefPointer<Part> >& parts) const
{
    if (parts.IsEmpty())
        return;
    SameDesign keep;
    keep.m_designID = picked->GetPrimitive()->GetDesignID();
    KeepParts(parts, keep);
}

// Mac 0x181570, Win 0x6602f4
void IncludeMaterialOnlyFilter::Apply(const PrimitiveElement* picked, BasicArray<RefPointer<Part> >& parts) const
{
    SameMaterial keep;
    keep.m_material = picked->GetPart()->GetMaterial(0);
    KeepParts(parts, keep);
}

} // namespace LEGO
