// Vortex 5.0.0 build 800 "vortexldd41" - VxDistanceJoint, the parts that survive in both builds
// (restored). Mac TU 481 (0x445f56-0x446013; static initialiser 0x445fba:
// <iostream> + MdtConstraintMaxSize from Mdt/MdtConstraint.h).
#include "Vx/VxDistanceJoint.h"
#include "Mdt/MdtConstraint.h"
#include <iostream>

namespace Vx {

// Mac 0x445f56, Win 0x823d20
int VxDistanceJoint::getStaticClassType()
{
    return 0x11d;
}

// Mac 0x445f82, Win 0x823dd0
VxReal VxDistanceJoint::getDistance() const
{
    return MdtType14GetValue(mConstraint);
}

// Mac 0x445f98, Win 0x823db0
void VxDistanceJoint::setDistance(VxReal d)
{
    MdtType14SetValue(mConstraint, d);
}

} // namespace Vx
