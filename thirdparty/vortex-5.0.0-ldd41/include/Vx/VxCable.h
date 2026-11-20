// Vortex 5.0.0 build 800 "vortexldd41" - VxCable: partial declaration.
// The cable module is Win-only (Win 0x775800-0x782000); Mac kept only the class id and one
// accessor (Mac TU 463), both used by VxMechanism::getReferencePart (Mac 0x4222b0). Real layout:
// VxAssemblyBase + VxCableBase at Mac +0x408; only what VxMechanism uses is declared.
#ifndef VX_VXCABLE_H
#define VX_VXCABLE_H

#include "Vx/VxAssemblyBase.h"

namespace Vx {

class VxCable : public VxAssemblyBase {
public:
    static int getStaticClassType();                    // Mac 0x43701a: 0x17e
    // part `index` of segment `segment` (segment 0 = the cable base at Mac +0x408), 0 when out
    // of range (Mac 0x437072 -> 0x43704e / 0x437024, Win 0x77ac80) (name inferred)
    VxPart* getPart(int segment, int index) const;
};

} // namespace Vx

#endif
