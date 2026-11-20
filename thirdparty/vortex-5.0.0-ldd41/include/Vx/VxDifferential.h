// Vortex 5.0.0 build 800 "vortexldd41" - VxDifferential (restored header: the
// Mac remnant TU 479). A constraint on an MdtType16 (three values after the MdtConstraint head).
// Mac keeps the class id and one getter (TU 479, 0x445e04-0x445ea7, + the MdtConstraintMaxSize
// copy); the rest is Win-only (vtable below; bodies 0x751840-0x751a50 and 0x751ca0-0x7522a0
// belong with the Win-only modules, which are not restored here).
#ifndef VX_VXDIFFERENTIAL_H
#define VX_VXDIFFERENTIAL_H

#include "Vx/VxConstraint.h"

namespace Vx {

class VxDifferential : public VxConstraint {
public:
    // Mac 0x445e04, Win 0x751830: 0x11c
    static int getStaticClassType();
    // Mac 0x445e30, Win 0x751a80: MdtType16GetValue(mConstraint, index) (name inferred)
    VxReal getValue(int index) const;

    // Win vtable (Win-only; Mac slot numbers): 0 0x751d90, 1 0x751ca0, D 0x7522a0 / 0x752290,
    // 4 0x71fec0, 5 0x751cc0, 6 0x71ffb0, 7 0x71ff80, 8 0x71ffe0, 9 0x720100, 10 0x720080,
    // 11 0x720150, 12 0x721a70, 13 0x721ba0, 14 0x7217d0, 15 0x7518a0, 16 0x716340
    virtual ~VxDifferential();

private:
    VxDifferential();
};

} // namespace Vx

#endif
