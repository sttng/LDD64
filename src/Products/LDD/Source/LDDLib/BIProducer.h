// LEGO Digital Designer — restored source
// LEGO::BIProducer — runs the analyzers over the graphs of a model and collects the proposed
//   substeps (BGAutomaticApplication creates one, +0x5c). Its analyzers: Groups (min 2), Direction
//   (2), Rigid (3), Articulation (11, 7, 4), BGAnalyzerMinifig and BGAnalyzerSkeleton;
//   Separation (min 2) is the analyzer of the whole-graph substeps.
//
// Home TU 182 "BIProducer.cpp" (Mac 0x236990-0x23796f).
// RTTI: class LEGO::BIProducer : BIFacade (: virtual Object).
//   Mac sizeof 0x38 + Object: vptr +0, m_parent +4, m_application +8, m_analyzers +0x0c,
//       m_name +0x1c, m_separation +0x2c, m_groups +0x30, m_articulation +0x34, Object +0x38.
//       Vtable 0x770eec {GetFacadeType 0x237520, GetName 0x237530, D1 0x237560, D0 0x2375c0}.
//   Win: vfptr +0, vbptr +4, m_parent +8, m_application +0xc, m_analyzers +0x10, m_name +0x20,
//       … Object at +0x30 + 0x10. Vtable 0xa2c560 {0x5914e0, 0x5914f0}, destructor 0x591560,
//       sdd 0x591620.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "BIFacade.h"
#include "BIGraph.h"

namespace LEGO {

class BIAnalyzer;
class BIBrickSubstep;
class BGApplication;

class BIProducer : public BIFacade {
public:
    // Creates the analyzers. Mac 0x236b40 (complete constructor) (names inferred)
    BIProducer(BIFacade* parent, const String& name, BGApplication* application);
    virtual ~BIProducer();                                                 // Mac 0x237560 / 0x2375c0, Win 0x591560 / sdd 0x591620

    virtual UInt32 GetFacadeType() const;                                  // Mac 0x237520, Win 0x5914e0 (5)
    virtual String GetName() const;                                        // Mac 0x237530, Win 0x5914f0 (m_name)

    // The number of analyzers, Separation included. Mac 0x237390 (name inferred)
    UInt32 GetAnalyzerCount() const;
    // For every graph: (with more than one graph) a substep of the whole graph (analyzer
    // Separation, min 2), then AnalyzeGraph; stops when the application is cancelled.
    // StepProgress before the loop and after every analyzer. Returns the number of new
    // substeps. Mac 0x2373b0, Win 0x592610 (name inferred)
    UInt32 Analyze(const BIGraph::GraphList& graphs, BasicArray<BIBrickSubstep*>& substeps);
    // Every analyzer of the list over `graph` (stops when cancelled). Inlined on Mac,
    // Win 0x592710 (name inferred)
    UInt32 AnalyzeGraph(BIGraph* graph, BasicArray<BIBrickSubstep*>& substeps);
    // BGApplication::IsCancelled. Mac 0x237500, Win 0x5927b0 (name inferred)
    bool IsCancelled() const;

    BGApplication* GetApplication() const { return m_application; }
    const BasicArray<RefPointer<BIAnalyzer> >& GetAnalyzers() const { return m_analyzers; }

    BGApplication*                      m_application;    // +0x08
    BasicArray<RefPointer<BIAnalyzer> > m_analyzers;      // +0x0c
    String                              m_name;           // +0x1c
    RefPointer<BIAnalyzer>              m_separation;     // +0x2c (names inferred)
    RefPointer<BIAnalyzer>              m_groups;         // +0x30
    RefPointer<BIAnalyzer>              m_articulation;   // +0x34
};

} // namespace LEGO
