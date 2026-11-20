// Vortex 5.0.0 build 800 "vortexldd41" - VxAngular1Position3 (restored).
// Mac TU 454 0x42405c-0x425346 (static initialiser 0x4252d6: <iostream>, MdtConstraintMaxSize),
// Win 0x72b830-0x72bef0. Two orientation equations, coordinates "Angular", "Linear0..2".
#include "Vx/VxAngular1Position3.h"
#include "Vx/ConstraintAccessor.h"
#include "Vx/VxFrame.h"
#include "Mdt/MdtConstraint.h"
#include <iostream>

namespace Vx {

// getStaticClassType Mac 0x42405c, isOfClassType Mac 0x424066 / Win 0x72b930, getStaticClassTypes
// Mac 0x42422e (atexit 0x4240f7) / Win 0x72b950, isOfType Mac 0x4245b6 / Win 0x72b9d0
VX_IMPLEMENT_CLASS_TYPES(VxAngular1Position3, VxConstraint, 0x184)
// new Mac 0x42421b / Win 0x72b830, delete Mac 0x424141 / Win 0x72b870 (+0x72b840)
VX_IMPLEMENT_ALLOCATOR(VxAngular1Position3)

const int VxAngular1Position3::kLinear0Coordinate = 1;   // Mac 0x643b00
const int VxAngular1Position3::kLinear1Coordinate = 2;   // Mac 0x643b04
const int VxAngular1Position3::kLinear2Coordinate = 3;   // Mac 0x643b08
const int VxAngular1Position3::kAngularCoordinate = 0;   // Mac 0x643b0c

// C2 Mac 0x425244 (C1 0x4252b0), Win 0x72bdd0
VxAngular1Position3::VxAngular1Position3()
    : VxConstraint(MdtAngular1Position3Create((VinPool*)VxFrame::instance()->mConstraintPool), 2)
{
    addAccessors();
}

// Win 0x72be40
VxAngular1Position3::VxAngular1Position3(VxPart* part0, VxPart* part1, const VxVector3& position,
                                         const VxVector3& primaryAxis, const VxVector3* secondaryAxis)
    : VxConstraint(MdtAngular1Position3Create((VinPool*)VxFrame::instance()->mConstraintPool), 2)
{
    addAccessors();
    setPartAndAttachment(0, part0, position, primaryAxis, secondaryAxis);
    setPartAndAttachment(1, part1, position, primaryAxis, secondaryAxis);
}

// D2 Mac 0x4241b4 (D1 0x424208, D0 0x424158), Win 0x72bef0
VxAngular1Position3::~VxAngular1Position3()
{
    setEnabled(false);
}

// Mac 0x4252c4, Win 0x72bee0
const char* VxAngular1Position3::getClassName() const
{
    return "VxAngular1Position3";
}

// Mac 0x4245fc, Win 0x72ba00
void VxAngular1Position3::addAccessors()
{
    addEquation(0, kSecondaryAxisOrientationName);
    addEquation(1, kThirdAxisOrientationName);
    addCoordinate(0, "Angular");
    addCoordinate(1, "Linear0");
    addCoordinate(2, "Linear1");
    addCoordinate(3, "Linear2");
}

// Mac 0x4240d4, Win 0x72b8d0
void VxAngular1Position3::resetParts()
{
    VxConstraint::resetParts();
    MdtAngular1Position3Reset(mConstraint);
}

// Mac 0x42407c, Win 0x72b8f0
bool VxAngular1Position3::getEquationFrictionIndex(int equation, int* friction) const
{
    if (equation == 0) {
        *friction = 4;
        return true;
    }
    if (equation == 1) {
        *friction = 5;
        return true;
    }
    return false;
}

} // namespace Vx
