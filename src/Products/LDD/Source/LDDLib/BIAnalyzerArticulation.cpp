// LEGO Digital Designer — restored source
// TU 163 "BIAnalyzerArticulation.cpp" (Mac 0x222f70-0x2232cf): LEGO::BIAnalyzerArticulation.
// The TU also emits BIAnalyzer's inline virtuals (Mac 0x2230a0-0x223140, restored in
// BIAnalyzer.cpp).
#include "BIAnalyzerArticulation.h"
#include "BIGraph.h"
#include "BIBrickPrimitive.h"

namespace LEGO {

// Mac 0x2231c0 / 0x2231e0, Win 0x5919b0 / sdd 0x591a20
BIAnalyzerArticulation::~BIAnalyzerArticulation()
{
}

// Mac 0x223190, Win 0x591950
String BIAnalyzerArticulation::GetName() const
{
    return String("Articulation");
}

// Mac 0x222f70, Win 0x59c6e0
UInt32 BIAnalyzerArticulation::Analyze(BIGraph* graph, BasicArray<BIBrickSubstep*>& substeps)
{
    UInt32 count = 0;
    const BasicSet<BIBrickPrimitive*>& primitives = graph->GetPrimitives();
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = primitives.Begin(); it != primitives.End(); ++it) {
        if (IsCancelled())
            return count;
        BasicSet<BIBrickPrimitive*> side;
        if (m_minSize <= graph->GetSmallSide(*it, side)) {
            if (AddSubstep(side, substeps, 2, 0))
                ++count;
        }
    }
    return count;
}

} // namespace LEGO
