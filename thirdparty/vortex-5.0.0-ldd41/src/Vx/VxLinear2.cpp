// Vortex 5.0.0 build 800 "vortexldd41" - VxLinear2 (restored).
// Mac TU 502 0x45d882-0x45e31a (static initialiser 0x45e2aa: <iostream>, MdtConstraintMaxSize),
// Win 0x715ed0-0x716360. Two position equations, no coordinate.
#include "Vx/VxLinear2.h"
#include "Vx/ConstraintAccessor.h"
#include "Vx/VxFrame.h"
#include "Mdt/MdtConstraint.h"
#include <iostream>

namespace Vx {

// getStaticClassType Mac 0x45d882, isOfClassType Mac 0x45d88c / Win 0x715fb0, getStaticClassTypes
// Mac 0x45da1e (atexit 0x45d8e7) / Win 0x715fd0, isOfType Mac 0x45dda6 / Win 0x716050
VX_IMPLEMENT_CLASS_TYPES(VxLinear2, VxConstraint, 0x122)
// new Mac 0x45da0b / Win 0x715e90, delete Mac 0x45d931 / Win 0x715ed0
VX_IMPLEMENT_ALLOCATOR(VxLinear2)

// C2 Mac 0x45e218 (C1 0x45e284), Win 0x716200
VxLinear2::VxLinear2()
    : VxConstraint(MdtLinear2Create((VinPool*)VxFrame::instance()->mConstraintPool), 2)
{
    addAccessors();
}

// Win 0x716270
VxLinear2::VxLinear2(VxPart* part0, VxPart* part1, const VxVector3& position, const VxVector3& axis)
    : VxConstraint(MdtLinear2Create((VinPool*)VxFrame::instance()->mConstraintPool), 2)
{
    addAccessors();
    setPart(0, part0);
    setPart(1, part1);
    MdtConstraintSetPosition(mConstraint, position[0], position[1], position[2]);
    MdtConstraintSetAxis(mConstraint, axis[0], axis[1], axis[2]);
}

// D2 Mac 0x45d9a4 (D1 0x45d9f8, D0 0x45d948), Win 0x716360
VxLinear2::~VxLinear2()
{
    setEnabled(false);
}

// Mac 0x45e298, Win 0x716350
const char* VxLinear2::getClassName() const
{
    return "VxLinear2";
}

// Mac 0x45ddec, Win 0x716080
void VxLinear2::addAccessors()
{
    addEquation(0, kSecondaryAxisPositionName);
    addEquation(1, kThirdAxisPositionName);
}

// Mac 0x45d8c4, Win 0x715f30
void VxLinear2::resetParts()
{
    VxConstraint::resetParts();
    MdtLinear2Reset(mConstraint);
}

// Win 0x715f50
void VxLinear2::setPosition(const VxVector3& p)
{
    MdtConstraintSetPosition(mConstraint, p[0], p[1], p[2]);
}

// Win 0x715f80
void VxLinear2::setAxis(const VxVector3& a)
{
    MdtConstraintSetAxis(mConstraint, a[0], a[1], a[2]);
}

} // namespace Vx
