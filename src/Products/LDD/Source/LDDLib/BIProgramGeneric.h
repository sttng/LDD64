// LEGO Digital Designer — restored source
// LEGO::BIProgramGeneric — the BIFilter preset of the automatic building guide: Substeps (1),
//   Sweeping (0.1), Integration (1), optional GroundUp (minimum point), Nearby (2 bricks) and
//   Balance with the given weights (when >= 0), Articulation (a filter when
//   `articulationFilter`, else pass 0 / fail 1) and Connectivity (a filter).
//   BuildingGuideGenerator (Mac 0x297060) and BGAutomaticMode::Enter create (true, true, 0.5, 0.5, 0.5).
//
// Home TU 162 "BIProgramGeneric.cpp" (Mac 0x221ab0-0x222f6f), which also holds BIProgram's and
// BIFilter's inline functions and BIFilter::ConditionTrue.
// RTTI: class LEGO::BIProgramGeneric : BIFilter. Mac sizeof 0x1c + Object, vtable 0x77013c
// (BIFilter's slots, GetName 0x222b50, D1 0x222b80, D0 0x222be0); Win vtable 0xa2bc90 (GetName
// 0x564360), sdd 0x565cf0.
#pragma once
#include "BIFilter.h"

namespace LEGO {

class BIProgramGeneric : public BIFilter {
public:
    // Mac 0x221ab0 (complete constructor), inlined on Win (names inferred)
    BIProgramGeneric(bool articulationFilter, bool connectivityFlag, Real32 groundUpWeight, Real32 nearbyWeight,
                     Real32 balanceWeight);
    virtual ~BIProgramGeneric();                       // Mac 0x222b80 / 0x222be0, Win sdd 0x565cf0

    virtual String GetName() const;                    // Mac 0x222b50, Win 0x564360 ("Generic")
};

} // namespace LEGO
