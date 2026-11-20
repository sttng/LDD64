// Vortex 5.0.0 build 800 "vortexldd41" - VxHinge (restored).
// Mac TU 494 0x45a07c-0x45b3d2 (static initialiser 0x45b362: <iostream>, MdtConstraintMaxSize),
// Win 0x717cf0-0x7184f0. One angular coordinate ("Angular"), five equations.
#include "Vx/VxHinge.h"
#include "Vx/ConstraintAccessor.h"
#include "Vx/VxFrame.h"
#include "Mdt/MdtConstraint.h"
#include <iostream>

namespace Vx {

// getStaticClassType Mac 0x45a07c, isOfClassType Mac 0x45a086 / Win 0x717ec0, getStaticClassTypes
// Mac 0x45a2b4 (atexit 0x45a17d) / Win 0x717ee0, isOfType Mac 0x45a63a / Win 0x717f60
VX_IMPLEMENT_CLASS_TYPES(VxHinge, VxConstraint, 0x120)
// new Mac 0x45a2a1 / Win 0x717cf0, delete Mac 0x45a1c7 / Win 0x717d30 (+0x717d00)
VX_IMPLEMENT_ALLOCATOR(VxHinge)

const int VxHinge::kAngularCoordinate = 0; // Mac 0x64407c

// C2 Mac 0x45b2d0 (C1 0x45b33c), Win 0x7183a0
VxHinge::VxHinge()
    : VxConstraint(MdtHingeCreate((VinPool*)VxFrame::instance()->mConstraintPool), 2)
{
    addAccessors();
}

// Win 0x718410
VxHinge::VxHinge(VxPart* part0, VxPart* part1, const VxVector3& position, const VxVector3& axis)
    : VxConstraint(MdtHingeCreate((VinPool*)VxFrame::instance()->mConstraintPool), 2)
{
    addAccessors();
    setPart(0, part0);
    setPart(1, part1);
    MdtConstraintSetAxis(mConstraint, axis[0], axis[1], axis[2]);
    MdtConstraintSetPosition(mConstraint, position[0], position[1], position[2]);
}

// D2 Mac 0x45a23a (D1 0x45a28e, D0 0x45a1de), Win 0x7184f0
VxHinge::~VxHinge()
{
    setEnabled(false);
}

// Mac 0x45b350, Win 0x7184e0
const char* VxHinge::getClassName() const
{
    return "VxHinge";
}

// Mac 0x45a680, Win 0x717f90
void VxHinge::addAccessors()
{
    addEquation(0, kPrimaryAxisPositionName);
    addEquation(1, kSecondaryAxisPositionName);
    addEquation(2, kThirdAxisPositionName);
    addEquation(4, kSecondaryAxisOrientationName);
    addEquation(3, kThirdAxisOrientationName);
    addCoordinate(0, "Angular");
}

// Mac 0x45a15a, Win 0x717d90
void VxHinge::resetParts()
{
    VxConstraint::resetParts();
    MdtHingeReset(mConstraint);
}

// Mac 0x45a09c, Win 0x717e10
bool VxHinge::getEquationFrictionIndex(int equation, int* friction) const
{
    switch (equation) {
    case 0: *friction = 0; return true;
    case 1: *friction = 1; return true;
    case 2: *friction = 2; return true;
    case 3: *friction = 5; return true;
    case 4: *friction = 4; return true;
    }
    return false;
}

// Mac 0x45a124, Win 0x717e90
void VxHinge::setPositionEquationReferencePart(int partIndex)
{
    ((MdtHinge*)mConstraint)->m_be0 = partIndex;
}

// Win 0x717db0
void VxHinge::setPosition(const VxVector3& p)
{
    MdtConstraintSetPosition(mConstraint, p[0], p[1], p[2]);
}

// Win 0x717de0
void VxHinge::setAxis(const VxVector3& a)
{
    MdtConstraintSetAxis(mConstraint, a[0], a[1], a[2]);
}

} // namespace Vx
