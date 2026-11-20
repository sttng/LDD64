// Vortex 5.0.0 build 800 "vortexldd41" - VxCylindrical (restored).
// Mac TU 476 0x442dc4-0x4440e2 (static initialiser 0x444072: <iostream>, MdtConstraintMaxSize),
// Win 0x724150-0x7249c0. Coordinates "Linear" and "Angular", four equations.
#include "Vx/VxCylindrical.h"
#include "Vx/ConstraintAccessor.h"
#include "Vx/VxFrame.h"
#include "Mdt/MdtConstraint.h"
#include <iostream>

namespace Vx {

// getStaticClassType Mac 0x442dc4, isOfClassType Mac 0x442dce / Win 0x7242d0, getStaticClassTypes
// Mac 0x442fc6 (atexit 0x442e8f / Win 0x724210) / Win 0x7242f0, isOfType Mac 0x44334e / Win 0x724370
VX_IMPLEMENT_CLASS_TYPES(VxCylindrical, VxConstraint, 0x11b)
// new Mac 0x442fb3 / Win 0x724150, delete Mac 0x442ed9 / Win 0x724190
VX_IMPLEMENT_ALLOCATOR(VxCylindrical)

const int VxCylindrical::kLinearCoordinate = 0;    // Mac 0x643e30
const int VxCylindrical::kAngularCoordinate = 1;   // Mac 0x643e34

// C2 Mac 0x443fe0 (C1 0x44404c), Win 0x724790
VxCylindrical::VxCylindrical()
    : VxConstraint(MdtCylindricalCreate((VinPool*)VxFrame::instance()->mConstraintPool), 2)
{
    addAccessors();
}

// Win 0x724800
VxCylindrical::VxCylindrical(VxPart* part0, VxPart* part1, const VxVector3& axis)
    : VxConstraint(MdtCylindricalCreate((VinPool*)VxFrame::instance()->mConstraintPool), 2)
{
    addAccessors();
    setPart(0, part0);
    setPart(1, part1);
    MdtConstraintSetAxis(mConstraint, axis[0], axis[1], axis[2]);
    setPartAttachmentPositionRel(0, 0, 0, 0);
    setPartAttachmentPositionRel(1, 0, 0, 0);
}

// D2 Mac 0x442f4c (D1 0x442fa0, D0 0x442ef0), Win 0x7249c0 (D2 0x724240)
VxCylindrical::~VxCylindrical()
{
    setEnabled(false);
}

// Mac 0x444060, Win 0x7249b0
const char* VxCylindrical::getClassName() const
{
    return "VxCylindrical";
}

// Mac 0x443394, Win 0x7243a0
void VxCylindrical::addAccessors()
{
    addEquation(0, kSecondaryAxisPositionName);
    addEquation(1, kThirdAxisPositionName);
    addEquation(3, kSecondaryAxisOrientationName);
    addEquation(2, kThirdAxisOrientationName);
    addCoordinate(0, "Linear");
    addCoordinate(1, "Angular");
}

// Mac 0x442e6c, Win 0x7241f0
void VxCylindrical::resetParts()
{
    VxConstraint::resetParts();
    MdtCylindricalReset(mConstraint);
}

// Mac 0x442de4, Win 0x724260
bool VxCylindrical::getEquationFrictionIndex(int equation, int* friction) const
{
    switch (equation) {
    case 0: *friction = 1; return true;
    case 1: *friction = 2; return true;
    case 2: *friction = 5; return true;
    case 3: *friction = 4; return true;
    }
    return false;
}

} // namespace Vx
