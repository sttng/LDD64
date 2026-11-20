// Vortex 5.0.0 build 800 "vortexldd41" - VxMotorConstraint (restored).
// Mac 0x463d8a-0x464682 (end of TU 504; static initialiser 0x464612: <iostream>,
// MdtConstraintMaxSize), Win 0x723e00-0x724120.
#include "Vx/VxMotorConstraint.h"
#include "Vx/ConstraintAccessor.h"
#include "Vx/VxFrame.h"
#include "Mdt/MdtConstraint.h"
#include <iostream>

namespace Vx {

// getStaticClassType Mac 0x463d8a, isOfClassType Mac 0x463d94 / Win 0x723e60,
// getStaticClassTypes Mac 0x464200, isOfType Mac 0x464588 / Win 0x723f00
VX_IMPLEMENT_CLASS_TYPES(VxMotorConstraint, VxConstraint, 0x197)
// new Mac 0x463f4b / Win 0x723d60, delete Mac 0x463e71 / Win 0x723e20
VX_IMPLEMENT_ALLOCATOR(VxMotorConstraint)

// C2 Mac 0x464180 (C1 0x4641ec), Win 0x723e80
VxMotorConstraint::VxMotorConstraint()
    : VxConstraint(MdtMotorCreate((VinPool*)VxFrame::instance()->mConstraintPool), 2)
{
    addCoordinateAccessor();
}

// D2 Mac 0x463ee4 (D1 0x463f38, D0 0x463e88 / Win 0x724120)
VxMotorConstraint::~VxMotorConstraint()
{
    setEnabled(false);
}

// Mac 0x4645ce, Win 0x724110
const char* VxMotorConstraint::getClassName() const
{
    return "VxMotorConstraint";
}

// Mac 0x463f5e, Win 0x723f30
void VxMotorConstraint::addCoordinateAccessor()
{
    std::string name("Coordinate");
    mCoordinates.push_back(new ConstraintCoordinate(this, 0, name));
}

// Mac 0x463daa
void VxMotorConstraint::setAngular(int, bool angular)
{
    ((MdtMotor*)getMdtConstraint())->limit.kind = angular;
}

// Mac 0x463de0, Win 0x723e40
void VxMotorConstraint::setLinearCoordinateReferencePart(int coordinate, int value)
{
    MdtConstraintGetLimit(getMdtConstraint(), coordinate)->me8 = value;
}

// Mac 0x463e04, Win 0x723e00
void VxMotorConstraint::resetParts()
{
    VxConstraint::resetParts();
    MdtMotorReset(getMdtConstraint());
}

} // namespace Vx
