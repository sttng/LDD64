// Vortex 5.0.0 build 800 "vortexldd41" - VxStateConstraint: the saved state of a constraint
// (restored header; Mac TU 520 0x47b736-0x47d60a, Win 0x7f7cc0-0x7f9730).
// Created by VxState::save for every constraint of an assembly.
#ifndef VX_VXSTATECONSTRAINT_H
#define VX_VXSTATECONSTRAINT_H

#include "Vx/VxStateBase.h"
#include "Vx/VxVector3.h"

namespace Vx {

class VxConstraint;
class VxPart;

// VxStateConstraint (Mac vtable 0x75e948, 11 slots; Win 10; slot 10 is VxStateBase's).
// Mac 0x4c bytes (new(0x4c)), Win 0x80. (member names inferred)
class VxStateConstraint : public VxStateBase {
public:
    // one coordinate of the constraint: Mac 0x50 bytes (default Mac 0x47b736 / 0x47b79c),
    // Win 0x58 (doubles 8-aligned)
    struct CoordinateState {
        CoordinateState();
        bool limitsActive;          // +0x00 getLimitsActive
        VxReal lowerLimit;          // Mac +0x04, Win +0x08 getLimitPosition(i, 0) (= DBL_MAX)
        VxReal upperLimit;          // Mac +0x0c, Win +0x10 getLimitPosition(i, 1) (= -DBL_MAX)
        VxReal lowerLimitVelocity;  // Mac +0x14, Win +0x18 getLimitVelocity(i, 0)
        VxReal upperLimitVelocity;  // Mac +0x1c, Win +0x20 getLimitVelocity(i, 1)
        int control;                // Mac +0x24, Win +0x28 getControl
        VxReal motorDesiredVelocity; // Mac +0x28, Win +0x30
        VxReal positionOffset;      // Mac +0x30, Win +0x38 getCoordinatePositionOffset
        VxReal motorLoss;           // Mac +0x38, Win +0x40
        VxReal lockPosition;        // Mac +0x40, Win +0x48
        VxReal storedPosition;      // Mac +0x48, Win +0x50 getCoordinateStoredPosition
    };
    // one part attachment: Mac 0x50 bytes (default Mac 0x47b7b0 / 0x47b80a), Win 0x68
    struct AttachmentState {
        AttachmentState();
        VxPart* part;               // +0x00 (found again by name when 0)
        std::string partName;       // +0x04
        VxVector3 position;         // Mac +0x08 (part frame)
        VxVector3 primaryAxis;      // Mac +0x20 (= (1, 0, 0))
        VxVector3 secondaryAxis;    // Mac +0x38 (= (0, 1, 0))
    };

    explicit VxStateConstraint(VxConstraint* c);        // C1 Mac 0x47cb78 -> C2 0x47ca3c, Win 0x7f8970
    VxStateConstraint(const VxStateConstraint& other);  // C1 Mac 0x47ca22 -> C2 0x47c836, Win 0x7f8500
    virtual ~VxStateConstraint();                       // Mac D1 0x47d2e0 / D0 0x47d41c, Win 0x7f93f0
    virtual VxStateBase* clone() const;                 // slot 3 Mac 0x47d55e, Win 0x7f96c0
    virtual void save();                                // slot 4 Mac 0x47c818, Win 0x7f8a00
    virtual void restore(bool notify);                  // slot 5 Mac 0x47beb0, Win 0x7f8360
    virtual void restoreKinematic(bool notify);         // slot 6 Mac 0x47be86, Win 0x7f8380
    virtual void restoreMode(int mode, bool notify);    // slot 7 Mac 0x47be34, Win 0x7f83a0 (0 full, 1 kinematic)
    virtual void reconnectBaseObject();                 // slot 8 Mac 0x47beda, Win 0x7f7cc0 ("VxStateConstraint::reconnectBaseObject")
    virtual void setBaseObject(VxBase* object);         // slot 9 Mac 0x47bf7a, Win 0x7f8450 (+ the assembly's name)

    void save(VxConstraint* c);                         // Mac 0x47c0ac, Win 0x7f8600 (name inferred)
    void restore(VxConstraint* c);                      // Mac 0x47b9f0, Win 0x7f7d70 ("VxStateConstraint::restore")
    void restoreKinematic(VxConstraint* c);             // Mac 0x47b83e, Win 0x7f8070 ("VxStateConstraint::restoreKinematic")
    // Win 0x7f81d0 (Win only; dead-stripped on Mac): printf dump of the saved values (name inferred)
    void print() const;

    VxReal mValue;                          // Mac +0x1c, Win +0x38: spring natural length / distance / winch length (= -1: none)
    bool mEnabled;                          // Mac +0x24, Win +0x40 (VxConstraint::mEnabled)
    VxArray<CoordinateState> mCoordinates;  // Mac +0x28, Win +0x44
    std::string mAssemblyName;              // Mac +0x38, Win +0x54
    VxArray<AttachmentState> mAttachments;  // Mac +0x3c, Win +0x70
};

} // namespace Vx

#endif
