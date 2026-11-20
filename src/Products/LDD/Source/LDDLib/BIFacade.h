// LEGO Digital Designer — restored source
// LEGO::BIFacade — base of the building-guide workers (generators, analyzers, strategies,
//   filters, programs, producers): a type number, a name and the worker it belongs to.
//
// Destructor in TU 149 "BIGenerator.cpp" (Mac 0x212910 / 0x212930, Win sdd 0x560160).
// Mac vtable 0x72b8dc {pure, pure, D1 0x212910, D0 0x212930}; Object is a virtual base.
// Layout: Mac vptr +0x00, m_parent +0x04; Win vfptr +0x00, vbptr +0x04, m_parent +0x08.
//
// GetFacadeType values: 1 generator (BIGenerator), 2 analyzer (BIAnalyzer) and program
// (BIProgram), 4 strategy (BIStrategy), 5 producer (BIProducer).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"

namespace LEGO {

class BIFacade : public virtual Object {
public:
    // `parent`: the worker this one belongs to — the BIProducer of an analyzer, the BIFilter
    // of a strategy added with BIFilter::AddStrategy(…, a, b, …); null for generators,
    // producers and programs. (name inferred)
    explicit BIFacade(BIFacade* parent = 0) : m_parent(parent) {}

    // 1 generator, 2 analyzer / program, 4 strategy, 5 producer (name inferred)
    virtual UInt32 GetFacadeType() const = 0;                              // idx 0
    // "Constructive", "Destructive", "Articulation", … (name inferred)
    virtual String GetName() const = 0;                                    // idx 1
    virtual ~BIFacade();                                                   // idx 2/3: Mac 0x212910 / 0x212930, Win sdd 0x560160

    BIFacade* GetParent() const { return m_parent; }

    BIFacade* m_parent;   // +0x04 (name inferred)
};

} // namespace LEGO
