// Vortex 5.0.0 build 800 "vortexldd41" - VxWinch, the parts that survive in both builds
// (restored). Mac 0x486ccc-0x486d8a (static initialiser 0x486d32: <iostream>
// + MdtConstraintMaxSize from Mdt/MdtConstraint.h), Win 0x823350 / 0x823540 / 0x823550.
#include "Vx/VxWinch.h"
#include "Mdt/MdtConstraint.h"
#include <iostream>

namespace Vx {

// Mac 0x486ccc, Win 0x823350
int VxWinch::getStaticClassType()
{
    return 0x129;
}

// Mac 0x486cf8, Win 0x823550
void VxWinch::setLength(VxReal length)
{
    MdtWinchSetLength(mConstraint, length);
}

// Mac 0x486d1c, Win 0x823540
VxReal VxWinch::getLength() const
{
    return MdtWinchGetLength(mConstraint);
}

} // namespace Vx
