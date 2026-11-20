// Vortex 5.0.0 build 800 "vortexldd41" - VxDifferential, the parts that survive in both builds
// (restored). Mac TU 479 (0x445e04-0x445ea7; static initialiser 0x445e4e:
// <iostream> + MdtConstraintMaxSize from Mdt/MdtConstraint.h).
#include "Vx/VxDifferential.h"
#include "Mdt/MdtConstraint.h"
#include <iostream>

namespace Vx {

// Mac 0x445e04, Win 0x751830
int VxDifferential::getStaticClassType()
{
    return 0x11c;
}

// Mac 0x445e30, Win 0x751a80
VxReal VxDifferential::getValue(int index) const
{
    return MdtType16GetValue(mConstraint, index);
}

} // namespace Vx
