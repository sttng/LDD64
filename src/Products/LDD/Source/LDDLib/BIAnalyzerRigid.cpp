// LEGO Digital Designer — restored source
// TU 193 "BIAnalyzerRigid.cpp" (Mac 0x246ed0-0x2475df): LEGO::BIAnalyzerRigid.
// Template code of the TU: BasicSet<RigidElement*> (destructors 0x247300 / 0x247430, Win sdd
// 0x5a0660; erase 0x247480, Win 0x5a05c0; insert 0x2474d0).
#include "BIAnalyzerRigid.h"
#include "BIGraph.h"
#include "BIBrickPrimitive.h"
#include "PrimitiveElement.h"
#include "PrimitiveRigidElement.h"
#include "RigidElement.h"

namespace LEGO {

// Mac 0x247360 / 0x247380, Win 0x591db0 / sdd 0x591e20
BIAnalyzerRigid::~BIAnalyzerRigid()
{
}

// Mac 0x247330, Win 0x591d50
String BIAnalyzerRigid::GetName() const
{
    return String("Rigid");
}

// inlined on Mac (0x246ed0), Win 0x5a00e0
bool BIAnalyzerRigid::CollectPrimitives(const PrimitiveRigidElement* rigid, BIGraph* graph,
                                        BasicSet<BIBrickPrimitive*>& primitives)
{
    const BasicArray<RefPointer<PrimitiveElement> >& elements = rigid->GetElements();
    const BasicSet<BIBrickPrimitive*>& candidates = graph->GetPrimitives();
    for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = elements.Begin(); e != elements.End(); ++e) {
        BIBrickPrimitive* found = 0;
        for (BasicSet<BIBrickPrimitive*>::ConstIterator it = candidates.Begin(); it != candidates.End(); ++it) {
            if ((*it)->ContainsElement(e->Get())) {
                found = *it;
                break;
            }
        }
        if (found == 0)
            return false;
        primitives.Insert(found);
    }
    return true;
}

// Mac 0x246ed0, Win 0x5a0160
UInt32 BIAnalyzerRigid::Analyze(BIGraph* graph, BasicArray<BIBrickSubstep*>& substeps)
{
    // the rigid bodies of the graph's primitives
    BasicSet<RigidElement*> rigids;
    const BasicSet<BIBrickPrimitive*>& primitives = graph->GetPrimitives();
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = primitives.Begin(); it != primitives.End(); ++it) {
        const BasicArray<PrimitiveElement*>& elements = (*it)->GetElements();
        for (BasicArray<PrimitiveElement*>::ConstIterator e = elements.Begin(); e != elements.End(); ++e) {
            if (IsCancelled())
                return 0;
            // NOTE: both builds look at the first element on every iteration (elements[0]),
            // not at *e.
            PrimitiveRigidElement* rigid = elements[0]->GetRigidElement();
            if (rigid != 0)
                rigids.Insert(rigid->GetRigidElement());
        }
    }

    UInt32 count = 0;
    UInt32 maxSize = graph->GetPrimitiveCount() / 2;
    for (BasicSet<RigidElement*>::ConstIterator r = rigids.Begin(); r != rigids.End(); ++r) {
        BasicSet<BIBrickPrimitive*> all;
        bool complete = true;
        const BasicArray<RefPointer<PrimitiveRigidElement> >& parts = (*r)->GetElements();
        for (BasicArray<RefPointer<PrimitiveRigidElement> >::ConstIterator part = parts.Begin(); part != parts.End(); ++part) {
            if (IsCancelled())
                return count;
            BasicSet<BIBrickPrimitive*> partPrimitives;
            if (!CollectPrimitives(part->Get(), graph, partPrimitives)) {
                complete = false;
                continue;
            }
            if (AddSubstep(partPrimitives, substeps, m_minSize, maxSize))
                ++count;
            if (complete) {
                for (BasicSet<BIBrickPrimitive*>::ConstIterator p = partPrimitives.Begin(); p != partPrimitives.End(); ++p)
                    all.Insert(*p);
            }
        }
        if (complete && AddSubstep(all, substeps, m_minSize, maxSize))
            ++count;
    }
    return count;
}

} // namespace LEGO
