// Vortex 5.0.0 build 800 "vortexldd41" - VxConstraint, ConstraintPartAttachment (restored).
//
// Mac TU 473 0x43cdac-0x441013 (static initialiser 0x440fa3: <iostream>, MdtConstraintMaxSize),
// Win 0x71fe90-0x722e70 (+ the Win-only members, see VxConstraint.h).
#include "Vx/VxConstraint.h"
#include "Vx/ConstraintAccessor.h"
#include "Vx/VxConstraintFriction.h"
#include "Vx/VxPart.h"
#include "Vx/VxUniverse.h"
#include "Vx/VxFrame.h"
#include "Vx/VxMessage.h"
#include "Mdt/MdtConstraint.h"
#include "Mdt/MdtWorld.h"
#include <iostream>
#include <math.h>

namespace Vx {

// Mac 0x7792e4-0x7792f8 (data, pointing at Mac 0x618e52..)
const char* VxConstraint::kPrimaryAxisOrientationName = "Primary axis orientation";
const char* VxConstraint::kSecondaryAxisOrientationName = "Secondary axis orientation";
const char* VxConstraint::kThirdAxisOrientationName = "Third axis orientation";
const char* VxConstraint::kPrimaryAxisPositionName = "Primary axis position";
const char* VxConstraint::kSecondaryAxisPositionName = "Secondary axis position";
const char* VxConstraint::kThirdAxisPositionName = "Third axis position";
VxBase::NameMap* VxConstraint::sNameMap = 0;  // Mac 0x7792e0, Win 0xaf1624

// getStaticClassType Mac 0x43cdac, isOfClassType Mac 0x43cdb6 / Win 0x720f30,
// getStaticClassTypes Mac 0x43e828 (atexit 0x43d898) / Win 0x7222f0, isOfType Mac 0x43ebb0 /
// Win 0x722370
VX_IMPLEMENT_CLASS_TYPES(VxConstraint, VxBaseUserData, 0x10b)
// new (inline in the subclasses' operator new), delete Mac 0x43d8e2
VX_IMPLEMENT_ALLOCATOR(VxConstraint)

namespace {

// The subscriber notification (inlined in Mac setPart / the destructor; Win 0x722bf0): the
// subscribers of the event first, then those of every event; each list is copied first.
void notifyList(const VxArray<VxConstraint::Subscriber*>& l, VxConstraint* c,
                VxConstraint::Event e, VxPart* part)
{
    const int n = l.size();
    if (n == 0)
        return;
    VxConstraint::Subscriber** copy = (VxConstraint::Subscriber**)vxAlloc(n * sizeof(VxConstraint::Subscriber*));
    for (int i = 0; i < n; ++i)
        copy[i] = l.begin()[i];
    for (int i = 0; i < n; ++i)
        copy[i]->notify(c, e, part);
    vxFree(copy);
}

void notifySubscribers(VxSubscriberCollection<VxConstraint::Event, VxConstraint::Subscriber, 2>& s,
                       VxConstraint* c, VxConstraint::Event e, VxPart* part)
{
    if (s.mEventSubscribers != 0)
        notifyList(s.mEventSubscribers[e], c, e, part);
    if (s.mSubscribers != 0)
        notifyList(*s.mSubscribers, c, e, part);
}

// part != its merge root: the part is merged and its frame is relative to the root's
bool isMerged(VxPart* p)
{
    return p != 0 && p->getMergeRoot() != p;
}

} // namespace

// ---- construction --------------------------------------------------------------------------------

// Mac 0x43ebf6, Win 0x7225e0
VxConstraint::VxConstraint(MdtConstraintID c, int partCount)
    : mConstraint(c), mUniverse(0), m34(0), mParts(partCount, (VxPart*)0), mEnabled(true),
      mActive(false)
{
    MdtConstraintSetUserData(mConstraint, this);
    for (int i = 0; i < partCount; ++i) {
        mPartAttachmentAccessors.push_back(new ConstraintPartAttachment(this, i));
        mPartAttachments.push_back(VxAttachment(VxAttachment::kRelativeToPart));
    }
}

// D2 Mac 0x43f7d0 (D1 0x43fe10, D0 0x43f184), Win D2 0x7223a0 (scalar deleting 0x722e70)
VxConstraint::~VxConstraint()
{
    for (int i = 0; i < mParts.size(); ++i) {
        VxPart* p = mParts.begin()[i];
        if (p != 0) {
            p->removeConstraint(this);
            notifySubscribers(mSubscribers, this, kEventPartDetached, mParts.begin()[i]);
            mParts.begin()[i] = 0;
            MdtConstraintSetBodyAt(mConstraint, i, 0);
        }
    }
    VxBase::setName(0, &sNameMap);
    for (ConstraintPartAttachment** a = mPartAttachmentAccessors.begin(); a != mPartAttachmentAccessors.end(); ++a)
        if (*a != 0)
            delete *a;
    for (ConstraintEquation** a = mEquations.begin(); a != mEquations.end(); ++a)
        if (*a != 0)
            delete *a;
    for (ConstraintCoordinate** a = mCoordinates.begin(); a != mCoordinates.end(); ++a)
        if (*a != 0)
            delete *a;
    MdtConstraintFreeToPool(mConstraint, (VinPool*)VxFrame::instance()->mConstraintPool);
}

// (inlined push_back of a new accessor, e.g. in VxHinge::addAccessors)
void VxConstraint::addEquation(int index, const char* name)
{
    std::string s(name);
    mEquations.push_back(new ConstraintEquation(this, index, s));
}

void VxConstraint::addCoordinate(int index, const char* name)
{
    std::string s(name);
    mCoordinates.push_back(new ConstraintCoordinate(this, index, s));
}

// Mac 0x43cdcc, Win 0x71feb0
bool VxConstraint::isClassType(int type) const
{
    return isOfClassType(type);
}

// Mac 0x43dec6, Win 0x71fec0
void VxConstraint::setName(const char* name)
{
    VxBase::setName(name, &sNameMap);
    if (mConstraint != 0)
        mConstraint->name = getName();
}

// ---- parts ----------------------------------------------------------------------------------------

// Mac 0x43ce40, Win 0x721d30
int VxConstraint::getPartCount() const
{
    int n = 0;
    for (int i = 0; i < mParts.size(); ++i)
        if (mParts.begin()[i] != 0)
            ++n;
    return n;
}

// Mac 0x43fe24, Win 0x7221a0
void VxConstraint::setPart(int index, VxPart* part)
{
    VxPart* old = mParts.begin()[index];
    if (part == 0 && old == 0)
        return;
    if (part != old) {
        if (old != 0) {
            old->removeConstraint(this);
            notifySubscribers(mSubscribers, this, kEventPartDetached, mParts.begin()[index]);
        }
        if (part != 0) {
            part->addConstraint(this);
            notifySubscribers(mSubscribers, this, kEventPartAttached, part);
        }
        mParts.begin()[index] = part;
    }
    suspend();
    MdtBodyID body = 0;
    if (part != 0)
        body = *part->getMergeRootBodyPtr();
    MdtConstraintSetBodyAt(mConstraint, index, body);
    if (mEnabled)
        setEnabled(true);
    int ignored = 1;
    const int n = mParts.size();
    for (int i = 1; i < n; ++i)
        if (mConstraint->body[0] != mConstraint->body[i]) {
            ignored = 0;
            break;
        }
    mConstraint->ignored = ignored;
    VxPart* p0 = mParts.begin()[0];
    if (p0 != 0 && p0 == mParts.begin()[1])
        VxInfo(0, "VxConstraint::setPart: constraint %s has its two first parts set to the same value, the constraint might not work correctly.\n",
               getName());
}

// Mac 0x43e1d6, Win 0x7218e0
void VxConstraint::resetParts()
{
    for (int i = 0; i < mParts.size(); ++i)
        if (mParts.begin()[i] != 0)
            setPart(i, 0);
}

// Mac 0x4403d4, Win 0x716340
bool VxConstraint::getEquationFrictionIndex(int, int*) const
{
    return false;
}

// Mac 0x43cdea, Win 0x7201d0
void VxConstraint::setPartAndAttachment(int index, VxPart* part, const VxVector3& position,
                                        const VxVector3& primaryAxis, const VxVector3* secondaryAxis)
{
    setPart(index, part);
    setPartAttachmentPosition(index, position);
    setPartAttachmentAxes(index, primaryAxis, secondaryAxis);
}

// Mac 0x43db02, Win 0x721cf0
void VxConstraint::setPartAndAttachmentRel(int index, VxPart* part, const VxVector3& position,
                                           const VxVector3& primaryAxis, const VxVector3& secondaryAxis)
{
    setPart(index, part);
    setPartAttachmentPositionRel(index, position);
    setPartAttachmentAxesRel(index, primaryAxis, secondaryAxis);
}

// ---- attachment frames ----------------------------------------------------------------------------

// Mac 0x43dcc4, Win 0x71ff80
void VxConstraint::setPartAttachmentPosition(int index, const VxVector3& p)
{
    MdtConstraintSetBodyPosition(mConstraint, index, p[0], p[1], p[2]);
}

// Mac 0x43dc86, Win 0x71ffb0
void VxConstraint::setPartAttachmentPosition(int index, VxReal x, VxReal y, VxReal z)
{
    MdtConstraintSetBodyPosition(mConstraint, index, x, y, z);
}

// Mac 0x43dc62, Win 0x71ffe0
void VxConstraint::getPartAttachmentPosition(int index, VxVector3& p) const
{
    MdtConstraintGetBodyPosition(mConstraint, index, p.data());
}

// Mac 0x43e08e, Win 0x720100: a unit vector perpendicular to the primary axis (in the plane of
// the two smaller components) as the secondary axis
void VxConstraint::setPartAttachmentAxis(int index, const VxVector3& primary)
{
    VxVector3 s;
    const VxReal x = primary[0];
    const VxReal z = primary[2];
    const VxReal y = primary[1];
    if (fabs(x) + fabs(z) <= fabs(y)) {
        const VxReal r = 1.0 / sqrt(z * z + y * y);
        s[0] = 0.0;
        s[1] = -primary[2] * r;
        s[2] = primary[1] * r;
    } else {
        const VxReal r = 1.0 / sqrt(z * z + x * x);
        s[0] = z * r;
        s[1] = 0.0;
        s[2] = -x * r;
    }
    setPartAttachmentAxes(index, primary, &s);
}

// Mac 0x43d926, Win 0x720080
void VxConstraint::setPartAttachmentAxes(int index, const VxVector3& primary, const VxVector3* secondary)
{
    if (secondary == 0)
        MdtConstraintSetBodyAxis(mConstraint, index, primary[0], primary[1], primary[2]);
    else
        MdtConstraintSetBodyAxes(mConstraint, index, primary[0], primary[1], primary[2],
                                 (*secondary)[0], (*secondary)[1], (*secondary)[2]);
}

// Mac 0x43d8fa, Win 0x720150
void VxConstraint::getPartAttachmentAxes(int index, VxVector3& primary, VxVector3& secondary) const
{
    MdtConstraintGetBodyAxes(mConstraint, index, primary.data(), secondary.data());
}

// Mac 0x43e398, Win 0x721a70: a merged part's axes are rotated into the merge root's frame
void VxConstraint::setPartAttachmentAxesRel(int index, const VxVector3& primary, const VxVector3& secondary)
{
    VxPart* p = mParts.begin()[index];
    if (isMerged(p)) {
        const VxTransform& m = p->getMergeRootTransform();
        const VxReal a0 = primary[0], a1 = primary[1], a2 = primary[2];
        const VxReal b0 = secondary[0], b1 = secondary[1], b2 = secondary[2];
        MdtConstraintSetBodyAxesRel(mConstraint, index,
                                    a0 * m.m[0][0] + a1 * m.m[1][0] + a2 * m.m[2][0],
                                    a0 * m.m[0][1] + a1 * m.m[1][1] + a2 * m.m[2][1],
                                    a0 * m.m[0][2] + a1 * m.m[1][2] + a2 * m.m[2][2],
                                    b0 * m.m[0][0] + b1 * m.m[1][0] + b2 * m.m[2][0],
                                    b0 * m.m[0][1] + b1 * m.m[1][1] + b2 * m.m[2][1],
                                    b0 * m.m[0][2] + b1 * m.m[1][2] + b2 * m.m[2][2]);
        return;
    }
    MdtConstraintSetBodyAxesRel(mConstraint, index, primary[0], primary[1], primary[2],
                                secondary[0], secondary[1], secondary[2]);
}

// Mac 0x43e58c, Win 0x721ba0
void VxConstraint::setPartAttachmentAxisRel(int index, const VxVector3& primary)
{
    VxPart* p = mParts.begin()[index];
    if (isMerged(p)) {
        const VxReal a0 = primary[0], a1 = primary[1], a2 = primary[2];
        const VxTransform& m = p->getMergeRootTransform();
        MdtConstraintSetBodyAxisRel(mConstraint, index,
                                    a0 * m.m[0][0] + a1 * m.m[1][0] + a2 * m.m[2][0],
                                    a0 * m.m[0][1] + a1 * m.m[1][1] + a2 * m.m[2][1],
                                    a0 * m.m[0][2] + a1 * m.m[1][2] + a2 * m.m[2][2]);
        return;
    }
    MdtConstraintSetBodyAxisRel(mConstraint, index, primary[0], primary[1], primary[2]);
}

// Mac 0x43d9c4, Win 0x721980
void VxConstraint::setPartAttachmentPositionRel(int index, const VxVector3& pos)
{
    VxPart* p = mParts.begin()[index];
    if (isMerged(p)) {
        const VxReal x = pos[0], y = pos[1], z = pos[2];
        const VxTransform& m = p->getMergeRootTransform();
        MdtConstraintSetBodyPositionCOM(mConstraint, index,
                                        x * m.m[0][0] + y * m.m[1][0] + z * m.m[2][0] + m.m[3][0],
                                        x * m.m[0][1] + y * m.m[1][1] + z * m.m[2][1] + m.m[3][1],
                                        x * m.m[0][2] + y * m.m[1][2] + z * m.m[2][2] + m.m[3][2]);
        return;
    }
    MdtConstraintSetBodyPositionCOM(mConstraint, index, pos[0], pos[1], pos[2]);
}

// Win 0x721a40
void VxConstraint::setPartAttachmentPositionRel(int index, VxReal x, VxReal y, VxReal z)
{
    VxVector3 v(x, y, z);
    setPartAttachmentPositionRel(index, v);
}

// Mac 0x43db58, Win 0x721920: the inverse of setPartAttachmentPositionRel
void VxConstraint::getPartAttachmentPositionRel(int index, VxVector3& pos) const
{
    MdtConstraintGetBodyPositionCOM(mConstraint, index, pos.data());
    VxPart* p = mParts.begin()[index];
    if (isMerged(p)) {
        const VxTransform& m = p->getMergeRootTransform();
        const VxReal z = pos[2] - m.m[3][2];
        const VxReal y = pos[1] - m.m[3][1];
        const VxReal x = pos[0] - m.m[3][0];
        pos[0] = x * m.m[0][0] + y * m.m[0][1] + z * m.m[0][2];
        pos[1] = x * m.m[1][0] + y * m.m[1][1] + z * m.m[1][2];
        pos[2] = x * m.m[2][0] + y * m.m[2][1] + z * m.m[2][2];
    }
}

// Mac 0x43e226, Win 0x721c60
void VxConstraint::getPartAttachmentAxesRel(int index, VxVector3& primary, VxVector3& secondary) const
{
    MdtConstraintGetBodyAxesRel(mConstraint, index, primary.data(), secondary.data());
    VxPart* p = mParts.begin()[index];
    if (isMerged(p)) {
        const VxTransform& m = p->getMergeRootTransform();
        VxReal x = primary[0], y = primary[1], z = primary[2];
        primary[0] = x * m.m[0][0] + y * m.m[0][1] + z * m.m[0][2];
        primary[1] = x * m.m[1][0] + y * m.m[1][1] + z * m.m[1][2];
        primary[2] = x * m.m[2][0] + y * m.m[2][1] + z * m.m[2][2];
        x = secondary[0];
        y = secondary[1];
        z = secondary[2];
        secondary[0] = x * m.m[0][0] + y * m.m[0][1] + z * m.m[0][2];
        secondary[1] = x * m.m[1][0] + y * m.m[1][1] + z * m.m[1][2];
        secondary[2] = x * m.m[2][0] + y * m.m[2][1] + z * m.m[2][2];
    }
}

// Win 0x720000
void VxConstraint::getPartAttachmentPositionCOMRel(int index, VxVector3& p) const
{
    MdtConstraintGetBodyPositionRel(mConstraint, index, p.data());
}

// Win 0x720020
void VxConstraint::setPartAttachmentPositionCOMRel(int index, const VxVector3& p)
{
    MdtConstraintSetBodyPositionRel(mConstraint, index, p[0], p[1], p[2]);
}

// Win 0x720050
void VxConstraint::setPartAttachmentPositionCOMRel(int index, VxReal x, VxReal y, VxReal z)
{
    MdtConstraintSetBodyPositionRel(mConstraint, index, x, y, z);
}

// Win 0x720170
void VxConstraint::alignOtherPartAttachment(int index)
{
    VxVector3 primary, secondary, position;
    getPartAttachmentAxes(index, primary, secondary);
    getPartAttachmentPosition(index, position);
    setPartAttachmentAxes(1 - index, primary, &secondary);
    setPartAttachmentPosition(1 - index, position);
}

// Win 0x721d70: a part-relative attachment goes through the "Rel" setters, any other kind is
// first converted into world coordinates; the attachment is cached
void VxConstraint::setPartAttachment(int index, const VxAttachment& a)
{
    VxTransform tm;
    if (a.mType == VxAttachment::kRelativeToPart) {
        a.getLocalTransform(tm);
        setPart(index, a.mPart);
        setPartAttachmentPositionRel(index, tm.getTranslation());
        setPartAttachmentAxesRel(index, tm.getAxis(0), tm.getAxis(1));
    } else {
        a.getTransform(tm);
        setPart(index, a.mPart);
        setPartAttachmentPosition(index, tm.getTranslation());
        const VxVector3 secondary = tm.getAxis(1);
        setPartAttachmentAxes(index, tm.getAxis(0), &secondary);
    }
    if (&mPartAttachments.begin()[index] != &a)
        mPartAttachments.begin()[index] = a;
}

// Win 0x721f40
void VxConstraint::refreshPartAttachment(int index)
{
    VxAttachment& a = mPartAttachments.begin()[index];
    a.setNode(VxNode::sNull);
    a.mPart = mParts.begin()[index];
    VxVector3 position, secondary;
    getPartAttachmentPositionRel(index, position);
    a.mPosition = position;
    getPartAttachmentAxesRel(index, position, secondary);
    a.mPrimaryAxis = position;
    a.mSecondaryAxis = secondary;
    a.orthonormalize();
    a.mType = VxAttachment::kRelativeToPart;
}

// Win 0x721e30
void VxConstraint::updatePartAttachment(int index)
{
    if (!mPartAttachments.begin()[index].isNull())
        return;
    VxAttachment& a = mPartAttachments.begin()[index];
    a.mType = VxAttachment::kRelativeToPart;
    a.setNode(VxNode::sNull);
    a.mPart = mParts.begin()[index];
    VxVector3 position, secondary;
    getPartAttachmentPositionRel(index, position);
    a.mPosition = position;
    getPartAttachmentAxesRel(index, position, secondary);
    a.mPrimaryAxis = position;
    a.mSecondaryAxis = secondary;
    a.orthonormalize();
}

// Win 0x722170
void VxConstraint::getPartAttachment(int index, VxAttachment& out)
{
    updatePartAttachment(index);
    VxAttachment& a = mPartAttachments.begin()[index];
    if (&out != &a)
        out = a;
}

// Mac 0x43dd04, Win 0x71ff60
void VxConstraint::getPartTorque(int index, VxVector3& t) const
{
    MdtConstraintGetTorque(mConstraint, index, t.data());
}

// Mac 0x43dd28, Win 0x71ff40
void VxConstraint::getPartForce(int index, VxVector3& f) const
{
    MdtConstraintGetForce(mConstraint, index, f.data());
}

// Mac 0x43df12, Win 0x71fe90
void VxConstraint::resetForces()
{
    MdtConstraintResetForces(mConstraint);
}

// ---- universe / enabling ----------------------------------------------------------------------

// Mac 0x43dd62, Win 0x71fef0
void VxConstraint::updateEnabled()
{
    if (mUniverse == 0)
        return;
    const bool active = mActive;
    if ((int)active != MdtConstraintIsEnabled(mConstraint)) {
        if (!mActive)
            MdtConstraintDisable(mConstraint);
        else
            MdtConstraintEnable(mConstraint);
    }
}

// Mac 0x43ddb6, Win 0x720f90
void VxConstraint::suspend()
{
    mActive = false;
    updateEnabled();
}

// Mac 0x43ddd0, Win 0x722030
void VxConstraint::setEnabled(bool enable)
{
    mEnabled = enable;
    mActive = enable;
    if (enable) {
        if (mUniverse == 0 || getPartCount() == 0) {
            mActive = false;
        } else {
            for (int i = 0; i < mParts.size(); ++i) {
                VxPart* p = mParts.begin()[i];
                if (p != 0 && p->mUniverse == 0)
                    mActive = false;
            }
        }
    }
    updateEnabled();
}

// Mac 0x43de50, Win 0x7220e0
bool VxConstraint::setUniverse(VxUniverse* universe)
{
    if (mUniverse != universe) {
        suspend();
        if (mUniverse != 0)
            MdtWorldRemoveConstraint(mConstraint);
        mUniverse = universe;
        if (universe != 0) {
            MdtWorldAddConstraint(universe->mWorld, mConstraint);
            if (mUniverse != 0 && mEnabled)
                setEnabled(true);
        }
    }
    return true;
}

// Mac 0x43cf12, Win 0x720bd0
void VxConstraint::wakeBodies()
{
    if (mUniverse != 0 && mActive)
        MdtConstraintEnableBodies(mConstraint);
}

// Win 0x720f50
void VxConstraint::setLocked(bool locked)
{
    if (!locked)
        MdtConstraintUnlockConstraint(mConstraint);
    else
        MdtConstraintLockConstraint(mConstraint);
    wakeBodies();
}

// Mac 0x43cef4, Win 0x720c40 (NOTE: the argument is not used)
void VxConstraint::setMdtFlag2(int)
{
    MdtConstraintSetFlags(mConstraint, 2);
}

// ---- coordinates ------------------------------------------------------------------------------

// Mac 0x43dd4c, Win 0x71ff30
int VxConstraint::getCoordinateCount() const
{
    return MdtConstraintGetNumLimits(mConstraint);
}

// Mac 0x43d260, Win 0x720210
MdtLimit* VxConstraint::getCoordinateLimit(int coordinate)
{
    return MdtConstraintGetLimit(mConstraint, coordinate);
}

// Mac 0x43cf92, Win 0x720230
const MdtLimit* VxConstraint::getCoordinateLimit(int coordinate) const
{
    return MdtConstraintGetLimit(mConstraint, coordinate);
}

// Mac 0x43d63a, Win 0x720290
MdtSingleLimit* VxConstraint::getLimit(int coordinate, int bound)
{
    if (bound == 0)
        return MdtLimitGetLowerLimit(getCoordinateLimit(coordinate));
    return MdtLimitGetUpperLimit(getCoordinateLimit(coordinate));
}

// Mac 0x43d186, Win 0x720250
const MdtSingleLimit* VxConstraint::getLimit(int coordinate, int bound) const
{
    MdtLimit* l = (MdtLimit*)getCoordinateLimit(coordinate);
    if (bound == 0)
        return MdtLimitGetLowerLimit(l);
    return MdtLimitGetUpperLimit(l);
}

// Win 0x720990
int VxConstraint::getCoordinateType(int coordinate) const
{
    const MdtLimit* l = getCoordinateLimit(coordinate);
    if (l->kind == kMdtLimitAngular)
        return kCoordinateAngular;
    if (l->kind == kMdtLimitDistance)
        return kCoordinateDistance;
    return kCoordinateLinear;
}

// Win 0x7209d0
bool VxConstraint::isCoordinateAngular(int coordinate) const
{
    return MdtLimitIsAngular(getCoordinateLimit(coordinate)) != 0;
}

// Win 0x720950
int VxConstraint::getLinearCoordinateReferencePart(int coordinate) const
{
    return getCoordinateLimit(coordinate)->me8;
}

// Mac 0x43d27e, Win 0x7217d0
void VxConstraint::setLinearCoordinateReferencePart(int coordinate, int partIndex)
{
    if (getCoordinateLimit(coordinate)->kind == kMdtLimitLinear) {
        getCoordinateLimit(coordinate)->me8 = partIndex;
        int f;
        if (getCoordinateFrictionIndex(coordinate, &f))
            getCoordinateFriction(coordinate)->setAxis(partIndex);
    }
}

// Mac 0x43cfb0, Win 0x720c80
bool VxConstraint::getCoordinateFrictionIndex(int coordinate, int* friction) const
{
    const MdtLimit* l = getCoordinateLimit(coordinate);
    if (l->kind == kMdtLimitLinear) {
        *friction = l->index;
        return true;
    }
    if (l->kind == kMdtLimitAngular) {
        *friction = l->index + 3;
        return true;
    }
    return false;
}

// Mac 0x43d006, Win 0x720d80
VxConstraintFriction* VxConstraint::getCoordinateFriction(int coordinate)
{
    int f;
    if (getCoordinateFrictionIndex(coordinate, &f))
        return &mConstraint->bcl.friction[f];
    return 0;
}

namespace {
// the force of a friction row in the last step (Win 0x720d10 / 0x720de0 / 0x720eb0, inlined)
VxReal frictionForce(const MdtConstraint* c, int f)
{
    const VxConstraintFriction& fr = c->bcl.friction[f];
    if (fr.mMaxForce != 0.0 && fr.mStatus != 0)
        return c->bcl.lambda[fr.mStatus];
    return 0;
}
} // namespace

// Win 0x720de0 (NOTE: the warning text names getConstraintEquationFrictionForceLastFrame)
VxReal VxConstraint::getCoordinateFrictionForce(int coordinate) const
{
    int f;
    if (!getCoordinateFrictionIndex(coordinate, &f)) {
        VxWarning(0, "VxConstraint::getConstraintEquationFrictionForceLastFrame: invalid coordinate.\n");
        return 0;
    }
    return frictionForce(mConstraint, f);
}

// Win 0x720e70
VxConstraintFriction* VxConstraint::getEquationFriction(int equation)
{
    int f = equation;
    if (getEquationFrictionIndex(equation, &f))
        return &mConstraint->bcl.friction[f];
    return 0;
}

// Win 0x720eb0
VxReal VxConstraint::getEquationFrictionForce(int equation) const
{
    int f = equation;
    if (!getEquationFrictionIndex(equation, &f)) {
        VxWarning(0, "VxConstraint::getConstraintEquationFrictionForceLastFrame: invalid coordinate.\n");
        return 0;
    }
    return frictionForce(mConstraint, f);
}

// Mac 0x43ce98, Win 0x720cd0
VxConstraintFriction* VxConstraint::getFriction(int axis, bool linear)
{
    VxAssert((unsigned)axis < 3, "VxConstraint::setFriction: axis must be 0,1,2\n");
    if (!linear)
        axis += 3;
    return &mConstraint->bcl.friction[axis];
}

// Win 0x720d10
VxReal VxConstraint::getFrictionForce(int axis, bool linear) const
{
    VxAssert((unsigned)axis < 3, "VxConstraint::setFriction: axis must be 0,1,2\n");
    if (!linear)
        axis += 3;
    return frictionForce(mConstraint, axis);
}

// Mac 0x43d80a, Win 0x720fe0: a lock / motor without a force range stays free
bool VxConstraint::setControl(int coordinate, CoordinateControlEnum control)
{
    MdtLimit* l = getCoordinateLimit(coordinate);
    int c = kMdtLimitFree;
    if (control == kControlLocked) {
        if (l->lockForceLo < l->lockForceHi)
            c = kMdtLimitLocked;
    } else if (control == kControlMotorized) {
        c = l->motorMaxForceLo < l->motorMaxForceHi ? kMdtLimitMotorized : kMdtLimitFree;
    }
    wakeBodies();
    const bool changed = l->control != c;
    if (changed)
        l->control = c;
    return changed;
}

// Mac 0x43d22a, Win 0x721080
VxConstraint::CoordinateControlEnum VxConstraint::getControl(int coordinate) const
{
    const int c = getCoordinateLimit(coordinate)->control;
    if (c == kMdtLimitMotorized)
        return kControlMotorized;
    if (c == kMdtLimitLocked)
        return kControlLocked;
    return kControlFree;
}

// Mac 0x43e1ac, Win 0x7208b0
bool VxConstraint::isCoordinateLocked(int coordinate) const
{
    return MdtLimitIsLocked(getCoordinateLimit(coordinate)) != 0;
}

// Mac 0x43d06e, Win 0x720890
bool VxConstraint::isCoordinateMotorized(int coordinate) const
{
    return MdtLimitIsMotorized(getCoordinateLimit(coordinate)) != 0;
}

// Win 0x720970
VxReal VxConstraint::getCoordinateForce(int coordinate) const
{
    return MdtConstraintGetLimitForce(mConstraint, coordinate);
}

// Mac 0x43e6c4, Win 0x721220
VxReal VxConstraint::getCoordinateCurrentPosition(int coordinate) const
{
    if (coordinate > -1 && coordinate < getCoordinateCount())
        return MdtConstraintCalculatePosition(mConstraint, coordinate);
    return 0;
}

// Mac 0x43e708, Win 0x7211e0
VxReal VxConstraint::getCoordinatePosition(int coordinate) const
{
    if (coordinate > -1 && coordinate < getCoordinateCount())
        return MdtConstraintGetPosition(mConstraint, coordinate);
    return 0;
}

// Mac 0x43e796, Win 0x721140
VxReal VxConstraint::getCoordinateStoredPosition(int coordinate) const
{
    if (coordinate > -1 && coordinate < getCoordinateCount())
        return MdtConstraintGetLimitPosition(mConstraint, coordinate);
    return 0;
}

// Mac 0x43e7da, Win 0x721100
void VxConstraint::setCoordinatePosition(int coordinate, VxReal position)
{
    if (coordinate > -1 && coordinate < getCoordinateCount())
        MdtConstraintSetLimitPosition(mConstraint, coordinate, position);
}

// Win 0x7210c0
void VxConstraint::setCoordinateCurrentPosition(int coordinate, VxReal position)
{
    if (coordinate > -1 && coordinate < getCoordinateCount())
        MdtConstraintSetLimitCurrentPosition(mConstraint, coordinate, position);
}

// Mac 0x43d20c, Win 0x721180
VxReal VxConstraint::getCoordinatePositionOffset(int coordinate) const
{
    return getCoordinateLimit(coordinate)->positionOffset;
}

// Mac 0x43e74c, Win 0x7211a0
void VxConstraint::setCoordinatePositionOffset(int coordinate, VxReal offset)
{
    if (coordinate > -1 && coordinate < getCoordinateCount())
        getCoordinateLimit(coordinate)->positionOffset = offset;
}

// Win 0x7208d0
void VxConstraint::setCoordinateFlag4(int coordinate, bool b)
{
    getCoordinateLimit(coordinate)->mf4 = b;
}

// Win 0x7208f0
bool VxConstraint::getCoordinateFlag4(int coordinate) const
{
    return getCoordinateLimit(coordinate)->mf4 != 0;
}

// Win 0x7209f0
void VxConstraint::setCoordinateFlag0(int coordinate, bool b)
{
    MdtLimitSetMf0(getCoordinateLimit(coordinate), b);
}

// Win 0x720930
VxReal VxConstraint::getCoordinateValue50(int coordinate) const
{
    return MdtLimitGetM50(getCoordinateLimit(coordinate));
}

// Win 0x720650
bool VxConstraint::isCoordinateLimitState2(int coordinate) const
{
    return getCoordinateLimit(coordinate)->limitsActive == 2;
}

// ---- motor ------------------------------------------------------------------------------------------

// Mac 0x43d340, Win 0x720730
void VxConstraint::setMotorMaximumForce(int coordinate, VxReal maxForce)
{
    MdtLimitSetMotorMaxForces(getCoordinateLimit(coordinate), -maxForce, maxForce);
}

// Mac 0x43d098, Win 0x720870
VxReal VxConstraint::getMotorMaximumForce(int coordinate) const
{
    return MdtLimitGetMotorMaxForce(getCoordinateLimit(coordinate));
}

// Mac 0x43d3c8, Win 0x721710
void VxConstraint::setMotorDesiredVelocity(int coordinate, VxReal velocity)
{
    MdtLimitSetDesiredVelocity(getCoordinateLimit(coordinate), velocity);
    wakeBodies();
}

// Mac 0x43d0dc, Win 0x720830
VxReal VxConstraint::getMotorDesiredVelocity(int coordinate) const
{
    return MdtLimitGetDesiredVelocity(getCoordinateLimit(coordinate));
}

// Mac 0x43d2f4, Win 0x7207c0
void VxConstraint::setMotorStiffnessAndDamping(int coordinate, VxReal k, VxReal d)
{
    MdtLimitSetMotorStiffnessDamping(getCoordinateLimit(coordinate), k, d);
}

// Win 0x721750
void VxConstraint::setMotorStiffness(int coordinate, VxReal k)
{
    const VxReal d = MdtLimitGetMotorDamping(getCoordinateLimit(coordinate));
    MdtLimitSetMotorStiffnessDamping(getCoordinateLimit(coordinate), k, d);
}

// Win 0x721790
void VxConstraint::setMotorDamping(int coordinate, VxReal d)
{
    const VxReal k = MdtLimitGetMotorStiffness(getCoordinateLimit(coordinate));
    MdtLimitSetMotorStiffnessDamping(getCoordinateLimit(coordinate), k, d);
}

// Mac 0x43d120, Win 0x7207f0
VxReal VxConstraint::getMotorStiffness(int coordinate) const
{
    return MdtLimitGetMotorStiffness(getCoordinateLimit(coordinate));
}

// Mac 0x43d0fe, Win 0x720810
VxReal VxConstraint::getMotorDamping(int coordinate) const
{
    return MdtLimitGetMotorDamping(getCoordinateLimit(coordinate));
}

// Mac 0x43d390, Win 0x720700
void VxConstraint::setMotorLoss(int coordinate, VxReal loss)
{
    MdtLimitSetM18(getCoordinateLimit(coordinate), loss);
}

// Mac 0x43d0ba, Win 0x720850
VxReal VxConstraint::getMotorLoss(int coordinate) const
{
    return MdtLimitGetM18(getCoordinateLimit(coordinate));
}

// Mac 0x43d40c, Win 0x721680
void VxConstraint::setMotorParameters(int coordinate, VxReal velocity, VxReal maxForce,
                                      VxReal stiffness, VxReal damping, VxReal loss)
{
    MdtLimitSetMotor(getCoordinateLimit(coordinate), velocity, -maxForce, maxForce);
    MdtLimitSetMotorStiffnessDamping(getCoordinateLimit(coordinate), stiffness, damping);
    MdtLimitSetM18(getCoordinateLimit(coordinate), loss);
    wakeBodies();
}

// ---- lock ---------------------------------------------------------------------------------------------

// Mac 0x43d4ee, Win 0x721520
void VxConstraint::setLockPosition(int coordinate, VxReal position)
{
    MdtLimit* l = getCoordinateLimit(coordinate);
    const VxReal hi = MdtLimitGetLockMaximumForce(l);
    const VxReal lo = MdtLimitGetLockMinimumForce(l);
    MdtLimitSetLock(l, position, lo, hi);
    wakeBodies();
}

// Mac 0x43d164, Win 0x720680
VxReal VxConstraint::getLockPosition(int coordinate) const
{
    return MdtLimitGetLockPosition(getCoordinateLimit(coordinate));
}

// Win 0x721580
void VxConstraint::setLockMaximumForce(int coordinate, VxReal maxForce)
{
    MdtLimit* l = getCoordinateLimit(coordinate);
    MdtLimitSetLock(l, MdtLimitGetLockPosition(l), -maxForce, maxForce);
    wakeBodies();
}

// Win 0x7215e0
void VxConstraint::setLockForceLimits(int coordinate, VxReal lo, VxReal hi)
{
    MdtLimit* l = getCoordinateLimit(coordinate);
    MdtLimitSetLock(l, MdtLimitGetLockPosition(l), lo, hi);
    wakeBodies();
}

// Mac 0x43d142, Win 0x7206c0
VxReal VxConstraint::getLockMaximumForce(int coordinate) const
{
    return MdtLimitGetLockMaximumForce(getCoordinateLimit(coordinate));
}

// Win 0x7206e0
VxReal VxConstraint::getLockMinimumForce(int coordinate) const
{
    return MdtLimitGetLockMinimumForce(getCoordinateLimit(coordinate));
}

// Win 0x721640
void VxConstraint::setLockStiffness(int coordinate, VxReal k)
{
    MdtLimitSetLockStiffness(getCoordinateLimit(coordinate), k);
    wakeBodies();
}

// Win 0x7206a0
VxReal VxConstraint::getLockStiffness(int coordinate) const
{
    return MdtLimitGetLockStiffness(getCoordinateLimit(coordinate));
}

// Mac 0x43d55e, Win 0x7214c0
void VxConstraint::setLockParameters(int coordinate, VxReal position, VxReal maxForce, VxReal stiffness)
{
    MdtLimit* l = getCoordinateLimit(coordinate);
    MdtLimitSetLock(l, position, -maxForce, maxForce);
    MdtLimitSetLockStiffness(l, stiffness);
    wakeBodies();
}

// ---- limits -------------------------------------------------------------------------------------------

// Mac 0x43d5f8, Win 0x7213e0
void VxConstraint::setLimitsActive(int coordinate, bool active)
{
    MdtLimitSetLimitsActive(getCoordinateLimit(coordinate), active);
    wakeBodies();
}

// Mac 0x43d044, Win 0x720910
bool VxConstraint::getLimitsActive(int coordinate) const
{
    return MdtLimitIsActive(getCoordinateLimit(coordinate)) != 0;
}

// Mac 0x43d6ba, Win 0x7212c0
void VxConstraint::setLimitPosition(int coordinate, int bound, VxReal position)
{
    getLimit(coordinate, bound)->stop = position;
    wakeBodies();
}

// Mac 0x43d1e8, Win 0x720370
VxReal VxConstraint::getLimitPosition(int coordinate, int bound) const
{
    return getLimit(coordinate, bound)->stop;
}

// Mac 0x43d6fc, Win 0x721260
void VxConstraint::setLimitPositions(int coordinate, VxReal lower, VxReal upper)
{
    getLimit(coordinate, 0)->stop = lower;
    getLimit(coordinate, 1)->stop = upper;
    wakeBodies();
}

// Mac 0x43d678, Win 0x721320
void VxConstraint::setLimitVelocity(int coordinate, int bound, VxReal v)
{
    getLimit(coordinate, bound)->m20 = v;
    wakeBodies();
}

// Mac 0x43d1c4, Win 0x7203b0
VxReal VxConstraint::getLimitVelocity(int coordinate, int bound) const
{
    return getLimit(coordinate, bound)->m20;
}

// Win 0x7203f0 (no wakeBodies)
void VxConstraint::setLimitRestitution(int coordinate, int bound, VxReal r)
{
    MdtSingleLimitSetRestitution(getLimit(coordinate, bound), r);
}

// Win 0x720450
VxReal VxConstraint::getLimitRestitution(int coordinate, int bound) const
{
    return MdtSingleLimitGetRestitution(getLimit(coordinate, bound));
}

// Win 0x721380
void VxConstraint::setLimitStiffness(int coordinate, int bound, VxReal k)
{
    MdtSingleLimitSetStiffness(getLimit(coordinate, bound), k);
    wakeBodies();
}

// Win 0x720550
VxReal VxConstraint::getLimitStiffness(int coordinate, int bound) const
{
    return MdtSingleLimitGetStiffness(getLimit(coordinate, bound));
}

// Win 0x7205a0 (no wakeBodies)
void VxConstraint::setLimitDamping(int coordinate, int bound, VxReal d)
{
    MdtSingleLimitSetDamping(getLimit(coordinate, bound), d);
}

// Win 0x720600
VxReal VxConstraint::getLimitDamping(int coordinate, int bound) const
{
    return MdtSingleLimitGetDamping(getLimit(coordinate, bound));
}

// Win 0x7204a0 (no wakeBodies)
void VxConstraint::setLimitMaximumForce(int coordinate, int bound, VxReal f)
{
    if (f <= 0.0) {
        VxWarning(0, "VxConstraint::setLimitMaximumForce: max force must be > 0\n");
        return;
    }
    getLimit(coordinate, bound)->m28 = f;
}

// Win 0x720510
VxReal VxConstraint::getLimitMaximumForce(int coordinate, int bound) const
{
    return getLimit(coordinate, bound)->m28;
}

// Mac 0x43d76c, Win 0x720320
void VxConstraint::setUpperLimit(int coordinate, VxReal position, VxReal velocity, VxReal restitution,
                                 VxReal stiffness, VxReal damping)
{
    MdtSingleLimit* l = getLimit(coordinate, 1);
    l->stop = position;
    l->m20 = velocity;
    l->restitution = restitution;
    MdtSingleLimitSetStiffness(l, stiffness);
    MdtSingleLimitSetDamping(l, damping);
}

// Win 0x7202d0
void VxConstraint::setLowerLimit(int coordinate, VxReal position, VxReal velocity, VxReal restitution,
                                 VxReal stiffness, VxReal damping)
{
    MdtSingleLimit* l = getLimit(coordinate, 0);
    l->stop = position;
    l->m20 = velocity;
    l->restitution = restitution;
    MdtSingleLimitSetStiffness(l, stiffness);
    MdtSingleLimitSetDamping(l, damping);
}

// Win 0x721420
bool VxConstraint::isCoordinateAtLimit(int coordinate, int bound, VxReal tolerance) const
{
    const VxReal p = getCoordinatePosition(coordinate);
    const VxReal stop = getLimit(coordinate, bound)->stop;
    return MdtLimitIsActive(getCoordinateLimit(coordinate)) != 0 && fabs(p - stop) < tolerance;
}

// ---- equations ----------------------------------------------------------------------------------------

// Win 0x721840
int VxConstraint::getEquationCount() const
{
    return mEquations.size();
}

// Mac 0x43cf38, Win 0x720ba0
void VxConstraint::setRelaxationActive(int equation, bool active)
{
    if (!active)
        MdtConstraintDisableRelaxConstraint(mConstraint, equation);
    else
        MdtConstraintEnableRelaxConstraint(mConstraint, equation);
}

// Mac 0x43cf6c, Win 0x720b80
bool VxConstraint::isRelaxationActive(int equation) const
{
    return MdtConstraintIsRelaxConstraintEnabled(mConstraint, equation) != 0;
}

// Mac 0x43e026, Win 0x721850
void VxConstraint::setRelaxationParameters(int equation, VxReal stiffness, VxReal damping, VxReal loss,
                                           bool active)
{
    MdtConstraintRelaxConstraint(mConstraint, equation, stiffness, damping, loss);
    setRelaxationActive(equation, active);
}

// Win 0x7218a0
void VxConstraint::getRelaxationParameters(int equation, VxReal* stiffness, VxReal* damping, VxReal* loss,
                                           bool* active) const
{
    MdtConstraintGetRelaxConstraint(mConstraint, equation, stiffness, damping, loss);
    *active = MdtConstraintIsRelaxConstraintEnabled(mConstraint, equation) != 0;
}

// Mac 0x43df28, Win 0x720b40
VxReal VxConstraint::getRelaxationStiffness(int equation) const
{
    return MdtConstraintGetRelaxConstraintStiffness(mConstraint, equation);
}

// Mac 0x43df46, Win 0x720ae0
void VxConstraint::setRelaxationStiffness(int equation, VxReal k)
{
    MdtConstraintSetRelaxConstraintStiffness(mConstraint, equation, k);
}

// Mac 0x43df6e, Win 0x720a80
void VxConstraint::setRelaxationStiffnessAndDamping(int equation, VxReal k, VxReal d)
{
    MdtConstraintSetRelaxConstraintStiffness(mConstraint, equation, k);
    MdtConstraintSetRelaxConstraintDamping(mConstraint, equation, d);
}

// Win 0x720b00
void VxConstraint::setRelaxationDamping(int equation, VxReal d)
{
    MdtConstraintSetRelaxConstraintDamping(mConstraint, equation, d);
}

// Win 0x720b60
VxReal VxConstraint::getRelaxationDamping(int equation) const
{
    return MdtConstraintGetRelaxConstraintDamping(mConstraint, equation);
}

// Win 0x720ac0
void VxConstraint::setRelaxationLoss(int equation, VxReal loss)
{
    MdtConstraintSetRelaxConstraintLoss(mConstraint, equation, loss);
}

// Win 0x720b20
VxReal VxConstraint::getRelaxationLoss(int equation) const
{
    return MdtConstraintGetRelaxConstraintLoss(mConstraint, equation);
}

// Mac 0x43dfc6, Win 0x720a40
void VxConstraint::getRelaxationForceLimits(int equation, VxReal* lo, VxReal* hi) const
{
    MdtConstraintGetForceLimits(mConstraint, equation, lo, hi);
}

// Mac 0x43dff2, Win 0x720a10
void VxConstraint::setRelaxationForceLimits(int equation, VxReal lo, VxReal hi)
{
    MdtConstraintSetForceLimits(mConstraint, equation, lo, hi);
}

// Win 0x720a60
VxReal VxConstraint::getEquationForce(int equation) const
{
    return MdtConstraintGetRowForce(mConstraint, equation);
}

// Win 0x720bf0
bool VxConstraint::isEquationAtLowerLimit(int equation) const
{
    return MdtConstraintIsRowAtLowerLimit(mConstraint, equation) != 0;
}

// Win 0x720c10
bool VxConstraint::isEquationAtUpperLimit(int equation) const
{
    return MdtConstraintIsRowAtUpperLimit(mConstraint, equation) != 0;
}

// ==========================================================================================
// ConstraintPartAttachment (Mac 0x4403e3-0x440a86, Win 0x722860-0x722b30)
// ==========================================================================================

// D1 Mac 0x440748, D0 0x440712, Win 0x715db0 (ICF)
ConstraintPartAttachment::~ConstraintPartAttachment()
{
}

// Mac 0x440440, Win 0x7185a0
VxConstraint* ConstraintPartAttachment::getConstraint() const
{
    return mConstraint;
}

// Mac 0x440456, Win 0x7859b0
int ConstraintPartAttachment::getIndex() const
{
    return mIndex;
}

// Mac 0x4403ee, Win 0x722860
void ConstraintPartAttachment::setPart(VxPart* part)
{
    mConstraint->setPart(mIndex, part);
}

// Mac 0x4407ca, Win 0x7228b0
void ConstraintPartAttachment::setPosition(const VxVector3& p)
{
    mConstraint->setPartAttachmentPositionRel(mIndex, p);
}

// Mac 0x440a86, Win 0x7228d0: the secondary axis is rebuilt perpendicular to the new primary
// axis (secondary = (primary x secondary) x new) unless it already is (|new . secondary| < 1e-7)
void ConstraintPartAttachment::setPrimaryAxis(const VxVector3& n)
{
    VxVector3 p, s;
    mConstraint->getPartAttachmentAxesRel(mIndex, p, s);
    const VxReal n0 = n[0], n1 = n[1], n2 = n[2];
    if (1e-07 <= fabs(n0 * s[0] + n1 * s[1] + n2 * s[2])) {
        const VxReal c0 = p[1] * s[2] - p[2] * s[1];
        const VxReal c1 = p[2] * s[0] - s[2] * p[0];
        const VxReal c2 = s[1] * p[0] - p[1] * s[0];
        s[0] = n2 * c1 - n1 * c2;
        s[1] = n0 * c2 - n2 * c0;
        s[2] = n1 * c0 - n0 * c1;
    }
    mConstraint->setPartAttachmentAxesRel(mIndex, n, s);
}

// Mac 0x440902, Win 0x7229c0: the same for the primary axis (primary = new x (primary x secondary))
void ConstraintPartAttachment::setSecondaryAxis(const VxVector3& n)
{
    VxVector3 p, s;
    mConstraint->getPartAttachmentAxesRel(mIndex, p, s);
    const VxReal n0 = n[0], n1 = n[1], n2 = n[2];
    if (1e-07 <= fabs(n0 * p[0] + n1 * p[1] + n2 * p[2])) {
        const VxReal c0 = p[1] * s[2] - p[2] * s[1];
        const VxReal c1 = p[2] * s[0] - s[2] * p[0];
        const VxReal c2 = s[1] * p[0] - p[1] * s[0];
        p[0] = n1 * c2 - n2 * c1;
        p[1] = n2 * c0 - n0 * c2;
        p[2] = n0 * c1 - n1 * c0;
    }
    mConstraint->setPartAttachmentAxesRel(mIndex, p, n);
}

// Mac 0x440422, Win 0x7228a0
VxPart* ConstraintPartAttachment::getPart() const
{
    return mConstraint->mParts.begin()[mIndex];
}

// Mac 0x44077a, Win 0x722ab0
VxVector3 ConstraintPartAttachment::getPosition() const
{
    VxVector3 p;
    mConstraint->getPartAttachmentPositionRel(mIndex, p);
    return p;
}

// Mac 0x44085e, Win 0x722ae0
VxVector3 ConstraintPartAttachment::getPrimaryAxis() const
{
    VxVector3 p, s;
    mConstraint->getPartAttachmentAxesRel(mIndex, p, s);
    return p;
}

// Mac 0x4407f8, Win 0x722b30
VxVector3 ConstraintPartAttachment::getSecondaryAxis() const
{
    VxVector3 p, s;
    mConstraint->getPartAttachmentAxesRel(mIndex, p, s);
    return s;
}

} // namespace Vx
