// Vortex 5.0.0 build 800 "vortexldd41" - VxGearRatio (restored header): the Mac
// functions of TU 489 + the class id, kept for VxUniverse::print and VxStateConstraint (names from
// RTTI / the Win warning strings). Otherwise Win-only: ctor Win 0x750d40, sdd 0x7517d0, the setters
// / getters Win 0x750da0-0x751360 over its Mdt gear-ratio constraint (MdtType10) are not restored.
// LDD never creates one.
#ifndef VX_VXGEARRATIO_H
#define VX_VXGEARRATIO_H

#include "Vx/VxConstraint.h"

namespace Vx {

class VxGearRatio : public VxConstraint {
public:
    static int getStaticClassType();                    // Mac 0x4574d8, Win 0x750d30: 0x11f
    VxReal getRatio() const;                            // Mac 0x457564, Win 0x750f40 (MdtType10GetValue) (name inferred)
    void setRatio(VxReal r);                            // Mac 0x4574e2 (Mdt constraint +0xb00) (name inferred)
    // Mac 0x45751c, Win 0x751190 ("VxGearRatio::setPartReferencePosition: Part index must be 0 or 1.")
    void setPartReferencePosition(unsigned int part, VxReal position);
};

} // namespace Vx

#endif
