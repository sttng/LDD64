// LEGO Digital Designer — restored source
// LEGO::BISubstepFilterBaseplate — rejects substeps that contain a baseplate (a primitive of
//   main group 218).
//
// Home TU 301 "BISubstepFilterBaseplate.cpp" (Mac 0x2af5e0-0x2af8bf).
// RTTI: class LEGO::BISubstepFilterBaseplate : BISubstepFilter. Mac sizeof 4 + Object,
// constructor 0x2af5e0, vtable 0x74440c {Accept 0x2af620, Begin 0x344d60, D1 0x2af680,
// D0 0x2af740, OnFinalRelease 0x2af830}, Object-in vtable 0x744438 {0x2af6e0, 0x2af7b0, thunk
// 0x2af850, …}; Win vtable 0xa2c93c {0x5a1890, 0x56b1c0}, Object-in {0x5a19d0, 0x5a19c0, …},
// sdd 0x5a1710.
#pragma once
#include "BISubstepFilter.h"

namespace LEGO {

class BISubstepFilterBaseplate : public BISubstepFilter {
public:
    BISubstepFilterBaseplate() {}                                          // Mac 0x2af5e0
    virtual ~BISubstepFilterBaseplate();                                   // Mac 0x2af680 / 0x2af740, Win sdd 0x5a1710

    // No primitive of the substep has the "maingroupid" 218. Mac 0x2af620, Win 0x5a1890
    virtual bool Accept(const BIBrick* substep);

protected:
    // Only calls Object's. Mac 0x2af830 (thunk 0x2af850), Win 0x5a19c0
    virtual void OnFinalRelease();
};

} // namespace LEGO
