// LEGO Digital Designer — restored source
// TU 167 "BIAnalyzer.cpp" (Mac 0x2262f0-0x22686f): LEGO::BIAnalyzer. See BIAnalyzer.h.
// GetFacadeType, the destructor and GetWeight are inline in the original (emitted in TU 163,
// Mac 0x2230a0-0x223140). Template code of the TU: BasicMap<BIBrickSubstep::Attribute, bool>
// (destructors 0x226700 / 0x226790, Win sdd 0x578990; erase 0x2267e0, Win 0x577dc0).
#include "BIAnalyzer.h"
#include "BIProducer.h"
#include "BIBrickRepository.h"
#include "BIBrickPrimitive.h"

namespace LEGO {

// Mac 0x2262f0 (base-object constructor), inlined on Win
BIAnalyzer::BIAnalyzer(BIProducer* producer, Real32 priority, Real32 weight)
    : BIFacade(producer),
      m_weight(weight),
      m_priority(priority)
{
}

// Mac 0x2230b0 / 0x2230d0 (TU 163), Win sdd 0x591470
BIAnalyzer::~BIAnalyzer()
{
}

// Mac 0x2230a0 (TU 163), Win 0x5913d0
UInt32 BIAnalyzer::GetFacadeType() const
{
    return 2;
}

// Mac 0x223110 (TU 163), Win 0x5913e0
Real32 BIAnalyzer::GetWeight() const
{
    return m_weight;
}

// Mac 0x226760, Win 0x559e90 (shared empty function)
void BIAnalyzer::GetAttributes(BIBrickSubstep::AttributeMap&) const
{
}

// Mac 0x226730
bool BIAnalyzer::AddSubstep(const BasicSet<BIBrickPrimitive*>& primitives, BasicArray<BIBrickSubstep*>& substeps,
                            UInt32 minSize, UInt32 maxSize) const
{
    return CreateSubstep(primitives, substeps, minSize, maxSize, this);
}

// Mac 0x226350
bool BIAnalyzer::CreateSubstep(const BasicSet<BIBrickPrimitive*>& primitives, BasicArray<BIBrickSubstep*>& substeps,
                               UInt32 minSize, UInt32 maxSize, const BIAnalyzer* analyzer)
{
    if (primitives.GetSize() < minSize)
        return false;
    if (maxSize != 0 && maxSize < primitives.GetSize())
        return false;

    BIBrickSubstep::AttributeMap attributes;
    analyzer->GetAttributes(attributes);
    for (BasicArray<BIBrickSubstep*>::Iterator it = substeps.Begin(); it != substeps.End(); ++it) {
        if ((*it)->IsEqual(primitives)) {
            (*it)->SetAttributes(attributes);
            return false;
        }
    }

    BIBrickSubstep* substep;
    {
        RefPointer<BIBrickSubstep> created = new BIBrickSubstep(primitives, analyzer, 1.0f);
        substep = static_cast<BIBrickSubstep*>(BIBrickRepository::GetInstance().AddSubstep(created));
    }
    substep->SetAttributes(attributes);
    substeps.PushBack(substep);
    return true;
}

// Mac 0x226340
bool BIAnalyzer::IsActive() const
{
    return true;
}

// Mac 0x226770
bool BIAnalyzer::IsCancelled() const
{
    return GetProducer()->IsCancelled();
}

BIProducer* BIAnalyzer::GetProducer() const
{
    return static_cast<BIProducer*>(m_parent);
}

} // namespace LEGO
