// LEGO Digital Designer — restored source
// TU 222 "BIAnalyzerGroups.cpp" (Mac 0x2672a0-0x267cef): LEGO::BIAnalyzerGroups.
// Template code of the TU: BasicSet<BrickGroup*> (destructors 0x267a10 / 0x267c10, Win sdd
// 0x5a14e0; insert 0x267b40; erase 0x267c60, Win 0x5a1480).
#include "BIAnalyzerGroups.h"
#include "BIGraph.h"
#include "BIBrickPrimitive.h"
#include "BIBrickRepository.h"
#include "BrickGroup.h"
#include "Part.h"
#include "PrimitiveElement.h"
#include <algorithm>

namespace LEGO {

// Mac 0x267a70 / 0x267a90, Win 0x5917b0 / sdd 0x591820
BIAnalyzerGroups::~BIAnalyzerGroups()
{
}

// Mac 0x267a40, Win 0x591750
String BIAnalyzerGroups::GetName() const
{
    return String("Groups");
}

// Mac 0x2672a0, Win 0x5a0eb0
void BIAnalyzerGroups::CollectGroups(BrickGroup* group, BasicSet<BrickGroup*>& groups)
{
    groups.Insert(group);
    const BasicArray<RefPointer<BrickGroup> >& children = group->GetGroups();
    for (BasicArray<RefPointer<BrickGroup> >::ConstIterator it = children.Begin(); it != children.End(); ++it)
        CollectGroups(it->Get(), groups);
}

// Mac 0x2672f0, Win 0x5a0f20
bool BIAnalyzerGroups::CollectPrimitives(BrickGroup* group, const BasicArray<PrimitiveElement*>& elements,
                                         BasicSet<BIBrickPrimitive*>& primitives)
{
    const BasicArray<RefPointer<Part> >& parts = group->GetParts();
    if (parts.IsEmpty())
        return false;
    BIBrickRepository& repository = BIBrickRepository::GetInstance();
    for (BasicArray<RefPointer<Part> >::ConstIterator part = parts.Begin(); part != parts.End(); ++part) {
        for (UInt32 i = 0; i < (*part)->GetElementCount(); ++i) {
            PrimitiveElement* element = (*part)->GetElement(i);
            if (std::find(elements.Begin(), elements.End(), element) == elements.End())
                return false;
            primitives.Insert(repository.GetPrimitive(element, false));
        }
    }
    const BasicArray<RefPointer<BrickGroup> >& children = group->GetGroups();
    for (BasicArray<RefPointer<BrickGroup> >::ConstIterator it = children.Begin(); it != children.End(); ++it) {
        if (!CollectPrimitives(it->Get(), elements, primitives))
            return false;
    }
    return true;
}

// Mac 0x2674a0, Win 0x5a10b0
UInt32 BIAnalyzerGroups::Analyze(BIGraph* graph, BasicArray<BIBrickSubstep*>& substeps)
{
    BasicSet<BrickGroup*> groups;
    BasicArray<PrimitiveElement*> elements;
    const BasicSet<BIBrickPrimitive*>& primitives = graph->GetPrimitives();
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = primitives.Begin(); it != primitives.End(); ++it) {
        const BasicArray<PrimitiveElement*>& brickElements = (*it)->GetElements();
        for (BasicArray<PrimitiveElement*>::ConstIterator e = brickElements.Begin(); e != brickElements.End(); ++e) {
            if (IsCancelled())
                return 0;
            elements.PushBack(*e);
            BrickGroup* group = (*e)->GetPart()->GetGroup();
            if (group != 0)
                CollectGroups(group, groups);
        }
    }

    UInt32 count = 0;
    for (BasicSet<BrickGroup*>::ConstIterator g = groups.Begin(); g != groups.End() && !IsCancelled(); ++g) {
        BasicSet<BIBrickPrimitive*> groupPrimitives;
        if (!CollectPrimitives(*g, elements, groupPrimitives))
            continue;
        // NOTE: the original starts at *Begin() unchecked; a group whose parts have no
        // elements gives an empty set (it reads the set's header node). Fixed: skipped.
        if (groupPrimitives.IsEmpty())
            continue;
        BasicSet<BIBrickPrimitive*> reached;
        graph->CountConnected(*groupPrimitives.Begin(), reached, groupPrimitives);
        if (groupPrimitives.GetSize() == reached.GetSize()) {
            if (AddSubstep(groupPrimitives, substeps, m_minSize, graph->GetPrimitiveCount() - 1))
                ++count;
        }
    }
    return count;
}

} // namespace LEGO
