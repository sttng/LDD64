// LEGO Digital Designer — restored source
// TU 165 "BIAnalyzerSeparation.cpp" (Mac 0x224600-0x22474f): LEGO::BIAnalyzerSeparation.
#include "BIAnalyzerSeparation.h"

namespace LEGO {

// Mac 0x224640 / 0x224660, Win 0x591fb0 / sdd 0x592020
BIAnalyzerSeparation::~BIAnalyzerSeparation()
{
}

// Mac 0x224610, Win 0x591f50
String BIAnalyzerSeparation::GetName() const
{
    return String("Separation");
}

// Mac 0x224600, Win 0x59c690
UInt32 BIAnalyzerSeparation::Analyze(BIGraph*, BasicArray<BIBrickSubstep*>&)
{
    return 0;
}

} // namespace LEGO
