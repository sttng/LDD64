// LEGO Digital Designer — restored source
// TU 182 "BIProducer.cpp" (Mac 0x236990-0x23796f): LEGO::BIProducer. See BIProducer.h.
// Template code of the TU: BasicArray<RefPointer<BIAnalyzer> > (PushBack 0x236990, destructors
// 0x236ad0 / 0x237710, Win sdd 0x5928a0).
#include "BIProducer.h"
#include "BIAnalyzer.h"
#include "BIAnalyzerArticulation.h"
#include "BIAnalyzerDirection.h"
#include "BIAnalyzerGroups.h"
#include "BIAnalyzerRigid.h"
#include "BIAnalyzerSeparation.h"
#include "BGAnalyzerMinifig.h"
#include "BGAnalyzerSkeleton.h"
#include "BGApplication.h"

namespace LEGO {

// Mac 0x236b40
BIProducer::BIProducer(BIFacade* parent, const String& name, BGApplication* application)
    : BIFacade(parent),
      m_application(application),
      m_name(name)
{
    m_separation = new BIAnalyzerSeparation(this, 10.0f, 1.0f, 2);
    m_groups = new BIAnalyzerGroups(this, 2.0f, 1.0f, 2);
    m_articulation = new BIAnalyzerArticulation(this, 1.0f, 0.1f, 4);
    m_analyzers.PushBack(m_groups);
    m_analyzers.PushBack(RefPointer<BIAnalyzer>(new BIAnalyzerDirection(this, 2.0f, 0.8f, 2)));
    m_analyzers.PushBack(RefPointer<BIAnalyzer>(new BIAnalyzerRigid(this, 2.0f, 0.6f, 3)));
    m_analyzers.PushBack(RefPointer<BIAnalyzer>(new BIAnalyzerArticulation(this, 1.0f, 0.5f, 11)));
    m_analyzers.PushBack(RefPointer<BIAnalyzer>(new BIAnalyzerArticulation(this, 1.0f, 0.3f, 7)));
    m_analyzers.PushBack(m_articulation);
    m_analyzers.PushBack(RefPointer<BIAnalyzer>(new BGAnalyzerMinifig(this, 11.0f, 1.0f)));
    m_analyzers.PushBack(RefPointer<BIAnalyzer>(new BGAnalyzerSkeleton(this, 11.0f, 1.0f)));
}

// Mac 0x237560 / 0x2375c0, Win 0x591560 / sdd 0x591620
BIProducer::~BIProducer()
{
}

// Mac 0x237520, Win 0x5914e0
UInt32 BIProducer::GetFacadeType() const
{
    return 5;
}

// Mac 0x237530, Win 0x5914f0
String BIProducer::GetName() const
{
    return m_name;
}

// Mac 0x237390
UInt32 BIProducer::GetAnalyzerCount() const
{
    return m_analyzers.GetSize() + 1;
}

// inlined on Mac (0x2373b0), Win 0x592710
UInt32 BIProducer::AnalyzeGraph(BIGraph* graph, BasicArray<BIBrickSubstep*>& substeps)
{
    UInt32 count = 0;
    for (BasicArray<RefPointer<BIAnalyzer> >::Iterator it = m_analyzers.Begin(); it != m_analyzers.End(); ++it) {
        if (IsCancelled())
            break;
        count += (*it)->Analyze(graph, substeps);
        m_application->StepProgress();
    }
    return count;
}

// Mac 0x2373b0, Win 0x592610
UInt32 BIProducer::Analyze(const BIGraph::GraphList& graphs, BasicArray<BIBrickSubstep*>& substeps)
{
    m_application->StepProgress();
    UInt32 count = 0;
    for (BIGraph::GraphList::ConstIterator it = graphs.Begin(); it != graphs.End(); ++it) {
        if (IsCancelled())
            return count;
        if (graphs.GetSize() > 1) {
            if (BIAnalyzer::CreateSubstep((*it)->GetPrimitives(), substeps, 2, 0, m_separation))
                ++count;
        }
        if (IsCancelled())
            return count;
        count += AnalyzeGraph(*it, substeps);
    }
    return count;
}

// Mac 0x237500, Win 0x5927b0
bool BIProducer::IsCancelled() const
{
    return m_application->IsCancelled();
}

} // namespace LEGO
