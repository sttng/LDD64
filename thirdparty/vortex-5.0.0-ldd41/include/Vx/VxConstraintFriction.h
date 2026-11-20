// Vortex 5.0.0 build 800 "vortexldd41" - VxConstraintFriction (restored header):
// out-of-line methods Mac TU 457 0x42fe38-0x42ff10, Win 0x72bf20-0x72c000 (+ 0x7859b0); method
// names inferred from the fields they touch. Mac vtable 0x75d2c8: D1, D0, slots 2-15 (Win: one
// scalar deleting dtor).
//
// Friction on one constraint coordinate. Six are embedded in every MdtConstraint (Mac +0x5a4
// stride 0x34, Win +0x5b0 stride 0x38), built in place by MdtConstraintCreateFromPool (Mac
// 0x48dc64) and handed out by VxConstraint::getCoordinateFriction (Mac 0x43d006). After each step
// MdtPartitionGetForces (Mac 0x491b42) turns a proportional one into a force limit:
// maxForce = coefficient * |constraint force perpendicular to the axis|.
#ifndef VX_VXCONSTRAINTFRICTION_H
#define VX_VXCONSTRAINTFRICTION_H

#include "Vx/VxPlatform.h"

namespace Vx {

class VxConstraintFriction {
public:
    // inlined (Mac 0x48dc64 / Win 0x72de10)
    VxConstraintFriction()
        : mEnabled(false), mProportional(false), mAxis(0), mMaxForce(0), mLoss(0),
          mCoefficient(0), mScale(1.0), mStatus(0), mDirty(0)
    {
    }
    virtual ~VxConstraintFriction();                    // slot 0/1 Mac 0x4303e2, Win 0x72c990

    virtual void setProportional(bool b);               // slot 2  Mac 0x42fe38, Win 0x72bf20
    virtual bool isProportional() const;                // slot 3  Mac 0x42fe56, Win 0x72bf40
    virtual void setMaxForce(VxReal f);                 // slot 4  Mac 0x42fe62, Win 0x72bf50
    virtual VxReal getMaxForce() const;                 // slot 5  Mac 0x42fe74, Win 0x72bf60
    virtual void setLoss(VxReal loss);                  // slot 6  Mac 0x42fe80, Win 0x72bf70
    virtual VxReal getLoss() const;                     // slot 7  Mac 0x42fe92, Win 0x72bf80
    virtual void setCoefficient(VxReal mu);             // slot 8  Mac 0x42fe9e, Win 0x72bf90
    virtual VxReal getCoefficient() const;              // slot 9  Mac 0x42feb0, Win 0x72bfa0
    virtual void setScale(VxReal s);                    // slot 10 Mac 0x42febc, Win 0x72bfb0
    virtual VxReal getScale() const;                    // slot 11 Mac 0x42fece, Win 0x72bfc0
    virtual void setEnabled(bool b);                    // slot 12 Mac 0x42feda, Win 0x72bfd0
    virtual bool isEnabled() const;                     // slot 13 Mac 0x42fef6, Win 0x72bff0
    virtual void setAxis(int axis);                     // slot 14 Mac 0x42ff02, Win 0x72c000
    virtual int getAxis() const;                        // slot 15 Mac 0x42ff10, Win 0x7859b0

    // Layout (Mac 0x34 / Win 0x38):
#if defined(_WIN32)
    // MSVC pads the vfptr slot to the class alignment (8, the doubles): the members start at +8
    // (Win 0x72de10 constructs the bools at +8 / +9 and the axis at +0xc). Explicit here so that
    // GCC-style layouts (zig / mingw) match.
    int mVfptrPad;          // Win +0x04 (never written)
#endif
    bool mEnabled;          // +0x04 / Win +0x08
    bool mProportional;     // +0x05 / +0x09 maxForce follows the normal force (MdtPartitionGetForces)
    int mAxis;              // +0x08 / +0x0c body (0 / 1) whose attachment frame gives the direction
    VxReal mMaxForce;       // +0x0c / +0x10
    VxReal mLoss;           // +0x14 / +0x18
    VxReal mCoefficient;    // +0x1c / +0x20
    VxReal mScale;          // +0x24 / +0x28 (1)
    int mStatus;            // +0x2c / +0x30 cleared by MdtConstraintResetForces (Mac 0x48df0e)
    char* mDirty;           // +0x30 / +0x34 -> MdtConstraint::frictionDirty, set when enabled
};

} // namespace Vx

#endif
