// Vortex 5.0.0 build 800 "vortexldd41" - VxStateConstraint (restored).
// Mac TU 520 0x47b736-0x47d60a (static initialiser 0x47d5a5: <iostream> only), Win
// 0x7f7cc0-0x7f9730. The std::vector-style helpers of the two arrays (Mac 0x47cb92-0x47d2df:
// uninitialized_copy / fill / _M_fill_insert / destroy of CoordinateState and AttachmentState;
// Win 0x7f8a10-0x7f9630) are VxArray's members here.
#include "Vx/VxStateConstraint.h"
#include "Vx/VxConstraint.h"
#include "Vx/VxAssemblyBase.h"
#include "Vx/VxEntity.h"
#include "Vx/VxPart.h"
#include "Vx/VxSpring.h"
#include "Vx/VxDistanceJoint.h"
#include "Vx/VxWinch.h"
#include "Vx/VxGearRatio.h"
#include "Vx/ConstraintAccessor.h"
#include "Vx/VxMessage.h"
#include <float.h>
#include <stdio.h>
#include <iostream>

namespace Vx {

// Mac 0x47b736 (C2) / 0x47b79c (C1); Win inlined
VxStateConstraint::CoordinateState::CoordinateState()
    : limitsActive(false), lowerLimit(DBL_MAX), upperLimit(-DBL_MAX), lowerLimitVelocity(0),
      upperLimitVelocity(0), control(0), motorDesiredVelocity(0), positionOffset(0), motorLoss(0),
      lockPosition(0), storedPosition(0)
{
}

// Mac 0x47b7b0 (C2) / 0x47b80a (C1); Win inlined
VxStateConstraint::AttachmentState::AttachmentState()
    : part(0), partName(), position(0, 0, 0), primaryAxis(1, 0, 0), secondaryAxis(0, 1, 0)
{
}

// Mac 0x47b83e, Win 0x7f8070: limits only where a limit velocity was saved, the motor velocity
// only with a motor loss (NOTE: the original's tests), offset and position always; the winch
// length
void VxStateConstraint::restoreKinematic(VxConstraint* c)
{
    if (c == 0) {
        VxWarning(0, "VxStateConstraint::restoreKinematic : constraint is NULL\n");
        return;
    }
    c->resetForces();
    for (int i = 0; i < mCoordinates.size(); ++i) {
        const CoordinateState& s = mCoordinates.begin()[i];
        if (s.limitsActive) {
            if (s.lowerLimitVelocity != 0.0)
                c->setLimitPosition(i, 0, s.lowerLimit);
            if (mCoordinates.begin()[i].upperLimitVelocity != 0.0)
                c->setLimitPosition(i, 1, mCoordinates.begin()[i].upperLimit);
        }
        if (mCoordinates.begin()[i].motorLoss != 0.0)
            c->setMotorDesiredVelocity(i, mCoordinates.begin()[i].motorDesiredVelocity);
        c->setCoordinatePositionOffset(i, mCoordinates.begin()[i].positionOffset);
        c->setCoordinatePosition(i, mCoordinates.begin()[i].storedPosition);
    }
    if (mValue >= 0.0 && c->isClassType(VxWinch::getStaticClassType()))
        static_cast<VxWinch*>(c)->setLength(mValue);
}

// Mac 0x47b9f0, Win 0x7f7d70: every coordinate value, the type value, the enabled flag and the
// part attachments (parts found again by name when they were deleted)
void VxStateConstraint::restore(VxConstraint* c)
{
    if (c == 0) {
        VxWarning(0, "VxStateConstraint::restore : constraint is NULL\n");
        return;
    }
    c->resetForces();
    for (int i = 0; i < mCoordinates.size(); ++i) {
        c->setLimitsActive(i, mCoordinates.begin()[i].limitsActive);
        if (mCoordinates.begin()[i].limitsActive) {
            c->setLimitPosition(i, 0, mCoordinates.begin()[i].lowerLimit);
            c->setLimitPosition(i, 1, mCoordinates.begin()[i].upperLimit);
            c->setLimitVelocity(i, 0, mCoordinates.begin()[i].lowerLimitVelocity);
            c->setLimitVelocity(i, 1, mCoordinates.begin()[i].upperLimitVelocity);
        }
        c->setControl(i, (VxConstraint::CoordinateControlEnum)mCoordinates.begin()[i].control);
        c->setMotorDesiredVelocity(i, mCoordinates.begin()[i].motorDesiredVelocity);
        c->setMotorLoss(i, mCoordinates.begin()[i].motorLoss);
        c->setLockPosition(i, mCoordinates.begin()[i].lockPosition);
        c->setCoordinatePositionOffset(i, mCoordinates.begin()[i].positionOffset);
        c->setCoordinatePosition(i, mCoordinates.begin()[i].storedPosition);
    }
    if (mValue >= 0.0) {
        if (c->isClassType(VxSpring::getStaticClassType())) {
            static_cast<VxSpring*>(c)->setNaturalLength(mValue);
        } else if (c->isClassType(VxDistanceJoint::getStaticClassType())) {
            static_cast<VxDistanceJoint*>(c)->setDistance(mValue);
        } else if (c->isClassType(VxWinch::getStaticClassType())) {
            static_cast<VxWinch*>(c)->setLength(mValue);
        } else if (c->isClassType(VxGearRatio::getStaticClassType())) {
            // NOTE: a gear ratio is reset, not restored (the original's)
            static_cast<VxGearRatio*>(c)->setPartReferencePosition(0, 0);
            static_cast<VxGearRatio*>(c)->setPartReferencePosition(1, 0);
            static_cast<VxGearRatio*>(c)->setRatio(0);
        }
    }
    c->setEnabled(mEnabled);
    int n = mAttachments.size();
    if (n == c->mParts.size() && n > 0) {
        for (int i = 0; i < c->mParts.size(); ++i) {
            AttachmentState& a = mAttachments.begin()[i];
            if (a.part == 0 && !a.partName.empty()) {
                VxEntity* e = VxEntity::findFromName(a.partName.c_str());
                a.part = e != 0 ? dynamic_cast<VxPart*>(e) : 0;
            }
            c->setPartAndAttachmentRel(i, a.part, a.position, a.primaryAxis, a.secondaryAxis);
        }
    }
}

// Mac 0x47be34, Win 0x7f83a0
void VxStateConstraint::restoreMode(int mode, bool)
{
    VxConstraint* c = static_cast<VxConstraint*>(mObject);
    if (c == 0)
        return;
    if (mode == 0)
        restore(c);
    else if (mode == 1)
        restoreKinematic(c);
}

// Mac 0x47be86, Win 0x7f8380
void VxStateConstraint::restoreKinematic(bool)
{
    if (mObject != 0)
        restoreKinematic(static_cast<VxConstraint*>(mObject));
}

// Mac 0x47beb0, Win 0x7f8360
void VxStateConstraint::restore(bool)
{
    if (mObject != 0)
        restore(static_cast<VxConstraint*>(mObject));
}

// Mac 0x47beda, Win 0x7f7cc0: by name in the assembly of that name, else in all constraints
void VxStateConstraint::reconnectBaseObject()
{
    if (mObject == 0) {
        VxBase* c = 0;
        VxAssemblyBase* a = VxAssemblyBase::findFromName(mAssemblyName.c_str());
        if (a != 0)
            c = a->findConstraint(mObjectName.c_str());
        if (c == 0) {
            if (VxConstraint::sNameMap != 0)
                c = VxBase::findFromName(mObjectName.c_str(), VxConstraint::sNameMap);
            if (c == 0)
                VxWarning(0, "VxStateConstraint::reconnectBaseObject : cannot find constraint named '%s'\n", mObjectName.c_str());
        }
        setBaseObject(c);
    }
    VxStateBase::reconnectBaseObject();
}

// Mac 0x47bf7a, Win 0x7f8450: the name of the constraint's assembly (VxConstraint +0x34) is kept
void VxStateConstraint::setBaseObject(VxBase* object)
{
    VxStateBase::setBaseObject(object);
    mAssemblyName.clear();
    if (object != 0) {
        VxConstraint* c = dynamic_cast<VxConstraint*>(object);
        if (c != 0 && c->m34 != 0)
            mAssemblyName = static_cast<VxBase*>(static_cast<VxAssemblyBase*>(c->m34))->getName();
    }
}

// Mac 0x47c0ac, Win 0x7f8600
void VxStateConstraint::save(VxConstraint* c)
{
    setBaseObject(c);
    int n = c->getCoordinateCount();
    mCoordinates.resize(n, CoordinateState());
    for (int i = 0; i < mCoordinates.size(); ++i) {
        CoordinateState& s = mCoordinates.begin()[i];
        s.limitsActive = c->getLimitsActive(i);
        if (s.limitsActive) {
            s.lowerLimit = c->getLimitPosition(i, 0);
            s.upperLimit = c->getLimitPosition(i, 1);
            s.lowerLimitVelocity = c->getLimitVelocity(i, 0);
            s.upperLimitVelocity = c->getLimitVelocity(i, 1);
        }
        s.control = c->getControl(i);
        s.motorDesiredVelocity = c->getMotorDesiredVelocity(i);
        s.motorLoss = c->getMotorLoss(i);
        s.lockPosition = c->getLockPosition(i);
        s.positionOffset = c->getCoordinatePositionOffset(i);
        s.storedPosition = c->getCoordinateStoredPosition(i);
    }
    // NOTE: mValue keeps its previous value (initially -1) for the other constraint types
    if (c->isClassType(VxSpring::getStaticClassType()))
        mValue = static_cast<VxSpring*>(c)->getNaturalLength();
    else if (c->isClassType(VxDistanceJoint::getStaticClassType()))
        mValue = static_cast<VxDistanceJoint*>(c)->getDistance();
    else if (c->isClassType(VxWinch::getStaticClassType()))
        mValue = static_cast<VxWinch*>(c)->getLength();
    mEnabled = c->mEnabled;
    mAttachments.resize(c->mParts.size(), AttachmentState());
    for (int i = 0; i < c->mParts.size(); ++i) {
        ConstraintPartAttachment* pa = c->mPartAttachmentAccessors.begin()[i];
        AttachmentState& a = mAttachments.begin()[i];
        a.part = pa->getPart();
        a.partName = a.part != 0 ? static_cast<VxBase*>(a.part)->getName() : "";
        a.position = pa->getPosition();
        a.primaryAxis = pa->getPrimaryAxis();
        a.secondaryAxis = pa->getSecondaryAxis();
    }
}

// Mac 0x47c818, Win 0x7f8a00
void VxStateConstraint::save()
{
    if (mObject != 0)
        save(static_cast<VxConstraint*>(mObject));
}

// C2 Mac 0x47c836 (C1 0x47ca22), Win 0x7f8500
VxStateConstraint::VxStateConstraint(const VxStateConstraint& o)
    : VxStateBase(o), mValue(o.mValue), mEnabled(o.mEnabled), mCoordinates(o.mCoordinates),
      mAssemblyName(o.mAssemblyName), mAttachments(o.mAttachments)
{
}

// C2 Mac 0x47ca3c (C1 0x47cb78), Win 0x7f8970
VxStateConstraint::VxStateConstraint(VxConstraint* c)
    : VxStateBase(c), mValue(-1.0), mEnabled(false), mCoordinates(), mAssemblyName(), mAttachments()
{
    save(c);
}

// Mac D1 0x47d2e0 / D0 0x47d41c, Win 0x7f93f0
VxStateConstraint::~VxStateConstraint()
{
}

// Mac 0x47d55e, Win 0x7f96c0
VxStateBase* VxStateConstraint::clone() const
{
    return new VxStateConstraint(*this);
}

// Win 0x7f81d0 (Win only). NOTE: the original's format strings: the lock line prints the motor
// velocity and the loss under "Lock pos ... vel" and drops the lock position; "Distance" is
// printed for the spring, distance and winch values alike.
void VxStateConstraint::print() const
{
    VxConstraint* c = dynamic_cast<VxConstraint*>(mObject);
    if (c != 0) {
        if (mEnabled)
            printf("Constraint Name: %s is enabled\n", c->getName());
        else
            printf("Constraint Name: %s disabled\n", c->getName());
    }
    for (int i = 0; i < mCoordinates.size(); ++i) {
        const CoordinateState& s = mCoordinates.begin()[i];
        printf("  Coordinate %d\n", i);
        if (s.limitsActive)
            printf("    Limited at %g %g and vel %g %g\n", s.lowerLimit, s.upperLimit, s.lowerLimitVelocity, s.upperLimitVelocity);
        printf("    Control %d\n", s.control);
        // NOTE: the original passes three values for two %g (the third is not printed)
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#endif
        printf("    Lock pos %g vel %g, motor vel\n", s.motorDesiredVelocity, s.motorLoss, s.lockPosition);
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
        printf("    Offset %g\n", s.positionOffset);
    }
    if (c != 0 && (c->isClassType(VxSpring::getStaticClassType()) || c->isClassType(VxDistanceJoint::getStaticClassType()) ||
                   c->isClassType(VxWinch::getStaticClassType())))
        printf("    Distance %g\n", mValue);
}

} // namespace Vx
