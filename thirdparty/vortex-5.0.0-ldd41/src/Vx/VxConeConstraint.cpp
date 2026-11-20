// Vortex 5.0.0 build 800 "vortexldd41" - VxConeConstraint (restored).
// Mac TU 472 0x43c4f4-0x43cdac (static initialiser 0x43cd3c: <iostream>, MdtConstraintMaxSize),
// Win 0x724a30-0x724f20. One equation ("Primary axis orientation"), no coordinate.
#include "Vx/VxConeConstraint.h"
#include "Vx/ConstraintAccessor.h"
#include "Vx/VxFrame.h"
#include "Mdt/MdtConstraint.h"
#include <iostream>

namespace Vx {

// getStaticClassType Mac 0x43c4f4, isOfClassType Mac 0x43c4fe / Win 0x724af0, getStaticClassTypes
// Mac 0x43c95c (atexit 0x43c587) / Win 0x724c70, isOfType Mac 0x43cce4 / Win 0x724cf0
VX_IMPLEMENT_CLASS_TYPES(VxConeConstraint, VxConstraint, 0x119)
// new Mac 0x43c5e8 / Win 0x7249f0, delete Mac 0x43c5d1 / Win 0x724a30
VX_IMPLEMENT_ALLOCATOR(VxConeConstraint)

namespace {
// Win 0x724b10 (TU-local): a vector perpendicular to v (v x (1,0,0) or v x (0,0,1)-like, the
// products with 0 kept as in the original)
VxVector3 perpendicular(const VxVector3& v)
{
    VxVector3 r;
    const VxReal x0 = v[0] * 0.0;
    if (v[2] * 0.0 + x0 + v[1] < v[1] * 0.0 + x0 + v[2]) {
        const VxReal y = v[1];
        r[0] = y * 0.0 - v[2];
        r[1] = v[2] * 0.0 - x0;
        r[2] = v[0] - y * 0.0;
    } else {
        const VxReal z = v[2];
        r[0] = v[1] - z * 0.0;
        r[1] = z * 0.0 - v[0];
        r[2] = x0 - v[1] * 0.0;
    }
    return r;
}
} // namespace

// C2 Mac 0x43c8dc (C1 0x43c948), Win 0x724e00
VxConeConstraint::VxConeConstraint()
    : VxConstraint(MdtConeLimitCreate((VinPool*)VxFrame::instance()->mConstraintPool), 2)
{
    addAccessors();
}

// Win 0x724e70
VxConeConstraint::VxConeConstraint(VxPart* part0, VxPart* part1, const VxVector3& axis0, const VxVector3& axis1)
    : VxConstraint(MdtConeLimitCreate((VinPool*)VxFrame::instance()->mConstraintPool), 2)
{
    addAccessors();
    setPart(0, part0);
    setPart(1, part1);
    setPart0Axis(axis0);
    setPart1Axis(axis1);
}

// D2 Mac 0x43c658 (D1 0x43c6ac, D0 0x43c5fc), Win 0x724f20
VxConeConstraint::~VxConeConstraint()
{
    setEnabled(false);
}

// Mac 0x43cd2a, Win 0x724f10
const char* VxConeConstraint::getClassName() const
{
    return "VxConeConstraint";
}

// Mac 0x43c6c0, Win 0x724d20
void VxConeConstraint::addAccessors()
{
    addEquation(0, kPrimaryAxisOrientationName);
}

// Mac 0x43c564, Win 0x724a90
void VxConeConstraint::resetParts()
{
    VxConstraint::resetParts();
    MdtConeLimitReset(mConstraint);
}

// Mac 0x43c536, Win 0x724ab0
void VxConeConstraint::setConeAngle(VxReal angle)
{
    MdtConeLimitSetConeHalfAngle(mConstraint, angle);
    wakeBodies();
}

// Win 0x724b90
void VxConeConstraint::setPart0Axis(const VxVector3& a)
{
    const VxVector3 v = a;
    const VxVector3 s = perpendicular(v);
    MdtConstraintSetBodyAxes(mConstraint, 0, a[0], a[1], a[2], s[0], s[1], s[2]);
}

// Win 0x724c00
void VxConeConstraint::setPart1Axis(const VxVector3& a)
{
    const VxVector3 v = a;
    const VxVector3 s = perpendicular(v);
    MdtConstraintSetBodyAxes(mConstraint, 1, a[0], a[1], a[2], s[0], s[1], s[2]);
}

} // namespace Vx
