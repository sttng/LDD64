// Vortex 5.0.0 build 800 "vortexldd41" - VxBallAndSocket, VxBallAndSocket::AngularFriction
// (restored).
// Mac TU 457 0x42f0dc-0x43046c (static initialiser 0x4303fc: <iostream>, MdtConstraintMaxSize),
// Win 0x72c010-0x72c9d0. VxConstraintFriction (Mac 0x42fe38-0x42ff10 / 0x4303b8, same TU) is in
// VxConstraintFriction.cpp. Three position equations, no coordinate.
#include "Vx/VxBallAndSocket.h"
#include "Vx/ConstraintAccessor.h"
#include "Vx/VxFrame.h"
#include "Mdt/MdtConstraint.h"
#include <iostream>

namespace Vx {

// getStaticClassType Mac 0x42f0dc / Win 0x72c040, isOfClassType Mac 0x42f0e6 / Win 0x72c1d0,
// getStaticClassTypes Mac 0x42f2e6 (atexit 0x42f18b) / Win 0x72c1f0, isOfType Mac 0x42f66e /
// Win 0x72c270
VX_IMPLEMENT_CLASS_TYPES(VxBallAndSocket, VxConstraint, 0x118)
// new Mac 0x42f2d3 / Win 0x72c010, delete Mac 0x42f1d5 / Win 0x72c050 (+0x72c020)
VX_IMPLEMENT_ALLOCATOR(VxBallAndSocket)

// C2 Mac 0x42fd4e (C1 0x42fe24), Win 0x72c510: the angular friction comes from the global
// operator new (Mac 0x40 bytes, Win 0x48)
VxBallAndSocket::VxBallAndSocket()
    : VxConstraint(MdtBSJointCreate((VinPool*)VxFrame::instance()->mConstraintPool), 2)
{
    mAngularFriction = new AngularFriction;
    addAccessors();
}

// Win 0x72c5c0
VxBallAndSocket::VxBallAndSocket(VxPart* part0, VxPart* part1, const VxVector3& position, const VxVector3* axis)
    : VxConstraint(MdtBSJointCreate((VinPool*)VxFrame::instance()->mConstraintPool), 2)
{
    mAngularFriction = new AngularFriction;
    addAccessors();
    setPart(0, part0);
    setPart(1, part1);
    if (axis != 0)
        MdtConstraintSetAxis(mConstraint, (*axis)[0], (*axis)[1], (*axis)[2]);
    MdtConstraintSetPosition(mConstraint, position[0], position[1], position[2]);
}

// D2 Mac 0x42f25a (D1 0x42f2c0, D0 0x42f1ec), Win 0x72c9d0
VxBallAndSocket::~VxBallAndSocket()
{
    setEnabled(false);
    if (mAngularFriction != 0)
        delete mAngularFriction;
}

// Mac 0x42ff1c, Win 0x72c9c0
const char* VxBallAndSocket::getClassName() const
{
    return "VxBallAndSocket";
}

// Mac 0x42f6b4, Win 0x72c2a0: three position equations; the angular friction drives the
// constraint's three angular frictions
void VxBallAndSocket::addAccessors()
{
    addEquation(0, kPrimaryAxisPositionName);
    addEquation(1, kSecondaryAxisPositionName);
    addEquation(2, kThirdAxisPositionName);
    AngularFriction* f = mAngularFriction;
    f->mFriction[0] = getFriction(0, false);
    f->mFriction[1] = getFriction(1, false);
    f->mFriction[2] = getFriction(2, false);
}

// Mac 0x42f168, Win 0x72c0c0
void VxBallAndSocket::resetParts()
{
    VxConstraint::resetParts();
    MdtBSJointReset(mConstraint);
}

// Mac 0x42f0fc, Win 0x72c150
bool VxBallAndSocket::getEquationFrictionIndex(int equation, int* friction) const
{
    if (equation == 1) {
        *friction = 1;
        return true;
    }
    if (equation == 2) {
        *friction = 2;
        return true;
    }
    if (equation == 0) {
        *friction = 0;
        return true;
    }
    return false;
}

// Win 0x72c0e0
void VxBallAndSocket::setPosition(const VxVector3& p)
{
    MdtConstraintSetPosition(mConstraint, p[0], p[1], p[2]);
}

// Win 0x72c110
void VxBallAndSocket::setAxis(const VxVector3& a)
{
    MdtConstraintSetAxis(mConstraint, a[0], a[1], a[2]);
}

// Win 0x72c1a0
void VxBallAndSocket::setPositionEquationReferencePart(int partIndex)
{
    ((MdtBSJoint*)mConstraint)->m_ae8 = partIndex;
}

// ---- AngularFriction -------------------------------------------------------------------------

// D1 Mac 0x43039e, D0 0x430374, Win 0x72c990 (ICF with ~VxConstraintFriction)
VxBallAndSocket::AngularFriction::~AngularFriction()
{
}

// Mac 0x42ff2e, Win 0x72c6e0
void VxBallAndSocket::AngularFriction::setProportional(bool b)
{
    mProportional = b;
    mFriction[0]->setProportional(b);
    mFriction[1]->setProportional(b);
    mFriction[2]->setProportional(b);
}

// Mac 0x42ff7c, Win 0x72c720
bool VxBallAndSocket::AngularFriction::isProportional() const
{
    return mFriction[0]->isProportional();
}

// Mac 0x42ff96, Win 0x72c730
void VxBallAndSocket::AngularFriction::setMaxForce(VxReal f)
{
    mMaxForce = f;
    mFriction[0]->setMaxForce(f);
    mFriction[1]->setMaxForce(f);
    mFriction[2]->setMaxForce(f);
}

// Mac 0x42fff2, Win 0x72c780: the largest of the three
VxReal VxBallAndSocket::AngularFriction::getMaxForce() const
{
    VxReal m = mFriction[1]->mMaxForce;
    if (m <= mFriction[0]->mMaxForce)
        m = mFriction[0]->mMaxForce;
    const VxReal f2 = mFriction[2]->mMaxForce;
    return m < f2 ? f2 : m;
}

// Mac 0x430042, Win 0x72c7d0
void VxBallAndSocket::AngularFriction::setLoss(VxReal loss)
{
    mLoss = loss;
    mFriction[0]->setLoss(loss);
    mFriction[1]->setLoss(loss);
    mFriction[2]->setLoss(loss);
}

// Mac 0x43009e, Win 0x72c820
VxReal VxBallAndSocket::AngularFriction::getLoss() const
{
    return mFriction[0]->getLoss();
}

// Mac 0x4300b4, Win 0x72c830
void VxBallAndSocket::AngularFriction::setCoefficient(VxReal mu)
{
    mCoefficient = mu;
    mFriction[0]->setCoefficient(mu);
    mFriction[1]->setCoefficient(mu);
    mFriction[2]->setCoefficient(mu);
}

// Mac 0x430110, Win 0x72c880
VxReal VxBallAndSocket::AngularFriction::getCoefficient() const
{
    return mFriction[0]->getCoefficient();
}

// Mac 0x430126, Win 0x72c890
void VxBallAndSocket::AngularFriction::setScale(VxReal s)
{
    mScale = s;
    mFriction[0]->setScale(s);
    mFriction[1]->setScale(s);
    mFriction[2]->setScale(s);
}

// Mac 0x430182, Win 0x72c8e0
VxReal VxBallAndSocket::AngularFriction::getScale() const
{
    return mFriction[0]->getScale();
}

// Mac 0x430198, Win 0x72c8f0
void VxBallAndSocket::AngularFriction::setEnabled(bool b)
{
    mEnabled = b;
    mFriction[0]->setEnabled(b);
    mFriction[1]->setEnabled(b);
    mFriction[2]->setEnabled(b);
}

// Mac 0x4301e6, Win 0x72c930
bool VxBallAndSocket::AngularFriction::isEnabled() const
{
    return mFriction[0]->isEnabled();
}

// Mac 0x430200, Win 0x72c940
void VxBallAndSocket::AngularFriction::setAxis(int axis)
{
    mAxis = axis;
    mFriction[0]->setAxis(axis);
    mFriction[1]->setAxis(axis);
    mFriction[2]->setAxis(axis);
}

// Mac 0x43024a, Win 0x72c980
int VxBallAndSocket::AngularFriction::getAxis() const
{
    return mFriction[0]->getAxis();
}

} // namespace Vx
