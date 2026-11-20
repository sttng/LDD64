// Vortex 5.0.0 build 800 "vortexldd41" - ConstraintAccessor, ConstraintEquation,
// ConstraintCoordinate (restored).
// Mac TU 511 0x474768-0x474c9c (static initialiser 0x474c2c: <iostream>, MdtConstraintMaxSize),
// Win 0x715db0, 0x718590-0x7185b0. The two VxArray push_back helpers of the TU (Mac 0x474816 /
// 0x47493a, with 0x4747ba / 0x4747e6) are the template instances the constraint constructors use.
#include "Vx/ConstraintAccessor.h"
#include "Mdt/MdtConstraint.h"
#include <iostream>

namespace Vx {

// D1 Mac 0x474a5e, D0 0x4747ec, Win 0x715db0
ConstraintAccessor::~ConstraintAccessor()
{
}

// D1 Mac 0x474aec, D0 0x474a78, Win 0x7185b0
ConstraintEquation::~ConstraintEquation()
{
}

// Mac 0x47478a, Win 0x718590
VxConstraint* ConstraintEquation::getConstraint() const
{
    return mConstraint;
}

// Mac 0x474796, Win 0x7185a0
int ConstraintEquation::getIndex() const
{
    return mIndex;
}

// D1 Mac 0x474b52, D0 0x474bb8, Win 0x7185b0 (ICF)
ConstraintCoordinate::~ConstraintCoordinate()
{
}

// Mac 0x4747a2, Win 0x718590 (ICF)
VxConstraint* ConstraintCoordinate::getConstraint() const
{
    return mConstraint;
}

// Mac 0x4747ae, Win 0x7185a0 (ICF)
int ConstraintCoordinate::getIndex() const
{
    return mIndex;
}

} // namespace Vx
