// Vortex 5.0.0 build 800 "vortexldd41" - VxConstraintFriction (restored for
// Mdt; Mac 0x42fe38-0x42ff10 + dtor 0x4303e2 inside the VxBallAndSocket TU 457, Win
// 0x72bf20-0x72c000 + 0x7859b0 / 0x72c990). Method names inferred.
#include "Vx/VxConstraintFriction.h"

namespace Vx {

// Mac 0x4303e2, Win 0x72c990
VxConstraintFriction::~VxConstraintFriction()
{
}

// Mac 0x42fe38, Win 0x72bf20: marks the owning constraint's frictions dirty when enabled
void VxConstraintFriction::setProportional(bool b)
{
    mProportional = b;
    if (mEnabled && mDirty != 0)
        *mDirty = 1;
}

// Mac 0x42fe56, Win 0x72bf40
bool VxConstraintFriction::isProportional() const
{
    return mProportional;
}

// Mac 0x42fe62, Win 0x72bf50
void VxConstraintFriction::setMaxForce(VxReal f)
{
    mMaxForce = f;
}

// Mac 0x42fe74, Win 0x72bf60
VxReal VxConstraintFriction::getMaxForce() const
{
    return mMaxForce;
}

// Mac 0x42fe80, Win 0x72bf70
void VxConstraintFriction::setLoss(VxReal loss)
{
    mLoss = loss;
}

// Mac 0x42fe92, Win 0x72bf80
VxReal VxConstraintFriction::getLoss() const
{
    return mLoss;
}

// Mac 0x42fe9e, Win 0x72bf90
void VxConstraintFriction::setCoefficient(VxReal mu)
{
    mCoefficient = mu;
}

// Mac 0x42feb0, Win 0x72bfa0
VxReal VxConstraintFriction::getCoefficient() const
{
    return mCoefficient;
}

// Mac 0x42febc, Win 0x72bfb0
void VxConstraintFriction::setScale(VxReal s)
{
    mScale = s;
}

// Mac 0x42fece, Win 0x72bfc0
VxReal VxConstraintFriction::getScale() const
{
    return mScale;
}

// Mac 0x42feda, Win 0x72bfd0
void VxConstraintFriction::setEnabled(bool b)
{
    mEnabled = b;
    if (b && mDirty != 0)
        *mDirty = 1;
}

// Mac 0x42fef6, Win 0x72bff0
bool VxConstraintFriction::isEnabled() const
{
    return mEnabled;
}

// Mac 0x42ff02, Win 0x72c000
void VxConstraintFriction::setAxis(int axis)
{
    mAxis = axis;
}

// Mac 0x42ff10, Win 0x7859b0
int VxConstraintFriction::getAxis() const
{
    return mAxis;
}

} // namespace Vx
